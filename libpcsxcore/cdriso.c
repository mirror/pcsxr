/***************************************************************************
 *   Copyright (C) 2007 PCSX-df Team                                       *
 *   Copyright (C) 2009 Wei Mingzhi                                        *
 *   Copyright (C) 2012 notaz                                              *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.           *
 ***************************************************************************/

#include "psxcommon.h"
#include "plugins.h"
#include "cdrom.h"
#include "cdriso.h"
#include "ppf.h"
#include "ecm.h"

#ifdef _WIN32
#include <process.h>
#include <windows.h>
#define strcasecmp _stricmp
#else
#include <sys/time.h>
#include <unistd.h>
#include <limits.h>
#endif
#include <zlib.h>

#ifdef ENABLE_CCDDA
#include "libavcodec/avcodec.h"
#include "libavutil/mathematics.h"
#include "libavformat/avformat.h"

#define INBUF_SIZE 4096
#define AUDIO_INBUF_SIZE INBUF_SIZE*4
#define AUDIO_REFILL_THRESH 4096
/*#ifndef AVCODEC_MAX_AUDIO_FRAME_SIZE
	#define 	AVCODEC_MAX_AUDIO_FRAME_SIZE   192000
#endif*/
#endif

unsigned int cdrIsoMultidiskCount;
unsigned int cdrIsoMultidiskSelect;

static FILE *cdHandle = NULL;
static FILE *subHandle = NULL;

static boolean subChanMixed = FALSE;
static boolean subChanRaw = FALSE;
static boolean subChanMissing = FALSE;

static boolean multifile = FALSE;
static boolean isMode1ISO = FALSE; // TODO: use sector size/mode info from CUE also?

static unsigned char cdbuffer[CD_FRAMESIZE_RAW];
static unsigned char subbuffer[SUB_FRAMESIZE];

static boolean playing = FALSE;
static boolean cddaBigEndian = FALSE;
static unsigned int cddaCurPos = 0;

/* Frame offset into CD image where pregap data would be found if it was there.
 * If a game seeks there we must *not* return subchannel data since it's
 * not in the CD image, so that cdrom code can fake subchannel data instead.
 * XXX: there could be multiple pregaps but PSX dumps only have one? */
static unsigned int pregapOffset;

// compressed image stuff
static struct {
	unsigned char buff_raw[16][CD_FRAMESIZE_RAW];
	unsigned char buff_compressed[CD_FRAMESIZE_RAW * 16 + 100];
	unsigned int *index_table;
	unsigned int index_len;
	unsigned int block_shift;
	unsigned int current_block;
	unsigned int sector_in_blk;
} *compr_img;

int (*cdimg_read_func)(FILE *f, unsigned int base, void *dest, int sector);

char* CALLBACK CDR__getDriveLetter(void);
long CALLBACK CDR__configure(void);
long CALLBACK CDR__test(void);
void CALLBACK CDR__about(void);
long CALLBACK CDR__setfilename(char *filename);
long CALLBACK CDR__getStatus(struct CdrStat *stat);

static void DecodeRawSubData(void);

struct trackinfo {
	enum {DATA=1, CDDA} type;
	u8 start[3];		// MSF-format
	u8 length[3];		// MSF-format
	FILE *handle;		// for multi-track images CDDA
	enum {NONE=0, BIN=1, CCDDA=2
#ifdef ENABLE_CCDDA1
		,MP3=AV_CODEC_ID_MP3, APE=AV_CODEC_ID_APE, FLAC=AV_CODEC_ID_FLAC
#endif
	} cddatype;	// BIN, WAV, MP3, APE
	void* decoded_buffer;
	u32	 len_decoded_buffer;
	char filepath[256];
	u32 start_offset; // byte offset from start of above file
};

#define MAXTRACKS 100 /* How many tracks can a CD hold? */

static int numtracks = 0;
static struct trackinfo ti[MAXTRACKS];

// get a sector from a msf-array
unsigned int msf2sec(char *msf) {
	return ((msf[0] * 60 + msf[1]) * 75) + msf[2];
}

void sec2msf(unsigned int s, char *msf) {
	msf[0] = s / 75 / 60;
	s = s - msf[0] * 75 * 60;
	msf[1] = s / 75;
	s = s - msf[1] * 75;
	msf[2] = s;
}

// divide a string of xx:yy:zz into m, s, f
static void tok2msf(char *time, char *msf) {
	char *token;

	token = strtok(time, ":");
	if (token) {
		msf[0] = atoi(token);
	}
	else {
		msf[0] = 0;
	}

	token = strtok(NULL, ":");
	if (token) {
		msf[1] = atoi(token);
	}
	else {
		msf[1] = 0;
	}

	token = strtok(NULL, ":");
	if (token) {
		msf[2] = atoi(token);
	}
	else {
		msf[2] = 0;
	}
}

static int get_cdda_type(const char *str)
{
	const size_t lenstr = strlen(str);
	if (strncmp((str+lenstr-3), "bin", 3) == 0) {
		return BIN;
	}
#ifdef ENABLE_CCDDA1
	else if (strncmp((str+lenstr-3), "mp3", 3) == 0) {
		return MP3;
	}
	else if (strncmp((str+lenstr-3), "ape", 3) == 0) {
		return APE;
	}
	else if (strncmp((str+lenstr-4), "flac", 4) == 0) {
		return FLAC;
	}
#endif
#ifdef ENABLE_CCDDA
	else {
		return CCDDA;
	}
#else
	else {
		static boolean ccddaWarn = TRUE;
		if (ccddaWarn) {
			SysMessage(_(" -> Compressed CDDA support is not compiled with this version. Such tracks will be silent."));
			ccddaWarn = FALSE;
		}
	}
#endif
	return BIN; // no valid extension or no support; assume bin
}

static int get_compressed_cdda_track_length(const char* filepath) {
	int seconds = -1;
#ifdef ENABLE_CCDDA
	av_register_all();

	AVFormatContext * inAudioFormat = NULL;
	inAudioFormat = avformat_alloc_context();
	int errorCode = avformat_open_input(&inAudioFormat, filepath, NULL, NULL);
	avformat_find_stream_info(inAudioFormat, NULL);
	seconds = (int)(inAudioFormat->duration/AV_TIME_BASE);
	avformat_close_input(&inAudioFormat);
#endif
	return seconds;
}


#ifdef ENABLE_CCDDA
static int decode_compressed_cdda_track(FILE* outfile, const char* infilepath, s32 id) {
	AVCodec *codec;
	AVCodecContext *c=NULL;
	AVFormatContext *inAudioFormat = NULL;
	s32 len;
	AVPacket avpkt;
	AVFrame *decoded_frame = NULL;
	s32 got_frame = 0, moreFrames = 1;
	s32 audio_stream_index;
	s32 ret;

	//av_init_packet(&avpkt);
	
	avcodec_register_all();
	
	inAudioFormat = avformat_alloc_context();
	int errorCode = avformat_open_input(&inAudioFormat, infilepath, NULL, NULL);
	if (errorCode) {
		SysMessage(_("Audio file opening failed!\n"));
		return errorCode;
	}
	avformat_find_stream_info(inAudioFormat, NULL);

	/* select the audio stream */
	ret = av_find_best_stream(inAudioFormat, AVMEDIA_TYPE_AUDIO, -1, -1, &codec, 0);
	if (ret < 0) {
		avformat_close_input(&inAudioFormat);
		SysMessage(_("Couldn't find any audio stream in file\n"));
		return ret;
	}
	audio_stream_index = ret;
	c = inAudioFormat->streams[audio_stream_index]->codec;
	av_opt_set_int(c, "refcounted_frames", 1, 0);

	c->sample_fmt = AV_SAMPLE_FMT_S16;
	c->channels = 2;
	c->sample_rate = 44100;

	/* open it */
	if (avcodec_open2(c, codec, NULL) < 0) {
		SysMessage(_("Audio decoder opening failed. Compressed audio support not available.\n"));
		avformat_close_input(&inAudioFormat);
		return 3; // codec open failed
	}
	//http://ffmpeg.org/doxygen/trunk/doc_2examples_2filtering_audio_8c-example.html#a80
	//http://blog.tomaka17.com/2012/03/libavcodeclibavformat-tutorial/
	do {
		if ((moreFrames=av_read_frame(inAudioFormat, &avpkt)) < 0) {// returns non-zero on error
			break;
		}

		if (avpkt.stream_index != audio_stream_index) {
			continue;
		}

		if (!decoded_frame) {
			if (!(decoded_frame = avcodec_alloc_frame())) {
				SysMessage(_(" -> Error allocating audio frame buffer. This track will not be available."));
				avformat_close_input(&inAudioFormat);
				av_free(&decoded_frame);
				return 1; // error decoding frame
			}
		} else {
			avcodec_get_frame_defaults(decoded_frame);
		}
		len = avcodec_decode_audio4(c, decoded_frame, &got_frame, &avpkt);
		if (len > 0 && got_frame) {
			/* if a frame has been decoded, output it */
			int data_size = av_samples_get_buffer_size(NULL, c->channels,
								decoded_frame->nb_samples,
								c->sample_fmt, 1);
			//printf ("Channels %i/%i: %i -> %i/%i\n", len, data_size, decoded_frame->sample_rate, c->channels, c->sample_rate);
			fwrite(decoded_frame->data[0], 1, data_size, outfile);
		}
		av_free_packet(&avpkt);
		//avcodec_free_frame(&decoded_frame);
	} while (moreFrames >= 0); // TODO: check for possible leaks

	// file will be closed later on, now just flush it
	fflush(outfile);

	avformat_close_input(&inAudioFormat);

	// TODO not sure if all resources are freed...
	//avcodec_close(c);
	//av_free(c);
	//av_free(&decoded_frame);
	return 0;
}
#endif

