// bdf2bmp  --  generate bmp-image-file from bdf-font-file
// version: 0.4
// date: Fri Dec 29 14:50:52 2000
// author: ITOU Hiroki (itouh@lycos.ne.jp)

/*
 * Copyright (c) 2000 ITOU Hiroki
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

//modify if you like; color of spacing: default #a0a0c4
#define COLOR_SPACING_RED 0xa0
#define COLOR_SPACING_GREEN 0xa0
#define COLOR_SPACING_BLUE 0xc4

#define VERBOSE

//if your computer is MSB-first(BigEndian),
//  please delete the next line, or please change 'LSB' to 'MSB'
#define LSB

#include <stdio.h>  //printf(), fopen(), fwrite()
#include <stdlib.h> //malloc(), EXIT_SUCCESS, strtol(), exit()
#include <string.h> //strcmp(), strcpy()
#include <limits.h> //strtol()

#define LINE_CHARMAX 1000 //number of max characters in bdf-font-file; number is without reason
#define FILENAME_CHARMAX 256 //number of max characters in filenames;  number is without reason
#define ON 1 //number is without reason; only needs the difference to OFF
#define OFF 0 //number is without reason; only needs the difference to ON
#ifdef DEBUG
#define d_printf(message,arg) printf(message,arg)
#else /* not DEBUG */
#define d_printf(message,arg)
#endif /* DEBUG */

#ifdef VERBOSE
#define v_printf(message,arg) printf(message,arg)
#else /* not VERBOSE */
#define v_printf(message,arg)
#endif /* VERBOSE */

//macro
#define STOREBITMAP()   if(flagBitmap == ON){\
                            memcpy(nextP, sP, length);\
                            nextP += length;\
                        }

struct boundingbox{
	int w; //width (pixel)
	int h; //height
	int offx; //offset y (pixel)
	int offy; //offset y
};

//global var
struct boundingbox font; //global info
static int chars; //number of Total glyphs
unsigned long bmpTotalSize; //bmp filesize (byte)

//func prototype
unsigned char *mwrite(const void *_p, int n, unsigned char *dstP);
unsigned char *assignBmp(unsigned char *bitmapP, int spacing, int colchar);
void assignBitmap(unsigned char *bitmapP, unsigned char *glyphP, int sizeglyphP, struct boundingbox glyph);
int getline(char* lineP, int max, FILE* inputP);
unsigned char *readBdfFile(unsigned char *bitmapP, FILE *readP);
void printhelp(void);
int main(int argc, char *argv[]);



// write to memory with arranging LSBfirst(LittleEndian)
//   parameter: data-address, the data size, address to write
//   return value; address to write next
unsigned char *mwrite(const void *_p, int n, unsigned char *dstP){
	const unsigned char *p = _p;
	int i;
#ifdef LSB
	//write without arranging if your machine is LSB(LittleEndian)
	// because bmp-image-file is defined as LittleEndian
	for(i=0; i<n; i++){
		*dstP = *(p+i);
		dstP++;
	}
#else /* not LSB */
	//write with arranging if your machine is MSB(BigEndian)
	for(i=n-1; i>=0; i--){
		*dstP = *(p+i);
		dstP++;
	}
#endif /* LSB */
	return dstP;
}



