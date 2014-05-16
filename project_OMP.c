
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <omp.h>
#include <limits.h>

////////////////////////////////////////////////////////////////////////////////
// OMP Implementation of Sequential Search by Michael Alexander 2014
////////////////////////////////////////////////////////////////////////////////

FILE* outfiles[8];

typedef struct
{
	int searchType;
	int textNumber;
	int patternNumber;
} test;

typedef struct
{
	int length;
	char *data;
} dataItem;

test tests[200];
dataItem *patterns[20];
dataItem *texts[20];

int seenPatterns[20];
int seenTexts[20];

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

int readText (int textNumber)
{
	char *textData;
	int textLength;

	FILE *f;
	char fileName[1000];
	sprintf (fileName, "inputs/text%d.txt", textNumber);
	f = fopen (fileName, "r");
	if (f == NULL)
		return 0;
	readFromFile (f, &textData, &textLength);
	fclose (f);
	dataItem textItem;
	textItem.data = textData;
	textItem.length = textLength;

	texts[textNumber] = malloc(sizeof(textItem));
	(*texts[textNumber]) = textItem;

	return 1;
}

int readPattern (int patternNumber)
{
	char *patternData;
	int patternLength;

	FILE *f;
	char fileName[1000];
	sprintf (fileName, "inputs/pattern%d.txt", patternNumber);
	f = fopen (fileName, "r");
	if (f == NULL)
		return 0;
	readFromFile (f, &patternData, &patternLength);
	fclose (f);
	dataItem patternItem;
	patternItem.data = patternData;
	patternItem.length = patternLength;

	patterns[patternNumber] = malloc(sizeof(patternItem));
	(*patterns[patternNumber]) = patternItem;

	return 1;
}

		
void hostMatchAll(int textNumber, int patternNumber)
{
	long pos, i, j, lastI;
	pos = LONG_MAX;
	lastI = (*texts[textNumber]).length-(*patterns[patternNumber]).length + 1;

	#pragma omp parallel private(i, j) shared(pos) num_threads(8)
	{
		i = omp_get_thread_num();
		j = 0;

		while(i < lastI)
		{
			while(j<(*patterns[patternNumber]).length && (*texts[textNumber]).data[i+j] == (*patterns[patternNumber]).data[j])
			{
				j++;
			}

			if (j == (*patterns[patternNumber]).length)
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

void hostMatchLeft(int textNumber, int patternNumber)
{
	long pos, i, j, lastI;
	pos = LONG_MAX;
	lastI = (*texts[textNumber]).length-(*patterns[patternNumber]).length+1;

	#pragma omp parallel private(i, j) shared(pos) num_threads(8)
	{
		i = omp_get_thread_num();

		while(i < lastI && i < pos)
		{
			j = 0;
			while(j<(*patterns[patternNumber]).length && (*texts[textNumber]).data[i+j] == (*patterns[patternNumber]).data[j])
			{
				j++;
			}

			if (j == (*patterns[patternNumber]).length)
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
	int all;
	char ofname[17];
	remove("result_OMP.txt"); // removing previous output

	// initialise all the seen patterns and texts to 0
	int i;
	for(i=0; i<20; i++)
	{
		seenTexts[i] = 0;
		seenPatterns[i] = 0;
	}

	// open all the file pointers for output
	i=0;
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

    int testCounter = 0;
    int o, t, p;
    while(fscanf(f, "%d %d %d", &o, &t, &p) == 3) {
        tests[testCounter].searchType = o;
        tests[testCounter].textNumber = t;
        tests[testCounter].patternNumber = p;

        seenTexts[t] = 1;
        seenPatterns[p] = 1;
        testCounter++;
    }

    fclose(f);

    // read in the patterns and texts we need in parallel
	#pragma omp parallel for schedule(static) private(i) num_threads(8)
	for(i=0; i<20; i++)
	{
		if (seenPatterns[i])
			readPattern(i);
		if (seenTexts[i])
			readText(i);
	}

	// time to do the tests!
	i=0;
	while(i<testCounter)
	{
		if((*patterns[tests[i].patternNumber]).length > (*texts[tests[i].textNumber]).length)
			fprintf(outfiles[0], "%d %d -1\n", tests[i].textNumber, tests[i].patternNumber);
		else if (tests[i].searchType)
			hostMatchAll(tests[i].textNumber, tests[i].patternNumber);
		else
			hostMatchLeft(tests[i].textNumber, tests[i].patternNumber);

		i++;
	}

	// close all the file pointers
    i=0;
    while(i < 8)
    {
    	fclose(outfiles[i]);
    	//sprintf(ofname, "result_OMP_%d.txt", i);
		//remove(ofname);
    	i++;
    }
	// concatinate all the results into the results file
    return system("cat result_OMP_* > result_OMP.txt");
}