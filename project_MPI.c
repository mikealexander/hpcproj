
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <mpi.h>

////////////////////////////////////////////////////////////////////////////////
// Program main
////////////////////////////////////////////////////////////////////////////////

char *textData;
int textLength, textNumber;

char *patternData;
int patternLength, patternNumber;

int startpos, endpos, numTests, testNum, all;

struct Control {
	int textNumber;
	int patternNumber;
	int all;
};

struct Control controls[20];

void outOfMemory()
{
	fprintf (stderr, "Out of memory\n");
	exit (0);
}

int writeOutput(int matchpos)
{
	FILE *f = fopen("result_MPI.txt", "a");
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


int hostMatch()
{
	int i,j,k, lastI;
	
	i=startpos;
	j=0;
	k=startpos;
	lastI = endpos-patternLength+1;

	while (i<=lastI && j<patternLength)
	{
		if (textData[k] == patternData[j])
		{
			k++;
			j++;
		}
		else
		{
			i++;
			k=i;
			j=0;
		}
	}
	if (j == patternLength)
	{
		printf("found a at %d\n", i);
		return i;
	}
	else
		return textLength+1;
}

int hostMatchAll()
{
	int foundFlag = -1;

	int i,j,k, lastI;
	
	i=startpos;
	j=0;
	k=startpos;
	lastI = endpos-patternLength+1;

	while (i<=lastI)
	{
		if(j==patternLength)
		{
			foundFlag = 1;
			printf("Found one at %d\n", i);
			writeOutput(i);
			i++;
			k=i;
			j=0;
		}
		else if (textData[k] == patternData[j])
		{

			k++;
			j++;
		}
		else
		{
			i++;
			k=i;
			j=0;
		}
	}

	return foundFlag;
}


int main(int argc, char **argv)
{
	remove("result_MPI.txt"); // removing previous output

	int npes, rank, result, globalresult;

	MPI_Init(&argc, &argv);	
    MPI_Comm_size(MPI_COMM_WORLD, &npes);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

	int tN, pN, a;

    FILE *f;
    char *fname = "inputs/control.txt";
    f = fopen(fname, "r");
    
    if(!f) {
    	puts("Couldn't open control file...");
        return 0;
    }

	numTests = 0;
    while(fscanf(f, "%d %d %d", &a, &tN, &pN) == 3) {
    	controls[numTests].textNumber = tN;
    	controls[numTests].patternNumber = pN;
    	controls[numTests].all = a;
    	numTests++;
	}

	fclose(f);
	MPI_Barrier(MPI_COMM_WORLD);

	testNum = 0;	
    while(testNum < numTests)
    {
    	textNumber = controls[testNum].textNumber;
    	patternNumber = controls[testNum].patternNumber;
    	all = controls[testNum].all;

		readData();

		startpos = rank * (textLength/npes);
		endpos = startpos + (textLength/npes);

		if(rank < npes - 1)
		{
			endpos += patternLength-2;
		}
		else
		{
			endpos += textLength%npes;
		}

		if(all == 0)
		{
			result = hostMatch();
			MPI_Reduce(&result, &globalresult, 1, MPI_INT, MPI_MIN, 0, MPI_COMM_WORLD);

			if(rank == 0)
			{
				if (globalresult == textLength+1)
					globalresult = -1;

				writeOutput(globalresult);
			}
		}
		else
		{
			result = hostMatchAll();
			MPI_Reduce(&result, &globalresult, 1, MPI_INT, MPI_MAX, 0, MPI_COMM_WORLD);

			if (rank == 0)
			{
				if (globalresult == -1)
					writeOutput(-1);
			}
		}

		MPI_Barrier(MPI_COMM_WORLD);
		testNum++;
    }

	MPI_Finalize();
}