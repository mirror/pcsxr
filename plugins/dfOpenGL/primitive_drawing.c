#include <stdlib.h>
#include <string.h>
#include "gpu_i.h"

typedef unsigned char byte;

byte texshade[256];
uint32_t torgba[65536];
uint32_t image[65536];
char curText=0;
TextureState_t texinfo;
struct texturecache texture[64];
GLuint nullid;

struct TransparencySettings{
	int mode;
	int src;
	int dst;
	byte alpha;
	bool change;
};

struct TransparencySettings TransRate[4]={
	{GL_FUNC_ADD, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA,128,TRUE},
	{GL_FUNC_ADD, GL_ONE, GL_ONE_MINUS_SRC_ALPHA,0,TRUE},
	{GL_FUNC_ADD, GL_ZERO, GL_ONE_MINUS_SRC_COLOR, 0,TRUE},
	{GL_FUNC_ADD, GL_SRC_ALPHA, GL_ONE,64,TRUE}
};
bool transparent=FALSE;
byte gAlpha;

void cmdSTP(unsigned char * baseAddr)
{
	uint32_t gdata = ((uint32_t*)baseAddr)[0];
	
	GPUstatusRet&=~0x1800;                                   // Clear the necessary bits
	GPUstatusRet|=((gdata & 0x03) << 11);                    // Set the necessary bits
	
	psxDraw.setmask = gdata & 1;
	psxDraw.testmask = (gdata & 2) >> 1;

	gllog(77," CMD: STP: %x",gdata&0xffffff);
}


inline void UpdateTextureInfo ( uint32_t gdata )
{
	texinfo.x = ( gdata << 6 ) & 0x3c0;             // texture addr
	texinfo.y = ( gdata << 4 ) & 0x100;

	//if ( gdata&200 ) iDither=iUseDither; else iDither=0;

	texinfo.colormode = ( gdata >> 7 ) & 0x3;             // tex mode (4bit,8bit,15direct)
	if ( texinfo.colormode==3 )
		texinfo.colormode=2;                // seen in Wild9 :(

	texinfo.abr = ( gdata >> 5 ) & 0x3;                 // blend mode
	
	texinfo.mirror = gdata&0x3000 >> 12;	//???

	GPUstatusRet&=~0x07ff;                               // Clear the necessary bits
	GPUstatusRet|= ( gdata & 0x07ff );                   // set the necessary bits
}

void cmdTexturePage(unsigned char * baseAddr)
{
	uint32_t gdata = ((uint32_t*)baseAddr)[0];

	int32_t tempABR = (gdata >> 5) & 0x3;
	if(texinfo.abr != tempABR){
		texinfo.abr = tempABR;
		//glBlendEquation(TransRate[texinfo.abr].mode);
		glBlendFunc(TransRate[texinfo.abr].src,TransRate[texinfo.abr].dst);
		gAlpha=TransRate[texinfo.abr].alpha;
		transparent=TRUE;
	}
	UpdateTextureInfo(gdata);


	/*switch(texinfo.abr){
	  case 0:
		glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
		gAlpha=128;
		break;
	  case 1:
		glBlendFunc(GL_ONE,GL_ONE_MINUS_SRC_ALPHA);
		gAlpha=0;
		break;
	  case 2:
		glBlendFunc(GL_ZERO,GL_ONE_MINUS_SRC_COLOR);
		gAlpha=0;
		break;
	  case 3:
		glBlendFunc(GL_SRC_ALPHA,GL_ONE);
		gAlpha=64;
		break;
	}*/
	//textREST = (gdata&0x00ffffff)>>9;
	//gllog(77," CMD: TPage: %d,%d TP=%d ABR=%d REST=%x\n",textAddrX,textAddrY,textTP,texinfo.abr,(gdata&0x00ffffff)>>9);
	return;
}

void cmdTextureWindow(unsigned char *baseAddr)
{
	//hopefully true psx texture window emulation, oddities & all
	//taken from peops soft gpu

	uint32_t gdata = ((uint32_t*)baseAddr)[0];
	//gllog(77,"CDM: TWin: %x *NI*", gdata&0xffffff);

	GPUInfoVals[INFO_TW]=gdata&0xFFFFF;

	uint32_t YAlign,XAlign;

	if(gdata & 0x020)
		psxDraw.texwinY2 = 8;    // xxxx1
	else if (gdata & 0x040)
		psxDraw.texwinY2 = 16;   // xxx10
	else if (gdata & 0x080)
		psxDraw.texwinY2 = 32;   // xx100
	else if (gdata & 0x100)
		psxDraw.texwinY2 = 64;   // x1000
	else if (gdata & 0x200)
		psxDraw.texwinY2 = 128;  // 10000
	else
		psxDraw.texwinY2 = 256;  // 00000
	
	// Texture window size is determined by the least bit set of the relevant 5 bits
	
	if (gdata & 0x001)
		psxDraw.texwinX2 = 8;    // xxxx1
	else if (gdata & 0x002)
		psxDraw.texwinX2 = 16;   // xxx10
	else if (gdata & 0x004)
		psxDraw.texwinX2 = 32;   // xx100
	else if (gdata & 0x008)
		psxDraw.texwinX2 = 64;   // x1000
	else if (gdata & 0x010)
		psxDraw.texwinX2 = 128;  // 10000
	else
		psxDraw.texwinX2 = 256;  // 00000
	
	// Re-calculate the bit field, because we can't trust what is passed in the data
	
	
	YAlign = (uint32_t)(32 - (psxDraw.texwinY2 >> 3));
	XAlign = (uint32_t)(32 - (psxDraw.texwinX2 >> 3));
	
	// Absolute position of the start of the texture window
	
	psxDraw.texwinY1 = (short)(((gdata >> 15) & YAlign) << 3);
	psxDraw.texwinX1 = (short)(((gdata >> 10) & XAlign) << 3);
	
	if((psxDraw.texwinX1 == 0 &&                          // tw turned off
		psxDraw.texwinY1 == 0 &&
		psxDraw.texwinX2 == 0 &&
		psxDraw.texwinY2 == 0) ||  
		(psxDraw.texwinX2 == 256 &&
		psxDraw.texwinY2 == 256))
	{
		psxDraw.texwinenabled = 0;                                 // -> just do it
	}
	else                                                  // otherwise
	{
		psxDraw.texwinenabled = 1;                                  // -> tw turned on
	}
}

void cmdDrawAreaStart(unsigned char * baseAddr)
{
	uint32_t gdata = ((uint32_t*)baseAddr)[0];
	GPUInfoVals[INFO_DRAWSTART]=gdata&0x3FFFFF;

	psxDraw.clipX1 = gdata & 0x3ff;
	psxDraw.clipY1 = (gdata>>10)&0x1ff;

	GLdouble equation[4]={0.0,0.0,0.0,0.0};
	equation[0]=1.0;
	equation[3]=(GLdouble)-(psxDraw.clipX1);
	glClipPlane(GL_CLIP_PLANE0,equation);

	equation[0]=0.0;
	equation[1]=1.0;
	equation[3]=(GLdouble)-(psxDraw.clipY1);
	glClipPlane(GL_CLIP_PLANE1,equation);

	//gllog(77," CMD: DrawArea Start %d,%d %x",drawX,drawY,gdata);
}

void cmdDrawAreaEnd(unsigned char * baseAddr)
{
	uint32_t gdata = ((uint32_t*)baseAddr)[0];
	GPUInfoVals[INFO_DRAWEND]=gdata&0x3FFFFF;

	psxDraw.clipX2 = gdata & 0x3ff;
	psxDraw.clipY2 = (gdata>>10)&0x1ff;

	GLdouble equation[4]={0.0,0.0,0.0,0.0};
	equation[0]=-1.0;
	equation[3]=(GLdouble)(psxDraw.clipX2);
	glClipPlane(GL_CLIP_PLANE2,equation);

	equation[0]=0.0;
	equation[1]=-1.0;
	equation[3]=(GLdouble)(psxDraw.clipY2);
	glClipPlane(GL_CLIP_PLANE3,equation);
	

	//gllog(77," CMD: DrawArea End %d,%d",drawW,drawH);
}

