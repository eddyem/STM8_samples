#include <stdio.h>
#include <stdint.h>

uint32_t U(uint32_t x){
	x *= 3584;
	x >>= 17;
	return x*10;
}

int main(int argc, char **argv){
	uint32_t x = 131072;
	int i;
	if(argc > 1){
		for(i = 1; i < argc; i++){
			x = atoi(argv[i]);
			printf("U(%u) = %u\n", x, U(x));
		}
	}else printf("U(%u) = %u\n", x, U(x));
	return 0;
}