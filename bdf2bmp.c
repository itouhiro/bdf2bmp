// bdf2bmp  --  generate bmp-image-file from bdf-font-file
// version: 0.2
// date: Thu Dec 21 12:06:57 2000
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

#define storebitmap()   if(flagBitmap == ON){\
                            memcpy(nextP, sP, length);\
                            nextP += length;\
                            if(nextP-glyphP > gboxw*gboxh){\
				    printf("error nextP");\
				    exit(-1);\
			    }\
                        }
                               //改行コードもコピーする
                               //改行コードを含めた文字列 sPの長さ

//global変数
static int chars; //総glyph数
static int gboxw; //フォント全体の グリフの 幅(Width) (単位 pixel)
static int gboxh; //フォント全体の (グローバル)なグリフの 高さ(Height) (単位 pixel)
static unsigned long bmpTotalSize; //BMPファイルのサイズ(単位 byte)
static int goffx; //フォント全体のオフセット(グローバルオフセット) X (単位 pixel)
static int goffy; //フォント全体のオフセット(グローバルオフセット) Y (単位 pixel)

//関数prototype
unsigned char*assignBmp(unsigned char* bitmapP);
unsigned char *mwrite(const void *_p, int n, unsigned char *dstP);
//void assignBitmap(const unsigned char *bitmapP, const unsigned char *glyphP, const int sizeglyphP, const int nowboxw, const int nowboxh, const int nowoffx, const int nowoffy);
unsigned char *readBdfFile(unsigned char *bitmapP, FILE *readP);
int getline(char* lineP, int max, FILE* inputP);
int main(int argc, char *argv[]);