void cmdDrawOffset(unsigned char * baseAddr)
{
	uint32_t gdata = ((uint32_t*)baseAddr)[0];
	GPUInfoVals[INFO_DRAWOFF]=gdata&0x7FFFFF;

	psxDraw.offsetX = (short)(gdata & 0x7ff);
	psxDraw.offsetY = (short)((gdata>>11)&0x7ff);

	psxDraw.offsetX=(short)(((int)psxDraw.offsetX<<21)>>21);
	psxDraw.offsetY=(short)(((int)psxDraw.offsetY<<21)>>21);
	
	//gllog(77," CMD: Draw Offset %d,%d",ofsX,ofsY,gdata);
}

void primLoadImage(unsigned char * baseAddr)
{
	short *sgpuData = ((short *) baseAddr);
	//uint32_t *gpuData = ((uint32_t *) baseAddr);

	short x = (sgpuData[2]<<21)>>21;
	short y = (sgpuData[3]<<21)>>21;
	short w = (sgpuData[4]<<21)>>21;
	short h = (sgpuData[5]<<21)>>21;

	if (w == -1024) w = 1024;
	if (h == -512) h = 512;

	if (x < 0 || y < 0 || w<0 || h<0 || x+w > 1024 || y+h > 512)
	{
		return;
	}

	vramWrite.curx = vramWrite.x = x;
	vramWrite.cury = vramWrite.y = y;
	vramWrite.w = w;
	vramWrite.h = h;
	vramWrite.extratarget = (uint32_t*)malloc(vramWrite.w*vramWrite.h*4);
	vramWrite.enabled = 1;

	imageTransfer = 1;

	int i;

	for(i=0;i<gpuConfig.nMaxTextures;i++){
		if(((texture[i].textAddrX+255)>=x)
		  &&((texture[i].textAddrY+255)>=y)
		  &&(texture[i].textAddrX<=(x+w))
		  &&(texture[i].textAddrY<=(y+h))){
			texture[i].Update=TRUE;
		}
	}

	//gllog(1," CMD: MEM->VRAM %d,%d %d,%d\n",imageX0,imageY0,imageX1,imageY1);
}

void primStoreImage(unsigned char * baseAddr)
{
	uint32_t *gpuData = ((uint32_t *) baseAddr);

	imageX0 = (short)(gpuData[1] & 0x3ff);
	imageY0 = (short)(gpuData[1]>>16) & 0x1ff;
	imageX1 = (short)(gpuData[2] & 0xffff);
	imageY1 = (short)((gpuData[2]>>16) & 0xffff);

	//gllog(1," CMD: VRAM->MEM %d,%d %d,%d\n",imageX0,imageY0,imageX1,imageY1);
	imTX=imageX0; imTY=imageY0;
	imTXc=imageX1; imTYc=imageY1;
	imSize=imageY1*imageX1/2;

	imageTransfer = 2;
	GPUstatusRet|=0x08000000;
}

void primMoveImage(unsigned char * baseAddr)
{
    // 0x80 image move (VRAM->VRAM)
	uint32_t *gpuData = ((uint32_t *) baseAddr);

	unsigned short imageX0,imageY0,imageX1,imageY1,imageSX,imageSY,i,j;

	imageX0 = (short)(gpuData[1] & 0x3ff);
	imageY0 = (short)(gpuData[1]>>16) & 0x1ff;
	
	imageX1 = (short)(gpuData[2] & 0x3ff);
	imageY1 = (short)((gpuData[2]>>16) & 0x1ff);
	
	imageSY = (short)((gpuData[3]>>16) & 0xffff);
	imageSX = (short)(gpuData[3] & 0xffff);

	for(i=0;i<gpuConfig.nMaxTextures;i++){
		if(((texture[i].textAddrX+255)>=imageX1)
		  &&((texture[i].textAddrY+255)>=imageY1)
		  &&(texture[i].textAddrX<=(imageX1+imageSX))
		  &&(texture[i].textAddrY<=(imageY1+imageSY))){
			texture[i].Update=TRUE;
		}
	}

	//gllog(1," CMD: VRAM->VRAM %d,%d -> %d,%d (%d,%d)\n",imageX0,imageY0,imageX1,imageY1,imageSX,imageSY);
    
	if((imageY0+imageSY)>512) imageSY=512-imageY0;
	if((imageY1+imageSY)>512) imageSY=512-imageY1;
	if((imageX0+imageSX)>1024) imageSX=1024-imageX0;
	if((imageX1+imageSX)>1024) imageSX=1024-imageX1;
	for(j=0;j<imageSY;j++)
		for(i=0;i<imageSX;i++)
			psxVuw[((imageY1+j)<<10)+imageX1+i]=psxVuw[((imageY0+j)<<10)+imageX0+i];
}




void primTileS(unsigned char * baseAddr)
{
	//gllog(2,"Tile S *NI*");//  : %04x,%04x - %04x,%04x",baseAddrW[addr+2],baseAddrW[addr+3],baseAddrW[addr+4],baseAddrW[addr+5]);
	//gllog(2,"Tile S *NI* : %04x,%04x - %04x,%04x",baseAddrW[addr+2],baseAddrW[addr+3],baseAddrW[addr+4],baseAddrW[addr+5]);
	
	//uint32_t *gpuData = (uint32_t *)baseAddr;
	unsigned short *gpuPoint = (unsigned short*) baseAddr;

	short x = (gpuPoint[2]<<21)>>21;
	short y = (gpuPoint[3]<<21)>>21;
	short w = (gpuPoint[4]<<21)>>21;
	short h = (gpuPoint[5]<<21)>>21;

	x += psxDraw.offsetX;
	y += psxDraw.offsetY;

	if(baseAddr[3]&2){	//if blending on
		//if(TransRate[texinfo.abr].change && !transparent){
			//glBlendEquation(TransRate[texinfo.abr].mode);
			glBlendFunc(TransRate[texinfo.abr].src,TransRate[texinfo.abr].dst);
			gAlpha=TransRate[texinfo.abr].alpha;
			transparent=TRUE;
		//}
		//if(baseAddr[3]&1){
		//	glColor4ub(255,255,255,gAlpha);
		//}else{
			glColor4ub(baseAddr[0],baseAddr[1],baseAddr[2],gAlpha);
		//}
	}else{
		//if(TransRate[texinfo.abr].change && transparent){
			//glBlendEquation(TransRate[0].mode);
			glBlendFunc(TransRate[0].src,TransRate[0].dst);
			transparent=FALSE;
		//}
		//if(baseAddr[3]&1){
		//	glColor3ub(255,255,255);
		//}else{
			glColor3ub(baseAddr[0],baseAddr[1],baseAddr[2]);
		//}
	}

	glBegin(GL_POLYGON);
	  glVertex2s(x,y);
	  glVertex2s(x+w,y);
	  glVertex2s(x+w,y+h);
	  glVertex2s(x,y+h);
	glEnd();


	return;
}

void primBlkFill(unsigned char * baseAddr)
{
	short * gpuPoint = (short*) baseAddr;
	//uint32_t *gpuData = ((uint32_t *) baseAddr);

	//gllog(2,"BlkFill : %04x,%04x - %04x,%04x",gpuPoint[2],gpuPoint[3],gpuPoint[4],gpuPoint[5]);

	short x = (gpuPoint[2]<<21)>>21;
	short y = (gpuPoint[3]<<21)>>21;
	short w = (gpuPoint[4]<<21)>>21;
	short h = (gpuPoint[5]<<21)>>21;

/*
	this function is something like clear_with_given_color
	and according to baseAddr
	struct packet
	{
		unsigned char R,G,B,code;
		unsigned short X,Y,W,H;
	}
	this function is not clipped bby drawing area, accesses whole VRAM
*/

	glDisable(GL_CLIP_PLANE0);
	glDisable(GL_CLIP_PLANE1);
	glDisable(GL_CLIP_PLANE2);
	glDisable(GL_CLIP_PLANE3);

	glColor3ub(baseAddr[0],baseAddr[1],baseAddr[2]);
	glBegin(GL_POLYGON);
	  glVertex2s(x,y);
	  glVertex2s(x+w,y);
	  glVertex2s(x+w,y+h);
	  glVertex2s(x,y+h);
	glEnd();

	glEnable(GL_CLIP_PLANE0);
	glEnable(GL_CLIP_PLANE1);
	glEnable(GL_CLIP_PLANE2);
	glEnable(GL_CLIP_PLANE3);
}



