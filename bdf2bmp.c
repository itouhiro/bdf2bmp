// bdf2bmp   --  convert bdf-font-file to bmp-image-file
// version: 0.1
// date: Sun Dec 17 19:59:20 2000
// author: ITOU Hiroki (itouh@lycos.ne.jp)

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

//関数prototype
unsigned char*assignBmp(unsigned char* bitmapP);
unsigned char *mwrite(const void *_p, int n, unsigned char *dstP);
void assignBitmap(unsigned char *bitmapP, unsigned char *lineP, int nowChar, int nowH);
unsigned char *readBdfFile(unsigned char *bitmapP, FILE *readP);
int getline(char* lineP, int max, FILE* inputP);
int main(int argc, char *argv[]);

//global変数
static int chars; //総glyph数
static int fbbxH; //フォント全体のBOUNDINGBOXの 高さ(Height) (単位 pixel)
static int fbbxW; //フォント全体のBOUNDINGBOXの 幅(Width) (単位 pixel)
static unsigned long bmpTotalSize; //BMPファイルのサイズ(単位 byte)

// メモリ上のbitmapを、BMP画像ファイルに変換しつつ書き込む
//  BMP画像ファイル: 無圧縮, 8bitColor, Windows Win32形式
//  横に32文字置く
// 返り値: BMP画像ファイルとなるデータがあるアドレス
unsigned char*assignBmp(unsigned char* bitmapP){
	long bmpW; //BMP画像ファイルの横幅(単位 pixel)
	long bmpH; //BMP画像ファイルの縦の高さ(単位 pixel)
	unsigned char *dstP; //BMPファイルをなすメモリの現在書き込み場所を指す
	unsigned char *bmpP; //BMPファイルをなすメモリの先頭書き込み場所を指す
	int wPadding;
	unsigned long u_long;
	unsigned short u_short;
	signed long s_long;
	unsigned char u_char;
	unsigned char *tmpP;
	int i,x,y,g,tmp;

	bmpW = fbbxW * 32; //横幅は、32文字ぶん
	bmpH = fbbxH * ( (chars/32) + (chars%32!=0) ); //縦の高さは、総文字数を32で割ったものに、あまりが0でないときさらに1文字ぶん高さを足す; 例) 34文字あるとき、32文字で 1line、残りの2文字で 1line、計 2line
	v_printf("BMP width=%dpixels\n", bmpW);
	v_printf("BMP height=%dpixels\n", bmpH);
	d_printf("horizontal glyphs=%d\n", 32);
	d_printf("vertical glyphs=%d\n", (chars/32)+(chars%32!=0) );

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
				g = x/fbbxW + (y/fbbxH)*32; //g..glyphNumber
				tmp = (fbbxH*fbbxW*g) + (fbbxW*(y%fbbxH)) + (x%fbbxW);
				if(tmp > chars*fbbxH*fbbxW){
					//bmp領域最終行の文字の無い場所のbitmapデータを求めている
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

	
//メモリにLSBfirst(LittleEndian)のデータ並びで書き込む
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
void assignBitmap(unsigned char *bitmapP, unsigned char *lineP, int nowChar, int nowH){
	static char* hex2binP[]= {
		"0000","0001","0010","0011","0100","0101","0110","0111",
		"1000","1001","1010","1011","1100","1101","1110","1111"
	};
	char tmpP[LINE_CHARMAX];
	char *sP;
	int i,j;
	int d; //decimal(10進数)
	unsigned char binP[LINE_CHARMAX];

	//linePの文字列(16進)を2進数にする
	// 1. linePから 1文字ぬきだして、それを2進数にする
	// 2. メモリに 1か 0を書き出す
	for(i=0; lineP[i]!='\0'; i++){
		sprintf(tmpP, "0x%c", lineP[i]); // 1文字ぬきだした
		d = (int)strtol(tmpP,(char **)NULL, 16);
		strcpy(binP, hex2binP[d]); //2進数にした(binPに代入)
		//d_printf("%s\n",binP);
		for(j=0; j<4; j++){
			*(bitmapP + (nowChar*fbbxH*fbbxW) + (nowH*fbbxW) + (i*4) + j) = binP[j] - '0';
		}
	}
}

unsigned char *readBdfFile(unsigned char *bitmapP, FILE *readP){
	int length;
	unsigned char sP[LINE_CHARMAX]; //strings 読みこまれたBDFファイルの1行
	int i;
	static int nowChar = 0;
	static int nowH = 0;
	int flagBitmap = OFF;
	unsigned char *tmpP;

	while(1){
		length = getline(sP, LINE_CHARMAX, readP);
		if(length == 0)
			break; //whileループを抜けて 「//★KOKO★」に移動

		//最初の1文字から分類
		switch(sP[0]){
		case 'F':
			if(strncmp(sP, "FONTBOUNDINGBOX ", 16) == 0){
				// 16にすれば '\0'は比較しない

				//fbbxW と fbbxHを取得する
				char *firstP;//取得したい文字列の最初の文字の位置
				char *lastP;//取得する文字列の最後の文字の位置

				firstP = strstr(sP, " ");
				*firstP = '\0'; //空白をNULLにする 実はしなくてもいいけど、とりあえずしておく
				firstP++; //空白の次の文字(取得したい文字列の最初)を指す
				lastP = strstr(firstP, " ");
				*lastP = '\0'; //取得したい文字列の最後の文字をNULLにしておく atoiにわたすため
				fbbxW = atoi(firstP);

				firstP = lastP + 1; //次のtokenの最初の文字を指す
				lastP = strstr(firstP, " ");
				*lastP = '\0'; //次のtokenの最後の文字をNULLにしておく
				fbbxH = atoi(firstP);
				v_printf("glyph width=%dpixels\n",fbbxW);
				v_printf("glyph height=%dpixels\n",fbbxH);
			}
			break;
		case 'C':
			if(strncmp(sP, "CHARS ", 6) == 0){
				char *firstP;//取得したい文字列の最初の文字の位置
				char *lastP;//取得する文字列の最後の文字の位置

				firstP = strstr(sP, " ");
				*firstP = '\0'; //空白をNULLにする
				firstP++; //空白の次の文字(取得したい文字列の最初)を指す
				lastP = strstr(firstP, "\n"); //普通のBDFファイルなら「CHARS [0-9]*」のあとは '\n'が来るはず
				*lastP = '\0'; //取得したい文字列の最後の文字をNULLにしておく atoiにわたすため
				chars = atoi(firstP);
				v_printf("total chars=%d\n",chars);

				//メモリをallocate(割り当て)する
				tmpP = malloc(chars * fbbxH * fbbxW );
				if(tmpP == NULL){
					printf("error malloc");
					exit(-1);
				}else
					bitmapP = tmpP;
				d_printf("malloc %dbytes\n",chars*fbbxH*fbbxW);
			}
			break;
		case 'B':
			if(strncmp(sP, "BITMAP", 6) == 0){
				flagBitmap = ON;
				nowH = 0;
				//d_printf("flagBitmap=%d\n",flagBitmap);
			}
			break;
		case 'E':
			if(strncmp(sP, "ENDCHAR", 7) == 0){
				flagBitmap = OFF;
				nowChar ++;
				//d_printf("flagBitmap=%d\n",flagBitmap);
			}
			break;
		default:
			if(flagBitmap == ON){
				assignBitmap(bitmapP, sP,nowChar,nowH);
				nowH++;
			}
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
	char readFilename[FILENAME_CHARMAX] = "courr18.bdf";
	char writeFilename[FILENAME_CHARMAX] = "bdf14.bin";
	int tmp;
	unsigned char *bmpP;
	unsigned char *bitmapP = NULL; //各glyphの BITMAPデータを格納する
	                               //BMP画像ファイルにする前の一時的な

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

	//bmpファイルを書き込む
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