// メモリ上のbitmapを、BMP画像ファイルに変換しつつ書き込む
//  BMP画像ファイル: 無圧縮, 8bitColor, Windows Win32形式
//  横に32文字置く
// 返り値: BMP画像ファイルとなるデータがあるアドレス
unsigned char *assignBmp(unsigned char* bitmapP){
	long bmpW; //BMP画像ファイルの横幅(単位 pixel)
	long bmpH; //BMP画像ファイルの縦の高さ(単位 pixel)
	unsigned char *dstP; //BMPファイルをなすメモリの現在書き込み場所を指す
	unsigned char *bmpP = NULL; //BMPファイルをなすメモリの先頭書き込み場所を指す
	int wPadding; //BMP画像ファイルで各行ごとにlong境界にあわせるためのpadding
	unsigned long u_long;
	unsigned short u_short;
	signed long s_long;
	unsigned char u_char;
	unsigned char *tmpP = NULL;
	int i,x,y,g,tmp;

	bmpW = gboxw * 32; //横幅は、32文字ぶん
	bmpH = gboxh * ( (chars/32) + (chars%32!=0) ); //縦の高さは、総文字数を32で割ったものに、あまりが0でないときさらに1文字ぶん高さを足す; 例) 34文字あるとき、32文字で 1line、残りの2文字で 1line、計 2line
	v_printf("BMP width=%dpixels ", bmpW);
	v_printf("height=%dpixels\n", bmpH);
	d_printf("number of glyphs horizontal=%d ", 32);
	d_printf("vertical=%d\n", (chars/32)+(chars%32!=0) );

	//メモリの確保
	wPadding = ((bmpW + 3) / 4 * 4) - bmpW;
	bmpTotalSize = (bmpW + wPadding) * bmpH
		+ sizeof(long)*11 + sizeof(short)*5 + sizeof(char)*4*256;
	tmpP = (unsigned char *)malloc(bmpTotalSize);
	if(tmpP == NULL){
		printf("error malloc bmp");
		exit(-1);
	}else
		bmpP = tmpP;
	memset(bmpP, 0, bmpTotalSize); //確保したメモリを 0クリア
	v_printf("BMP filesize=%dbytes\n", bmpTotalSize);

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
	dstP = mwrite(&bmpW, 4, dstP); //biWidth
	s_long = bmpH;
	dstP = mwrite(&s_long, 4, dstP); //biHeight: 負の値にすると top-down形式;正の値だとdown-top(上下が表示されるのと逆)
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

	//  パレット[2]; とりあえず色は #a0a0c4 にしてあります
	u_char = 0xc4;
	dstP = mwrite(&u_char, 1, dstP); //B
	u_char = 0xa0;
	dstP = mwrite(&u_char, 1, dstP); //G
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
	for(y=bmpH-1; y>=0; y--){
		for(x=0; x<bmpW+wPadding; x++){
			if(x>=bmpW){
				//パディング: 1ラインごとのlong境界にあわせる
				u_char = 0;
				dstP = mwrite(&u_char, 1, dstP); //0で埋める
			}else{
				//bitmapデータを読んで、bmp領域に書く
				g = x/gboxw + (y/gboxh)*32; //g..glyphNumber
				tmp = (gboxh*gboxw*g) + (gboxw*(y%gboxh)) + (x%gboxw);
				if(tmp > chars*gboxh*gboxw){
					//bmp領域最終行の文字の無い場所のbitmapデータが求められている
					u_char = 2; //パレット[2]で埋める
				}else
					u_char = *( bitmapP + tmp);
				dstP = mwrite(&u_char, 1, dstP);
			}
		}
		//d_printf("tmp=%d\n",tmp);
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
// bitmapP に格納されたデータの構造: 1文字ごとに、右上端から横に書いて一段下の右端に移ってまた書いて、をくりかえす。1文字書きおわったその次のバイトから、次の文字が書かれる。
void assignBitmap(unsigned char *bitmapP, unsigned char *glyphP, int sizeglyphP, int nowboxw, int nowboxh, int nowoffx, int nowoffy){
	static char* hex2binP[]= {
		"0000","0001","0010","0011","0100","0101","0110","0111",
		"1000","1001","1010","1011","1100","1101","1110","1111"
	};
	int d; //decimal(10進数)
	int hexlen;
	unsigned char binP[LINE_CHARMAX]; //2進数の文字列を一時的に格納する
	static int nowchar = 0; //現在書きこもうとしているグリフが何個めか
	unsigned char* tmpP = NULL;
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

	//ちゃんと書き込めてるかチェック
	for(i=0; i<bitAh; i++){
		for(j=0; j<bitAw; j++){
			d_printf("%d", *(bitAP + i*hexlen*4 + j));
		}
		d_printf("\n",i); //iは意味なし(argが1つ必要なので置いてるだけ)
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
	d_printf("topoff=%d ",topoff);
	d_printf("leftoff=%d\n",leftoff);
	//  bitA領域をbitBにコピー(範囲外の部分はすでに「パレット[0] で埋め」てある
	for(i=0; i<nowboxh; i++)
		for(j=0; j<nowboxw; j++)
			*(bitBP + (i+topoff)*gboxw + (j+leftoff))
				= *(bitAP + i*bitAw + j);

	//ちゃんと書き込めてるかチェック
	for(i=0; i<gboxh; i++){
		for(j=0; j<gboxw; j++){
			d_printf("%d", *(bitBP + i*gboxw + j));
		}
		d_printf("\n",i); //iは意味なし(argが1つ必要なので置いてるだけ)
	}

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
				v_printf("global glyph width=%dpixels ",gboxw);
				v_printf("height=%dpixels\n",gboxh);
				v_printf("global glyph offset x=%dpixels ",goffx);
				v_printf("y=%dpixels\n",goffy);
				//d_printf("gboxw*gboxh=%d\n",gboxw*gboxh);
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
				v_printf("total chars=%d\n",chars);
				cnt=0;

				//メモリをallocate(割り当て)する
				bitmapP = (unsigned char*)malloc(chars * gboxh * gboxw );
				if(bitmapP == NULL){
					printf("error malloc");
					exit(-1);
				}
				d_printf("malloc %dbytes\n",chars*gboxh*gboxw);
				//d_printf("malloc bitmapP=%p\n",bitmapP);
			}else
				storebitmap();
			break;
		case 'B':
			if(strncmp(sP, "BITMAP", 6) == 0){
				//ビットマップデータ(16進数の文字列)を一時保存する領域、を確保する
				glyphP = (unsigned char*)malloc(gboxw*gboxh); //ほんとはこんなにいらないけど、多めに確保
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
				d_printf("glyph width=%dpixels ",nowboxw);
				d_printf("height=%dpixels\n",nowboxh);
				d_printf("glyph offset x=%dpixels ",nowoffx);
				d_printf("y=%dpixels\n",nowoffy);
			}else
				storebitmap();
			break;
		case 'E':
			if(strncmp(sP, "ENDCHAR", 7) == 0){
				assignBitmap(bitmapP, glyphP, nextP - glyphP, nowboxw, nowboxh, nowoffx, nowoffy);
				flagBitmap = OFF;
				//d_printf("nextP=%p\n",nextP);
				//d_printf("nextP-glyphP=%d\n",nextP-glyphP);
				//for(j=0;j<gboxw*gboxh; j++){
				//	if(j%16==0)printf("\n");
				//	printf("%02d ", *(glyphP+j));
				//}
				d_printf("%s\n",glyphP);
				d_printf("glyphP=%p\n",glyphP);
				//d_printf("free: glyphP=%p\n",glyphP);
				free(glyphP);
				d_printf("glyph %d\n", cnt);
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
	char readFilename[FILENAME_CHARMAX] = "courR18.bdf";
	char writeFilename[FILENAME_CHARMAX] = "courR18.bmp";
	int tmp;
	unsigned char *bmpP = NULL;
	unsigned char *bitmapP = NULL; //各glyphの BITMAPデータ(BMP画像にする前の一時的データ)を格納する

	if(argc != 3){
		printf("error argc\n");
		printf("ex)  bdf2bmp input.bdf output.bmp\n\n");
		exit(-1);
	}

	//ファイルの読み書きの準備
	strcpy(readFilename, argv[1]);
	readP = fopen(readFilename, "r");
	if(readP == NULL){
		printf("error fopen r");
		exit(-1);
	}
	strcpy(writeFilename, argv[2]);
	writeP=fopen(writeFilename, "wb");
	if(writeP == NULL){
		printf("error fopen w");
		exit(-1);
	}

	//BDFファイルを読みこむ
	bitmapP = readBdfFile(bitmapP, readP);
	fclose(readP);

	//BMPファイルを書き込む
	bmpP = assignBmp(bitmapP);
	tmp = fwrite(bmpP, 1, bmpTotalSize, writeP);
	if((unsigned long)tmp != bmpTotalSize){
		printf("error fwrite %d %d\n", bmpTotalSize, tmp);
		free(bitmapP);
		free(bmpP);
		exit(-1);
	}
	tmp = fclose(writeP);
	if(tmp == EOF){
		printf("error 3");
		free(bitmapP);
		free(bmpP);
		exit(-1);
	}
	free(bitmapP);
	free(bmpP);

	return EXIT_SUCCESS;
}
//end of file
