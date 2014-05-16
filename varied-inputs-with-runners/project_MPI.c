
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <mpi.h>
#include <limits.h>

////////////////////////////////////////////////////////////////////////////////
// Program main
////////////////////////////////////////////////////////////////////////////////

char *textData;
int textLength, textNumber;

char *patternData;
int patternLength, patternNumber;

int start, num, all;
long localResult = LONG_MAX;

FILE *threadFile;

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


void hostMatchLeft()
{
	long i,j,k, lastI;
	
	i=start;
	j=0;
	k=start;
	lastI = textLength-patternLength;

	while (i<=lastI && j<patternLength)
	{
		if (textData[k] == patternData[j])
		{
			k++;
			j++;
		}
		else
		{
			i+=num;
			k=i;
			j=0;
		}
	}
	if (j == patternLength)
	{
		localResult = i;
	}
}

void hostMatchAll()
{
	long i,j, lastI;
	long r = LONG_MAX;

	i=start;
	j=0;
	lastI = textLength-patternLength;

	while (i<=lastI)
	{
		while(j<patternLength && textData[i+j] == patternData[j])
		{
			j++;
		}
		if(j==patternLength)
		{
			localResult = i;
			fprintf(threadFile, "%d %d %ld\n", textNumber, patternNumber, i);
		}

		j=0;
		i+=num;
	}
}

int main(int argc, char **argv)
{
	int rank, npes;
	MPI_Init(&argc, &argv);	
    MPI_Comm_size(MPI_COMM_WORLD, &npes);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    start = rank;
    num = npes;

    if (rank == 0)
    {
		remove("result_MPI.txt"); // removing previous output
	}

	char filename[18];
	sprintf(filename, "result_MPI_%d.txt", rank);
	threadFile = fopen(filename, "a");

    FILE *f;
    char *fname = "inputs/control.txt";
    f = fopen(fname, "r");

    while(fscanf(f, "%d %d %d", &all, &textNumber, &patternNumber) == 3) 
    {
    	// read pattern and text (all threads)
    	readData();

    	// perform search
    	if (all)
    	{
    		hostMatchAll();
    	}
    	else
    	{
    		hostMatchLeft();
    	}


		long globalResult;
    	
		MPI_Reduce(&localResult, &globalResult, 1, MPI_LONG, MPI_MIN, 0, MPI_COMM_WORLD);

		if (rank == 0)
		{
			if (globalResult == LONG_MAX)
			{
				fprintf(threadFile, "%d %d %d\n", textNumber, patternNumber, -1);
			}
			else if (!all)
			{
				fprintf(threadFile, "%d %d %ld\n", textNumber, patternNumber, globalResult);
			}
		}
    	
    	MPI_Barrier(MPI_COMM_WORLD);
    	localResult = LONG_MAX;
	}

	fclose(f);
	fclose(threadFile);
	
	MPI_Barrier(MPI_COMM_WORLD);
	if(rank == 0)
	{
		if(!system("cat result_MPI_*.txt > result_MPI.txt"))
		{
			// things
		}
	}
	MPI_Barrier(MPI_COMM_WORLD);

	remove(filename);

	MPI_Finalize();
}