static int do_decode_cdda(struct trackinfo* tri, u32 tracknumber) {
#ifndef ENABLE_CCDDA
	return 4; // support is not compiled in
#else
	tri->decoded_buffer = malloc(tri->len_decoded_buffer);
	FILE* decoded_cdda = fmemopen(tri->decoded_buffer, tri->len_decoded_buffer, "wb");

	if (decoded_cdda == NULL || tri->decoded_buffer == NULL) {
		SysMessage(_("Could not allocate memory to decode CDDA TRACK: %s\n"), tri->filepath);
	}

	fclose(tri->handle); // encoded file handle not needed anymore

	int ret;
	SysPrintf(_("Decoding audio tr#%u (%s)..."), tracknumber, tri->filepath);
	// decode 2nd input param to 1st output param
	if ((ret=decode_compressed_cdda_track(decoded_cdda, tri->filepath /*tri->handle*/, tri->cddatype)) == 0) {
		int len1 = ftell(decoded_cdda);
		if (len1 > tri->len_decoded_buffer) {
			SysPrintf(_("Buffer overflow..."));
		}
		//printf("actual %i vs. %i estimated", len1, tri->len_decoded_buffer);
		fclose(decoded_cdda); // close wb file now and will be opened as rb
		tri->handle = fmemopen(tri->decoded_buffer, len1, "rb"); // change handle to decoded one
		SysPrintf(_("OK\n"), tri->filepath);
	}
	tri->cddatype = BIN;
	return ret;
#endif
}

// this function tries to get the .toc file of the given .bin
// the necessary data is put into the ti (trackinformation)-array
static int parsetoc(const char *isofile) {
	char			tocname[MAXPATHLEN];
	FILE			*fi;
	char			linebuf[256], tmp[256], name[256];
	char			*token;
	char			time[20], time2[20];
	unsigned int	t, sector_offs, sector_size;
	unsigned int	current_zero_gap = 0;

	numtracks = 0;

	// copy name of the iso and change extension from .bin to .toc
	strncpy(tocname, isofile, sizeof(tocname));
	tocname[MAXPATHLEN - 1] = '\0';
	if (strlen(tocname) >= 4) {
		strcpy(tocname + strlen(tocname) - 4, ".toc");
	}
	else {
		return -1;
	}

	if ((fi = fopen(tocname, "r")) == NULL) {
		// try changing extension to .cue (to satisfy some stupid tutorials)
		strcpy(tocname + strlen(tocname) - 4, ".cue");
		if ((fi = fopen(tocname, "r")) == NULL) {
			// if filename is image.toc.bin, try removing .bin (for Brasero)
			strcpy(tocname, isofile);
			t = strlen(tocname);
			if (t >= 8 && strcmp(tocname + t - 8, ".toc.bin") == 0) {
				tocname[t - 4] = '\0';
				if ((fi = fopen(tocname, "r")) == NULL) {
					return -1;
				}
			}
			else {
				return -1;
			}
		}
	}

	memset(&ti, 0, sizeof(ti));
	cddaBigEndian = TRUE; // cdrdao uses big-endian for CD Audio

	sector_size = CD_FRAMESIZE_RAW;
	sector_offs = 2 * 75;

	// parse the .toc file
	while (fgets(linebuf, sizeof(linebuf), fi) != NULL) {
		// search for tracks
		strncpy(tmp, linebuf, sizeof(linebuf));
		token = strtok(tmp, " ");

		if (token == NULL) continue;

		if (!strcmp(token, "TRACK")) {
			sector_offs += current_zero_gap;
			current_zero_gap = 0;

			// get type of track
			token = strtok(NULL, " ");
			numtracks++;

			if (!strncmp(token, "MODE2_RAW", 9)) {
				ti[numtracks].type = DATA;
				sec2msf(2 * 75, ti[numtracks].start); // assume data track on 0:2:0

				// check if this image contains mixed subchannel data
				token = strtok(NULL, " ");
				if (token != NULL && !strncmp(token, "RW", 2)) {
					sector_size = CD_FRAMESIZE_RAW + SUB_FRAMESIZE;
					subChanMixed = TRUE;
					if (!strncmp(token, "RW_RAW", 6))
						subChanRaw = TRUE;
				}
			}
			else if (!strncmp(token, "AUDIO", 5)) {
				ti[numtracks].type = CDDA;
			}
		}
		else if (!strcmp(token, "DATAFILE")) {
			if (ti[numtracks].type == CDDA) {
				sscanf(linebuf, "DATAFILE \"%[^\"]\" #%d %8s", name, &t, time2);
				ti[numtracks].start_offset = t;
				t = t / sector_size + sector_offs;
				sec2msf(t, (char *)&ti[numtracks].start);
				tok2msf((char *)&time2, (char *)&ti[numtracks].length);
			}
			else {
				sscanf(linebuf, "DATAFILE \"%[^\"]\" %8s", name, time);
				tok2msf((char *)&time, (char *)&ti[numtracks].length);
			}
		}
		else if (!strcmp(token, "FILE")) {
			sscanf(linebuf, "FILE \"%[^\"]\" #%d %8s %8s", name, &t, time, time2);
			tok2msf((char *)&time, (char *)&ti[numtracks].start);
			t += msf2sec(ti[numtracks].start) * sector_size;
			ti[numtracks].start_offset = t;
			t = t / sector_size + sector_offs;
			sec2msf(t, (char *)&ti[numtracks].start);
			tok2msf((char *)&time2, (char *)&ti[numtracks].length);
		}
		else if (!strcmp(token, "ZERO") || !strcmp(token, "SILENCE")) {
			// skip unneeded optional fields
			while (token != NULL) {
				token = strtok(NULL, " ");
				if (strchr(token, ':') != NULL)
					break;
			}
			if (token != NULL) {
				tok2msf(token, tmp);
				current_zero_gap = msf2sec(tmp);
			}
			if (numtracks > 1) {
				t = ti[numtracks - 1].start_offset;
				t /= sector_size;
				pregapOffset = t + msf2sec(ti[numtracks - 1].length);
			}
		}
		else if (!strcmp(token, "START")) {
			token = strtok(NULL, " ");
			if (token != NULL && strchr(token, ':')) {
				tok2msf(token, tmp);
				t = msf2sec(tmp);
				ti[numtracks].start_offset += (t - current_zero_gap) * sector_size;
				t = msf2sec(ti[numtracks].start) + t;
				sec2msf(t, (char *)&ti[numtracks].start);
			}
		}
	}

	fclose(fi);

	return 0;
}