void setupTexture(int32_t clutP){
	int i;
	int32_t sprCY,sprCX,tbw;
	short color,tC;
	int ctext;
	union uPointers psxVOffset,psxVOffset2;

	if( (texinfo.colormode&2) == 2 )
		clutP=nullclutP;

	ctext=curText;
	for(i=0;i<gpuConfig.nMaxTextures;i++){
		if(texture[i].textAddrX==texinfo.x && texture[i].textAddrY==texinfo.y && clutP==texture[i].clutP)
		{
			if(texture[i].Update==TRUE || texinfo.colormode!=texture[i].textTP)
			{
				ctext=i;
				break;
			}
			glBindTexture(GL_TEXTURE_2D,texture[i].id);
			return;
		}
	}

	texture[ctext].textAddrX=texinfo.x;
	texture[ctext].textAddrY=texinfo.y;
	texture[ctext].textTP=texinfo.colormode;
	texture[ctext].clutP=clutP;
	texture[ctext].Update=FALSE;
	glBindTexture(GL_TEXTURE_2D,texture[ctext].id);
	if(ctext==curText){
		curText++;
		if(curText>=gpuConfig.nMaxTextures)curText=0;
	}


	tbw=256;
	psxVOffset.w=psxVuw+(texinfo.y<<10)+texinfo.x;

	uint32_t *pimage = image;

	switch(texinfo.colormode)
	{
	//4bit clut
	case 0:
		for (sprCY=0;sprCY<256;sprCY++){
			psxVOffset2.b=psxVOffset.b;
			for (sprCX=0;sprCX<256;sprCX+=2){
				tC = (*psxVOffset2.b)&0x0f;
				*pimage = torgba[psxVuw[clutP+tC]];
				pimage++;

				tC = (*psxVOffset2.b>>4)&0x0f;
				*pimage = torgba[psxVuw[clutP+tC]];
				pimage++;

				psxVOffset2.b++;
			}
			psxVOffset.w+=1024;
		}
		break;

	//8bit clut
	case 1:
		if(texinfo.x==960)tbw=128;
		for (sprCY=0;sprCY<256;sprCY++){
			psxVOffset2.b=psxVOffset.b;
			for (sprCX=0;sprCX<tbw;sprCX++){
				tC = *psxVOffset2.b;
				image[(sprCY<<8)+sprCX] = torgba[psxVuw[clutP+tC]];
				psxVOffset2.b++;
			}
			psxVOffset.w+=1024;
		}
		break;

	//15bit direct
	case 2:
		if(texinfo.x==960)tbw=64;
		else if(texinfo.x==896)tbw=128;
		else if(texinfo.x==832)tbw=192;

		for (sprCY=0;sprCY<256;sprCY++){
			psxVOffset2.w=psxVOffset.w;
			for (sprCX=0;sprCX<tbw;sprCX++){
				color = *psxVOffset2.w;
				image[(sprCY<<8)+sprCX] = torgba[color];
				psxVOffset2.w++;
			}
			psxVOffset.w+=1024;
		}
		break;
	}
	glTexSubImage2D(GL_TEXTURE_2D,0,0,0,256,256,GL_RGBA,GL_UNSIGNED_BYTE,image);
}


void setSpriteBlendMode(int32_t command){
	unsigned char *baseAddr = (unsigned char *)&command;
		
	//color info is 0-127, or bit 0x80 to indicate full color
	//note: above applies to texture (and sprite) color shading info only?
	unsigned char r,g,b;
	r = texshade[baseAddr[0]];
	g = texshade[baseAddr[1]];
	b = texshade[baseAddr[2]];

	if(baseAddr[3]&2){	//if blending on
		//if(TransRate[texinfo.abr].change && !transparent){
			//glBlendEquation(TransRate[texinfo.abr].mode);
			glBlendFunc(TransRate[texinfo.abr].src,TransRate[texinfo.abr].dst);
			transparent=TRUE;
		//}
		if(baseAddr[3]&1){	//should always be 0 for sprite?  Apparantly not...
			glColor4ub(255,255,255,gAlpha);
		}else{
			glColor4ub(r,g,b,gAlpha);
		}
	}else{
		//if(TransRate[texinfo.abr].change && transparent){
			//glBlendEquation(TransRate[0].mode);
			glBlendFunc(TransRate[0].src,TransRate[0].dst);
			transparent=FALSE;
		//}
		if(baseAddr[3]&1){
			glColor3ub(255,255,255);
		}else{
			glColor3ub(r,g,b);
		}
	}
}

void primSprt8(unsigned char * baseAddr)
{
/* 
	fixed size (8x8) sprite
*/
	uint32_t *gpuData = (uint32_t *)baseAddr;
	short *gpuPoint = (short*) baseAddr;
	int32_t clutP;
	short tx,ty;
	short x,y;

	//gllog(2,"Sprt8x8 : %d,%d %02x",gpuPoint[2],gpuPoint[3],baseAddr[3]);
    clutP  = (gpuData[2]>>12) & 0x7fff0;//0xffff0;
    tx = (short)(gpuData[2] & 0x000000ff);
    ty = (short)((gpuData[2]>>8) & 0x000000ff);

	x = (gpuPoint[2]<<21)>>21;
	y = (gpuPoint[3]<<21)>>21;

	x += psxDraw.offsetX;
	y += psxDraw.offsetY;

	setupTexture(clutP);
	setSpriteBlendMode(*gpuData);
	
	/*switch(baseAddr[3]&3){
	  case 0:
		glColor3ub(baseAddr[0],baseAddr[1],baseAddr[2]);
		break;
	  case 1:
		glColor3ub(255,255,255);
		break;
	  case 2:
		glColor4ub(baseAddr[0],baseAddr[1],baseAddr[2],gAlpha);
		break;
	  case 3:
		glColor4ub(255,255,255,gAlpha);
		break;
	}*/

	glBegin(GL_POLYGON);
	  glTexCoord2s(tx,ty);
	  glVertex2s(x,y);
	  glTexCoord2s(tx+7,ty);
	  glVertex2s(x+8,y);
	  glTexCoord2s(tx+7,ty+7);
	  glVertex2s(x+8,y+8);
	  glTexCoord2s(tx,ty+7);
	  glVertex2s(x,y+8);
	glEnd();

	//draw opaque areas of texture
	if(baseAddr[3]&2){
		glEnable(GL_ALPHA_TEST);
		glAlphaFunc(GL_EQUAL, 1);
		glColor3ub(255,255,255);
		glDisable(GL_BLEND);
		glBegin(GL_POLYGON);
			glTexCoord2s(tx,ty);
			glVertex2s(x,y);
			glTexCoord2s(tx+7,ty);
			glVertex2s(x+8,y);
			glTexCoord2s(tx+7,ty+7);
			glVertex2s(x+8,y+8);
			glTexCoord2s(tx,ty+7);
			glVertex2s(x,y+8);
		glEnd();
		glEnable(GL_BLEND);
		glDisable(GL_ALPHA_TEST);
	}

	glBindTexture(GL_TEXTURE_2D,nullid);
}

void primSprt16(unsigned char * baseAddr)
{
/* 
	fixed size (16x16) sprite
*/
	uint32_t *gpuData = (uint32_t *)baseAddr;
	short *gpuPoint = (short*) baseAddr;
	int32_t clutP;
	short tx,ty;
	short x,y;

	//gllog(2,"Sprt16x16 : %d,%d %02x",gpuPoint[2],gpuPoint[3],baseAddr[3]);
    clutP  = (gpuData[2]>>12) & 0x7fff0;//0xffff0;
    tx = (short)(gpuData[2] & 0x000000ff);
    ty = (short)((gpuData[2]>>8) & 0x000000ff);

	x = (gpuPoint[2]<<21)>>21;
	y = (gpuPoint[3]<<21)>>21;

	x += psxDraw.offsetX;
	y += psxDraw.offsetY;

	setupTexture(clutP);
	setSpriteBlendMode(*gpuData);
	
	glBegin(GL_POLYGON);
	  glTexCoord2s(tx,ty);
	  glVertex2s(x,y);
	  glTexCoord2s(tx+15,ty);
	  glVertex2s(x+16,y);
	  glTexCoord2s(tx+15,ty+15);
	  glVertex2s(x+16,y+16);
	  glTexCoord2s(tx,ty+15);
	  glVertex2s(x,y+16);
	glEnd();

	//draw opaque areas of texture
	if(baseAddr[3]&2){
		glEnable(GL_ALPHA_TEST);
		glAlphaFunc(GL_EQUAL, 1);
		glColor3ub(255,255,255);
		glDisable(GL_BLEND);
		glBegin(GL_POLYGON);
			glTexCoord2s(tx,ty);
			glVertex2s(x,y);
			glTexCoord2s(tx+15,ty);
			glVertex2s(x+16,y);
			glTexCoord2s(tx+15,ty+15);
			glVertex2s(x+16,y+16);
			glTexCoord2s(tx,ty+15);
			glVertex2s(x,y+16);
		glEnd();
		glEnable(GL_BLEND);
		glDisable(GL_ALPHA_TEST);
	}


	glBindTexture(GL_TEXTURE_2D,nullid);
}

