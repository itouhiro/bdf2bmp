// bdf2bmp  --  generate bmp-image-file from bdf-font-file
// version: 0.3
// Thu Dec 21 18:20:32 2000
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

#define VERBOSE
#define LSB //LittleEndian; if your computer is MSB(BigEndian), erase this line.

#include <stdio.h>  //printf(), fopen(), fwrite()
#include <stdlib.h> //malloc(), EXIT_SUCCESS, strtol()
#include <string.h> //strcmp(), strcpy()
#include <limits.h> //strtol()

#define LINE_CHARMAX 1000 //根拠なし BDFファイルの1行の最大文字数
#define FILENAME_CHARMAX 256 //根拠なし ファイル名の最大文字数
#define ON 1 //根拠なし
#define OFF 0 //根拠なし ONとちがってればいい
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

//改行コードもコピーしている
#define storebitmap()   if(flagBitmap == ON){\
                            memcpy(nextP, sP, length);\
                            nextP += length;\
                        }

//global変数
static int chars; //総グリフ数
static int gboxw; //フォント全体の(グローバルな)グリフの 幅(Width) (単位 pixel)
static int gboxh; //フォント全体のグリフの高さ(Height)
static unsigned long bmpTotalSize; //BMPファイルのサイズ(単位 byte)
static int goffx; //フォント全体の(グローバルな)オフセット(グローバルな) X (単位 pixel)
static int goffy; //フォント全体のオフセット Y

//関数prototype
unsigned char *assignBmp(unsigned char* bitmapP, int spacing,
  unsigned char p2r, unsigned char p2g, unsigned char p2b);
unsigned char *mwrite(const void *_p, int n, unsigned char *dstP);
void assignBitmap(unsigned char *bitmapP, unsigned char *glyphP, int sizeglyphP,
  int nowboxw, int nowboxh, int nowoffx, int nowoffy);
unsigned char *readBdfFile(unsigned char *bitmapP, FILE *readP);
int getline(char* lineP, int max, FILE* inputP);
int main(int argc, char *argv[]);
void printhelp(void);