// this function tries to get the .cue file of the given .bin
// the necessary data is put into the ti (trackinformation)-array
static int parsecue(const char *isofile) {
	char			cuename[MAXPATHLEN];
	char			filepath[MAXPATHLEN];
	char			*incue_fname;
	FILE			*fi;
	char			*token;
	char			time[20];
	char			*tmp;
	char			linebuf[256], tmpb[256], dummy[256];
	unsigned int	incue_max_len;
	unsigned int	t, file_len, mode, sector_offs;
	unsigned int	sector_size = 2352;

	numtracks = 0;

	// copy name of the iso and change extension from .bin to .cue
	strncpy(cuename, isofile, sizeof(cuename));
	cuename[MAXPATHLEN - 1] = '\0';
	if (strlen(cuename) >= 4) {
		strcpy(cuename + strlen(cuename) - 4, ".cue");
	}
	else {
		return -1;
	}

	if ((fi = fopen(cuename, "r")) == NULL) {
		return -1;
	}

	// Some stupid tutorials wrongly tell users to use cdrdao to rip a
	// "bin/cue" image, which is in fact a "bin/toc" image. So let's check
	// that...
	if (fgets(linebuf, sizeof(linebuf), fi) != NULL) {
		if (!strncmp(linebuf, "CD_ROM_XA", 9)) {
			// Don't proceed further, as this is actually a .toc file rather
			// than a .cue file.
			fclose(fi);
			return parsetoc(isofile);
		}
		fseek(fi, 0, SEEK_SET);
	}

	// build a path for files referenced in .cue
	strncpy(filepath, cuename, sizeof(filepath));
	tmp = strrchr(filepath, '/');
	if (tmp == NULL)
		tmp = strrchr(filepath, '\\');
	if (tmp != NULL)
		tmp++;
	else
		tmp = filepath;
	*tmp = 0;
	filepath[sizeof(filepath) - 1] = 0;
	incue_fname = tmp;
	incue_max_len = sizeof(filepath) - (tmp - filepath) - 1;

	memset(&ti, 0, sizeof(ti));

	file_len = 0;
	sector_offs = 2 * 75;

	while (fgets(linebuf, sizeof(linebuf), fi) != NULL) {
		strncpy(dummy, linebuf, sizeof(linebuf));
		token = strtok(dummy, " ");

		if (token == NULL) {
			continue;
		}

		if (!strcmp(token, "TRACK")) {
			numtracks++;

			sector_size = 0;
			if (strstr(linebuf, "AUDIO") != NULL) {
				ti[numtracks].type = CDDA;
				sector_size = CD_FRAMESIZE_RAW;
				// Check if extension is mp3, etc, for compressed audio formats
				if (multifile && (ti[numtracks].cddatype = get_cdda_type(filepath)) > BIN) {
					int seconds = get_compressed_cdda_track_length(filepath) + 0;
					const boolean lazy_decode = TRUE; // TODO: config param

					// TODO: get frame length for compressed audio as well
					ti[numtracks].len_decoded_buffer = 44100 * (16/8) * 2 * seconds;
					strcpy(ti[numtracks].filepath, filepath);
					file_len = ti[numtracks].len_decoded_buffer/CD_FRAMESIZE_RAW;

					// Send to decoder if not lazy decoding
					if (!lazy_decode) {
						do_decode_cdda(&(ti[numtracks]), numtracks);
						fseek(ti[numtracks].handle, 0, SEEK_END);
						file_len = ftell(ti[numtracks].handle) / CD_FRAMESIZE_RAW; // accurate length
					}
				}
			}
			else if (sscanf(linebuf, " TRACK %u MODE%u/%u", &t, &mode, &sector_size) == 3) {
				s32 accurate_len;
				// TODO: if 2048 frame length -> recalculate file_len?
				ti[numtracks].type = DATA;
				// detect if ECM or compressed & get accurate length
				if (handleecm(filepath, cdHandle, &accurate_len) == 0 ||
					handlearchive(filepath, &accurate_len) == 0) {
					file_len = accurate_len;
				}
			} else {
				SysPrintf(".cue: failed to parse TRACK\n");
				ti[numtracks].type = numtracks == 1 ? DATA : CDDA;
			}
			if (sector_size == 0) // TODO isMode1ISO?
				sector_size = CD_FRAMESIZE_RAW;
		}
		else if (!strcmp(token, "INDEX")) {
			if (sscanf(linebuf, " INDEX %02d %8s", &t, time) != 2)
				SysPrintf(".cue: failed to parse INDEX\n");
			tok2msf(time, (char *)&ti[numtracks].start);

			t = msf2sec(ti[numtracks].start);
			ti[numtracks].start_offset = t * sector_size;
			t += sector_offs;
			sec2msf(t, ti[numtracks].start);

			// default track length to file length
			t = file_len - ti[numtracks].start_offset / sector_size;
			sec2msf(t, ti[numtracks].length);

			if (numtracks > 1 && ti[numtracks].handle == NULL) {
				// this track uses the same file as the last,
				// start of this track is last track's end
				t = msf2sec(ti[numtracks].start) - msf2sec(ti[numtracks - 1].start);
				sec2msf(t, ti[numtracks - 1].length);
			}
			if (numtracks > 1 && pregapOffset == -1)
				pregapOffset = ti[numtracks].start_offset / sector_size;
		}
		else if (!strcmp(token, "PREGAP")) {
			if (sscanf(linebuf, " PREGAP %8s", time) == 1) {
				tok2msf(time, dummy);
				sector_offs += msf2sec(dummy);
			}
			pregapOffset = -1; // mark to fill track start_offset
		}
		else if (!strcmp(token, "FILE")) {
			t = sscanf(linebuf, " FILE \"%255[^\"]\"", tmpb);
			if (t != 1)
				sscanf(linebuf, " FILE %255s", tmpb);

			// absolute path?
			ti[numtracks + 1].handle = fopen(tmpb, "rb");
			if (ti[numtracks + 1].handle == NULL) {
				// relative to .cue?
				tmp = strrchr(tmpb, '\\');
				if (tmp == NULL)
					tmp = strrchr(tmpb, '/');
				if (tmp != NULL)
					tmp++;
				else
					tmp = tmpb;
				strncpy(incue_fname, tmp, incue_max_len);
				ti[numtracks + 1].handle = fopen(filepath, "rb");
			}

			// update global offset if this is not first file in this .cue
			if (numtracks + 1 > 1) {
				multifile = TRUE;
				sector_offs += file_len;
			}

			file_len = 0;
			if (ti[numtracks + 1].handle == NULL) {
				SysMessage(_("\ncould not open: %s\n"), filepath);
				continue;
			}

			// File length, compressed audio length will be calculated in AUDIO tag
			fseek(ti[numtracks + 1].handle, 0, SEEK_END);
			file_len = ftell(ti[numtracks + 1].handle) / CD_FRAMESIZE_RAW;

			if (numtracks == 0 && strlen(isofile) >= 4 &&
				strcmp(isofile + strlen(isofile) - 4, ".cue") == 0)
			{
				// user selected .cue as image file, use its data track instead
				fclose(cdHandle);
				cdHandle = fopen(filepath, "rb");
			}
		}
	}

	fclose(fi);

	return 0;
}

// this function tries to get the .ccd file of the given .img
// the necessary data is put into the ti (trackinformation)-array
static int parseccd(const char *isofile) {
	char			ccdname[MAXPATHLEN];
	FILE			*fi;
	char			linebuf[256];
	unsigned int	t;

	numtracks = 0;

	// copy name of the iso and change extension from .img to .ccd
	strncpy(ccdname, isofile, sizeof(ccdname));
	ccdname[MAXPATHLEN - 1] = '\0';
	if (strlen(ccdname) >= 4) {
		strcpy(ccdname + strlen(ccdname) - 4, ".ccd");
	}
	else {
		return -1;
	}

	if ((fi = fopen(ccdname, "r")) == NULL) {
		return -1;
	}

	memset(&ti, 0, sizeof(ti));

	while (fgets(linebuf, sizeof(linebuf), fi) != NULL) {
		if (!strncmp(linebuf, "[TRACK", 6)){
			numtracks++;
		}
		else if (!strncmp(linebuf, "MODE=", 5)) {
			sscanf(linebuf, "MODE=%d", &t);
			ti[numtracks].type = ((t == 0) ? CDDA : DATA);
		}
		else if (!strncmp(linebuf, "INDEX 1=", 8)) {
			sscanf(linebuf, "INDEX 1=%d", &t);
			sec2msf(t + 2 * 75, ti[numtracks].start);
			ti[numtracks].start_offset = t * 2352;

			// If we've already seen another track, this is its end
			if (numtracks > 1) {
				t = msf2sec(ti[numtracks].start) - msf2sec(ti[numtracks - 1].start);
				sec2msf(t, ti[numtracks - 1].length);
			}
		}
	}

	fclose(fi);

	// Fill out the last track's end based on size
	if (numtracks >= 1) {
		fseek(cdHandle, 0, SEEK_END);
		t = ftell(cdHandle) / CD_FRAMESIZE_RAW - msf2sec(ti[numtracks].start) + 2 * 75;
		sec2msf(t, ti[numtracks].length);
	}

	return 0;
}

// this function tries to get the .mds file of the given .mdf
// the necessary data is put into the ti (trackinformation)-array
static int parsemds(const char *isofile) {
	char			mdsname[MAXPATHLEN];
	FILE			*fi;
	unsigned int	offset, extra_offset, l, i;
	unsigned short	s;

	numtracks = 0;

	// copy name of the iso and change extension from .mdf to .mds
	strncpy(mdsname, isofile, sizeof(mdsname));
	mdsname[MAXPATHLEN - 1] = '\0';
	if (strlen(mdsname) >= 4) {
		strcpy(mdsname + strlen(mdsname) - 4, ".mds");
	}
	else {
		return -1;
	}

	if ((fi = fopen(mdsname, "rb")) == NULL) {
		return -1;
	}

	memset(&ti, 0, sizeof(ti));

	// check if it's a valid mds file
	fread(&i, 1, sizeof(unsigned int), fi);
	i = SWAP32(i);
	if (i != 0x4944454D) {
		// not an valid mds file
		fclose(fi);
		return -1;
	}

	// get offset to session block
	fseek(fi, 0x50, SEEK_SET);
	fread(&offset, 1, sizeof(unsigned int), fi);
	offset = SWAP32(offset);

	// get total number of tracks
	offset += 14;
	fseek(fi, offset, SEEK_SET);
	fread(&s, 1, sizeof(unsigned short), fi);
	s = SWAP16(s);
	numtracks = s;

	// get offset to track blocks
	fseek(fi, 4, SEEK_CUR);
	fread(&offset, 1, sizeof(unsigned int), fi);
	offset = SWAP32(offset);

	// skip lead-in data
	while (1) {
		fseek(fi, offset + 4, SEEK_SET);
		if (fgetc(fi) < 0xA0) {
			break;
		}
		offset += 0x50;
	}

	// check if the image contains mixed subchannel data
	fseek(fi, offset + 1, SEEK_SET);
	subChanMixed = subChanRaw = (fgetc(fi) ? TRUE : FALSE);

	// read track data
	for (i = 1; i <= numtracks; i++) {
		fseek(fi, offset, SEEK_SET);

		// get the track type
		ti[i].type = ((fgetc(fi) == 0xA9) ? CDDA : DATA);
		fseek(fi, 8, SEEK_CUR);

		// get the track starting point
		ti[i].start[0] = fgetc(fi);
		ti[i].start[1] = fgetc(fi);
		ti[i].start[2] = fgetc(fi);

		fread(&extra_offset, 1, sizeof(unsigned int), fi);
		extra_offset = SWAP32(extra_offset);

		// get track start offset (in .mdf)
		fseek(fi, offset + 0x28, SEEK_SET);
		fread(&l, 1, sizeof(unsigned int), fi);
		l = SWAP32(l);
		ti[i].start_offset = l;

		// get pregap
		fseek(fi, extra_offset, SEEK_SET);
		fread(&l, 1, sizeof(unsigned int), fi);
		l = SWAP32(l);
		if (l != 0 && i > 1)
			pregapOffset = msf2sec(ti[i].start);

		// get the track length
		fread(&l, 1, sizeof(unsigned int), fi);
		l = SWAP32(l);
		sec2msf(l, ti[i].length);

		offset += 0x50;
	}

	fclose(fi);
	return 0;
}

