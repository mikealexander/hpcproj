#include <stdlib.h>
#include <stdio.h>

int main(int argc, char **argv)
{
	char line[10];
	int all, text, pattern;

	FILE *f;
	char *fname = "inputs/control.txt";
	f = fopen(fname, "r");
	
	if(!f) {
		puts("Couldn't open file");
		return 0;
	}

	int d = 0;
	while(fscanf(f, "%d %d %d", &all, &text, &pattern) == 3) {
		printf("%d %d %d\n", all, text, pattern);
	}

	return 1;
}
