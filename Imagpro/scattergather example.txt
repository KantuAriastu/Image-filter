/* Image filter using MPI		using 4 core	                              */
/******************************************************************************/
/* Source Code:                                                               */
/* Include all library we need                                                */
#include <stdio.h>
#include "mpi.h"
#include <stdlib.h>
#include <math.h>
#include <string.h>

int main(int argc, char **argv) {
	int size, rank;

	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &size);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);

	
	if (rank == 0) {
		int *globaldata = NULL;
		int localdata;
		globaldata = (int*)malloc(size * sizeof(int));
		for (int i = 0; i<size; i++)
			globaldata[i] = 2 * i + 1;

		printf("Processor %d has data: ", rank);
		for (int i = 0; i<size; i++)
			printf("%d ", globaldata[i]);
		printf("\n");
		MPI_Scatter(globaldata, 1, MPI_INT, &localdata, 1, MPI_INT, 0, MPI_COMM_WORLD);

		printf("Processor %d has data %d\n", rank, localdata);
		localdata *= 2;
		printf("Processor %d doubling the data, now has %d\n", rank, localdata);

	
		MPI_Gather(&localdata, 1, MPI_INT, globaldata, 1, MPI_INT, 0, MPI_COMM_WORLD);
		printf("Processor %d has data: ", rank);
		for (int i = 0; i<size; i++)
			printf("%d ", globaldata[i]);
		printf("\n");
		free(globaldata);
	}

	else {
		int localdata;
		void *globaldata = NULL;
		MPI_Scatter(globaldata, 1, MPI_INT, &localdata, 1, MPI_INT, 0, MPI_COMM_WORLD);
		printf("Processor %d has data %d\n", rank, localdata);
		localdata *= 2;
		printf("Processor %d doubling the data, now has %d\n", rank, localdata);
		MPI_Gather(&localdata, 1, MPI_INT, globaldata, 1, MPI_INT, 0, MPI_COMM_WORLD);
	}


	MPI_Finalize();
	return 0;
}