static int handlepbp(const char *isofile) {
	struct {
		unsigned int sig;
		unsigned int dontcare[8];
		unsigned int psar_offs;
	} pbp_hdr;
	struct {
		unsigned char type;
		unsigned char pad0;
		unsigned char track;
		char index0[3];
		char pad1;
		char index1[3];
	} toc_entry;
	struct {
		unsigned int offset;
		unsigned int size;
		unsigned int dontcare[6];
	} index_entry;
	char psar_sig[11];
	unsigned int t, cd_length, cdimg_base;
	unsigned int offsettab[8], psisoimg_offs;
	const char *ext = NULL;
	int i, ret;

	if (strlen(isofile) >= 4)
		ext = isofile + strlen(isofile) - 4;
	if (ext == NULL || (strcmp(ext, ".pbp") != 0 && strcmp(ext, ".PBP") != 0))
		return -1;
	
	fseek(cdHandle, 0, SEEK_SET);

	numtracks = 0;

	ret = fread(&pbp_hdr, 1, sizeof(pbp_hdr), cdHandle);
	if (ret != sizeof(pbp_hdr)) {
		SysPrintf("failed to read pbp\n");
		goto fail_io;
	}

	ret = fseek(cdHandle, pbp_hdr.psar_offs, SEEK_SET);
	if (ret != 0) {
		SysPrintf("failed to seek to %x\n", pbp_hdr.psar_offs);
		goto fail_io;
	}

	psisoimg_offs = pbp_hdr.psar_offs;
	fread(psar_sig, 1, sizeof(psar_sig), cdHandle);
	psar_sig[10] = 0;
	if (strcmp(psar_sig, "PSTITLEIMG") == 0) {
		// multidisk image?
		ret = fseek(cdHandle, pbp_hdr.psar_offs + 0x200, SEEK_SET);
		if (ret != 0) {
			SysPrintf("failed to seek to %x\n", pbp_hdr.psar_offs + 0x200);
			goto fail_io;
		}

		if (fread(&offsettab, 1, sizeof(offsettab), cdHandle) != sizeof(offsettab)) {
			SysPrintf("failed to read offsettab\n");
			goto fail_io;
		}

		for (i = 0; i < sizeof(offsettab) / sizeof(offsettab[0]); i++) {
			if (offsettab[i] == 0)
				break;
		}
		cdrIsoMultidiskCount = i;
		if (cdrIsoMultidiskCount == 0) {
			SysPrintf("multidisk eboot has 0 images?\n");
			goto fail_io;
		}

		if (cdrIsoMultidiskSelect >= cdrIsoMultidiskCount)
			cdrIsoMultidiskSelect = 0;

		psisoimg_offs += offsettab[cdrIsoMultidiskSelect];

		ret = fseek(cdHandle, psisoimg_offs, SEEK_SET);
		if (ret != 0) {
			SysPrintf("failed to seek to %x\n", psisoimg_offs);
			goto fail_io;
		}

		fread(psar_sig, 1, sizeof(psar_sig), cdHandle);
		psar_sig[10] = 0;
	}

	if (strcmp(psar_sig, "PSISOIMG00") != 0) {
		SysPrintf("bad psar_sig: %s\n", psar_sig);
		goto fail_io;
	}

	// seek to TOC
	ret = fseek(cdHandle, psisoimg_offs + 0x800, SEEK_SET);
	if (ret != 0) {
		SysPrintf("failed to seek to %x\n", psisoimg_offs + 0x800);
		goto fail_io;
	}

	// first 3 entries are special
	fseek(cdHandle, sizeof(toc_entry), SEEK_CUR);
	fread(&toc_entry, 1, sizeof(toc_entry), cdHandle);
	numtracks = btoi(toc_entry.index1[0]);

	fread(&toc_entry, 1, sizeof(toc_entry), cdHandle);
	cd_length = btoi(toc_entry.index1[0]) * 60 * 75 +
		btoi(toc_entry.index1[1]) * 75 + btoi(toc_entry.index1[2]);

	for (i = 1; i <= numtracks; i++) {
		fread(&toc_entry, 1, sizeof(toc_entry), cdHandle);

		ti[i].type = (toc_entry.type == 1) ? CDDA : DATA;

		ti[i].start_offset = btoi(toc_entry.index0[0]) * 60 * 75 +
			btoi(toc_entry.index0[1]) * 75 + btoi(toc_entry.index0[2]);
		ti[i].start_offset *= 2352;
		ti[i].start[0] = btoi(toc_entry.index1[0]);
		ti[i].start[1] = btoi(toc_entry.index1[1]);
		ti[i].start[2] = btoi(toc_entry.index1[2]);

		if (i > 1) {
			t = msf2sec(ti[i].start) - msf2sec(ti[i - 1].start);
			sec2msf(t, ti[i - 1].length);
		}
	}
	t = cd_length - ti[numtracks].start_offset / 2352;
	sec2msf(t, ti[numtracks].length);

	// seek to ISO index
	ret = fseek(cdHandle, psisoimg_offs + 0x4000, SEEK_SET);
	if (ret != 0) {
		SysPrintf("failed to seek to ISO index\n");
		goto fail_io;
	}

	compr_img = calloc(1, sizeof(*compr_img));
	if (compr_img == NULL)
		goto fail_io;

	compr_img->block_shift = 4;
	compr_img->current_block = (unsigned int)-1;

	compr_img->index_len = (0x100000 - 0x4000) / sizeof(index_entry);
	compr_img->index_table = malloc((compr_img->index_len + 1) * sizeof(compr_img->index_table[0]));
	if (compr_img->index_table == NULL)
		goto fail_io;

	cdimg_base = psisoimg_offs + 0x100000;
	for (i = 0; i < compr_img->index_len; i++) {
		ret = fread(&index_entry, 1, sizeof(index_entry), cdHandle);
		if (ret != sizeof(index_entry)) {
			SysPrintf("failed to read index_entry #%d\n", i);
			goto fail_index;
		}

		if (index_entry.size == 0)
			break;

		compr_img->index_table[i] = cdimg_base + index_entry.offset;
	}
	compr_img->index_table[i] = cdimg_base + index_entry.offset + index_entry.size;

	return 0;

fail_index:
	free(compr_img->index_table);
	compr_img->index_table = NULL;
fail_io:
	if (compr_img != NULL) {
		free(compr_img);
		compr_img = NULL;
	}
	return -1;
}

static int handlecbin(const char *isofile) {
	struct
	{
		char magic[4];
		unsigned int header_size;
		unsigned long long total_bytes;
		unsigned int block_size;
		unsigned char ver;		// 1
		unsigned char align;
		unsigned char rsv_06[2];
	} ciso_hdr;
	const char *ext = NULL;
	unsigned int index = 0, plain;
	int i, ret;

	if (strlen(isofile) >= 5)
		ext = isofile + strlen(isofile) - 5;
	if (ext == NULL || (strcasecmp(ext + 1, ".cbn") != 0 && strcasecmp(ext, ".cbin") != 0))
		return -1;
	
	fseek(cdHandle, 0, SEEK_SET);

	ret = fread(&ciso_hdr, 1, sizeof(ciso_hdr), cdHandle);
	if (ret != sizeof(ciso_hdr)) {
		SysPrintf("failed to read ciso header\n");
		return -1;
	}

	if (strncmp(ciso_hdr.magic, "CISO", 4) != 0 || ciso_hdr.total_bytes <= 0 || ciso_hdr.block_size <= 0) {
		SysPrintf("bad ciso header\n");
		return -1;
	}
	if (ciso_hdr.header_size != 0 && ciso_hdr.header_size != sizeof(ciso_hdr)) {
		ret = fseek(cdHandle, ciso_hdr.header_size, SEEK_SET);
		if (ret != 0) {
			SysPrintf("failed to seek to %x\n", ciso_hdr.header_size);
			return -1;
		}
	}

	compr_img = calloc(1, sizeof(*compr_img));
	if (compr_img == NULL)
		goto fail_io;

	compr_img->block_shift = 0;
	compr_img->current_block = (unsigned int)-1;

	compr_img->index_len = ciso_hdr.total_bytes / ciso_hdr.block_size;
	compr_img->index_table = malloc((compr_img->index_len + 1) * sizeof(compr_img->index_table[0]));
	if (compr_img->index_table == NULL)
		goto fail_io;

	ret = fread(compr_img->index_table, sizeof(compr_img->index_table[0]), compr_img->index_len, cdHandle);
	if (ret != compr_img->index_len) {
		SysPrintf("failed to read index table\n");
		goto fail_index;
	}

	for (i = 0; i < compr_img->index_len + 1; i++) {
		index = compr_img->index_table[i];
		plain = index & 0x80000000;
		index &= 0x7fffffff;
		compr_img->index_table[i] = (index << ciso_hdr.align) | plain;
	}
	if ((long long)index << ciso_hdr.align >= 0x80000000ll)
		SysPrintf("warning: ciso img too large, expect problems\n");

	return 0;

fail_index:
	free(compr_img->index_table);
	compr_img->index_table = NULL;
fail_io:
	if (compr_img != NULL) {
		free(compr_img);
		compr_img = NULL;
	}
	return -1;
}

