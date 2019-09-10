/* Image Filter modified, based on 587144_imagpro.c, Yudi Satria Gondokaryono */
/* Ngakan Putu Ariastu, 4 Desember 2017										  */
/*															                  */
/*  																	      */
/******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

/* Declare all constant                                                       */
#define IM_SIZE 256 // maximum image size
#define FILTER_SIZE 3 // filter size

typedef struct picture {
	int row;
	int col;
	int* data;
}picture;

/* Declare all function prototype                                             */
int PictureNew ( picture *m, int x, int y );
void read_pict(picture *pict, int *r, int *c);
void image_filter(picture *pict, int r, int c, double filter[][FILTER_SIZE],picture *new_pict);
void write_pict(picture *pict, int r, int c);


/* Begin the Main Function                                                    */
int main( void )
{
	int width, height; // actual image size
	picture pict;
	picture new_pict;
	double flt[FILTER_SIZE][FILTER_SIZE]={{-1.25,  0, -1.25},
										  {	   0, 10,     0},
									   	  {-1.25,  0, -1.25}};
	
	
	/* create matrix to store picture and filtered picture */
	if (PictureNew(&new_pict,IM_SIZE,IM_SIZE)!=1)
		printf("creating picture matrix failed\n"); 
	if (PictureNew(&pict,IM_SIZE,IM_SIZE)!=1)
		printf("creating picture matrix failed\n"); 
	
	/* read picture, process, & write */
	read_pict(&pict, &height, &width);
	image_filter(&pict, height, width, flt, &new_pict);
	write_pict(&new_pict, height, width);
	
	/* dont forget to free memory used */
	free(pict.data);
	pict.data = NULL;
	free(new_pict.data);
	new_pict.data = NULL;
	
	return(0);
/* End Main Function                                                          */
}

/* function implementation                                                     */
int PictureNew ( picture *m, int x, int y )
{
	m->row = x;
	m->col = y;
	m->data = (int*)calloc(m->row*m->col,sizeof(int));
	
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
	
	in=fopen("original.pgm", "r");	// f16 image
	if(in == NULL)
	{
		printf("Error reading original.pgm\n");
		exit(1);
	}
	
	fgets(line, 199, in); // get PGM Type
	
	line[strlen(line)-1]='\0';  // get rid of '\n'
	
	if(strcmp(line, "P2") != 0)
	{
		printf("Cannot process %s PGM format, only P2 type\n", line);
		exit(2);
	}
	
	fgets(line, 199, in); // get comment
		
	fscanf(in, "%d %d", c, r); // get size width x height
	
	fscanf(in, "%d", &max); // get maximum pixel value
	
	for(i = 0; i < *r; i++)
		for(j = 0; j < *c; j++)
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
	for(i = 0; i < r; i++)
	{
		new_pict->data[i*new_pict->col + 0] = pict->data[i*pict->col + 0];
		new_pict->data[i*new_pict->col + c-1] = pict->data[i*pict->col + c-1];
	}

	for(j = 0; j < c; j++)
	{
		new_pict->data[0*new_pict->col + j] = pict->data[0*pict->col + j];
		new_pict->data[(r-1)*new_pict->col + j] = pict->data[(r-1)*pict->col + j];
	}

/*  compute coefficient                                                       */	
	for(i = 0; i < FILTER_SIZE; i++)
		for(j = 0; j < FILTER_SIZE; j++)
			coeff += filter[i][j];

/*  filter the image                                                          */
	for(i = 1; i < r-1; i++)
		for(j = 1; j < c-1; j++)
		{
			sum = 0;
			for(m = 0; m < FILTER_SIZE; m++)
				for(n = 0; n < FILTER_SIZE; n++)
					sum += pict->data[(i+(m-1))*pict->col + (j+(n-1))] * filter[m][n];
			new_pict->data[i*new_pict->col + j] = (int)sum;
		} 

	if(coeff != 0)
	{
		for(i = 1; i < r-1; i++)
			for(j = 1; j < c-1; j++)
				new_pict->data[i*new_pict->col + j] = (int)(new_pict->data[i*new_pict->col + j]/coeff);
	}
	
/*  check for pixel > 255 and pixel < 0                                       */
	for(i = 1; i < r-1; i++)
		for(j = 1; j < c-1; j++)
		{
			if(new_pict->data[i*new_pict->col + j] < 0)
				new_pict->data[i*new_pict->col + j] = 0;
			else if(new_pict->data[i*new_pict->col + j] > 255)
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
	
	out=fopen("new.pgm", "w");

	fprintf(out, "P2\n");
	fprintf(out, "# new.pgm\n");
	fprintf(out, "%d %d\n", r, c);
	fprintf(out, "255\n");
	
	for(i = 0; i < r; i++)
	{
		for(j = 0; j < c; j++)
			fprintf(out, "%5d", pict->data[i*pict->col + j]);
		fprintf(out, "\n");
	}
	
	fclose(out);
	return;
/* End write_pict function                                                    */
}