void primSprtS(unsigned char * baseAddr)
{
/* 
	free size (up to 256x256) sprite
*/
	uint32_t *gpuData = (uint32_t *)baseAddr;
	short *gpuPoint = (short*) baseAddr;
	int32_t clutP;
	short tx,ty,sprtW,sprtH;
	short tx1,tx2,ty1,ty2;
	short x,y;

    clutP  = (gpuData[2]>>12) & 0x7fff0;//0xffff0;

    tx = (short)(gpuData[2] & 0x000000ff);
    ty = (short)((gpuData[2]>>8) & 0x000000ff);
	sprtW = (short)(gpuData[3] & 0x3ff);
	sprtH = (short)((gpuData[3]>>16) & 0x1ff);

	x = (gpuPoint[2]<<21)>>21;
	y = (gpuPoint[3]<<21)>>21;

	x += psxDraw.offsetX;
	y += psxDraw.offsetY;

	//gllog(2,"Sprt : %04x,%04x - %04x,%04x code=%02x pal=%05x",gpuPoint[2],gpuPoint[3],gpuPoint[6],gpuPoint[7],baseAddr[3],clutP);
	/*if(textAddrX==320&&textAddrY==0){
		gllog(2,"Sprt : %d,%d - %d,%d pal=%05x",gpuPoint[2],gpuPoint[3],gpuPoint[6],gpuPoint[7],gpuData[2]>>12);
	}*/

	//scaled instead of repeated for now...
	#define min(a,b) ((a) < (b) ? (a) : (b))

	short minw = min(psxDraw.texwinX2, sprtW);
	short minh = min(psxDraw.texwinY2, sprtH);

	tx1 = tx + psxDraw.texwinX1;
	tx2 = tx1 + minw - 1;
	ty1 = ty + psxDraw.texwinY1;
	ty2 = ty1 + minh - 1;
/*	
	short tswap;

	#define swap(a,b)	tswap = (a); (a)=(b); (b) = tswap

	switch (texinfo.mirror)
	{
	case 1:
		swap(tx1, tx2);
		break;
	case 2:
		swap(ty1, ty2);
		break;
	case 3:
		swap(tx1,tx2);
		swap(ty1,ty2);
		break;
	}
*/

	setupTexture(clutP);
	setSpriteBlendMode(*gpuData);

	glBegin(GL_POLYGON);
	  glTexCoord2s(tx1, ty1);
	  glVertex2s(x,y);
	  glTexCoord2s(tx2, ty1);
	  glVertex2s(x+sprtW,y);
	  glTexCoord2s(tx2, ty2);
	  glVertex2s(x+sprtW,y+sprtH);
	  glTexCoord2s(tx1, ty2);
	  glVertex2s(x,y+sprtH);
	glEnd();
	
	//draw opaque areas of texture
	if(baseAddr[3]&2){
		glEnable(GL_ALPHA_TEST);
		glAlphaFunc(GL_EQUAL, 1);
		glDisable(GL_BLEND);
		glColor3ub(255,255,255);
		glBegin(GL_POLYGON);
			glTexCoord2s(tx1, ty1);
			glVertex2s(x,y);
			glTexCoord2s(tx2, ty1);
			glVertex2s(x+sprtW,y);
			glTexCoord2s(tx2, ty2);
			glVertex2s(x+sprtW,y+sprtH);
			glTexCoord2s(tx1, ty2);
			glVertex2s(x,y+sprtH);
		glEnd();
		glEnable(GL_BLEND);
		glDisable(GL_ALPHA_TEST);
	}

	glBindTexture(GL_TEXTURE_2D,nullid);
}


void primLineF2(unsigned char *baseAddr)
{
/* 
	Flat Line 2 vertices
*/
	short *gpuPoint=((short *) baseAddr);
	short x1,y1,x2,y2;

	x1 = (gpuPoint[2]<<21)>>21;
	y1 = (gpuPoint[3]<<21)>>21;
	x2 = (gpuPoint[4]<<21)>>21;
	y2 = (gpuPoint[5]<<21)>>21;

	x1 += psxDraw.offsetX;
	y1 += psxDraw.offsetY;
	x2 += psxDraw.offsetX;
	y2 += psxDraw.offsetY;


	if(baseAddr[3]&2){
		if(TransRate[texinfo.abr].change&&!transparent){
			glBlendFunc(TransRate[texinfo.abr].src,TransRate[texinfo.abr].dst);
			transparent=TRUE;
		}
		glColor4ub(baseAddr[0],baseAddr[1],baseAddr[2],gAlpha);
	}else{
		if(TransRate[texinfo.abr].change&&transparent){
			glBlendFunc(TransRate[0].src,TransRate[0].dst);
			transparent=FALSE;
		}
		glColor3ub(baseAddr[0],baseAddr[1],baseAddr[2]);
	}
	/*if(baseAddr[3]&2){
		glColor4ub(baseAddr[0],baseAddr[1],baseAddr[2],gAlpha);
	}else{
		glColor3ub(baseAddr[0],baseAddr[1],baseAddr[2]);
	}*/
	glBegin(GL_LINES);
		glVertex2s(x1,y1);
		glVertex2s(x2,y2);
	glEnd();
}

void primLineG2(unsigned char *baseAddr)
{
/* 
	Goraud Line 2 vertices
*/
	short *gpuPoint=((short *) baseAddr);
	short x1,y1,x2,y2;
	unsigned char alpha;

	x1 = (gpuPoint[2]<<21)>>21;
	y1 = (gpuPoint[3]<<21)>>21;
	x2 = (gpuPoint[6]<<21)>>21;
	y2 = (gpuPoint[7]<<21)>>21;

	x1 += psxDraw.offsetX;
	y1 += psxDraw.offsetY;
	x2 += psxDraw.offsetX;
	y2 += psxDraw.offsetY;


	if(baseAddr[3]&2){
		if(TransRate[texinfo.abr].change&&!transparent){
			glBlendFunc(TransRate[texinfo.abr].src,TransRate[texinfo.abr].dst);
			transparent=TRUE;
		}
		alpha=gAlpha;
	}else{
		if(TransRate[texinfo.abr].change&&transparent){
			glBlendFunc(TransRate[0].src,TransRate[0].dst);
			transparent=FALSE;
		}
		alpha=255;
	}
	/*if(baseAddr[3]&2){
		alpha=gAlpha;
	}else{
		alpha=255;
	}*/
	glBegin(GL_LINES);
		glColor4ub(baseAddr[0],baseAddr[1],baseAddr[2],alpha);
		glVertex2s(x1,y1);
		glColor4ub(baseAddr[8],baseAddr[9],baseAddr[10],alpha);
		glVertex2s(x2,y2);
	glEnd();
}

void primPolyF3(unsigned char *baseAddr)
{	
/* 
	Flat Shaded Polygon 3 vertices
*/
	short *gpuPoint=((short *) baseAddr);
	short x1,x2,x3,y1,y2,y3;

	x1 = (gpuPoint[2]<<21)>>21;
	y1 = (gpuPoint[3]<<21)>>21;
	x2 = (gpuPoint[4]<<21)>>21;
	y2 = (gpuPoint[5]<<21)>>21;
	x3 = (gpuPoint[6]<<21)>>21;
	y3 = (gpuPoint[7]<<21)>>21;

	x1 += psxDraw.offsetX;
	y1 += psxDraw.offsetY;
	x2 += psxDraw.offsetX;
	y2 += psxDraw.offsetY;
	x3 += psxDraw.offsetX;
	y3 += psxDraw.offsetY;

	//gllog(77,"!GPU! P3 F %d,%d %d,%d %d,%d\n",lx0,ly0,lx1,ly1,lx2,ly2);

	if(baseAddr[3]&2){
		if(TransRate[texinfo.abr].change&&!transparent){
			glBlendFunc(TransRate[texinfo.abr].src,TransRate[texinfo.abr].dst);
			transparent=TRUE;
		}
		gAlpha = TransRate[texinfo.abr].alpha;
		glColor4ub(baseAddr[0],baseAddr[1],baseAddr[2],gAlpha);
	}else{
		if(TransRate[texinfo.abr].change&&transparent){
			glBlendFunc(TransRate[0].src,TransRate[0].dst);
			transparent=FALSE;
		}
		glColor3ub(baseAddr[0],baseAddr[1],baseAddr[2]);
	}
	glBegin(GL_POLYGON);
		glVertex2s(x1,y1);
		glVertex2s(x2,y2);
		glVertex2s(x3,y3);
	glEnd();
}