// this function tries to get the .sub file of the given .img
static int opensubfile(const char *isoname) {
	char		subname[MAXPATHLEN];

	// copy name of the iso and change extension from .img to .sub
	strncpy(subname, isoname, sizeof(subname));
	subname[MAXPATHLEN - 1] = '\0';

	if (strlen(subname) >= 4) {
		strcpy(subname + strlen(subname) - 4, ".sub");
	}

	subHandle = fopen(subname, "rb");
	if (subHandle != NULL) {
		return 0;
	}

	if (strlen(subname) >= 8) {
		strcpy(subname + strlen(subname) - 8, ".sub");
	}

	subHandle = fopen(subname, "rb");
	if (subHandle == NULL) {
		return -1;
	}

	return 0;
}

static int opensbifile(const char *isoname) {
	char		sbiname[MAXPATHLEN];

	strncpy(sbiname, isoname, sizeof(sbiname));
	sbiname[MAXPATHLEN - 1] = '\0';
	if (strlen(sbiname) >= 4) {
		strcpy(sbiname + strlen(sbiname) - 4, ".sbi");
	}
	else {
		return -1;
	}

	return LoadSBI(sbiname);
}

static int cdread_normal(FILE *f, unsigned int base, void *dest, int sector)
{
	fseek(f, base + sector * CD_FRAMESIZE_RAW, SEEK_SET);
	return fread(dest, 1, CD_FRAMESIZE_RAW, f);
}

static int cdread_sub_mixed(FILE *f, unsigned int base, void *dest, int sector)
{
	int ret;

	fseek(f, base + sector * (CD_FRAMESIZE_RAW + SUB_FRAMESIZE), SEEK_SET);
	ret = fread(dest, 1, CD_FRAMESIZE_RAW, f);
	fread(subbuffer, 1, SUB_FRAMESIZE, f);

	if (subChanRaw) DecodeRawSubData();

	return ret;
}

static int cdriso_uncompress2(void *out, unsigned long *out_size, void *in, unsigned long in_size)
{
	static z_stream z;
	int ret = 0;

	if (z.zalloc == NULL) {
		// XXX: one-time leak here..
		z.next_in = Z_NULL;
		z.avail_in = 0;
		z.zalloc = Z_NULL;
		z.zfree = Z_NULL;
		z.opaque = Z_NULL;
		ret = inflateInit2(&z, -15);
	}
	else
		ret = inflateReset(&z);
	if (ret != Z_OK)
		return ret;

	z.next_in = in;
	z.avail_in = in_size;
	z.next_out = out;
	z.avail_out = *out_size;

	ret = inflate(&z, Z_NO_FLUSH);
	//inflateEnd(&z);

	*out_size -= z.avail_out;
	return ret == 1 ? 0 : ret;
}

static int cdread_compressed(FILE *f, unsigned int base, void *dest, int sector)
{
	unsigned long cdbuffer_size, cdbuffer_size_expect;
	unsigned int start_byte, size;
	int is_compressed;
	int ret, block;

	if (base)
		sector += base / 2352;

	block = sector >> compr_img->block_shift;
	compr_img->sector_in_blk = sector & ((1 << compr_img->block_shift) - 1);

	if (block == compr_img->current_block) {
		//printf("hit sect %d\n", sector);
		goto finish;
	}

	if (sector >= compr_img->index_len * 16) {
		SysPrintf("sector %d is past img end\n", sector);
		return -1;
	}

	start_byte = compr_img->index_table[block] & 0x7fffffff;
	if (fseek(cdHandle, start_byte, SEEK_SET) != 0) {
		SysPrintf("seek error for block %d at %x: ",
			block, start_byte);
		perror(NULL);
		return -1;
	}

	is_compressed = !(compr_img->index_table[block] & 0x80000000);
	size = (compr_img->index_table[block + 1] & 0x7fffffff) - start_byte;
	if (size > sizeof(compr_img->buff_compressed)) {
		SysPrintf("block %d is too large: %u\n", block, size);
		return -1;
	}

	if (fread(is_compressed ? compr_img->buff_compressed : compr_img->buff_raw[0],
				1, size, cdHandle) != size) {
		SysPrintf("read error for block %d at %x: ", block, start_byte);
		perror(NULL);
		return -1;
	}

	if (is_compressed) {
		cdbuffer_size_expect = sizeof(compr_img->buff_raw[0]) << compr_img->block_shift;
		cdbuffer_size = cdbuffer_size_expect;
		ret = cdriso_uncompress2(compr_img->buff_raw[0], &cdbuffer_size, compr_img->buff_compressed, size);
		if (ret != 0) {
			SysPrintf("uncompress failed with %d for block %d, sector %d\n",
					ret, block, sector);
			return -1;
		}
		if (cdbuffer_size != cdbuffer_size_expect)
			SysPrintf("cdbuffer_size: %lu != %lu, sector %d\n", cdbuffer_size,
					cdbuffer_size_expect, sector);
	}

	// done at last!
	compr_img->current_block = block;

finish:
	if (dest != cdbuffer) // copy avoid HACK
		memcpy(dest, compr_img->buff_raw[compr_img->sector_in_blk],
			CD_FRAMESIZE_RAW);
	return CD_FRAMESIZE_RAW;
}

static int cdread_2048(FILE *f, unsigned int base, void *dest, int sector)
{
	int ret;

	fseek(f, base + sector * 2048, SEEK_SET);
	ret = fread((char *)dest + 12 * 2, 1, 2048, f);

	// not really necessary, fake mode 2 header
	memset(cdbuffer, 0, 12 * 2);
	sec2msf(sector + 2 * 75, (char *)&cdbuffer[12]);
	cdbuffer[12 + 3] = 1;

	return ret;
}