// 3. transfer bitmapAREA(onMemory) to bmpAREA(onMemory) with adding spacing
// BMP-file: noCompression(BI_RGB), 8bitColor, Windows-Win32 type
// return value: bmpADDRESS
unsigned char *assignBmp(unsigned char *bitmapP, int spacing, int colchar){
	long bmpw; //bmp-image width (pixel)
	long bmph; //bmp-image height (pixel)
	unsigned char *dstP; //the writing address of bmpAREA
	unsigned char *bmpP; //the top address of bmpAREA
	int bmppad; //number of padding pixels: bmp-file-spec defines 
	            //  bmp-lines needs to be long alined and padded with 0
	unsigned long u_long;
	unsigned short u_short;
	signed long s_long;
	unsigned char u_char;
	int i,x,y,g,tmp;
	int rowchar; //number of row glyphs
	int bx, by;

	//bmp-image width
	bmpw = (font.w+spacing)*colchar + spacing;

	//bmp-image height
	rowchar = (chars/colchar) + (chars%colchar!=0);
	bmph = (font.h+spacing)*rowchar + spacing;

	v_printf("  BMP width=%dpixels\n", (int)bmpw);
	v_printf("  BMP height=%dpixels\n", (int)bmph);
	d_printf("number of glyphs column=%d ", colchar);
	d_printf("row=%d\n", rowchar);

	//allocate bmpAREA
	bmppad = ((bmpw + 3) / 4 * 4) - bmpw;
	bmpTotalSize = (bmpw + bmppad) * bmph
		+ sizeof(long)*11 + sizeof(short)*5 + sizeof(char)*4*256;
	bmpP = (unsigned char *)malloc(bmpTotalSize);
	if(bmpP == NULL){
		printf("error malloc bmp");
		exit(EXIT_FAILURE);
	}
	memset(bmpP, 0, bmpTotalSize); //zero-clear
	v_printf("  BMP filesize=%dbytes\n", (int)bmpTotalSize);

	//BITMAPFILEHEADER struct
	u_short = 0x4d42;
	dstP = mwrite(&u_short, 2, bmpP); //4d = 'M', 42 = 'B'
	dstP = mwrite(&bmpTotalSize, 4, dstP);
	u_short = 0x00;
	dstP = mwrite(&u_short, 2, dstP); // 2 means sizeof(unsigned short)
	dstP = mwrite(&u_short, 2, dstP);
	u_long = sizeof(long)*11 + sizeof(short)*5 + sizeof(char)*4*256;
	dstP = mwrite(&u_long, 4, dstP); //bfOffBits: offset to image-data array

	//BITMAPINFOHEADER struct
	u_long = 40;
	dstP = mwrite(&u_long, 4, dstP); //when Windows-BMP, this is 40
	dstP = mwrite(&bmpw, 4, dstP); //biWidth
	s_long = bmph;
	dstP = mwrite(&s_long, 4, dstP); //biHeight: plus value -> down-top
	u_short = 1;
	dstP = mwrite(&u_short, 2, dstP); //biPlanes: must be 1
	u_short = 8;
	dstP = mwrite(&u_short, 2, dstP); //biBitCount: 8bitColor
	u_long = 0;
	dstP = mwrite(&u_long, 4, dstP); //biCompression: noCompression(BI_RGB)
	dstP = mwrite(&u_long, 4, dstP); //biSizeImage: when noComprsn, 0 is ok
	s_long = 0;
	dstP = mwrite(&s_long, 4, dstP); //biXPelsPerMeter: resolution x, 0 ok
	dstP = mwrite(&s_long, 4, dstP); //biYPelsPerMeter: res y, 0 is ok
	u_long = 0;
	dstP = mwrite(&u_long, 4, dstP); //biClrUsed: optimized color palette, not used
	dstP = mwrite(&u_long, 4, dstP); //biClrImportant: 0 is ok

	//RGBQUAD[256]: color palette
	//  palette[0]: background of glyphs
	u_char = 0xff;
	dstP = mwrite(&u_char, 1, dstP); //rgbBlue: B
	dstP = mwrite(&u_char, 1, dstP); //rgbGreen: G
	dstP = mwrite(&u_char, 1, dstP); //rgbRed: R;  palette[0]: #ffffff
	u_char = 0;
	dstP = mwrite(&u_char, 1, dstP); //rgbReserved: must be 0

	//  palette[1]: foreground of glyphs
	u_char = 0;
	for(i=0; i<4; i++)
		dstP = mwrite(&u_char, 1, dstP); //palette[1]: color #000000

	//  palette[2]: spacing
	u_char = COLOR_SPACING_BLUE;
	dstP = mwrite(&u_char, 1, dstP); //B
	u_char = COLOR_SPACING_GREEN;
	dstP = mwrite(&u_char, 1, dstP); //G
	u_char = COLOR_SPACING_RED;
	dstP = mwrite(&u_char, 1, dstP); //R
	u_char = 0;
	dstP = mwrite(&u_char, 1, dstP); //must be 0
		
	//  palette[3] to palette[255]: not used
	for(i=3; i<256; i++){
		u_char = 0x00;
		dstP = mwrite(&u_char, 1, dstP);
		dstP = mwrite(&u_char, 1, dstP);
		dstP = mwrite(&u_char, 1, dstP);
		dstP = mwrite(&u_char, 1, dstP); //palette[3to255]: #000000
	}

	//IMAGE DATA array
	for(y=bmph-1; y>=0; y--){
		for(x=0; x<bmpw+bmppad; x++){
			if(x>=bmpw){
				//padding: long aligned
				u_char = 0; //must pad with 0
				dstP = mwrite(&u_char, 1, dstP);
			}else{
				if( (y%(font.h+spacing)<spacing) || (x%(font.w+spacing)<spacing) ){
					//spacing
					u_char = 2; //fill palette[2]
					dstP = mwrite(&u_char, 1, dstP);
				}else{
					//read bitmapAREA & write bmpAREA
					g = (x/(font.w+spacing)) + (y/(font.h+spacing)*colchar);
					//d_printf("g=%d ", g);
					bx = x - (spacing*(g%colchar)) - spacing;
					by = y - (spacing*(g/colchar)) - spacing;
					//d_printf("bx=%d ", bx);
					//d_printf("by=%d\n", by);
					tmp = g*(font.h*font.w) + (by%font.h)*font.w + (bx%font.w);
					if(tmp >= chars*font.h*font.w){
						//required over bitmapAREA
						u_char = 2; //fill palette[2]
					}else
						u_char = *( bitmapP + tmp);
					dstP = mwrite(&u_char, 1, dstP);
				}
			}
		}
	}
	return bmpP;
}

	


