/* Image filter, modified parallel using MPI scatter gather					  */
/* Ngakan Putu Ariastu, 5 Desember 2017														  */
/*	get time not using MPI = ~265ms							                  */
/*  after using MPI		   = ~300ms										      */
/*  tested on Intel i7-3517u 4 core											  */
/******************************************************************************/

/* Source Code:                                                               */
/* Include all library we need                                                */
#include <stdio.h>
#include "mpi.h"
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <Windows.h>

/* type def struct for picture data											  */
typedef struct picture {
	int row;
	int col;
	int* data;
}picture;


/* Declare all constant                                                       */
#define IM_SIZE 256 // maximum image size
#define FILTER_SIZE 3 // filter size
volatile DWORD dwStart;


/* Declare all function prototype                                             */
int PictureNew(picture *m, int x, int y);
void read_pict(picture *pict, int *r, int *c);
void image_filter(picture *pict, int r, int c, double filter[][FILTER_SIZE], picture *new_pict);
void write_pict(picture *pict, int r, int c);


/* Begin the Main Function                                                    */
int main(int argc, char *argv[])
{
	double flt[FILTER_SIZE][FILTER_SIZE] = { { -1.25	, 0		,-1.25 },
											 { 0		, 10	, 0 },
											 { -1.25	, 0		, -1.25 } };
	const int aveheight = IM_SIZE/4;
	
	int	numtasks,              /* number of tasks in partition */
		taskid,                /* a task identifier */
		i, j, rc;				   /* misc */

	dwStart = GetTickCount();
	/* Initializing MPI environment */
	MPI_Status status;
	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &taskid);
	MPI_Comm_size(MPI_COMM_WORLD, &numtasks);
	if (numtasks < 2) {
		printf("Need at least two MPI tasks. Quitting...\n");
		MPI_Abort(MPI_COMM_WORLD, rc);
		exit(1);
	}

	if (taskid == 0)
	{
		/* actual height and width of pict */
		int width, height; 

		/* create main matrix to store picture*/
		picture pict;
		picture newpict;
		if (PictureNew(&newpict, IM_SIZE, IM_SIZE) != 1)
			printf("creating main new picture matrix failed\n");
		if (PictureNew(&pict, IM_SIZE, IM_SIZE) != 1)
			printf("creating main ori picture matrix failed\n"); 

		/* create local matrix to worked by root */
		picture local_pict;
		picture local_newpict;
		if (PictureNew(&local_newpict, aveheight+1, IM_SIZE) != 1)
			printf("creating local new picture matrix at workder %d failed\n", taskid); 
		if (PictureNew(&local_pict, aveheight+1, IM_SIZE) != 1)
			printf("creating local ori picture matrix at workder %d failed\n", taskid); 

		/* read picture file */
		read_pict(&pict, &height, &width);
		
		/* scatter file for other worker */
		MPI_Scatter(pict.data, aveheight*pict.col, MPI_INT, local_pict.data, 
					aveheight*local_pict.col, MPI_INT, 0, MPI_COMM_WORLD);
		
		/* send N-1 & N+1 part to other worker and self*/
		/* N-1 and N+1 for worker 1 */
		MPI_Send(pict.data + (1 * aveheight - 1) * pict.col, pict.col, MPI_INT, 1, 0, MPI_COMM_WORLD);
		MPI_Send(pict.data + 2 * aveheight * pict.col, pict.col, MPI_INT, 1, 0, MPI_COMM_WORLD);

		/* N-1 and N+1 for worker 2 */
		MPI_Send(pict.data + (2 * aveheight - 1) * pict.col, pict.col, MPI_INT, 2, 0, MPI_COMM_WORLD);
		MPI_Send(pict.data + 3 * aveheight * pict.col, pict.col, MPI_INT, 2, 0, MPI_COMM_WORLD);

		/* N-1 for worker 3 */
		MPI_Send(pict.data + (3 * aveheight - 1) * pict.col, pict.col, MPI_INT, 3, 0, MPI_COMM_WORLD);

		/* N+1 for worker 0*/
		for (i = 0;i < IM_SIZE;i++)
			local_pict.data[aveheight*pict.col + i] = pict.data[aveheight*pict.col + i];
		
		/* do local filtering for image */
		image_filter(&local_pict, local_pict.row, local_newpict.col, flt, &local_newpict);

		/* gather back result to root */
		MPI_Gather(local_newpict.data, aveheight*local_newpict.col, MPI_INT, 
			newpict.data, aveheight*local_newpict.col, MPI_INT, 0, MPI_COMM_WORLD );
		
		/* print the image */
		write_pict(&newpict, height, width);

		/* Dont forget to free memory used :) */
		free(pict.data);
		pict.data = NULL;
		free(newpict.data);
		newpict.data = NULL;
		free(local_pict.data);
		local_pict.data = NULL;
		free(local_newpict.data);
		local_newpict.data = NULL;
		printf_s("time taken, %d milliseconds\n", GetTickCount() - dwStart);
	}


	else if (taskid == 1)
	{
		/* create local matrix to worked by worker 1 */
		picture local_pict;
		picture local_newpict;
		if (PictureNew(&local_newpict, aveheight + 2, IM_SIZE) != 1)
			printf("creating local new picture matrix at workder %d failed\n", taskid); 
		if (PictureNew(&local_pict, aveheight + 2, IM_SIZE) != 1)
			printf("creating local ori picture matrix at workder %d failed\n", taskid); 

		/* receive scatter data */
		MPI_Scatter(NULL, aveheight*local_pict.col, MPI_INT, local_pict.data + 1 * local_pict.col,
			aveheight*local_pict.col, MPI_INT, 0, MPI_COMM_WORLD);
		
		/* receive N-1 and N+1 data */
		MPI_Recv(local_pict.data, local_pict.col, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);
		MPI_Recv(local_pict.data + (aveheight + 1)*local_pict.col, local_pict.col, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);

		/* do local filtering for image */
		image_filter(&local_pict, local_pict.row, local_newpict.col, flt, &local_newpict);

		/* gather back result to root */
		MPI_Gather(local_newpict.data + 1 * local_newpict.col, aveheight*local_newpict.col, MPI_INT,
			NULL, aveheight*local_newpict.col, MPI_INT, 0, MPI_COMM_WORLD);
		
		//write_pict(&local_newpict, aveheight+2, IM_SIZE);
		
		/* Dont forget to free memory used */
		free(local_pict.data);
		local_pict.data = NULL;
		free(local_newpict.data);
		local_newpict.data = NULL;
	}

	else if (taskid == 2)
	{
		/* create local matrix to worked by worker 2 */
		picture local_pict;
		picture local_newpict;
		if (PictureNew(&local_newpict, aveheight + 2, IM_SIZE) != 1)
			printf("creating local new picture matrix at workder %d failed\n", taskid);
		if (PictureNew(&local_pict, aveheight + 2, IM_SIZE) != 1)
			printf("creating local ori picture matrix at workder %d failed\n", taskid); 

		/* receive scatter data */
		MPI_Scatter(NULL, aveheight*local_pict.col, MPI_INT, local_pict.data + 1 * local_pict.col,
			aveheight*local_pict.col, MPI_INT, 0, MPI_COMM_WORLD);

		/* receive N-1 and N+1 data */
		MPI_Recv(local_pict.data, local_pict.col, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);
		MPI_Recv(local_pict.data + (aveheight + 1)*local_pict.col, local_pict.col, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);

		/* do local filtering for image */
		image_filter(&local_pict, local_pict.row, local_newpict.col, flt, &local_newpict);

		/* gather back result to root */
		MPI_Gather(local_newpict.data + 1 * local_newpict.col, aveheight*local_newpict.col, MPI_INT,
			NULL, aveheight*local_newpict.col, MPI_INT, 0, MPI_COMM_WORLD);

		//write_pict(&local_newpict, aveheight + 2, IM_SIZE);
		
		/* Dont forget to free memory used */
		free(local_pict.data);
		local_pict.data = NULL;
		free(local_newpict.data);
		local_newpict.data = NULL;
	}


	else if (taskid == 3)
	{
		/* create local matrix to worked by worker 3 */
		picture local_pict;
		picture local_newpict;
		if (PictureNew(&local_newpict, aveheight + 1, IM_SIZE) != 1)
			printf("creating local new picture matrix at workder %d failed\n", taskid); 
		if (PictureNew(&local_pict, aveheight + 1, IM_SIZE) != 1)
			printf("creating local ori picture matrix at workder %d failed\n", taskid);

		/* receive scatter data */
		MPI_Scatter(NULL, aveheight*local_pict.col, MPI_INT, local_pict.data + 1 * local_pict.col,
					aveheight*local_pict.col, MPI_INT, 0, MPI_COMM_WORLD);

		/* receive N-1 data */
		MPI_Recv(local_pict.data, local_pict.col, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);
	
		/* do local filtering for image */
		image_filter(&local_pict, local_pict.row, local_newpict.col, flt, &local_newpict);

		/* gather back result to root */
		MPI_Gather(local_newpict.data + 1 * local_pict.col, aveheight*local_newpict.col, MPI_INT,
			NULL, aveheight*local_newpict.col, MPI_INT, 0, MPI_COMM_WORLD);
		
		//write_pict(&local_pict, aveheight+1, IM_SIZE);
		
		/* Dont forget to free memory used */
		free(local_pict.data);
		local_pict.data = NULL;
		free(local_newpict.data);
		local_newpict.data = NULL;
	}

	MPI_Finalize();


	return(0);
	/* End Main Function                                                          */
}

