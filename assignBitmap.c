//tested .. OK! no error.
#include <stdio.h>
#include <stdlib.h> //malloc(), EXIT_FAILURE, strtol()
#include <string.h> //strcmp(), strcpy()
#include <limits.h> //strtol()

static int fbbxH;
static int fbbxW;

//bitmapを格納する
void
bitmapAssign(unsigned char *bitmapP, unsigned char *lineP, int nowChar, int nowH){
	static char* hex2binP[]= {
		"0000","0001","0010","0011","0100","0101","0110","0111",
		"1000","1001","1010","1011","1100","1101","1110","1111"
	};
	char tmpP[128]; //128は根拠なし 横幅128pixelまでのglyph扱える
	char *sP;
	int i,j;
	int d; //decimal(10進数)
	unsigned char binP[128]; //128は根拠なし 横幅128pixelまでのglyph扱える

	//linePの文字列(16進)を2進数にする
	// 1. linePから 1文字ぬきだして、それを2進数にする
	// 2. メモリに 1か 0を書き出す
	for(i=0; lineP[i]!='\0'; i++){
		sprintf(tmpP, "0x%c", lineP[i]); // 1文字ぬきだした
		d = (int)strtol(tmpP,(char **)NULL, 16);
		strcpy(binP, hex2binP[d]); //2進数にした(binPに代入)
		printf("%s\n",binP);
		for(j=0; j<4; j++){
			*(bitmapP + (nowChar*fbbxH*fbbxW) + (nowH*fbbxW) + (i*4) + j) = binP[j] - '0';
		}
	}
}


void main(void){
	unsigned char lineP[] = "8f";
	int chars = 255;
	FILE *writeP;
	unsigned char *bitmapP; //各glyphの BITMAPデータを格納する
	int tmp;

	fbbxW = fbbxH = 14;
	bitmapP =  malloc(chars * fbbxH * fbbxW );
	writeP=fopen("bdf03.tst", "wb");
	if(writeP == NULL){
		printf("error 1");
		exit(-1);
	}

	bitmapAssign(bitmapP, lineP, 0, 0);

	tmp = fwrite(bitmapP, 1, (chars*fbbxW*fbbxH), writeP);
	if(tmp != chars*fbbxW*fbbxH){
		printf("error 2 %d %d\n", chars*fbbxW*fbbxH, tmp);
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
}