//2. transfer bdf-font-file to bitmapAREA(onMemory) one glyph by one
void assignBitmap(unsigned char *bitmapP, unsigned char *glyphP, int sizeglyphP, struct boundingbox glyph){
	static char* hex2binP[]= {
		"0000","0001","0010","0011","0100","0101","0110","0111",
		"1000","1001","1010","1011","1100","1101","1110","1111"
	};
	int d; //decimal
	int hexlen; //a line length(without newline code)
	unsigned char binP[LINE_CHARMAX]; //temporal buffer of binary strings
	static int nowchar = 0; //ordinal number of writing glyphs now
	unsigned char* tmpP;
	char tmpsP[LINE_CHARMAX];
	int bitAh, bitAw; //bitA width, height
	int topoff, leftoff; //glyph offset top, left
	unsigned char* bitAP;
	unsigned char* bitBP;
	int i,j,x,y;
	
	//2.1) change hexadecimal strings to a bitmap of glyph( called bitA)
	tmpP = strstr(glyphP, "\n");
	if(tmpP == NULL){
		//if there is BITMAP\nENDCHAR in a given bdf-file(e.g. UTRG__18.bdf)
		tmpP = glyphP + 2;
		sizeglyphP = 3;
	}
	hexlen = tmpP - glyphP;
	bitAw = hexlen*4;
	bitAh = sizeglyphP/(hexlen+1);
	bitAP = malloc(bitAw * bitAh); //address of bitA
	if(bitAP == NULL){
		printf("error bitA bitmap\n");
		d_printf("&tmpP=%p ", tmpP);
		d_printf("&glyphP=%p ", glyphP);
		d_printf("sizeglyphP=%d\n", sizeglyphP);
		d_printf("bitAw=%d ", bitAw);
		d_printf("bitAh=%d\n", bitAh);
		exit(EXIT_FAILURE);
	}
	for(i=0,x=0,y=0; i<sizeglyphP; i++){
		if(glyphP[i] == '\n'){
			x=0; y++;
		}else{
			sprintf(tmpsP, "0x%c", glyphP[i]); //get one character from hexadecimal strings
			d = (int)strtol(tmpsP,(char **)NULL, 16);
			strcpy(binP, hex2binP[d]); //change hexa strings to bin strings
			for(j=0; j<4; j++,x++)
				*(bitAP + y*bitAw + x) = binP[j] - '0';
		}
	}

	//2.2)make an one-glyph bitmap area(called bitB)
	bitBP = malloc(font.w * font.h); //address of bitB
	if(bitBP == NULL){
		printf("error bitB bitmap");
		exit(EXIT_FAILURE);
	}
	memset(bitBP, 0, font.w*font.h); //fill palette[0]

	//2.3) copy bitA to bitB with shifting(under the offsets)
	//   bitA(smaller) to bitB(bigger), a scope beyond bitA is already filled with palette[0]
	topoff = font.h - glyph.h - ((-1)*font.offy+glyph.offy);
	leftoff = (-1)*font.offx + glyph.offx;
	for(i=0; i<glyph.h; i++)
		for(j=0; j<glyph.w; j++)
			*(bitBP + (i+topoff)*font.w + (j+leftoff))
				= *(bitAP + i*bitAw + j);

	//2.4) copy bitB to bitmapAREA
	for(i=0; i<font.h; i++)
		for(j=0; j<font.w; j++)
			*(bitmapP + (nowchar*font.w*font.h) + (i*font.w) + j) = *(bitBP + i*font.w + j);

	nowchar++;
}