int PictureNew(picture *m, int x, int y)
{
	m->row = x;
	m->col = y;
	m->data = (int*)calloc(m->row*m->col, sizeof(int));

	if (m->data)
		return 1;
	else
		return 0;
}


/* Begin read_pict function                                                   */
/******************************************************************************/
/* Purpose : This function reads the image from original.pgm                  */
/******************************************************************************/
/* Variable Definitions                                                       */
/* Variable Name          Type     Description                                */
/* pict[][]               int      array address                              */
/* r                      int *    # of rows pointer                          */
/* c                      int *    # of column pointer                        */
/* max                    int      maximum pixel value                        */
/* i                      int      loop counter                               */
/* j                      int      loop counter                               */
/* line[200]              char     string                                     */
/* in                     FILE *   input file pointer                         */
/******************************************************************************/
/* Source Code:                                                               */
void read_pict(picture *pict, int *r, int *c)
{
	int i, j, max;
	FILE *in;
	char line[200];

	in = fopen("original.pgm", "r");	// f16 image
	if (in == NULL)
	{
		printf("Error reading original.pgm\n");
		exit(1);
	}

	fgets(line, 199, in); // get PGM Type

	line[strlen(line) - 1] = '\0';  // get rid of '\n'

	if (strcmp(line, "P2") != 0)
	{
		printf("Cannot process %s PGM format, only P2 type\n", line);
		exit(2);
	}

	fgets(line, 199, in); // get comment

	fscanf(in, "%d %d", c, r); // get size width x height

	fscanf(in, "%d", &max); // get maximum pixel value

	for (i = 0; i < *r; i++)
		for (j = 0; j < *c; j++)
			fscanf(in, "%d", &pict->data[i*pict->col + j]);
	fclose(in);

	return;
	/* End read_pict function                                                     */
}

