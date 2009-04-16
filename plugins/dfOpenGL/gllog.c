// comment next line and all gllog(xx,....) will go to gpulog.txt (enable logging)
#define _COMMENTLOG

void gllog(int level, char *fmt,...)
{
#ifndef _COMMENTLOG
	static FILE *out=NULL;
	static char tmpbuf[200], tmpbufold[200];

	static int repeatcount=0;

	if(out==NULL)
	{
		// open output file !
		if((out=fopen("gpulog.txt","wb"))==NULL)exit(1);
	}
	// file already opened

    va_list marker;
	va_start(marker,fmt);
	vsprintf(tmpbuf, fmt, marker );
	va_end(marker);

	// we dont log the same line multiple times 
	if(strcmp(tmpbufold,tmpbuf)==0)
	{
		repeatcount++;
		return;
	}
	else
	{
		// copy the new line to the old buffer
		strcpy(tmpbufold,tmpbuf);
		if(repeatcount>0)
		{
			fprintf(out,"Last message repeated %d times\n",repeatcount);
			repeatcount=0;
		}
		fprintf(out,"Level: %d msg: %s\n",level,tmpbuf);
		fflush(out);
	}
#endif
}
