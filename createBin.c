//tested..OK

#include <stdio.h>  //printf(), fopen(), fwrite()
#include <stdlib.h> //malloc(), EXIT_SUCCESS, strtol()
#include <string.h> //strcmp(), strcpy()
#include <limits.h> //strtol()

#define FONTBBX_W_MAX 128 //根拠なし 横幅128pixelまでのglyphが扱える
#define LINE_CHARMAX 1000 //根拠なし BDFファイルの1行の最大文字数
#define FILENAME_CHARMAX 256 //根拠なし ファイル名の最大文字数
#define ON 1 //根拠なし
#define OFF 0 //根拠なし ONとちがってればいい
#ifdef DEBUG
#define d_printf(message,arg) printf(message,arg)
#else /* not DEBUG */
#define d_printf(message,arg)
#endif /* DEBUG */

//関数prototype

//global変数
static int chars; //総glyph数
static int fbbxH; //フォント全体のBOUNDINGBOXの 高さ(Height)
static int fbbxW; //フォント全体のBOUNDINGBOXの 幅(Width)

//bitmapを格納する
void bitmapAssign(unsigned char *bitmapP, unsigned char *lineP, int nowChar, int nowH){
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
		d_printf("%s\n",binP);
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
				d_printf("fbbxW=%d\n",fbbxW);
				d_printf("fbbxH=%d\n",fbbxH);
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
				d_printf("chars=%d\n",chars);

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
				d_printf("flagBitmap=%d\n",flagBitmap);
			}
			break;
		case 'E':
			if(strncmp(sP, "ENDCHAR", 7) == 0){
				flagBitmap = OFF;
				nowChar ++;
				d_printf("flagBitmap=%d\n",flagBitmap);
			}
			break;
		default:
			if(flagBitmap == ON){
				bitmapAssign(bitmapP, sP,nowChar,nowH);
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

int main(void){
	FILE *readP;
	FILE *writeP;
	char readFilename[FILENAME_CHARMAX] = "courr18.bdf";
	char writeFilename[FILENAME_CHARMAX] = "bdf13.bin";
	int tmp;
	unsigned char *bitmapP = NULL; //各glyphの BITMAPデータを格納する


	//ファイルの読み書きの準備
	readP = fopen(readFilename, "r");
	if(readP == NULL){
		printf("error fopen r");
		exit(-1);
	}
	writeP=fopen(writeFilename, "wb");
	if(writeP == NULL){
		printf("error fopen w");
		exit(-1);
	}

	//BDFファイルを読みこむ
	bitmapP = readBdfFile(bitmapP, readP);
	fclose(readP);

	//binファイルを書き込む
	tmp = fwrite(bitmapP, 1, (chars*fbbxW*fbbxH), writeP);
	if(tmp != chars*fbbxW*fbbxH){
		printf("error fwrite %d %d\n", chars*fbbxW*fbbxH, tmp);
		free(bitmapP);
		exit(-1);
	}
	tmp = fclose(writeP);
	if(tmp == EOF){
		printf("error 3");
		free(bitmapP);
		exit(-1);
	}
	free(bitmapP);

	return EXIT_SUCCESS;
}