// メモリ上のbitmapを、BMP画像ファイルに変換しつつ書き込む
//   BMP画像ファイル: 無圧縮, 8bitColor, Windows Win32形式
// 返り値: BMP画像ファイルとなるデータがあるアドレス
unsigned char *assignBmp(unsigned char* bitmapP, int spacing,
  unsigned char p2r, unsigned char p2g, unsigned char p2b){
	long bmpw; //BMP画像ファイルの横幅(単位 pixel)
	long bmph; //BMP画像ファイルの縦の高さ(単位 pixel)
	unsigned char *dstP; //BMPファイルをなすメモリの現在書き込み場所を指す
	unsigned char *bmpP; //BMPファイルをなすメモリの先頭書き込み場所を指す
	int wPadding; //BMP画像ファイルで各行ごとにlong境界にあわせるためのpadding
	unsigned long u_long;
	unsigned short u_short;
	signed long s_long;
	unsigned char u_char;
	int i,x,y,g,tmp;
	int colchar, rowchar; //横方向の文字数、縦方向の文字数
	int bx, by;

	//横幅は、32文字ぶん
	colchar = 32;
	bmpw = (gboxw+spacing)*colchar + spacing;

	//縦の高さは、総文字数を32で割ったものに、あまりが0でないとき
	//さらに1文字ぶん高さを足す
	//例) 34文字あるとき、32文字で 1line、残りの2文字で 1line、計 2line
	rowchar = (chars/colchar) + (chars%colchar!=0);
	bmph = (gboxh+spacing)*rowchar + spacing;

	v_printf("  BMP width=%dpixels\n", (int)bmpw);
	v_printf("  BMP height=%dpixels\n", (int)bmph);
	d_printf("number of glyphs column=%d ", colchar);
	d_printf("row=%d\n", rowchar);

	//メモリの確保
	wPadding = ((bmpw + 3) / 4 * 4) - bmpw;
	bmpTotalSize = (bmpw + wPadding) * bmph
		+ sizeof(long)*11 + sizeof(short)*5 + sizeof(char)*4*256;
	bmpP = (unsigned char *)malloc(bmpTotalSize);
	if(bmpP == NULL){
		printf("error malloc bmp");
		exit(-1);
	}
	memset(bmpP, 0, bmpTotalSize); //確保したメモリを 0クリア
	v_printf("  BMP filesize=%dbytes\n", (int)bmpTotalSize);

	//BITMAPFILEHEADER構造体
	u_short = 0x4d42;
	dstP = mwrite(&u_short, 2, bmpP); //4d = 'M', 42 = 'B'
	dstP = mwrite(&bmpTotalSize, 4, dstP);
	u_short = 0x00;
	dstP = mwrite(&u_short, 2, dstP); //0を書き込む(何かへんだけど"\0\0")
	dstP = mwrite(&u_short, 2, dstP);
	u_long = sizeof(long)*11 + sizeof(short)*5 + sizeof(char)*4*256;
	dstP = mwrite(&u_long, 4, dstP); //bfOffBits: イメージデータ配列までのoffset

	//BITMAPINFOHEADER構造体
	u_long = 40;
	dstP = mwrite(&u_long, 4, dstP); //Windows形式BMPは 40
	dstP = mwrite(&bmpw, 4, dstP); //biWidth
	s_long = bmph;
	dstP = mwrite(&s_long, 4, dstP); //biHeight: 正の値だとdown-top(上下が逆)
	u_short = 1;
	dstP = mwrite(&u_short, 2, dstP); //biPlanes: 必ず 1
	u_short = 8;
	dstP = mwrite(&u_short, 2, dstP); //biBitCount: 8bitColor
	u_long = 0;
	dstP = mwrite(&u_long, 4, dstP); //biCompression: 無圧縮(BI_RGB)
	dstP = mwrite(&u_long, 4, dstP); //biSizeImage: 無圧縮なら 0でよし
	s_long = 0;
	dstP = mwrite(&s_long, 4, dstP); //biXPelsPerMeter: 解像度 0でよし
	dstP = mwrite(&s_long, 4, dstP); //biYPelsPerMeter: 解像度 0でよし
	u_long = 0;
	dstP = mwrite(&u_long, 4, dstP); //biClrUsed: 最適化カラーパレット無し
	dstP = mwrite(&u_long, 4, dstP); //biClrImportant: 0でよし

	//RGBQUAD[256]: カラーパレット
	//  パレット[0]
	u_char = 0xff;
	dstP = mwrite(&u_char, 1, dstP); //rgbBlue: B
	dstP = mwrite(&u_char, 1, dstP); //rgbGreen: G
	dstP = mwrite(&u_char, 1, dstP); //rgbRed: R;  パレット[0]は #ffffff
	u_char = 0;
	dstP = mwrite(&u_char, 1, dstP); //rgbReserved: 必ず 0

	//  パレット[1]
	u_char = 0;
	for(i=0; i<4; i++)
		dstP = mwrite(&u_char, 1, dstP); //パレット[1]は #000000

	//  パレット[2]; 変更可能
	u_char = p2b;
	dstP = mwrite(&u_char, 1, dstP); //B
	u_char = p2g;
	dstP = mwrite(&u_char, 1, dstP); //G
	u_char = p2r;
	dstP = mwrite(&u_char, 1, dstP); //R
	u_char = 0;
	dstP = mwrite(&u_char, 1, dstP); //必ず 0
		
	//  パレット[3] to パレット[255]
	for(i=3; i<256; i++){
		u_char = 0x00;
		dstP = mwrite(&u_char, 1, dstP);
		dstP = mwrite(&u_char, 1, dstP);
		dstP = mwrite(&u_char, 1, dstP);
		dstP = mwrite(&u_char, 1, dstP); //パレット[3to255]は #000000
	}

	//イメージデータ
	for(y=bmph-1; y>=0; y--){
		for(x=0; x<bmpw+wPadding; x++){
			if(x>=bmpw){
				//パディング: BMPファイル 1ラインごとのlong境界にあわせる
				u_char = 0;
				dstP = mwrite(&u_char, 1, dstP); //0で埋めることに決まっている
			}else{
				if( (y%(gboxh+spacing)<spacing) || (x%(gboxw+spacing)<spacing) ){
					//仕切り
					u_char = 2; //パレット[2]で埋める
					dstP = mwrite(&u_char, 1, dstP);
				}else{
					//bitmapデータを読みとる
					g = (x/(gboxw+spacing)) + (y/(gboxh+spacing)*colchar);
					d_printf("g=%d ", g);
					bx = x - (spacing*(g%colchar)) - spacing;
					by = y - (spacing*(g/colchar)) - spacing;
					d_printf("bx=%d ", bx);
					d_printf("by=%d\n", by);
					tmp = g*(gboxh*gboxw) + (by%gboxh)*gboxw + (bx%gboxw);
					if(tmp >= chars*gboxh*gboxw){
					//bmp領域最終行の文字の無い場所のbitmapデータが求められている
						u_char = 2; //パレット[2]で埋める
					}else
						u_char = *( bitmapP + tmp);
					dstP = mwrite(&u_char, 1, dstP);
				}
			}
		}
	}
	return bmpP; //bmpTotalSizeはglobal varなので返さなくていい
}

	
//LSBfirst(LittleEndian)のデータ並びで書き込む
// 引き数: 書き込む数値のあるアドレス、そのデータは何バイトか、書き込み先
// 返り値: 次に書き込む先
unsigned char *mwrite(const void *_p, int n, unsigned char *dstP){
	const unsigned char *p = _p;
	int i;
#ifdef LSB
	//LSB(LittleEndian)なら、そのまま書く(BMP画像ファイルはlittleEndianと決まっているから)
	for(i=0; i<n; i++){
		*dstP = *(p+i);
		dstP++;
	}
#else /* not LSB */
	//MSB(BigEndian)なら、データを逆にする
	for(i=n-1; i>=0; i--){
		*dstP = *(p+i);
		dstP++;
	}
#endif /* LSB */
	return dstP;
}