/* Adapted from ecm.c:unecmify() (C) Neill Corlett */
//TODO: move this func to ecm.h
static int cdread_ecm_decode(FILE *f, unsigned int base, void *dest, int sector) {
	u32 output_edc=0, b=0, writebytecount=0, num;
	s32 sectorcount=0;
	s8 type = 0; // mode type 0 (META) or 1, 2 or 3 for CDROM type
	u8 sector_buffer[CD_FRAMESIZE_RAW];
	boolean processsectors = (boolean)decoded_ecm_sectors; // this flag tells if to decode all sectors or just skip to wanted sector
	ECMFILELUT* pos = &(ecm_savetable[0]); // points always to beginning of ECM DATA

	// If not pointing to ECM file but CDDA file or some other track
	if(f != cdHandle) {
		//printf("BASETR %i %i\n", base, sector);
		return cdimg_read_func_o(f, base, dest, sector);
	}
	// When sector exists in decoded ECM file buffer
	else if (decoded_ecm_sectors && sector < decoded_ecm_sectors) {
		//printf("ReadSector %i %i\n", sector, savedsectors);
		return cdimg_read_func_o(decoded_ecm, base, dest, sector);
	}
	// To prevent invalid seek
	/* else if (sector > len_ecm_savetable) {
		SysPrintf("ECM: invalid sector requested\n");
		return -1;
	}*/
	//printf("SeekSector %i %i %i %i\n", sector, pos->sector, prevsector, base);


	if (sector <= len_ecm_savetable) {
		// get sector from LUT which points to wanted sector or close to
		// TODO: What would be optimal maximum to search near sector?
		//       Might cause slowdown if too small but too big also..
		for (sectorcount = sector; ((sectorcount > 0) && ((sector-sectorcount) <= 50000)); sectorcount--) {
			if (ecm_savetable[sectorcount].filepos >= ECM_HEADER_SIZE) {
				pos = &(ecm_savetable[sectorcount]);
				//printf("LUTSector %i %i %i %i\n", sector, pos->sector, prevsector, base);
				break;
			}
		}
		// if suitable sector was not found from LUT use last sector if less than wanted sector
		if (pos->filepos <= ECM_HEADER_SIZE && sector > prevsector) pos=&(ecm_savetable[prevsector]);
	}

	writebytecount = pos->sector * CD_FRAMESIZE_RAW;
	sectorcount = pos->sector;
	if (decoded_ecm_sectors) fseek(decoded_ecm, writebytecount, SEEK_SET); // rewind to last pos
	fseek(f, /*base+*/pos->filepos, SEEK_SET);
	while(sector >= sectorcount) { // decode ecm file until we are past wanted sector
		int c = fgetc(f);
		int bits = 5;
		if(c == EOF) { goto error_in; }
		type = c & 3;
		num = (c >> 2) & 0x1F;
		//printf("ECM1 file; count %x\n", c);
		while(c & 0x80) {
			c = fgetc(f);
			//printf("ECM2 file; count %x\n", c);
			if(c == EOF) { goto error_in; }
			if( (bits > 31) ||
					((uint32_t)(c & 0x7F)) >= (((uint32_t)0x80000000LU) >> (bits-1))
					) {
				//SysMessage(_("Corrupt ECM file; invalid sector count\n"));
				goto error;
			}
			num |= ((uint32_t)(c & 0x7F)) << bits;
			bits += 7;
		}
		if(num == 0xFFFFFFFF) {
			// End indicator
			len_decoded_ecm_buffer = writebytecount;
			len_ecm_savetable = len_decoded_ecm_buffer/CD_FRAMESIZE_RAW;
			break;
		}
		num++;
		while(num) {
			if (!processsectors && sectorcount >= (sector-1)) { // ensure that we read the sector we are supposed to
				processsectors = TRUE;
				//printf("Saving at %i\n", sectorcount);
			} else if (processsectors && sectorcount > sector) {
				//printf("Terminating at %i\n", sectorcount);
				break;
			}
			/*printf("Type %i Num %i SeekSector %i ProcessedSectors %i(%i) Bytecount %i Pos %li Write %u\n",
					type, num, sector, sectorcount, pos->sector, writebytecount, ftell(f), processsectors);*/
			switch(type) {
			case 0: // META
				b = num;
				if(b > sizeof(sector_buffer)) { b = sizeof(sector_buffer); }
				writebytecount += b;
				if (!processsectors) { fseek(f, +b, SEEK_CUR); break; } // seek only
				if(fread(sector_buffer, 1, b, f) != b) {
					goto error_in;
				}
				//output_edc = edc_compute(output_edc, sector_buffer, b);
				if(decoded_ecm_sectors && fwrite(sector_buffer, 1, b, decoded_ecm) != b) { // just seek or write also
					goto error_out;
				}
				break;
			case 1: //Mode 1
				b=1;
				writebytecount += ECM_SECTOR_SIZE[type];
				if(fread(sector_buffer + 0x00C, 1, 0x003, f) != 0x003) { goto error_in; }
				if(fread(sector_buffer + 0x010, 1, 0x800, f) != 0x800) { goto error_in; }
				if (!processsectors) break; // seek only
				reconstruct_sector(sector_buffer, type);
				//output_edc = edc_compute(output_edc, sector_buffer, ECM_SECTOR_SIZE[type]);
				if(decoded_ecm_sectors && fwrite(sector_buffer, 1, ECM_SECTOR_SIZE[type], decoded_ecm) != ECM_SECTOR_SIZE[type]) { goto error_out; }
				break;
			case 2: //Mode 2 (XA), form 1
				b=1;
				writebytecount += ECM_SECTOR_SIZE[type];
				if (!processsectors) { fseek(f, +0x804, SEEK_CUR); break; } // seek only
				if(fread(sector_buffer + 0x014, 1, 0x804, f) != 0x804) { goto error_in; }
				reconstruct_sector(sector_buffer, type);
				//output_edc = edc_compute(output_edc, sector_buffer + 0x10, ECM_SECTOR_SIZE[type]);
				if(decoded_ecm_sectors && fwrite(sector_buffer + 0x10, 1, ECM_SECTOR_SIZE[type], decoded_ecm) != ECM_SECTOR_SIZE[type]) { goto error_out; }
				break;
			case 3: //Mode 2 (XA), form 2
				b=1;
				writebytecount += ECM_SECTOR_SIZE[type];
				if (!processsectors) { fseek(f, +0x918, SEEK_CUR); break; } // seek only
				if(fread(sector_buffer + 0x014, 1, 0x918, f) != 0x918) { goto error_in; }
				reconstruct_sector(sector_buffer, type);
				//output_edc = edc_compute(output_edc, sector_buffer + 0x10, ECM_SECTOR_SIZE[type]);
				if(decoded_ecm_sectors && fwrite(sector_buffer + 0x10, 1, ECM_SECTOR_SIZE[type], decoded_ecm) != ECM_SECTOR_SIZE[type]) { goto error_out; }
				break;
			}
			sectorcount=((writebytecount/CD_FRAMESIZE_RAW) - 0);
			num -= b;
		}
		if (type && sectorcount > 0 && ecm_savetable[sectorcount].filepos <= ECM_HEADER_SIZE ) {
			ecm_savetable[sectorcount].filepos = ftell(f)/*-base*/;
			ecm_savetable[sectorcount].sector = sectorcount;
			//printf("Marked %i at pos %i\n", ecm_savetable[sectorcount].sector, ecm_savetable[sectorcount].filepos);
		}
	}

	if (decoded_ecm_sectors) {
		fflush(decoded_ecm);
		fseek(decoded_ecm, -1*CD_FRAMESIZE_RAW, SEEK_CUR);
		num = fread(sector_buffer, 1, CD_FRAMESIZE_RAW, decoded_ecm);
		decoded_ecm_sectors = MAX(decoded_ecm_sectors, sectorcount);
	} else {
		num = CD_FRAMESIZE_RAW;
	}

	memcpy(dest, sector_buffer, CD_FRAMESIZE_RAW);
	prevsector = sectorcount;
	//printf("OK: Frame decoded %i %i\n", sectorcount-1, writebytecount);
	return num;

error_in:
error:
error_out:
	//memset(dest, 0x0, CD_FRAMESIZE_RAW);
	SysPrintf("Error decoding ECM image: WantedSector %i Type %i Base %i Sectors %i(%i) Pos %i(%li)\n",
				sector, type, base, sectorcount, pos->sector, writebytecount, ftell(f));
	return -1;
}

int handleecm(const char *isoname, FILE* cdh, s32* accurate_length) {
	// Rewind to start and check ECM header and filename suffix validity
	fseek(cdh, 0, SEEK_SET);
	if(
		(fgetc(cdh) == 'E') &&
		(fgetc(cdh) == 'C') &&
		(fgetc(cdh) == 'M') &&
		(fgetc(cdh) == 0x00) &&
		(strncmp((isoname+strlen(isoname)-5), ".ecm", 4))
	) {
		// Function used to read CD normally
		// TODO: detect if 2048 and use it
		cdimg_read_func_o = cdread_normal;

		// Function used to decode ECM data
		cdimg_read_func = cdread_ecm_decode;

		// Last accessed sector
		prevsector = 0;

		// Already analyzed during this session, use cached results
		if (ecm_file_detected) {
			if (accurate_length) *accurate_length = len_ecm_savetable;
			return 0;
		}

		SysPrintf(_("\nDetected ECM file with proper header and filename suffix.\n"));

		// Init ECC/EDC tables
		eccedc_init();

		// Reserve maximum known sector ammount for LUT (80MIN CD)
		len_ecm_savetable = 75*80*60; //2*(accurate_length/CD_FRAMESIZE_RAW);

		// Index 0 always points to beginning of ECM data
		ecm_savetable = calloc(len_ecm_savetable, sizeof(ECMFILELUT)); // calloc returns nulled data
		ecm_savetable[0].filepos = ECM_HEADER_SIZE;

		if (accurate_length || decoded_ecm_sectors) {
			u8 tbuf1[CD_FRAMESIZE_RAW];
			len_ecm_savetable = 0; // indicates to cdread_ecm_decode that no lut has been built yet
			cdread_ecm_decode(cdh, 0U, tbuf1, INT_MAX); // builds LUT completely
			if (accurate_length)*accurate_length = len_ecm_savetable;
		}

		// Full image decoded? Needs fmemopen()
#ifdef ENABLE_ECM_FULL
		if (decoded_ecm_sectors) {
			len_decoded_ecm_buffer = len_ecm_savetable*CD_FRAMESIZE_RAW;
			decoded_ecm_buffer = malloc(len_decoded_ecm_buffer);
			if (decoded_ecm_buffer) {
				//printf("Memory ok1 %u %p\n", len_decoded_ecm_buffer, decoded_ecm_buffer);
				decoded_ecm = fmemopen(decoded_ecm_buffer, len_decoded_ecm_buffer, "w+b");
				decoded_ecm_sectors = 1;
			} else {
				SysMessage("Could not reserve memory for full ECM buffer. Only LUT will be used.");
				decoded_ecm_sectors = 0;
			}
		}
#endif

		ecm_file_detected = TRUE;

		return 0;
	}
	return -1;
}

int (*cdimg_read_func_archive)(FILE *f, unsigned int base, void *dest, int sector) = NULL;
#ifdef HAVE_LIBARCHIVE
#include <archive.h>
#include <archive_entry.h>

struct archive *a = NULL;
u32 len_uncompressed_buffer = 0;
void *cdimage_buffer_mem = NULL;
FILE* cdimage_buffer = NULL; //cdHandle to store file

