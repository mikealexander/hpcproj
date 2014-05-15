
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <time.h>

////////////////////////////////////////////////////////////////////////////////
// Program main
////////////////////////////////////////////////////////////////////////////////

char *textData;
int textLength, textNumber;

char *patternData;
int patternLength, patternNumber;

void outOfMemory()
{
	fprintf (stderr, "Out of memory\n");
	exit (0);
}

int writeOutput(int matchpos)
{
	FILE *f = fopen("result_OMP.txt", "a");
	if(!f) {
		puts("Unable to open output file... very bad.");
		return -1;
	}
	fprintf(f, "%d %d %d\n", textNumber, patternNumber, matchpos);
	fclose(f);
	
	return 0;
}

void readFromFile (FILE *f, char **data, int *length)
{
	int ch;
	int allocatedLength;
	char *result;
	int resultLength = 0;

	allocatedLength = 0;
	result = NULL;

	ch = fgetc (f);
	while (ch >= 0)
	{
		resultLength++;
		if (resultLength > allocatedLength)
		{
			allocatedLength += 10000;
			result = (char *) realloc (result, sizeof(char)*allocatedLength);
			if (result == NULL)
				outOfMemory();
		}
		result[resultLength-1] = ch;
		ch = fgetc(f);
	}
	*data = result;
	*length = resultLength;
}

int readData ()
{
	FILE *f;
	char fileName[1000];
	sprintf (fileName, "inputs/text%d.txt", textNumber);
	f = fopen (fileName, "r");
	if (f == NULL)
		return 0;
	readFromFile (f, &textData, &textLength);
	fclose (f);
	sprintf (fileName, "inputs/pattern%d.txt", patternNumber);
	f = fopen (fileName, "r");
	if (f == NULL)
		return 0;
	readFromFile (f, &patternData, &patternLength);
	fclose (f);

	return 1;
}
		
void hostMatch(int findAll) {
	int pos, i, j, lastI;
	pos = -1;
	lastI = textLength-patternLength;
	
	#pragma omp parallel private(i, j) shared(pos) num_threads(8)
	#pragma omp for schedule(dynamic)
	for(i=0; i <= lastI; i++) 
	{
		if (findAll == 1 || (pos == -1 || i<pos)) { 
			j=0;
		
			while(j < patternLength && textData[i+j] == patternData[j]) {
				j++;
			}

			if (j == patternLength) 
			{
				if(findAll)
                {
                	writeOutput(i);
                }
				#pragma omp critical (posaccess)
				{
					if(pos == -1 || i<pos)
					{
						pos = i; 
					}
				}
			}
		}
	}

	if(findAll == 0 || (findAll == 1 && pos == -1)) {
		writeOutput(pos);
	}
}

int main(int argc, char **argv)
{
	remove("result_OMP.txt"); // removing previous output

    int all;

    FILE *f;
    char *fname = "inputs/control.txt";
    f = fopen(fname, "r");
    
    if(!f) {
            puts("Couldn't open control file...");
            return 0;
    }

    while(fscanf(f, "%d %d %d", &all, &textNumber, &patternNumber) == 3) {
            readData();
	if(patternLength > textLength)
       		writeOutput(-1);
	   	else
            	hostMatch(all);

    }
    fclose(f);
}