/* Begin image_filter function                                                */
/******************************************************************************/
/* Purpose : This function filter the image and create a new image            */
/******************************************************************************/
/* Variable Definitions                                                       */
/* Variable Name          Type     Description                                */
/* pict[][]               int      array address                              */
/* new_pict[][]           int      array address                              */
/* r                      int      # of rows                                  */
/* c                      int      # of column                                */
/* i                      int      loop counter                               */
/* j                      int      loop counter                               */
/* m                      int      loop counter                               */
/* n                      int      loop counter                               */
/* coeff                  double   coefficient                                */
/* sum                    double   sum                                        */
/******************************************************************************/
/* Source Code:                                                               */
void image_filter(picture *pict, int r, int c, double filter[][FILTER_SIZE],
	picture *new_pict)
{
	double coeff = 0;
	double sum;
	int i, j, m, n;

	/*  copy edges                                                                */
	for (i = 0; i < r; i++)
	{
		new_pict->data[i*new_pict->col + 0] = pict->data[i*pict->col + 0];
		new_pict->data[i*new_pict->col + c - 1] = pict->data[i*pict->col + c - 1];
	}

	for (j = 0; j < c; j++)
	{
		new_pict->data[0 * new_pict->col + j] = pict->data[0 * pict->col + j];
		new_pict->data[(r - 1)*new_pict->col + j] = pict->data[(r - 1)*pict->col + j];
	}

	/*  compute coefficient                                                       */
	for (i = 0; i < FILTER_SIZE; i++)
		for (j = 0; j < FILTER_SIZE; j++)
			coeff += filter[i][j];

	/*  filter the image                                                          */
	for (i = 1; i < r - 1; i++)
		for (j = 1; j < c - 1; j++)
		{
			sum = 0;
			for (m = 0; m < FILTER_SIZE; m++)
				for (n = 0; n < FILTER_SIZE; n++)
					sum += pict->data[(i + (m - 1))*pict->col + (j + (n - 1))] * filter[m][n];
			new_pict->data[i*new_pict->col + j] = (int)sum;
		}

	if (coeff != 0)
	{
		for (i = 1; i < r - 1; i++)
			for (j = 1; j < c - 1; j++)
				new_pict->data[i*new_pict->col + j] = (int)(new_pict->data[i*new_pict->col + j] / coeff);
	}

	/*  check for pixel > 255 and pixel < 0                                       */
	for (i = 1; i < r - 1; i++)
		for (j = 1; j < c - 1; j++)
		{
			if (new_pict->data[i*new_pict->col + j] < 0)
				new_pict->data[i*new_pict->col + j] = 0;
			else if (new_pict->data[i*new_pict->col + j] > 255)
				new_pict->data[i*new_pict->col + j] = 255;
		}

	return;
	/* End image_filter function                                                  */
}

/* Begin write_pict function                                                  */
/******************************************************************************/
/* Purpose : This function write the filtered image to new.pgm                */
/******************************************************************************/
/* Variable Definitions                                                       */
/* Variable Name          Type     Description                                */
/* pict[][]               int      array address                              */
/* new_pict[][]           int      array address                              */
/* r                      int      # of rows                                  */
/* c                      int      # of column                                */
/* i                      int      loop counter                               */
/* j                      int      loop counter                               */
/* out                    FILE *   output FILE pointer                        */
/******************************************************************************/
/* Source Code:                                                               */
void write_pict(picture *pict, int r, int c)
{
	int i, j;
	FILE *out;

	out = fopen("new.pgm", "w");

	fprintf(out, "P2\n");
	fprintf(out, "# new.pgm\n");
	fprintf(out, "%d %d\n", r, c);
	fprintf(out, "255\n");

	for (i = 0; i < r; i++)
	{
		for (j = 0; j < c; j++)
			fprintf(out, "%5d", pict->data[i*pict->col + j]);
		fprintf(out, "\n");
	}

	fclose(out);
	return;
	/* End write_pict function                                                    */
}



