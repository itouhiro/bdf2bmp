//tested.. OK
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

static int chars; //総glyph数
static int fbbxH; //フォント全体のBOUNDINGBOXの 高さ(Height)
static int fbbxW; //フォント全体のBOUNDINGBOXの 幅(Width)

void readBdfFile(unsigned char *bitmapP, FILE *readP){
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
				//fbbxW = atoi(strtok(sP, " "));
				//fbbxH = atoi(strtok(sP, " "));
				//d_printf("fbbxH=%d\n",fbbxH);

				printf("get:%s\n",sP);
			}
			break;
		case 'C':
			if(strncmp(sP, "CHARS ", 6) == 0){
				printf("get:%s\n",sP);
				//chars = atoi(strtok(sP, " "));
				//d_printf("chars=%d\n",chars);

				//メモリをallocate(割り当て)する
				//tmpP = malloc(chars * fbbxH * fbbxW );
				//if(tmpP == NULL){
				//	printf("error malloc");
				//	exit(-1);
				//}else
				//	bitmapP = tmpP;
				//d_printf("malloc %dbytes\n",chars*fbbxH*fbbxW);
			}
			break;
		case 'B':
			if(strncmp(sP, "BITMAP", 6) == 0){
				printf("get:%s\n",sP);

//				flagBitmap = ON;
//				nowH = 0;
//				d_printf("flagBitmap=%d\n",flagBitmap);
			}
			break;
		case 'E':
			if(strncmp(sP, "ENDCHAR", 7) == 0){
				printf("get:%s\n",sP);
//				flagBitmap = OFF;
//				nowChar ++;
//				d_printf("flagBitmap=%d\n",flagBitmap);
			}
			break;
		default:
			if(flagBitmap == ON){
//				assignBitmap(bitmapP, sP,nowChar,nowH);
				nowH++;
			}
			break;
		}//switch
	}//while
	//★koko★
}

//テキストファイルを 1行ごと読みこむ
int getline(char* lineP, int max, FILE* inputP){
	if (fgets(lineP, max, inputP) == NULL)
		return 0;
	else
		return strlen(lineP); //fgetsは'\n'を含めた文字列を返す
}

main(){
	FILE *readP;
	unsigned char *bitmapP = NULL; //各glyphの BITMAPデータを格納する
	char readFilename[FILENAME_CHARMAX] = "courr18.bdf";

	fbbxW = fbbxH = 14;
	chars = 255;

	//ファイルの読み書きの準備
	readP = fopen(readFilename, "r");
	if(readP == NULL){
		printf("error fopen r");
		exit(-1);
	}

	readBdfFile(bitmapP, readP);

	fclose(readP);
	return EXIT_SUCCESS;
}
