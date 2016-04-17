#include <stdio.h>
#include <stdlib.h>

int main() {
	int *ptr;
	ptr = (int *) malloc(4);
	int arr[4] = {1, 2, 3, 4};
	for(int i = 0; i < 4; i++) {
		printf("%d\n", *ptr);
		*ptr++ = arr[i];
		printf("%d\n", *(ptr - 1));
	}

	return 0;
}