/* tested .. OK */
#include <stdio.h>
#include <string.h>

main(){
	char lineP[] = "FONTBOUNDINGBOX 14 14 0 -2";
	char *fbbxWP;
	char *fbbxHP;
	char *tmpP;
	char strP[100];

	fbbxWP = strstr(lineP, " ");
	if(fbbxWP == NULL){
		printf("error 1");
		exit(-1);
	}
	printf("lineP=%p  fbbxWP=%p  fbbxWP=%s\n",lineP,fbbxWP,fbbxWP); 
	*fbbxWP = '\0';
	fbbxWP++;
	fbbxHP = strstr(fbbxWP, " ");
	if(fbbxHP == NULL){
		printf("error 2");
		exit(-1);
	}
	*fbbxHP = '\0';
 	fbbxHP++;
	tmpP = strchr(fbbxHP, ' ');
	if(tmpP == NULL){
		printf("error 3");
		exit(-1);
	}
	*tmpP = '\0';

	printf("fbbxW=%s\n",fbbxWP);
	printf("fbbxH=%s\n",fbbxHP);

	return 0;
}
/*
lineP=0xbfbfdbe0  fbbxWP=0xbfbfdbef  fbbxWP= 14 14 0 -2
fbbxW=14
fbbxH=14

*/