//bitmapをメモリに格納する
// bitmapP に格納されたデータの構造: 1文字ごとに、右上端から横に書いて
//一段下の右端に移ってまた書いて、の繰り返し
//1文字書きおわったその次のバイトから、次の文字が書かれる
void assignBitmap(unsigned char *bitmapP, unsigned char *glyphP, int sizeglyphP,
  int nowboxw, int nowboxh, int nowoffx, int nowoffy){
	static char* hex2binP[]= {
		"0000","0001","0010","0011","0100","0101","0110","0111",
		"1000","1001","1010","1011","1100","1101","1110","1111"
	};
	int d; //decimal(10進数)
	int hexlen;
	unsigned char binP[LINE_CHARMAX]; //2進数の文字列を一時的に格納する
	static int nowchar = 0; //現在書きこもうとしているグリフが何個めか
	unsigned char* tmpP;
	char tmpsP[LINE_CHARMAX];
	int bitAh, bitAw; //bitAの縦高さ,横幅
	int topoff, leftoff; //グリフ上、左のオフセット値
	unsigned char* bitAP;
	unsigned char* bitBP;
	int i,j,x,y;
	
	//1) まず、うけとった16進数の文字列からbitmap(呼称 bitA)を作る
	tmpP = strstr(glyphP, "\n");
	hexlen = tmpP - glyphP;
	bitAw = hexlen*4; //bitAの横幅: 改行を含まない1行の長さ*4
	bitAh = sizeglyphP/(hexlen+1); //bitAの縦の高さ
	bitAP = malloc(bitAw * bitAh); //bitAの領域
	if(bitAP == NULL){
		printf("error bitA bitmap");
		exit(-1);
	}

	//  16進数の文字列を bitmapに変換する
	for(i=0,x=0,y=0; i<sizeglyphP; i++){
		if(glyphP[i] == '\n'){
			x=0; y++;
		}else{
			sprintf(tmpsP, "0x%c", glyphP[i]); // 1文字ぬきだした
			d = (int)strtol(tmpsP,(char **)NULL, 16);
			strcpy(binP, hex2binP[d]); //2進数にした(binPに代入)
			for(j=0; j<4; j++,x++)
				*(bitAP + y*bitAw + x) = binP[j] - '0';
		}
	}

	//2)別に、グローバルboxW*グローバルboxHの大きさがある領域(呼称 bitB)も作る
	bitBP = malloc(gboxw * gboxh); //bitBの領域
	if(bitBP == NULL){
		printf("error bitB bitmap");
		exit(-1);
	}
	memset(bitBP, 0, gboxw*gboxh); //パレット[0] で埋める

	//3) bitA をオフセットを考慮しながら、bitBにコピーする
	topoff = gboxh - nowboxh - ((-1)*goffy+nowoffy);
	leftoff = (-1)*goffx + nowoffx;
	//d_printf("topoff=%d ",topoff);
	//d_printf("leftoff=%d\n",leftoff);
	//  bitA領域をbitBにコピー(範囲外の部分はすでに「パレット[0] で埋め」てある
	for(i=0; i<nowboxh; i++)
		for(j=0; j<nowboxw; j++)
			*(bitBP + (i+topoff)*gboxw + (j+leftoff))
				= *(bitAP + i*bitAw + j);

	//4) bitBをbitmap用領域に書き込む
	for(i=0; i<gboxh; i++)
		for(j=0; j<gboxw; j++)
			*(bitmapP + (nowchar*gboxw*gboxh) + (i*gboxw) + j) = *(bitBP + i*gboxw + j);

	nowchar++;
}