//read oneline from textfile
int getline(char* lineP, int max, FILE* inputP){
	if (fgets(lineP, max, inputP) == NULL)
		return 0;
	else
		return strlen(lineP); //fgets returns strings included '\n'
}

unsigned char *readBdfFile(unsigned char *bitmapP, FILE *readP){
	int length;
	char sP[LINE_CHARMAX]; //one line(strings) from bdf-font-file
	//int i,j;
	static int cnt; //only used in debugging: counter of appeared glyphs
	struct boundingbox glyph; //an indivisual glyph width, height,offset x,y
	int flagBitmap = OFF; //this line is bitmap-data?(ON) or no?(OFF)
	char *tokP; //top address of a token from strings
	char *glyphP = NULL; //temporal buffer of bitmap-data(hexadecimal strings)
	char* nextP; //address of writing next in glyphP

	while(1){
		length = getline(sP, LINE_CHARMAX, readP);
		if(length == 0)
			break; //escape from while-loop

		//classify from the top character of sP
		switch(sP[0]){
		case 'F':
			if(strncmp(sP, "FONTBOUNDINGBOX ", 16) == 0){
				// 16 means no comparing '\0'

				//get font.w, font.h, font.offx, and font.offy
				tokP = strtok(sP, " ");//tokP addresses next space of FONTBOUNDINGBOX
				tokP += (strlen(tokP)+1);//tokP addresses top character of width in FONTBOUNDINGBOX
				tokP = strtok(tokP, " ");//set NUL on space after width
				font.w = atoi(tokP);
				tokP += (strlen(tokP)+1);//height in FONTBOUNDINGBOX
				tokP = strtok(tokP, " ");
				font.h = atoi(tokP);
				tokP += (strlen(tokP)+1);
				tokP = strtok(tokP, " ");
				font.offx = atoi(tokP);
				tokP += (strlen(tokP)+1);
				tokP = strtok(tokP, "\n");
				font.offy = atoi(tokP);
				d_printf("global glyph width=%dpixels ",font.w);
				d_printf("height=%dpixels\n",font.h);
				d_printf("global glyph offset x=%dpixels ",font.offx);
				d_printf("y=%dpixels\n",font.offy);
			}else
				STOREBITMAP();
			break;
		case 'C':
			if(strncmp(sP, "CHARS ", 6) == 0){
				//get chars
				tokP = strtok(sP, " ");
				tokP += (strlen(tokP)+1);
				tokP = strtok(tokP, "\n");
				chars = atoi(tokP);
				v_printf("  Total chars=%d\n",chars);
				cnt=0;

				//allocate bitmapAREA
				bitmapP = (unsigned char*)malloc(chars * font.h * font.w );
				if(bitmapP == NULL){
					printf("error malloc");
					exit(EXIT_FAILURE);
				}
				//d_printf("malloc %dbytes\n",chars*font.h*font.w);
			}else
				STOREBITMAP();
			break;
		case 'B':
			if(strncmp(sP, "BITMAP", 6) == 0){
				//allocate glyphP
				glyphP = (unsigned char*)malloc(font.w*font.h); //allocate more room
				if(glyphP == NULL){
					printf("error malloc bdf");
					exit(EXIT_FAILURE);
				}
				//d_printf("malloc: glyphP=%p\n",glyphP);
				memset(glyphP, 0, font.w*font.h); //zero clear
				nextP = glyphP;
				flagBitmap = ON;
			}else if(strncmp(sP, "BBX ", 4) == 0){
				//get glyph.offx, glyph.offy, glyph.w, and glyph.h
				tokP = strtok(sP, " ");//space after 'BBX'
				tokP += (strlen(tokP)+1);//top of width
				tokP = strtok(tokP, " ");//set NUL on space after width
				glyph.w = atoi(tokP);
				tokP += (strlen(tokP)+1);//height
				tokP = strtok(tokP, " ");
				glyph.h = atoi(tokP);
				tokP += (strlen(tokP)+1);//offx
				tokP = strtok(tokP, " ");
				glyph.offx = atoi(tokP);
				tokP += (strlen(tokP)+1);//offy
				tokP = strtok(tokP, "\n");
				glyph.offy = atoi(tokP);
				//d_printf("glyph width=%dpixels ",glyph.w);
				//d_printf("height=%dpixels\n",glyph.h);
				//d_printf("glyph offset x=%dpixels ",glyph.offx);
				//d_printf("y=%dpixels\n",glyph.offy);
			}else
				STOREBITMAP();
			break;
		case 'E':
			if(strncmp(sP, "ENDCHAR", 7) == 0){
				d_printf("\nglyph %d\n", cnt);
				d_printf("%s\n",glyphP);
				assignBitmap(bitmapP, glyphP, nextP - glyphP, glyph);
				flagBitmap = OFF;
				//d_printf("nextP-glyphP=%d\n",nextP-glyphP);
				free(glyphP);
				cnt++;
			}else
				STOREBITMAP();
			break;
		default:
			STOREBITMAP();
			break;
		}//switch
	}//while
	//'break' goto here
	return bitmapP;
}