int aropen(FILE* fparchive, const char* _fn) {
	s32 r;
	u64 length = 0, length_peek;
	boolean use_temp_file = FALSE; // TODO make a config param
	static struct archive_entry *ae=NULL;
	struct archive_entry *ae_peek;

	if (a == NULL && cdimage_buffer == NULL) {
		// We open file twice. First to peek sizes. This nastyness due used interface.
		a = archive_read_new();
		r = archive_read_support_compression_all(a);
		r = archive_read_support_format_all(a);
		//r = archive_read_support_filter_all(a);
		//r = archive_read_support_format_raw(a);
		//r = archive_read_open_FILE(a, archive);
		archive_read_open_filename(a, _fn, 75*CD_FRAMESIZE_RAW);
		if (r != ARCHIVE_OK) {
			SysPrintf("Archive open failed (%i).\n", r);
			archive_read_free(a);
			a = NULL;
			return -1;
		}
		// Get the biggest file in archive
		while ((r=archive_read_next_header(a, &ae_peek)) == ARCHIVE_OK) {
			length_peek = archive_entry_size(ae_peek);
			//printf("Entry canditate %s %i\n", archive_entry_pathname(ae_peek), length_peek);
			length = MAX(length_peek, length);
			ae = (ae == NULL ? ae_peek : ae);
		}
		archive_read_free(a);
		if (ae == NULL) {
			SysPrintf("Archive entry read failed (%i).\n", r);
			a = NULL;
			return -1;
		}
		//Now really open the file
		a = archive_read_new();
		r = archive_read_support_compression_all(a);
		r = archive_read_support_format_all(a);
		archive_read_open_filename(a, _fn, 75*CD_FRAMESIZE_RAW);
		while ((r=archive_read_next_header(a, &ae)) == ARCHIVE_OK) {
			length_peek = archive_entry_size(ae);
			if (length_peek == length) {
				//ae = ae_peek;
				SysPrintf(" -- Selected entry %s %i", archive_entry_pathname(ae), length);
				break;
			}
		}

		len_uncompressed_buffer = length?length:700*1024*1024;
	}

	if (use_temp_file && (cdimage_buffer == NULL || cdHandle != cdimage_buffer)) {
		cdimage_buffer = fopen("/tmp/pcsxr.tmp.bin", "w+b");
	}
	else if (!use_temp_file && (cdimage_buffer == NULL || cdHandle != cdimage_buffer)) {
		if (cdimage_buffer_mem == NULL && ((cdimage_buffer_mem = malloc(len_uncompressed_buffer)) == NULL)) {
			SysMessage("Could not reserve enough memory for full image buffer.\n");
			exit(3);
		}
		//printf("Memory ok2 %u %p\n", len_uncompressed_buffer, cdimage_buffer_mem);
		cdimage_buffer = fmemopen(cdimage_buffer_mem, len_uncompressed_buffer, "w+b");
	} else {

	}

	if (cdHandle != cdimage_buffer) {
		fclose(cdHandle); // opened thru archive so this not needed anymore
		cdHandle = cdimage_buffer;
	}

	return 0; 
}

static int cdread_archive(FILE *f, unsigned int base, void *dest, int sector)
{
	s32 r;
	size_t size;
	size_t readsize;
	static off_t offset = 0; // w/o read always or static/ftell
	const void *buff;

	// If not pointing to archive file but CDDA file or some other track
	if(f != cdHandle) {
		return cdimg_read_func_archive(f, base, dest, sector);
	}

	// Jump if already completely read
	if (a != NULL /*&& (ecm_file_detected || sector*CD_FRAMESIZE_RAW <= len_uncompressed_buffer)*/) {
		readsize = (sector+1) * CD_FRAMESIZE_RAW; 
		for (fseek(cdimage_buffer, offset, SEEK_SET); offset < readsize;) {
			r = archive_read_data_block(a, &buff, &size, &offset);
			offset += size;
			SysPrintf("ReadArchive seek:%u(%u) cur:%u(%u)\r", sector, readsize/1024, offset/CD_FRAMESIZE_RAW, offset/1024);
			fwrite(buff, size, 1, cdimage_buffer);
			if (r != ARCHIVE_OK) {
				//SysPrintf("End of archive.\n");
				archive_read_free(a);
				a = NULL;
				readsize = offset;
				fflush(cdimage_buffer);
				fseek(cdimage_buffer, 0, SEEK_SET);
			}
		}
	} else {
		//SysPrintf("ReadSectorArchSector: %u(%u)\n", sector, sector*CD_FRAMESIZE_RAW);
	}

	// TODO what causes req sector to be greater than CD size?
	r = cdimg_read_func_archive(cdimage_buffer, base, dest, sector);
	return r;
}
int handlearchive(const char *isoname, s32* accurate_length) {
	u32 read_size = accurate_length?MSF2SECT(70,70,16) : MSF2SECT(0,0,16);
	int ret = -1;
	if ((ret=aropen(cdHandle, isoname)) == 0) {
		cdimg_read_func = cdread_archive;
		SysPrintf("[+archive]");
		if (!ecm_file_detected) {
#ifndef ENABLE_ECM_FULL
			//Detect ECM inside archive
			cdimg_read_func_archive = cdread_normal;
			cdread_archive(cdHandle, 0, cdbuffer, read_size);
			if (handleecm("test.ecm", cdimage_buffer, accurate_length) != -1) {
				cdimg_read_func_archive = cdread_ecm_decode;
				cdimg_read_func = cdread_archive;
				SysPrintf("[+ecm]");
			}
#endif
		} else {
			SysPrintf("[+ecm]");
		}
	}
	return ret;
}
#else
int aropen(FILE* fparchive, const char* _fn) {return -1;}
static int cdread_archive(FILE *f, unsigned int base, void *dest, int sector) {return -1;}
int handlearchive(const char *isoname, s32* accurate_length) {return -1;}
#endif

static unsigned char * CALLBACK ISOgetBuffer_compr(void) {
	return compr_img->buff_raw[compr_img->sector_in_blk] + 12;
}

static unsigned char * CALLBACK ISOgetBuffer(void) {
	return cdbuffer + 12;
}

static void PrintTracks(void) {
	int i;

	for (i = 1; i <= numtracks; i++) {
		SysPrintf(_("Track %.2d (%s) - Start %.2d:%.2d:%.2d, Length %.2d:%.2d:%.2d\n"),
			i, (ti[i].type == DATA ? "DATA" : ti[i].cddatype == CCDDA ? "CZDA" : "CDDA"),
			ti[i].start[0], ti[i].start[1], ti[i].start[2],
			ti[i].length[0], ti[i].length[1], ti[i].length[2]);
	}
}

// This function is invoked by the front-end when opening an ISO
// file for playback
static long CALLBACK ISOopen(void) {
	if (cdHandle != NULL) {
		return 0; // it's already open
	}

	cdHandle = fopen(GetIsoFile(), "rb");
	if (cdHandle == NULL) {
		return -1;
	}

	SysPrintf(_("Loaded CD Image: %s"), GetIsoFile());

	cddaBigEndian = FALSE;
	subChanMixed = FALSE;
	subChanRaw = FALSE;
	pregapOffset = 0;
	cdrIsoMultidiskCount = 1;
	multifile = FALSE;

	CDR_getBuffer = ISOgetBuffer;
	cdimg_read_func = cdread_normal;

	if (parsecue(GetIsoFile()) == 0) {
		SysPrintf("[+cue]");
	}
	else if (parsetoc(GetIsoFile()) == 0) {
		SysPrintf("[+toc]");
	}
	else if (parseccd(GetIsoFile()) == 0) {
		SysPrintf("[+ccd]");
	}
	else if (parsemds(GetIsoFile()) == 0) {
		SysPrintf("[+mds]");
	}
	// TODO Is it possible that cue/ccd+ecm? otherwise use else if below to supressn extra checks
	if (handlepbp(GetIsoFile()) == 0) {
		SysPrintf("[pbp]");
		CDR_getBuffer = ISOgetBuffer_compr;
		cdimg_read_func = cdread_compressed;
	}
	else if (handlecbin(GetIsoFile()) == 0) {
		SysPrintf("[cbin]");
		CDR_getBuffer = ISOgetBuffer_compr;
		cdimg_read_func = cdread_compressed;
	}
	else if ((handleecm(GetIsoFile(), cdHandle, NULL) == 0)) {
		SysPrintf("[+ecm]");
	}
	else if (handlearchive(GetIsoFile(), NULL) == 0) {
	}

	if (!subChanMixed && opensubfile(GetIsoFile()) == 0) {
		SysPrintf("[+sub]");
	}
	if (opensbifile(GetIsoFile()) == 0) {
		SysPrintf("[+sbi]");
	}

	if (!ecm_file_detected) {
		// guess whether it is mode1/2048
		fseek(cdHandle, 0, SEEK_END);
		if (ftell(cdHandle) % 2048 == 0) {
			unsigned int modeTest = 0;
			fseek(cdHandle, 0, SEEK_SET);
			fread(&modeTest, 4, 1, cdHandle);
			if (SWAP32(modeTest) != 0xffffff00) {
				SysPrintf("[2048]");
				isMode1ISO = TRUE;
			}
		}
		fseek(cdHandle, 0, SEEK_SET);
	}

	SysPrintf(".\n");

	PrintTracks();

	if (subChanMixed && (cdimg_read_func == cdread_normal))
		cdimg_read_func = cdread_sub_mixed;
	else if (isMode1ISO && (cdimg_read_func == cdread_normal))
		cdimg_read_func = cdread_2048;
		else if (isMode1ISO && (cdimg_read_func_archive == cdread_normal))
		cdimg_read_func_archive = cdread_2048;

	// make sure we have another handle open for cdda
	if (numtracks > 1 && ti[1].handle == NULL) {
		ti[1].handle = fopen(GetIsoFile(), "rb");
	}

	return 0;
}

