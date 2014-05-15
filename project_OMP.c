
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <omp.h>
#include <limits.h>

////////////////////////////////////////////////////////////////////////////////
// Program main
////////////////////////////////////////////////////////////////////////////////

char *textData;
int textLength, textNumber;

char *patternData;
int patternLength, patternNumber;

FILE* outfiles[8];

void outOfMemory()
{
	fprintf (stderr, "Out of memory\n");
	exit (0);
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
		
void hostMatchAll()
{
	long pos, i, j, lastI;
	pos = LONG_MAX;
	lastI = textLength-patternLength+1;

	#pragma omp parallel private(i, j) shared(pos) num_threads(8)
	{
		i = omp_get_thread_num();
		j = 0;

		while(i < lastI)
		{
			while(j<patternLength && textData[i+j] == patternData[j])
			{
				j++;
			}

			if (j == patternLength)
			{
				pos = i;
				fprintf(outfiles[omp_get_thread_num()], "%d %d %ld\n", textNumber, patternNumber, i);
			}

			j = 0;
			i += 8;
		}
	}

	if(pos == LONG_MAX) 
	{
		fprintf(outfiles[0], "%d %d %d\n", textNumber, patternNumber, -1);
	}
}

void hostMatchLeft()
{
	long pos, i, j, lastI;
	pos = LONG_MAX;
	lastI = textLength-patternLength+1;

	#pragma omp parallel private(i, j) shared(pos) num_threads(8)
	{
		i = omp_get_thread_num();

		while(i < lastI && i < pos)
		{
			j = 0;
			while(j<patternLength && textData[i+j] == patternData[j])
			{
				j++;
			}

			if (j == patternLength)
			{
				#pragma omp critical (posaccess)
				{
					if (i < pos)
						pos = i;
				}
			}
			i += 8;
		}
	}

	if (pos == LONG_MAX)
	{
		fprintf(outfiles[0], "%d %d %d\n", textNumber, patternNumber, -1);
	}
	else
	{
		fprintf(outfiles[0], "%d %d %ld\n", textNumber, patternNumber, pos);
	}
}

int main(int argc, char **argv)
{
	remove("result_OMP.txt"); // removing previous output

	int all;
	int i = 0;
	char ofname[17];

	while(i < 8)
	{
		sprintf(ofname, "result_OMP_%d.txt", i);
		remove(ofname);
		outfiles[i] = fopen(ofname, "a");
		if(!outfiles[i])
		{
			printf("Couldn't open an output file for thread %d. Exiting...\n", i);
			return -1;
		}
		i++;
	}

    FILE *f;
    char *fname = "inputs/control.txt";
    f = fopen(fname, "r");
    
    if(!f) {
            puts("Couldn't open control file...");
            return -1;
    }

    while(fscanf(f, "%d %d %d", &all, &textNumber, &patternNumber) == 3) {
        readData();

		if(patternLength > textLength)
       		fprintf(outfiles[0], "%d %d -1\n", textNumber, patternNumber);
	   	else if (all)
        	hostMatchAll();
        else 
        	hostMatchLeft();
    }

    i=0;
    while(i < 8)
    {
    	fclose(outfiles[i]);
    	i++;
    }
    
    if(!system("cat result_OMP_* > result_OMP.txt"))
    {
    	// shiieeee
    }

    i=0;
    while(i<8)
    {
    	sprintf(ofname, "result_OMP_%d.txt", i);
    	remove(ofname);
    	i++;
    }

    fclose(f);
}
