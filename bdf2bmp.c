/*
 * bdf2bmp  --  output all glyphs in a bdf-font to a bmp-image-file
 * version: 0.5.2
 * date:    Thu Jan 04 00:54:00 2001
 * author:  ITOU Hiroki (itouh@lycos.ne.jp)
 */

/*
 * Copyright (c) 2000,2001 ITOU Hiroki
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
//modify if you like; color of out-of-dwidth: default #d4d4dd
#define COLOR_DWIDTH_RED 0xd4
#define COLOR_DWIDTH_GREEN 0xd4
#define COLOR_DWIDTH_BLUE 0xdd

#define VERBOSE


/* If your computer is MSB-first(BigEndian), please delete the next line. */
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
struct boundingbox font; //global boundingbox
static int chars; //total number of glyphs in a bdf file
static int dwflag = OFF; //device width; only used for proportional-fonts

//func prototype
void dwrite(const void *ptrP, int n, FILE *outP);
void writeBmpFile(unsigned char *bitmapP, int spacing, int colchar, FILE *bmpP);
void assignBitmap(unsigned char *bitmapP, char *glyphP, int sizeglyphP, struct boundingbox glyph, int dw);
int getline(char* lineP, int max, FILE* inputP);
unsigned char *readBdfFile(unsigned char *bitmapP, FILE *readP);
void printhelp(void);
int main(int argc, char *argv[]);



/*
 * write to disk; with arranging LSBfirst(LittleEndian) byte order,
 *                 because BMP-file is defined as LSB-first
 */