static long CALLBACK ISOclose(void) {
	int i;

	if (cdHandle != NULL) {
		fclose(cdHandle);
		cdHandle = NULL;
		//cdimage_buffer = NULL;
	}
	if (subHandle != NULL) {
		fclose(subHandle);
		subHandle = NULL;
	}

	if (compr_img != NULL) {
		free(compr_img->index_table);
		free(compr_img);
		compr_img = NULL;
	}

	for (i = 1; i <= numtracks; i++) {
		if (ti[i].handle != NULL) {
			fclose(ti[i].handle);
			ti[i].handle = NULL;
			if (ti[i].decoded_buffer != NULL) {
				free(ti[i].decoded_buffer);
			}
			ti[i].cddatype = NONE;
		}
	}
	numtracks = 0;
	ti[1].type = 0;
 
	memset(cdbuffer, 0, sizeof(cdbuffer));
	CDR_getBuffer = ISOgetBuffer;

	return 0;
}

long CALLBACK ISOinit(void) {
	assert(cdHandle == NULL);
	assert(subHandle == NULL);
	assert(ecm_file_detected == FALSE);
	assert(decoded_ecm_buffer == NULL);
	assert(decoded_ecm == NULL);

	return 0; // do nothing
}

static long CALLBACK ISOshutdown(void) {
	ISOclose();

	// ECM LUT
	free(ecm_savetable);
	ecm_savetable = NULL;

	if (decoded_ecm != NULL) {
		fclose(decoded_ecm);
		free(decoded_ecm_buffer);
		decoded_ecm_buffer = NULL;
		decoded_ecm	= NULL;
	}
	ecm_file_detected = FALSE;

#ifdef HAVE_LIBARCHIVE
	if (cdimage_buffer != NULL) {
		//fclose(cdimage_buffer);
		free(cdimage_buffer_mem);
		cdimage_buffer_mem = NULL;
		cdimage_buffer = NULL;
		if (a) {
			archive_read_free(a);
			a = NULL;
		}
	}
#endif

	return 0;
}

// return Starting and Ending Track
// buffer:
//  byte 0 - start track
//  byte 1 - end track
static long CALLBACK ISOgetTN(unsigned char *buffer) {
	buffer[0] = 1;

	if (numtracks > 0) {
		buffer[1] = numtracks;
	}
	else {
		buffer[1] = 1;
	}

	return 0;
}

// return Track Time
// buffer:
//  byte 0 - frame
//  byte 1 - second
//  byte 2 - minute
static long CALLBACK ISOgetTD(unsigned char track, unsigned char *buffer) {
	if (track == 0) {
		unsigned int sect;
		unsigned char time[3];
		sect = msf2sec(ti[numtracks].start) + msf2sec(ti[numtracks].length);
		sec2msf(sect, (char *)time);
		buffer[2] = time[0];
		buffer[1] = time[1];
		buffer[0] = time[2];
	}
	else if (numtracks > 0 && track <= numtracks) {
		buffer[2] = ti[track].start[0];
		buffer[1] = ti[track].start[1];
		buffer[0] = ti[track].start[2];
	}
	else {
		buffer[2] = 0;
		buffer[1] = 2;
		buffer[0] = 0;
	}

	return 0;
}

// decode 'raw' subchannel data ripped by cdrdao
static void DecodeRawSubData(void) {
	unsigned char subQData[12];
	int i;

	memset(subQData, 0, sizeof(subQData));

	for (i = 0; i < 8 * 12; i++) {
		if (subbuffer[i] & (1 << 6)) { // only subchannel Q is needed
			subQData[i >> 3] |= (1 << (7 - (i & 7)));
		}
	}

	memcpy(&subbuffer[12], subQData, 12);
}

// read track
// time: byte 0 - minute; byte 1 - second; byte 2 - frame
// uses bcd format
static long CALLBACK ISOreadTrack(unsigned char *time) {
	int sector = MSF2SECT(btoi(time[0]), btoi(time[1]), btoi(time[2]));
	long ret;

	if (cdHandle == NULL) {
		return -1;
	}

	if (pregapOffset) {
		subChanMissing = FALSE;
		if (sector >= pregapOffset) {
			sector -= 2 * 75;
			if (sector < pregapOffset)
				subChanMissing = TRUE;
		}
	}

	ret = cdimg_read_func(cdHandle, 0, cdbuffer, sector);
	if (ret < 0)
		return -1;

	if (subHandle != NULL) {
		fseek(subHandle, sector * SUB_FRAMESIZE, SEEK_SET);
		fread(subbuffer, 1, SUB_FRAMESIZE, subHandle);

		if (subChanRaw) DecodeRawSubData();
	}

	return 0;
}

// plays cdda audio
// sector: byte 0 - minute; byte 1 - second; byte 2 - frame
// does NOT uses bcd format
static long CALLBACK ISOplay(unsigned char *time) {
	playing = TRUE;
	return 0;
}

// stops cdda audio
static long CALLBACK ISOstop(void) {
	playing = FALSE;
	return 0;
}

// gets subchannel data
static unsigned char* CALLBACK ISOgetBufferSub(void) {
	if ((subHandle != NULL || subChanMixed) && !subChanMissing) {
		return subbuffer;
	}

	return NULL;
}

static long CALLBACK ISOgetStatus(struct CdrStat *stat) {
	u32 sect;
	
	CDR__getStatus(stat);
	
	if (playing) {
		stat->Type = 0x02;
		stat->Status |= 0x80;
	}
	else {
		// BIOS - boot ID (CD type)
		stat->Type = ti[1].type;
	}
	
	// relative -> absolute time
	sect = cddaCurPos;
	sec2msf(sect, (u8 *)stat->Time);
	
	return 0;
}

// read CDDA sector into buffer
long CALLBACK ISOreadCDDA(unsigned char m, unsigned char s, unsigned char f, unsigned char *buffer) {
	unsigned char msf[3] = {m, s, f};
	unsigned int file, track, track_start = 0;
	int ret;

	cddaCurPos = msf2sec(msf);

	// find current track index
	for (track = numtracks; ; track--) {
		track_start = msf2sec(ti[track].start);
		if (track_start <= cddaCurPos)
			break;
		if (track == 1)
			break;
	}

	// data tracks play silent (or CDDA set to silent)
	if (ti[track].type != CDDA || Config.Cdda == CDDA_DISABLED) {
		memset(buffer, 0, CD_FRAMESIZE_RAW);
		return 0;
	}

	file = 1;
	if (multifile) {
		// find the file that contains this track
		for (file = track; file > 1; file--)
			if (ti[file].handle != NULL)
				break;
	}

	/* Need to decode audio track first if compressed still (lazy) */
	if (ti[file].cddatype > BIN) {
		do_decode_cdda(&(ti[file]), file);
	}

	ret = cdimg_read_func(ti[file].handle, ti[track].start_offset,
		buffer, cddaCurPos - track_start);
	if (ret != CD_FRAMESIZE_RAW) {
		memset(buffer, 0, CD_FRAMESIZE_RAW);
		return -1;
	}

	if (Config.Cdda == CDDA_ENABLED_BE || cddaBigEndian) {
		int i;
		unsigned char tmp;

		for (i = 0; i < CD_FRAMESIZE_RAW / 2; i++) {
			tmp = buffer[i * 2];
			buffer[i * 2] = buffer[i * 2 + 1];
			buffer[i * 2 + 1] = tmp;
		}
	}

	return 0;
}

void cdrIsoInit(void) {
	CDR_init = ISOinit;
	CDR_shutdown = ISOshutdown;
	CDR_open = ISOopen;
	CDR_close = ISOclose;
	CDR_getTN = ISOgetTN;
	CDR_getTD = ISOgetTD;
	CDR_readTrack = ISOreadTrack;
	CDR_getBuffer = ISOgetBuffer;
	CDR_play = ISOplay;
	CDR_stop = ISOstop;
	CDR_getBufferSub = ISOgetBufferSub;
	CDR_getStatus = ISOgetStatus;
	CDR_readCDDA = ISOreadCDDA;

	CDR_getDriveLetter = CDR__getDriveLetter;
	CDR_configure = CDR__configure;
	CDR_test = CDR__test;
	CDR_about = CDR__about;
	CDR_setfilename = CDR__setfilename;

	numtracks = 0;
}

int cdrIsoActive(void) {
	return (cdHandle != NULL || ecm_savetable != NULL || decoded_ecm != NULL);
}