void primPolyF4(unsigned char *baseAddr)
{
/* 
	Flat Shaded Polygon 4 vertices
*/
	short *gpuPoint=((short *) baseAddr);
	short x1,x2,x3,x4,y1,y2,y3,y4;

	x1 = (gpuPoint[2]<<21)>>21;
	y1 = (gpuPoint[3]<<21)>>21;
	x2 = (gpuPoint[4]<<21)>>21;
	y2 = (gpuPoint[5]<<21)>>21;
	x3 = (gpuPoint[6]<<21)>>21;
	y3 = (gpuPoint[7]<<21)>>21;
	x4 = (gpuPoint[8]<<21)>>21;
	y4 = (gpuPoint[9]<<21)>>21;

	x1 += psxDraw.offsetX;
	y1 += psxDraw.offsetY;
	x2 += psxDraw.offsetX;
	y2 += psxDraw.offsetY;
	x3 += psxDraw.offsetX;
	y3 += psxDraw.offsetY;
	x4 += psxDraw.offsetX;
	y4 += psxDraw.offsetY;

	//gllog(77,"!GPU! P4 F %d,%d %d,%d %d,%d %d,%d %6x\n",lx0,ly0,lx1,ly1,lx2,ly2,lx3,ly3,gpuData[0]);

	if(baseAddr[3]&2){
		if(TransRate[texinfo.abr].change&&!transparent){
			glBlendFunc(TransRate[texinfo.abr].src,TransRate[texinfo.abr].dst);
			transparent=TRUE;
		}
		gAlpha = TransRate[texinfo.abr].alpha;
		glColor4ub(baseAddr[0],baseAddr[1],baseAddr[2],gAlpha);
	}else{
		if(TransRate[texinfo.abr].change&&transparent){
			glBlendFunc(TransRate[0].src,TransRate[0].dst);
			transparent=FALSE;
		}
		glColor3ub(baseAddr[0],baseAddr[1],baseAddr[2]);
	}
	glBegin(GL_POLYGON);
		glVertex2s(x1,y1);
		glVertex2s(x2,y2);
		glVertex2s(x4,y4);
		glVertex2s(x3,y3);
	glEnd();
}

void primPolyG3(unsigned char *baseAddr)
{	
/* 
	Goraud Shaded Polygon 3 vertices
*/
	short *gpuPoint=((short *) baseAddr);
	unsigned char alpha;
	short x1,x2,x3,y1,y2,y3;

	x1 = (gpuPoint[2]<<21)>>21;
	y1 = (gpuPoint[3]<<21)>>21;
	x2 = (gpuPoint[6]<<21)>>21;
	y2 = (gpuPoint[7]<<21)>>21;
	x3 = (gpuPoint[10]<<21)>>21;
	y3 = (gpuPoint[11]<<21)>>21;

	x1 += psxDraw.offsetX;
	y1 += psxDraw.offsetY;
	x2 += psxDraw.offsetX;
	y2 += psxDraw.offsetY;
	x3 += psxDraw.offsetX;
	y3 += psxDraw.offsetY;

	//gllog(77,"!GPU! P3 G %d,%d %d,%d %d,%d\n",lx0,ly0,lx1,ly1,lx2,ly2);

	if(baseAddr[3]&2){
		if(TransRate[texinfo.abr].change&&!transparent){
			glBlendFunc(TransRate[texinfo.abr].src,TransRate[texinfo.abr].dst);
			transparent=TRUE;
		}
		alpha = TransRate[texinfo.abr].alpha;
	}else{
		if(TransRate[texinfo.abr].change&&transparent){
			glBlendFunc(TransRate[0].src,TransRate[0].dst);
			transparent=FALSE;
		}
		alpha=255;
	}
	glBegin(GL_POLYGON);
		glColor4ub(baseAddr[0],baseAddr[1],baseAddr[2],alpha);
		glVertex2s(x1,y1);
		glColor4ub(baseAddr[8],baseAddr[9],baseAddr[10],alpha);
		glVertex2s(x2,y2);
		glColor4ub(baseAddr[16],baseAddr[17],baseAddr[18],alpha);
		glVertex2s(x3,y3);
	glEnd();
}

void primPolyG4(unsigned char * baseAddr)
{
/* 
	Goraud Shaded Polygon 4 vertices
*/
	short *gpuPoint=((short *) baseAddr);
	short x1,x2,x3,x4,y1,y2,y3,y4;
	unsigned char alpha;

	x1 = (gpuPoint[2]<<21)>>21;
	y1 = (gpuPoint[3]<<21)>>21;
	x2 = (gpuPoint[6]<<21)>>21;
	y2 = (gpuPoint[7]<<21)>>21;
	x3 = (gpuPoint[10]<<21)>>21;
	y3 = (gpuPoint[11]<<21)>>21;
	x4 = (gpuPoint[14]<<21)>>21;
	y4 = (gpuPoint[15]<<21)>>21;

	x1 += psxDraw.offsetX;
	y1 += psxDraw.offsetY;
	x2 += psxDraw.offsetX;
	y2 += psxDraw.offsetY;
	x3 += psxDraw.offsetX;
	y3 += psxDraw.offsetY;
	x4 += psxDraw.offsetX;
	y4 += psxDraw.offsetY;

	if(baseAddr[3]&2){
		if(TransRate[texinfo.abr].change&&!transparent){
			glBlendFunc(TransRate[texinfo.abr].src,TransRate[texinfo.abr].dst);
			transparent=TRUE;
		}
		alpha = TransRate[texinfo.abr].alpha;
	}else{
		if(TransRate[texinfo.abr].change&&transparent){
			glBlendFunc(TransRate[0].src,TransRate[0].dst);
			transparent=FALSE;
		}
		alpha=255;
	}
	glBegin(GL_POLYGON);
		glColor4ub(baseAddr[0],baseAddr[1],baseAddr[2],alpha);
		glVertex2s(x1,y1);
		glColor4ub(baseAddr[8],baseAddr[9],baseAddr[10],alpha);
		glVertex2s(x2,y2);
		glColor4ub(baseAddr[24],baseAddr[25],baseAddr[26],alpha);
		glVertex2s(x4,y4);
		glColor4ub(baseAddr[16],baseAddr[17],baseAddr[18],alpha);
		glVertex2s(x3,y3);
	glEnd();
}