unsigned char *readBdfFile(unsigned char *bitmapP, FILE *readP){
	int length;
	char sP[LINE_CHARMAX]; //strings 読みこまれたBDFファイルの1行
	//int i,j;
	static int cnt; //今までにでてきた文字数(デバッグに利用するだけ)
	int nowboxw, nowboxh; //「個々のグリフ」の横幅、縦高さ
	int nowoffx, nowoffy; //「個々のグリフ」オフセット X,Y
	int flagBitmap = OFF;
	char *tokP; //文字列から切り出したtoken、の最初の文字の位置
	char *glyphP = NULL; //グリフのビットマップデータ(16進数の文字列)を一時保存するところ
	char* nextP; //glyphPの中で次に書き込むアドレス(さっき書いた改行文字のひとつ後ろ)

	while(1){
		length = getline(sP, LINE_CHARMAX, readP);
		if(length == 0)
			break; //whileループを抜けて 「//★KOKO★」に移動

		//最初の1文字から分類
		switch(sP[0]){
		case 'F':
			if(strncmp(sP, "FONTBOUNDINGBOX ", 16) == 0){
				// 16にすれば '\0'は比較しない

				//gboxw と gboxh, goffx,goffy を取得する
				tokP = strtok(sP, " ");//tokPは、FONTBOUNDINGBOXの次の空白を指す
				tokP += (strlen(tokP)+1);//tokPは、FONTBOUNDINGBOXのWidthの最初の文字を指す
				tokP = strtok(tokP, " ");//Widthのあとの空白をNULにする
				gboxw = atoi(tokP);
				tokP += (strlen(tokP)+1);//FONTBOUNDINGBOXのheight
				tokP = strtok(tokP, " ");
				gboxh = atoi(tokP);
				tokP += (strlen(tokP)+1);
				tokP = strtok(tokP, " ");
				goffx = atoi(tokP);
				tokP += (strlen(tokP)+1);
				tokP = strtok(tokP, "\n");
				goffy = atoi(tokP);
				d_printf("global glyph width=%dpixels ",gboxw);
				d_printf("height=%dpixels\n",gboxh);
				d_printf("global glyph offset x=%dpixels ",goffx);
				d_printf("y=%dpixels\n",goffy);
			}else
				storebitmap();
			break;
		case 'C':
			if(strncmp(sP, "CHARS ", 6) == 0){
				//charsを取得する
				tokP = strtok(sP, " ");
				tokP += (strlen(tokP)+1);
				tokP = strtok(tokP, "\n");
				chars = atoi(tokP);
				v_printf("  Total chars=%d\n",chars);
				cnt=0;

				//メモリをallocate(割り当て)する
				bitmapP = (unsigned char*)malloc(chars * gboxh * gboxw );
				if(bitmapP == NULL){
					printf("error malloc");
					exit(-1);
				}
				//d_printf("malloc %dbytes\n",chars*gboxh*gboxw);
			}else
				storebitmap();
			break;
		case 'B':
			if(strncmp(sP, "BITMAP", 6) == 0){
				//ビットマップデータ(16進数の文字列)を一時保存する領域、を確保する
				glyphP = (unsigned char*)malloc(gboxw*gboxh); //こんなにいらないけど、多めに確保
				if(glyphP == NULL){
					printf("error malloc bdf");
					exit(-1);
				}
				//d_printf("malloc: glyphP=%p\n",glyphP);
				memset(glyphP, 0, gboxw*gboxh); //0クリア
				nextP = glyphP;
				flagBitmap = ON;
			}else if(strncmp(sP, "BBX ", 4) == 0){
				//nowoffx, nowoffy, nowboxw, nowboxhを取得
				tokP = strtok(sP, " ");//BBXのあとの空白
				tokP += (strlen(tokP)+1);//widthの先頭
				tokP = strtok(tokP, " ");//widthのあとの空白をNULにする
				nowboxw = atoi(tokP);
				tokP += (strlen(tokP)+1);//height
				tokP = strtok(tokP, " ");
				nowboxh = atoi(tokP);
				tokP += (strlen(tokP)+1);//offx
				tokP = strtok(tokP, " ");
				nowoffx = atoi(tokP);
				tokP += (strlen(tokP)+1);//offy
				tokP = strtok(tokP, "\n");
				nowoffy = atoi(tokP);
				//d_printf("glyph width=%dpixels ",nowboxw);
				//d_printf("height=%dpixels\n",nowboxh);
				//d_printf("glyph offset x=%dpixels ",nowoffx);
				//d_printf("y=%dpixels\n",nowoffy);
			}else
				storebitmap();
			break;
		case 'E':
			if(strncmp(sP, "ENDCHAR", 7) == 0){
				d_printf("glyph %d\n", cnt);
				//d_printf("%s\n",glyphP);
				assignBitmap(bitmapP, glyphP, nextP - glyphP, nowboxw, nowboxh, nowoffx, nowoffy);
				flagBitmap = OFF;
				//d_printf("nextP-glyphP=%d\n",nextP-glyphP);
				//d_printf("glyphP=%p\n",glyphP);
				free(glyphP);
				cnt++;
			}else
				storebitmap();
			break;
		default:
			storebitmap();
			break;
		}//switch
	}//while
	//★koko★
	return bitmapP;
}