void printhelp(void){
	printf("bdf2bmp version 0.4\n");
	printf("Usage: bdf2bmp [-option] input-bdf-file output-bmp-file\n");
	printf("Option:\n");
	printf("  -sN    spacing N pixels (default N=2)\n");
	printf("             N is limited from 0 to 32\n");
	printf("             DO NOT INSERT SPACE BETWEEN 's' AND 'N'\n");
	printf("  -cN    specifying N colomns in grid (default N=32)\n");
	printf("             N is limited from 1 to 65536\n");
	printf("             DO NOT INSERT SPACE BETWEEN 'c' AND 'N'\n");
	printf("  -h     print help\n");
	exit(EXIT_FAILURE);
}


int main(int argc, char *argv[]){
	FILE *readP;
	FILE *writeP;
	char readFilename[FILENAME_CHARMAX] = "input.bdf";
	char writeFilename[FILENAME_CHARMAX] = "output.bmp";
	int tmp, i;
	unsigned char *bmpP; //address of bmpAREA
	unsigned char *bitmapP; //address of bitmapAREA
	int spacing = 2; //breadth of spacing (default 2)
	int flag;
	int colchar = 32; //number of columns(horizontal) (default 32)

	//deal with arguments
	if(argc < 2){
		//not enough arguments
		//printf("error: not enough arguments !\n");
		printhelp();
	}
	for(i=1, flag=0; i<argc; i++){
		switch( argv[i][0] ){
		case '-':
			if(argv[i][1] == 's')
				spacing = atoi(&argv[i][2]);
			if(argv[i][1] == 'c')
				colchar = atoi(&argv[i][2]);
			else if( (argv[i][1]=='v') || (argv[i][1]=='h'))
				printhelp();
			break;
		default:
			if(flag == 0){
				strcpy(readFilename, argv[i]);
				flag ++;
			}else{
				strcpy(writeFilename, argv[i]);
				if(strcmp(readFilename, writeFilename) == 0){
					printf("error: input-filename and output-filename are same\n");
					exit(EXIT_FAILURE);
				}
				flag ++;
			}
			break;
		}
	}

	if(flag < 2){
		printf("error; not enough arguments !\n");
		printhelp();
	}

	//colchar is limited from 1 to 65536
	if(colchar  < 1)
		colchar = 1;
	else if(colchar > 65536)
		colchar = 65536;

	//spacing is limited from 0 to 32
	if(spacing < 0)
		spacing = 0;
	else if(spacing > 32)
		spacing = 32;

	//prepare to read&write files
	readP = fopen(readFilename, "r");
	if(readP == NULL){
		printf("error: %s does not exist!\n", readFilename);
		exit(EXIT_FAILURE);
	}
	writeP=fopen(writeFilename, "wb");
	if(writeP == NULL){
		printf("error: cannot write %s !\n", writeFilename);
		exit(EXIT_FAILURE);
	}

	//read bdf-font-file
	bitmapP = readBdfFile(bitmapP, readP);
	fclose(readP);

	//write bmp-image-file
	bmpP = assignBmp(bitmapP, spacing, colchar);
	tmp = fwrite(bmpP, 1, bmpTotalSize, writeP);
	if((unsigned long)tmp != bmpTotalSize){
		printf("error: cannot write %s !\n", writeFilename);
		free(bitmapP);
		free(bmpP);
		exit(EXIT_FAILURE);
	}
	tmp = fclose(writeP);
	if(tmp == EOF){
		printf("error: cannot write %s !\n", writeFilename);
		free(bitmapP);
		free(bmpP);
		exit(EXIT_FAILURE);
	}
	free(bitmapP);
	free(bmpP);
	return EXIT_SUCCESS;
}