void primPolyFT3(unsigned char * baseAddr)
{
/* 
	Flat Shaded Textured Polygon 3 vertices
*/
	uint32_t *gpuData = (uint32_t *)baseAddr;
	short *gpuPoint=((short *) baseAddr);
	int32_t clutP;
	short x1,x2,x3,y1,y2,y3;
	short tx0,ty0,tx1,ty1,tx2,ty2;
	unsigned short gpuDataX;


	x1 = (gpuPoint[2]<<21)>>21;
	y1 = (gpuPoint[3]<<21)>>21;
	x2 = (gpuPoint[6]<<21)>>21;
	y2 = (gpuPoint[7]<<21)>>21;
	x3 = (gpuPoint[10]<<21)>>21;
	y3 = (gpuPoint[11]<<21)>>21;

	x1 += psxDraw.offsetX;
	y1 += psxDraw.offsetY;
	x2 += psxDraw.offsetX;
	y2 += psxDraw.offsetY;
	x3 += psxDraw.offsetX;
	y3 += psxDraw.offsetY;

	tx0 = (short)(gpuData[2] & 0xff);
	ty0 = (short)((gpuData[2]>>8) & 0xff);
	tx1 = (short)(gpuData[4] & 0xff);
	ty1 = (short)((gpuData[4]>>8) & 0xff);
	tx2 = (short)(gpuData[6] & 0xff);
	ty2 = (short)((gpuData[6]>>8) & 0xff);
	//gllog(77,"!GPU! P3 F %d,%d %d,%d %d,%d\n",lx0,ly0,lx1,ly1,lx2,ly2);
	gpuDataX=(unsigned short)(gpuData[4]>>16);

	int32_t tempABR = (gpuDataX >>5) & 0x3;
	if(texinfo.abr!=tempABR){
		texinfo.abr = tempABR;
		glBlendFunc(TransRate[texinfo.abr].src,TransRate[texinfo.abr].dst);
		gAlpha=TransRate[texinfo.abr].alpha;
		transparent=TRUE;
	}
	UpdateTextureInfo(gpuDataX);

	/*switch(texinfo.abr){
	  case 0:
		glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
		gAlpha=128;
		break;
	  case 1:
		glBlendFunc(GL_ONE,GL_ONE_MINUS_SRC_ALPHA);
		gAlpha=0;
		break;
	  case 2:
		glBlendFunc(GL_ZERO,GL_ONE_MINUS_SRC_COLOR);
		gAlpha=0;
		break;
	  case 3:
		glBlendFunc(GL_SRC_ALPHA,GL_ONE);
		gAlpha=64;
		break;
	}*/
	//gllog(77," CMD: P3TF: %d,%d TP=%d",textAddrX,textAddrY,textTP);
	//gllog(77," CMD: P3TF: %d,%d %02x",lx0,ly0,baseAddr[3]);

    clutP  = (gpuData[2]>>12) & 0x7fff0;//0xffff0;
	setupTexture(clutP);
	//setPolyTexBlendMode(*gpuData);

	baseAddr[0] = texshade[baseAddr[0]];
	baseAddr[1] = texshade[baseAddr[1]];
	baseAddr[2] = texshade[baseAddr[2]];

	if(baseAddr[3]&2){
		if(TransRate[texinfo.abr].change&&!transparent){
			glBlendFunc(TransRate[texinfo.abr].src,TransRate[texinfo.abr].dst);
			transparent=TRUE;
		}
		if(baseAddr[3]&1){
			glColor4ub(255,255,255,gAlpha);
			//glColor4ub(255,255,255,255);
		}else{
			glColor4ub(baseAddr[0],baseAddr[1],baseAddr[2],gAlpha);
		}
	}else{
		//if(TransRate[texinfo.abr].change&&transparent){
			glBlendFunc(TransRate[0].src,TransRate[0].dst);
			transparent=FALSE;
		//}
		if(baseAddr[3]&1){
			glColor3ub(255,255,255);
		}else{
			glColor3ub(baseAddr[0],baseAddr[1],baseAddr[2]);
		}
	}

	glMatrixMode(GL_TEXTURE);
	glPushMatrix();
	
	short xtest = (x1-x2)*(tx0-tx1);
	if (xtest == 0)
		xtest = (x1-x3)*(tx0-tx2);
	if (xtest < 0)
		glTranslatef(1.0f, 0, 0);

	short ytest = (y1-y2)*(ty0-ty1);
	if (ytest == 0)
		ytest = (y1-y3)*(ty0-ty2);
	if (ytest < 0)
		glTranslatef(0, 1.0f, 0);

	glBegin(GL_POLYGON);
		glTexCoord2s(tx0,ty0);
		glVertex2s(x1,y1);
		glTexCoord2s(tx1,ty1);
		glVertex2s(x2,y2);
		glTexCoord2s(tx2,ty2);
		glVertex2s(x3,y3);
	glEnd();

	glPopMatrix();

	glBindTexture(GL_TEXTURE_2D,nullid);
}


void primPolyFT4(unsigned char * baseAddr)
{
/* 
	Flat Shaded Textured Polygon 4 vertices
*/

	uint32_t *gpuData = (uint32_t *)baseAddr;
	short *gpuPoint=((short *) baseAddr);
	int32_t clutP;
	short x1,x2,x3,x4,y1,y2,y3,y4;
	short tx0,ty0,tx1,ty1,tx2,ty2,tx3,ty3;
	unsigned short gpuDataX;

	x1 = (gpuPoint[2]<<21)>>21;
	y1 = (gpuPoint[3]<<21)>>21;
	x2 = (gpuPoint[6]<<21)>>21;
	y2 = (gpuPoint[7]<<21)>>21;
	x3 = (gpuPoint[10]<<21)>>21;
	y3 = (gpuPoint[11]<<21)>>21;
	x4 = (gpuPoint[14]<<21)>>21;
	y4 = (gpuPoint[15]<<21)>>21;

	x1 += psxDraw.offsetX;
	y1 += psxDraw.offsetY;
	x2 += psxDraw.offsetX;
	y2 += psxDraw.offsetY;
	x3 += psxDraw.offsetX;
	y3 += psxDraw.offsetY;
	x4 += psxDraw.offsetX;
	y4 += psxDraw.offsetY;

	tx0 = (short)(gpuData[2] & 0xff);
	ty0 = (short)((gpuData[2]>>8) & 0xff);
	tx1 = (short)(gpuData[4] & 0xff);
	ty1 = (short)((gpuData[4]>>8) & 0xff);
	tx2 = (short)(gpuData[6] & 0xff);
	ty2 = (short)((gpuData[6]>>8) & 0xff);
	tx3 = (short)(gpuData[8] & 0xff);
	ty3 = (short)((gpuData[8]>>8) & 0xff);
	//gllog(77,"!GPU! P4 F %d,%d %d,%d %d,%d %d,%d %6x\n",tx0,ty0,tx1,ty1,tx2,ty2,tx3,ty3,gpuData[0]);

	gpuDataX=(unsigned short)(gpuData[4]>>16);

	int32_t tempABR = (gpuDataX >> 5) & 0x3;
	if(texinfo.abr!=tempABR){
		texinfo.abr = tempABR;
		glBlendFunc(TransRate[texinfo.abr].src,TransRate[texinfo.abr].dst);
		gAlpha=TransRate[texinfo.abr].alpha;
		transparent=TRUE;
	}
	UpdateTextureInfo(gpuDataX);

	/*switch(texinfo.abr){
	  case 0:
		glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
		gAlpha=128;
		break;
	  case 1:
		glBlendFunc(GL_ONE,GL_ONE_MINUS_SRC_ALPHA);
		gAlpha=0;
		break;
	  case 2:
		glBlendFunc(GL_ZERO,GL_ONE_MINUS_SRC_COLOR);
		gAlpha=0;
		break;
	  case 3:
		glBlendFunc(GL_SRC_ALPHA,GL_ONE);
		gAlpha=64;
		break;
	}*/
	//gllog(77," CMD: P4TF: %d,%d TP=%d",textAddrX,textAddrY,textTP);
	//gllog(77," CMD: P4TG: %d,%d,%d,%d",baseAddr[3],baseAddr[15],baseAddr[39],baseAddr[27]);
	//gllog(77," CMD: P4TF: %d,%d %02x",lx0,ly0,baseAddr[3]);

    clutP  = (gpuData[2]>>12) & 0x7fff0;//0xffff0;
	setupTexture(clutP);
	
	if(baseAddr[3]&2){
		if(TransRate[texinfo.abr].change&&!transparent){
			glBlendFunc(TransRate[texinfo.abr].src,TransRate[texinfo.abr].dst);
			transparent=TRUE;
		}
		if(baseAddr[3]&1){
			glColor4ub(255,255,255,gAlpha);
			//glColor4ub(255,255,255,255);
		}else{
			glColor4ub(texshade[baseAddr[0]],texshade[baseAddr[1]],texshade[baseAddr[2]],gAlpha);
		}
	}else{
		//if(TransRate[texinfo.abr].change&&transparent){
			glBlendFunc(TransRate[0].src,TransRate[0].dst);
			transparent=FALSE;
		//}
		if(baseAddr[3]&1){
			glColor3ub(255,255,255);
		}else{
			glColor3ub(texshade[baseAddr[0]],texshade[baseAddr[1]],texshade[baseAddr[2]]);
		}
	}

	//verify order, otherwise it will pick wrong texture pixel if backwards
	glMatrixMode(GL_TEXTURE);
	glPushMatrix();
	
	short xtest = (x1-x2)^(tx0-tx1);
	if (xtest == 0)
		xtest = (x1-x3)^(tx0-tx2);
	if (xtest < 0)
		glTranslatef(1.0f, 0, 0);

	short ytest = (y1-y2)^(ty0-ty1);
	if (ytest == 0)
		ytest = (y1-y3)^(ty0-ty2);
	if (ytest < 0)
		glTranslatef(0, 1.0f, 0);

	glBegin(GL_POLYGON);
		glTexCoord2s(tx0,ty0);
		glVertex2s(x1,y1);
		glTexCoord2s(tx1,ty1);
		glVertex2s(x2,y2);
		glTexCoord2s(tx3,ty3);
		glVertex2s(x4,y4);
		glTexCoord2s(tx2,ty2);
		glVertex2s(x3,y3);
	glEnd();

	glPopMatrix();

	glBindTexture(GL_TEXTURE_2D,nullid);
}