void dwrite(const void *ptrP, int n, FILE *outP){
        const unsigned char *p = ptrP;
        int i;
        unsigned char tmp;
#ifdef LSB
        //write without arranging if your machine is LSB(LittleEndian)
        for(i=0; i<n; i++){
#else /* not LSB */
        //write with arranging if your machine is MSB(BigEndian)
        for(i=n-1; i>=0; i--){
#endif /* LSB */
                tmp = fwrite(p+i, 1, sizeof(unsigned char), outP);
                if(tmp != sizeof(unsigned char)){
                        printf("error: cannot write an output-file\n");
                        exit(EXIT_FAILURE);
                }
        }
}



/*
 * 3. read bitmapAREA(onMemory) and write bmpFile with adding spacing
 *    BMP-file: noCompression(BI_RGB), 8bitColor, Windows-Win32 type
 */
void writeBmpFile(unsigned char *bitmapP, int spacing, int colchar, FILE *bmpP){
        long bmpw; //bmp-image width (pixel)
        long bmph; //bmp-image height (pixel)
        int bmppad; //number of padding pixels
        unsigned long bmpTotalSize; //bmp filesize (byte)
        // bmp-lines needs to be long alined and padded with 0
        unsigned long ulong;
        unsigned short ushort;
        signed long slong;
        unsigned char uchar;
        int i,x,y,g,tmp;
        int rowchar; //number of row glyphs
        int bx, by;

        //bmp-image width
        bmpw = (font.w+spacing)*colchar + spacing;

        //bmp-image height
        rowchar = (chars/colchar) + (chars%colchar!=0);
        bmph = (font.h+spacing)*rowchar + spacing;

        v_printf("  BMP width = %d pixels\n", (int)bmpw);
        v_printf("  BMP height = %d pixels\n", (int)bmph);
        d_printf("  number of glyphs column=%d ", colchar);
        d_printf("row=%d\n", rowchar);

        bmppad = ((bmpw + 3) / 4 * 4) - bmpw;
        bmpTotalSize = (bmpw + bmppad) * bmph
                + sizeof(long)*11 + sizeof(short)*5 + sizeof(char)*4*256;
        v_printf("  BMP filesize = %d bytes\n", (int)bmpTotalSize);


        /*
         * BITMAPFILEHEADER struct
         */
        ushort = 0x4d42; //4d = 'M', 42 = 'B'
        dwrite(&ushort, sizeof(ushort), bmpP);
        ulong = bmpTotalSize;
        dwrite(&ulong, sizeof(ulong), bmpP);
        ushort = 0x00;
        dwrite(&ushort, sizeof(ushort), bmpP); //reserved as 0
        dwrite(&ushort, sizeof(ushort), bmpP); //reserved as 0

        //bfOffBits: offset to image-data array
        ulong = sizeof(long)*11 + sizeof(short)*5 + sizeof(char)*4*256;
        dwrite(&ulong, sizeof(ulong), bmpP);


        /*
         * BITMAPINFOHEADER struct
         */
        ulong = 40;
        dwrite(&ulong, sizeof(ulong), bmpP); //when Windows-BMP, this is 40
        slong = bmpw;
        dwrite(&slong, sizeof(slong), bmpP); //biWidth
        slong = bmph;
        dwrite(&slong, sizeof(slong), bmpP); //biHeight: plus value -> down-top
        ushort = 1;
        dwrite(&ushort, sizeof(ushort), bmpP); //biPlanes: must be 1
        ushort = 8;
        dwrite(&ushort, sizeof(ushort), bmpP); //biBitCount: 8bitColor
        ulong = 0;
        dwrite(&ulong, sizeof(ulong), bmpP); //biCompression: noCompression(BI_RGB)
        dwrite(&ulong, sizeof(ulong), bmpP); //biSizeImage: when noComprsn, 0 is ok
        slong = 0;
        dwrite(&slong, sizeof(slong), bmpP); //biXPelsPerMeter: resolution x; 0 ok
        dwrite(&slong, sizeof(slong), bmpP); //biYPelsPerMeter: res y; 0 is ok
        ulong = 0;
        dwrite(&ulong, sizeof(ulong), bmpP); //biClrUsed: optimized color palette; not used
        dwrite(&ulong, sizeof(ulong), bmpP); //biClrImportant: 0 is ok

        /*
         * RGBQUAD[256]: color palette
         */
        //  palette[0]: background of glyphs
        uchar = 0xff;
        dwrite(&uchar, sizeof(uchar), bmpP); //rgbBlue: B
        dwrite(&uchar, sizeof(uchar), bmpP); //rgbGreen: G
        dwrite(&uchar, sizeof(uchar), bmpP); //rgbRed: R;  palette[0]: #ffffff
        uchar = 0;
        dwrite(&uchar, sizeof(uchar), bmpP); //rgbReserved: must be 0

        //  palette[1]: foreground of glyphs
        uchar = 0;
        for(i=0; i<4; i++)
                dwrite(&uchar, sizeof(uchar), bmpP); //palette[1]: #000000

        //  palette[2]: spacing
        uchar = COLOR_SPACING_BLUE;
        dwrite(&uchar, sizeof(uchar), bmpP); //B
        uchar = COLOR_SPACING_GREEN;
        dwrite(&uchar, sizeof(uchar), bmpP); //G
        uchar = COLOR_SPACING_RED;
        dwrite(&uchar, sizeof(uchar), bmpP); //R
        uchar = 0;
        dwrite(&uchar, sizeof(uchar), bmpP); //must be 0

        //  palette[3]: out of dwidth
        uchar = COLOR_DWIDTH_BLUE;
        dwrite(&uchar, sizeof(uchar), bmpP); //B
        uchar = COLOR_DWIDTH_GREEN;
        dwrite(&uchar, sizeof(uchar), bmpP); //G
        uchar = COLOR_DWIDTH_RED;
        dwrite(&uchar, sizeof(uchar), bmpP); //R
        uchar = 0;
        dwrite(&uchar, sizeof(uchar), bmpP); //must be 0
                
        //  palette[4] to palette[255]: not used
        for(i=4; i<256; i++){
                uchar = 0x00;
                dwrite(&uchar, sizeof(uchar), bmpP);
                dwrite(&uchar, sizeof(uchar), bmpP);
                dwrite(&uchar, sizeof(uchar), bmpP);
                dwrite(&uchar, sizeof(uchar), bmpP); //palette[4to255]: #000000
        }

        /*
         * IMAGE DATA array
         */
        for(y=bmph-1; y>=0; y--){
                for(x=0; x<bmpw+bmppad; x++){
                        if(x>=bmpw){
                                //padding: long(4bytes) aligned
                                uchar = 0; //must pad with 0
                                dwrite(&uchar, sizeof(uchar), bmpP);
                        }else{
                                if( (y%(font.h+spacing)<spacing) || (x%(font.w+spacing)<spacing) ){
                                        //spacing
                                        uchar = 2; //fill palette[2]
                                        dwrite(&uchar, sizeof(uchar), bmpP);
                                }else{
                                        //read bitmapAREA & write bmpFile
                                        g = (x/(font.w+spacing)) + (y/(font.h+spacing)*colchar);
                                        bx = x - (spacing*(g%colchar)) - spacing;
                                        by = y - (spacing*(g/colchar)) - spacing;
                                        tmp = g*(font.h*font.w) + (by%font.h)*font.w + (bx%font.w);
                                        if(tmp >= chars*font.h*font.w){
                                                //spacing over the last glyph
                                                uchar = 2; //fill palette[2]
                                        }else
                                                uchar = *( bitmapP + tmp);
                                        dwrite(&uchar, sizeof(uchar), bmpP);
                                }
                        }
                }
        }
        return;
}

        


/*
 * 2. transfer bdf-font-file to bitmapAREA(onMemory) one glyph by one
 */
void assignBitmap(unsigned char *bitmapP, char *glyphP, int sizeglyphP, struct boundingbox glyph, int dw){
        static char *hex2binP[]= {
                "0000","0001","0010","0011","0100","0101","0110","0111",
                "1000","1001","1010","1011","1100","1101","1110","1111"
        };
        int d; //decimal number translated from hexNumber
        int hexlen; //a line length(without newline code)
        char binP[LINE_CHARMAX]; //binary strings translated from decimal number
        static int nowchar = 0; //number of glyphs handlled until now
        char *tmpP;
        char tmpsP[LINE_CHARMAX];
        int bitAh, bitAw; //bitA width, height
        int offtop, offbottom, offright, offleft; //glyph offset
        unsigned char *bitAP;
        unsigned char *bitBP;
        int i,j,x,y;
        
        /*
         * 2.1) change hexadecimal strings to a bitmap of glyph( called bitA)
         */
        tmpP = strstr(glyphP, "\n");
        if(tmpP == NULL){
                //if there is BITMAP\nENDCHAR in a given bdf-file(e.g. UTRG__18.bdf)
                *glyphP = '0';
                *(glyphP+1) = '0';
                *(glyphP+2) = '\n';
                tmpP = glyphP + 2;
                sizeglyphP = 3;
        }
        hexlen = tmpP - glyphP;
        bitAw = hexlen * 4;
        bitAh = sizeglyphP / (hexlen+1);
        bitAP = malloc(bitAw * bitAh); //address of bitA
        if(bitAP == NULL){
                printf("error bitA malloc\n");
                exit(EXIT_FAILURE);
        }
        for(i=0,x=0,y=0; i<sizeglyphP; i++){
                if(glyphP[i] == '\n'){
                        x=0; y++;
                }else{
                        sprintf(tmpsP, "0x%c", glyphP[i]); //get one character from hexadecimal strings
                        d = (int)strtol(tmpsP,(char **)NULL, 16);
                        strcpy(binP, hex2binP[d]); //change hexa strings to bin strings
                        for(j=0; j<4; j++,x++){
                                if( bitAP+y*bitAw+x > bitAP+bitAw*bitAh ){
                                        printf("error: bitA pointer\n");
                                        exit(EXIT_FAILURE);
                                }else{
                                        *(bitAP + y*bitAw + x) = binP[j] - '0';
                                }
                        }
                }
        }

        /*
         * 2.2)make another bitmap area(called bitB)
         *      bitB is sized to FONTBOUNDINGBOX
         */
        bitBP = malloc(font.w * font.h); //address of bitB
        if(bitBP == NULL){
                printf("error bitB malloc\n");
                exit(EXIT_FAILURE);
        }
        for(i=0; i<font.h; i++){
                for(j=0; j<font.w; j++){
                        if(dwflag == OFF){
                                *(bitBP + i*font.w + j) = 0; //fill palette[0]
                        }else{
                                if( (j < (-1)*font.offx) || (j >= (-1)*font.offx+dw))
                                        *(bitBP + i*font.w + j) = 3; //fill palette[3]
                                else
                                        *(bitBP + i*font.w + j) = 0; //fill palette[0]
                        }
                }
        }

        /*
         * 2.3) copy bitA inside BBX (smaller) to bitB (bigger)
         *       with offset-shifting;
         *      a scope beyond bitA is already filled with palette[0or3]
         */
        offleft = (-1)*font.offx + glyph.offx;
        //offright = font.w - glyph.w - offleft;

        offbottom = (-1)*font.offy + glyph.offy;
        offtop = font.h - glyph.h - offbottom;

        for(i=0; i<font.h; i++){
                if( i<offtop || i>=offtop+glyph.h )
                        ; //do nothing
                else
                        for(j=0; j<font.w; j++)
                                if( j<offleft || j>=offleft+glyph.w )
                                        ; //do nothing
                                else
                                        *(bitBP + i*font.w + j) = *(bitAP + (i-offtop)*bitAw + (j-offleft));
        }

        /*
         * 2.4) copy bitB to bitmapAREA
         */
        for(i=0; i<font.h; i++)
                for(j=0; j<font.w; j++)
                        *(bitmapP + (nowchar*font.w*font.h) + (i*font.w) + j) = *(bitBP + i*font.w + j);

        nowchar++;
        free(bitAP);
        free(bitBP);
}


/*
 * read oneline from textfile
 */
int getline(char* lineP, int max, FILE* inputP){
        if (fgets(lineP, max, inputP) == NULL)
                return 0;
        else
                return strlen(lineP); //fgets returns strings included '\n'
}


/*
 * 1. read BDF-file and transfer to assignBitmap()
 */
unsigned char *readBdfFile(unsigned char *bitmapP, FILE *readP){
        int i;
        int length;
        char sP[LINE_CHARMAX]; //one line(strings) from bdf-font-file
        static int cnt; //only used in debugging: counter of appeared glyphs
        struct boundingbox glyph; //an indivisual glyph width, height,offset x,y
        int flagBitmap = OFF; //this line is bitmap-data?(ON) or no?(OFF)
        char *tokP; //top address of a token from strings
        char *glyphP = NULL; //temporal buffer of bitmap-data(hexadecimal strings)
        char* nextP = NULL; //address of writing next in glyphP
        int dw = 0; //dwidth
        static int bdfflag = OFF; //the given bdf-file is valid or not

        while(1){
                length = getline(sP, LINE_CHARMAX, readP);
                if((bdfflag == OFF) && (length == 0)){
                        //given input-file is not a bdf-file
                        printf("error: input-file is not a bdf file\n");
                        exit(EXIT_FAILURE);
                }
                if(length == 0)
                        break; //escape from while-loop

                //remove carraige-return(CR)
                for(i=0; i<length; i++){
                        if(sP[i] == '\r'){
                                sP[i] = '\n';
                                length--;
                        }
                }

                //classify from the top character of sP
                switch(sP[0]){
                case 'S':
                        if(bdfflag == OFF){
                                //top of the bdf-file
                                if(strncmp(sP, "STARTFONT ", 10) == 0){
                                        bdfflag = ON;
                                        d_printf("startfont exists %d\n", bdfflag);
                                }else{
                                        //given input-file is not a bdf-file
                                        printf("error: input-file is not a bdf file\n");
                                        exit(EXIT_FAILURE);
                                }
                        }
                        break;
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
                                v_printf("  Total glyphs = %d\n",chars);
                                cnt=0;

                                //allocate bitmapAREA
                                bitmapP = (unsigned char*)malloc(chars * font.h * font.w );
                                if(bitmapP == NULL){
                                        printf("error malloc\n");
                                        exit(EXIT_FAILURE);
                                }
                        }else
                                STOREBITMAP();
                        break;
                case 'D':
                        if(strncmp(sP, "DWIDTH ", 7) == 0){
                                //get dw
                                tokP = strtok(sP, " ");
                                tokP += (strlen(tokP)+1);
                                tokP = strtok(tokP, " ");
                                dw = atoi(tokP);
                        }else
                                STOREBITMAP();
                        break;
                case 'B':
                        if(strncmp(sP, "BITMAP", 6) == 0){
                                //allocate glyphP
                                glyphP = (char*)malloc(font.w*font.h); //allocate more room
                                if(glyphP == NULL){
                                        printf("error malloc bdf\n");
                                        exit(EXIT_FAILURE);
                                }
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
                                assignBitmap(bitmapP, glyphP, nextP - glyphP, glyph, dw);
                                flagBitmap = OFF;
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


/*
 * 
 */
void printhelp(void){
        printf("bdf2bmp version 0.5.2\n");
        printf("Usage: bdf2bmp [-option] input-bdf-file output-bmp-file\n");
        printf("Option:\n");
        printf("  -sN    spacing N pixels (default N=2)\n");
        printf("             N is limited from 0 to 32\n");
        printf("             DO NOT INSERT SPACE BETWEEN 's' AND 'N'\n");
        printf("  -cN    specifying N colomns in grid (default N=32)\n");
        printf("             N is limited from 1 to 4096\n");
        printf("             DO NOT INSERT SPACE BETWEEN 'c' AND 'N'\n");
        printf("  -w     showing glyph widths with a gray color\n");
        printf("  -h     print help\n");
        exit(EXIT_FAILURE);
}



/*
 *
 */
int main(int argc, char *argv[]){
        FILE *readP;
        FILE *writeP;
        char readFilename[FILENAME_CHARMAX] = "input.bdf";
        char writeFilename[FILENAME_CHARMAX] = "output.bmp";
        int tmp, i;
        unsigned char *bitmapP = NULL; //address of bitmapAREA
        int spacing = 2; //breadth of spacing (default 2)
        int flag;
        int colchar = 32; //number of columns(horizontal) (default 32)

        //deal with arguments
        if(argc < 2){
                //printf("error: not enough arguments\n");
                printhelp();
        }
        for(i=1, flag=0; i<argc; i++){
                switch( argv[i][0] ){
                case '-':
                        if(argv[i][1] == 's')
                                spacing = atoi(&argv[i][2]);
                        else if(argv[i][1] == 'c')
                                colchar = atoi(&argv[i][2]);
                        else if(argv[i][1] == 'w')
                                dwflag = ON;
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
                printf("error: not enough arguments\n");
                printf("Usage: bdf2bmp [-option] input-bdf-file output-bmp-file\n");
                exit(EXIT_FAILURE);
        }

        //colchar is limited from 1 to 4096
        if(colchar  < 1)
                colchar = 1;
        else if(colchar > 4096)
                colchar = 4096;

        //spacing is limited from 0 to 32
        if(spacing < 0)
                spacing = 0;
        else if(spacing > 32)
                spacing = 32;

        //prepare to read&write files
        readP = fopen(readFilename, "r");
        if(readP == NULL){
                printf("error: %s does not exist\n", readFilename);
                exit(EXIT_FAILURE);
        }
        writeP=fopen(writeFilename, "wb");
        if(writeP == NULL){
                printf("error: cannot write %s\n", writeFilename);
                exit(EXIT_FAILURE);
        }

        //read bdf-font-file
        bitmapP = readBdfFile(bitmapP, readP);
        fclose(readP);

        //write bmp-image-file
        writeBmpFile(bitmapP, spacing, colchar, writeP);
        tmp = fclose(writeP);
        if(tmp == EOF){
                printf("error: cannot write %s\n", writeFilename);
                free(bitmapP);
                exit(EXIT_FAILURE);
        }

        free(bitmapP);
        return EXIT_SUCCESS;
}
