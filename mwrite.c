#define DEBUG
#define VERBOSE
#define LSB //LittleEndian; if your computer is MSB(BigEndian), erase this.

#include <stdio.h>  //printf(), fopen(), fwrite()
#include <stdlib.h> //malloc(), EXIT_SUCCESS, strtol()
#include <string.h> //strcmp(), strcpy()
#include <limits.h> //strtol()

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

main(){
	FILE *writeP;
	int tmp;
	unsigned int i = 0x12345678;
	unsigned int j = 0x90123456;
	unsigned char sP[] = "1234567890";
	unsigned char* tmpP;

	writeP=fopen("bdf21.bin", "wb");
	if(writeP == NULL){
		printf("error fopen w");
		exit(-1);
	}

	tmpP = mwrite(&i, 4, sP);
	printf("tmpP=%p\n", tmpP);
	tmpP = mwrite(&j, 4, tmpP);
	printf("tmpP=%p\n", tmpP);

	tmp = fwrite(sP, 1, sizeof(sP), writeP);
	if(tmp != sizeof(sP)){
		printf("error fwrite %d %d\n", sizeof(sP), tmp);
		exit(-1);
	}
	tmp = fclose(writeP);
	if(tmp == EOF){
		printf("error 3");
		exit(-1);
	}
	return EXIT_SUCCESS;
}