void primPolyGT3(unsigned char *baseAddr)
{	
/* 
	Goraud Shaded Textured Polygon 3 vertices
*/
	uint32_t *gpuData = (uint32_t *)baseAddr;
	short *gpuPoint=((short *) baseAddr);
	int32_t clutP;
	short x1,x2,x3,y1,y2,y3;
	short tx0,ty0,tx1,ty1,tx2,ty2;
	unsigned short gpuDataX;
	unsigned char alpha;

	x1 = (gpuPoint[2]<<21)>>21;
	y1 = (gpuPoint[3]<<21)>>21;
	x2 = (gpuPoint[8]<<21)>>21;
	y2 = (gpuPoint[9]<<21)>>21;
	x3 = (gpuPoint[14]<<21)>>21;
	y3 = (gpuPoint[15]<<21)>>21;

	x1 += psxDraw.offsetX;
	y1 += psxDraw.offsetY;
	x2 += psxDraw.offsetX;
	y2 += psxDraw.offsetY;
	x3 += psxDraw.offsetX;
	y3 += psxDraw.offsetY;


	tx0 = (short)(gpuData[2] & 0xff);
	ty0 = (short)((gpuData[2]>>8) & 0xff);
	tx1 = (short)(gpuData[5] & 0xff);
	ty1 = (short)((gpuData[5]>>8) & 0xff);
	tx2 = (short)(gpuData[8] & 0xff);
	ty2 = (short)((gpuData[8]>>8) & 0xff);
	//gllog(77,"!GPU! P3TG %d,%d %d,%d %d,%d\n",lx0,ly0,lx1,ly1,lx2,ly2);

	gpuDataX=(unsigned short)(gpuData[5]>>16);
	int32_t tempABR = (gpuDataX >> 5) & 0x3;
	if(texinfo.abr!=tempABR){
		texinfo.abr = tempABR;
		glBlendFunc(TransRate[texinfo.abr].src,TransRate[texinfo.abr].dst);
		gAlpha=TransRate[texinfo.abr].alpha;
		transparent=TRUE;
	}
	UpdateTextureInfo(gpuDataX);

	/*switch(texinfo.abr){
	  case 0:
		glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
		gAlpha=128;
		break;
	  case 1:
		glBlendFunc(GL_ONE,GL_ONE_MINUS_SRC_ALPHA);
		gAlpha=0;
		break;
	  case 2:
		glBlendFunc(GL_ZERO,GL_ONE_MINUS_SRC_COLOR);
		gAlpha=0;
		break;
	  case 3:
		glBlendFunc(GL_SRC_ALPHA,GL_ONE);
		gAlpha=64;
		break;
	}*/

	//gllog(77," CMD: P3TG: %d,%d TP=%d",textAddrX,textAddrY,textTP);
	//gllog(77," CMD: P3TG: %d,%d %02x",lx0,ly0,baseAddr[3]);

    clutP  = (gpuData[2]>>12) & 0x7fff0;//0xffff0;
	setupTexture(clutP);
	if(baseAddr[3]&2){
		//if(TransRate[texinfo.abr].change&&!transparent){
			glBlendFunc(TransRate[texinfo.abr].src,TransRate[texinfo.abr].dst);
			transparent=TRUE;
		//}
		alpha=gAlpha;
	}else{
		//if(TransRate[texinfo.abr].change&&transparent){
			glBlendFunc(TransRate[0].src,TransRate[0].dst);
			transparent=FALSE;
		//}
		alpha=255;
	}

	glMatrixMode(GL_TEXTURE);
	glPushMatrix();
	
	short xtest = (x1-x2)*(tx0-tx1);
	if (xtest == 0)
		xtest = (x1-x3)*(tx0-tx2);
	if (xtest < 0)
		glTranslatef(1.0f, 0, 0);

	short ytest = (y1-y2)*(ty0-ty1);
	if (ytest == 0)
		ytest = (y1-y3)*(ty0-ty2);
	if (ytest < 0)
		glTranslatef(0, 1.0f, 0);

	glBegin(GL_POLYGON);
		glColor4ub(texshade[baseAddr[0]],texshade[baseAddr[1]],texshade[baseAddr[2]],alpha);
		glTexCoord2s(tx0,ty0);
		glVertex2s(x1,y1);
		glColor4ub(texshade[baseAddr[12]],texshade[baseAddr[13]],texshade[baseAddr[14]],alpha);
		glTexCoord2s(tx1,ty1);
		glVertex2s(x2,y2);
		glColor4ub(texshade[baseAddr[24]],texshade[baseAddr[25]],texshade[baseAddr[26]],alpha);
		glTexCoord2s(tx2,ty2);
		glVertex2s(x3,y3);
	glEnd();

	glPopMatrix();

	glBindTexture(GL_TEXTURE_2D,nullid);
}

void primPolyGT4(unsigned char *baseAddr)
{	
/* 
	Goraud Shaded Textured Polygon 4 vertices
*/
	uint32_t *gpuData = (uint32_t *)baseAddr;
	short *gpuPoint=((short *) baseAddr);
	int32_t clutP;
	short x1,y1,x2,y2,x3,y3,x4,y4;
	short tx0,ty0,tx1,ty1,tx2,ty2,tx3,ty3;
	unsigned short gpuDataX;
	unsigned char alpha;

	x1 = (gpuPoint[2]<<21)>>21;
	y1 = (gpuPoint[3]<<21)>>21;
	x2 = (gpuPoint[8]<<21)>>21;
	y2 = (gpuPoint[9]<<21)>>21;
	x3 = (gpuPoint[14]<<21)>>21;
	y3 = (gpuPoint[15]<<21)>>21;
	x4 = (gpuPoint[20]<<21)>>21;
	y4 = (gpuPoint[21]<<21)>>21;

	x1 += psxDraw.offsetX;
	y1 += psxDraw.offsetY;
	x2 += psxDraw.offsetX;
	y2 += psxDraw.offsetY;
	x3 += psxDraw.offsetX;
	y3 += psxDraw.offsetY;
	x4 += psxDraw.offsetX;
	y4 += psxDraw.offsetY;

	tx0 = (short)(gpuData[2] & 0xff);
	ty0 = (short)((gpuData[2]>>8) & 0xff);
	tx1 = (short)(gpuData[5] & 0xff);
	ty1 = (short)((gpuData[5]>>8) & 0xff);
	tx2 = (short)(gpuData[8] & 0xff);
	ty2 = (short)((gpuData[8]>>8) & 0xff);
	tx3 = (short)(gpuData[11] & 0xff);
	ty3 = (short)((gpuData[11]>>8) & 0xff);

	gpuDataX=(unsigned short)(gpuData[5]>>16);
	int32_t tempABR = (gpuDataX >> 5) & 0x3;
	if(texinfo.abr!=tempABR){
		texinfo.abr = tempABR;
		glBlendFunc(TransRate[texinfo.abr].src,TransRate[texinfo.abr].dst);
		gAlpha=TransRate[texinfo.abr].alpha;
		transparent=TRUE;
	}
	UpdateTextureInfo(gpuDataX);

	/*switch(texinfo.abr){
	  case 0:
		glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
		gAlpha=128;
		break;
	  case 1:
		glBlendFunc(GL_ONE,GL_ONE_MINUS_SRC_ALPHA);
		gAlpha=0;
		break;
	  case 2:
		glBlendFunc(GL_ZERO,GL_ONE_MINUS_SRC_COLOR);
		gAlpha=0;
		break;
	  case 3:
		glBlendFunc(GL_SRC_ALPHA,GL_ONE);
		gAlpha=64;
		break;
	}*/

	//gllog(77," CMD: P4TG: %d,%d TP=%d",textAddrX,textAddrY,textTP);
	//gllog(77," CMD: P4TG: %d,%d %02x",lx0,ly0,baseAddr[3]);

    clutP  = (gpuData[2]>>12) & 0x7fff0;//0xffff0;
	setupTexture(clutP);
	if(baseAddr[3]&2){
		//if(TransRate[texinfo.abr].change&&!transparent){
			glBlendFunc(TransRate[texinfo.abr].src,TransRate[texinfo.abr].dst);
			transparent=TRUE;
		//}
		alpha=gAlpha;
	}else{
		//if(TransRate[texinfo.abr].change&&transparent){
			glBlendFunc(TransRate[0].src,TransRate[0].dst);
			transparent=FALSE;
		//}
		alpha=255;
	}

	glMatrixMode(GL_TEXTURE);
	glPushMatrix();

	short xtest = (x1-x2)*(tx0-tx1);
	if (xtest == 0)
		xtest = (x1-x3)*(tx0-tx2);
	if (xtest < 0)
		glTranslatef(1.0f, 0, 0);

	short ytest = (y1-y2)*(ty0-ty1);
	if (ytest == 0)
		ytest = (y1-y3)*(ty0-ty2);
	if (ytest < 0)
		glTranslatef(0, 1.0f, 0);

	glBegin(GL_POLYGON);
		glColor4ub(texshade[baseAddr[0]],texshade[baseAddr[1]],texshade[baseAddr[2]],alpha);
		glTexCoord2s(tx0,ty0);
		glVertex2s(x1,y1);
		glColor4ub(texshade[baseAddr[12]],texshade[baseAddr[13]],texshade[baseAddr[14]],alpha);
		glTexCoord2s(tx1,ty1);
		glVertex2s(x2,y2);
		glColor4ub(texshade[baseAddr[36]],texshade[baseAddr[37]],texshade[baseAddr[38]],alpha);
		glTexCoord2s(tx3,ty3);
		glVertex2s(x4,y4);
		glColor4ub(texshade[baseAddr[24]],texshade[baseAddr[25]],texshade[baseAddr[26]],alpha);
		glTexCoord2s(tx2,ty2);
		glVertex2s(x3,y3);
	glEnd();

	glPopMatrix();

	glBindTexture(GL_TEXTURE_2D,nullid);
}