//テキストファイルを 1行ごと読みこむ
int getline(char* lineP, int max, FILE* inputP){
	if (fgets(lineP, max, inputP) == NULL)
		return 0;
	else
		return strlen(lineP); //fgetsは'\n'を含めた文字列を返す
}

int main(int argc, char *argv[]){
	FILE *readP;
	FILE *writeP;
	char readFilename[FILENAME_CHARMAX] = "input.bdf";
	char writeFilename[FILENAME_CHARMAX] = "output.bmp";
	int tmp, i;
	unsigned char *bmpP; //BMP画像ファイルを成すデータのあるところ
	unsigned char *bitmapP = NULL; //各glyphの BITMAPデータ(BMP画像にする前の一時的データ)を格納する
	int spacing = 2; //文字仕切りの幅 (default 2)
	int flag;
	unsigned char p2r, p2g, p2b; //パレット[2]の色: Red, Green, Blue

	//引き数の処理
	if(argc < 3){
		//引き数が足りない場合
		//printf("error: not enough argument!\n");
		printhelp();
	}else if(argc > 4){
		printf("error: too many argument!\n");
		printhelp();
	}
	p2r = 0xa0; p2g = 0xa0; p2b = 0xc4; //パレット[2]のデフォルト色 #a0a0c4
	for(i=1, flag=0; i<argc; i++){
		switch( argv[i][0] ){
		case '-':
			if(argv[i][1]=='s')
				spacing = atoi(&argv[i][2]);
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
					exit(-1);
				}
			}
			break;
		}
	}
	//仕切り は、0to32ピクセルまで
	if( (spacing<0) || (spacing>32) )
		spacing = 2;

	//ファイルの読み書きの準備
	readP = fopen(readFilename, "r");
	if(readP == NULL){
		printf("error: %s does not exist!\n", readFilename);
		exit(-1);
	}
	writeP=fopen(writeFilename, "wb");
	if(writeP == NULL){
		printf("error: cannot write %s !\n", writeFilename);
		exit(-1);
	}

	//BDFファイルを読みこむ
	bitmapP = readBdfFile(bitmapP, readP);
	fclose(readP);

	//BMPファイルを書き込む
	bmpP = assignBmp(bitmapP, spacing, p2r, p2g, p2b);
	tmp = fwrite(bmpP, 1, bmpTotalSize, writeP);
	if((unsigned long)tmp != bmpTotalSize){
		printf("error: cannot write %s !\n", writeFilename);
		free(bitmapP);
		free(bmpP);
		exit(-1);
	}
	tmp = fclose(writeP);
	if(tmp == EOF){
		printf("error: cannot write %s !\n", writeFilename);
		free(bitmapP);
		free(bmpP);
		exit(-1);
	}
	free(bitmapP);
	free(bmpP);
	return EXIT_SUCCESS;
}

void printhelp(void){
	printf("bdf2bmp version 0.3\n");
	printf("Usage: bdf2bmp [-option] input-bdf-file output-bmp-file\n");
	printf("Option:\n");
	printf("  -sN    spacing N pixels; N is limited to 0 to 32 (default N=2)\n");
	printf("             DO NOT INSERT SPACE BETWEEN 's' AND 'N'\n");
	printf("  -h     print help\n");
	//printf("  -v             print version\n");
	exit(-1);
}
