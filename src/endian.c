#include <stdio.h>

static int am_big_endian(void) {
	long one = 1;
	return !(*((char *)(&one)));
}

int main(int argc, char **argv) {
	printf("#define __LITTLE_ENDIAN 1234\n");
	printf("#define __BIG_ENDIAN    4321\n");
	printf("#define __BYTE_ORDER __%s_ENDIAN\n", am_big_endian() ? "BIG" : "LITTLE");
	return 0;
}