void primNI(unsigned char *bA)
{
	// primitive not implemented
	if (bA[3]) gllog(255,"PRIM: *NI* %02x",bA[3]);
}


unsigned char primTableC[256] =
{
	// 00
	0,0,3,0,0,0,0,0,
	// 08
	0,0,0,0,0,0,0,0,
	// 10
	0,0,0,0,0,0,0,0,
	// 18
	0,0,0,0,0,0,0,0,
	// 20
	4,4,4,4,7,7,7,7,
	// 28
	5,5,5,5,9,9,9,9,
	// 30
	6,6,6,6,9,9,9,9,
	// 38
	8,8,8,8,12,12,12,12,
	// 40
	3,3,3,3,0,0,0,0,    //  LineF2
	// 48
	5,5,5,5,6,6,6,6,	//	LineF3	LineF4
	// 50
	4,4,4,4,0,0,0,0,    //  LineG2
	// 58
	7,7,7,7,9,9,9,9,	//	LineG3	LineG4
	// 60
	3,3,3,3,4,4,4,4,	//	Tile	Sprt
	// 68
	2,2,2,2,0,0,0,0,	//	Tile1
	// 70
	2,2,2,2,3,3,3,3,	//	Tile8	Sprt8
	// 78
	2,2,2,2,3,3,3,3,	//	Tile16	Sprt16
	// 80
	4,0,0,0,0,0,0,0,
	// 88
	0,0,0,0,0,0,0,0,
	// 90
	0,0,0,0,0,0,0,0,
	// 98
	0,0,0,0,0,0,0,0,
	// a0
	3,0,0,0,0,0,0,0,
	// a8
	0,0,0,0,0,0,0,0,
	// b0
	0,0,0,0,0,0,0,0,
	// b8
	0,0,0,0,0,0,0,0,
	// c0
	3,0,0,0,0,0,0,0,
	// c8
	0,0,0,0,0,0,0,0,
	// d0
	0,0,0,0,0,0,0,0,
	// d8
	0,0,0,0,0,0,0,0,
	// e0
	0,1,1,1,1,1,1,0,
	// e8
	0,0,0,0,0,0,0,0,
	// f0
	0,0,0,0,0,0,0,0,
	// f8
	0,0,0,0,0,0,0,0

};

void (*primTableJ[256])(unsigned char *) = 
{
	// 00
	primNI,primNI,primBlkFill,primNI,primNI,primNI,primNI,primNI,
	// 08
	primNI,primNI,primNI,primNI,primNI,primNI,primNI,primNI,
	// 10
	primNI,primNI,primNI,primNI,primNI,primNI,primNI,primNI,
	// 18
	primNI,primNI,primNI,primNI,primNI,primNI,primNI,primNI,
	// 20
	primPolyF3,primPolyF3,primPolyF3,primPolyF3,primPolyFT3,primPolyFT3,primPolyFT3,primPolyFT3,
	// 28
	primPolyF4,primPolyF4,primPolyF4,primPolyF4,primPolyFT4,primPolyFT4,primPolyFT4,primPolyFT4,
	// 30
	primPolyG3,primPolyG3,primPolyG3,primPolyG3,primPolyGT3,primPolyGT3,primPolyGT3,primPolyGT3,
	// 38
	primPolyG4,primPolyG4,primPolyG4,primPolyG4,primPolyGT4,primPolyGT4,primPolyGT4,primPolyGT4,
	// 40
	primLineF2,primLineF2,primLineF2,primLineF2,primNI,primNI,primNI,primNI,
	// 48
	primNI,primNI,primNI,primNI,primNI,primNI,primNI,primNI,
	// 50
	primLineG2,primLineG2,primLineG2,primLineG2,primNI,primNI,primNI,primNI,
	// 58
	primNI,primNI,primNI,primNI,primNI,primNI,primNI,primNI,
	// 60
	primTileS,primTileS,primTileS,primTileS,primSprtS,primSprtS,primSprtS,primSprtS,
	// 68
	primNI,primNI,primNI,primNI,primNI,primNI,primNI,primNI,
	// 70
	primNI,primNI,primNI,primNI,primSprt8,primSprt8,primSprt8,primSprt8,
	// 78
	primNI,primNI,primNI,primNI,primSprt16,primSprt16,primSprt16,primSprt16,
	// 80
	primMoveImage,primNI,primNI,primNI,primNI,primNI,primNI,primNI,
	// 88
	primNI,primNI,primNI,primNI,primNI,primNI,primNI,primNI,
	// 90
	primNI,primNI,primNI,primNI,primNI,primNI,primNI,primNI,
	// 98
	primNI,primNI,primNI,primNI,primNI,primNI,primNI,primNI,
	// a0
	primLoadImage,primNI,primNI,primNI,primNI,primNI,primNI,primNI,
	// a8
	primNI,primNI,primNI,primNI,primNI,primNI,primNI,primNI,
	// b0
	primNI,primNI,primNI,primNI,primNI,primNI,primNI,primNI,
	// b8
	primNI,primNI,primNI,primNI,primNI,primNI,primNI,primNI,
	// c0
	primStoreImage,primNI,primNI,primNI,primNI,primNI,primNI,primNI,
	// c8
	primNI,primNI,primNI,primNI,primNI,primNI,primNI,primNI,
	// d0
	primNI,primNI,primNI,primNI,primNI,primNI,primNI,primNI,
	// d8
	primNI,primNI,primNI,primNI,primNI,primNI,primNI,primNI,
	// e0
	primNI,cmdTexturePage,cmdTextureWindow,cmdDrawAreaStart,cmdDrawAreaEnd,cmdDrawOffset,cmdSTP,primNI,
	// e8
	primNI,primNI,primNI,primNI,primNI,primNI,primNI,primNI,
	// f0
	primNI,primNI,primNI,primNI,primNI,primNI,primNI,primNI,
	// f8
	primNI,primNI,primNI,primNI,primNI,primNI,primNI,primNI
};

