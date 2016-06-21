#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include <string.h>
#include <assert.h>
#include "mpi.h"

#define SIZE 100000000

int *array;

// sort an array between indexers lo and hi
void quicksort(int *a, int lo, int hi)
{
    int i=lo,j=hi,h;
    int x=a[(lo+hi)/2];
    
    // partition
    do
    {    
        while(a[i]<x) i++; 
        while(a[j]>x) j--;
        if(i<=j)
        {
            h=a[i]; a[i]=a[j]; a[j]=h;
            i++; j--;
        }
    } while(i<=j);
    
    // recursion
    if(lo<j) quicksort(a,lo,j);
    if(i<hi) quicksort(a,i,hi);
}

// initialize global array
void initialize() 
{
    int i;
    array = (int *) malloc(SIZE*sizeof(int));
    
    srand(0);
    
    for(i=0; i<SIZE; i++) {
        array[i]=rand();
    }
}


// print array
void print(int *a, int s) 
{
	for(int i=0;i<s;i++) {
		printf("%d ",a[i]);
	}
	printf("\n");
}

// checks if the array is sorted
void validate() 
{	
    int i,error=0;
    for(i=0;i<SIZE&&!error;i++)
        if(array[i-1]>array[i]) error=1;
    
    if(error) printf("Error\n");
    else printf("Ok\n");
}

int main(int argc, char *argv[])
{
    int numtasks,
        taskid,
        nelems;
    
    MPI_Status status;

    MPI_Init( &argc, &argv );
    MPI_Comm_rank( MPI_COMM_WORLD, &taskid );
    MPI_Comm_size( MPI_COMM_WORLD, &numtasks );
    
    // init of the array
    if(taskid == 0) {
        initialize();
        //print(array,SIZE); puts("----------");
    }

    // init time measuring
    double wtime = omp_get_wtime();
    
    // calculates the number of elements per process
    nelems = SIZE/numtasks;
    int *sub_array = (int *) malloc(nelems*sizeof(int));
    assert(sub_array != NULL);

    double stime = omp_get_wtime();
    /* scatter */
    MPI_Scatter(array,nelems,MPI_INT,sub_array,nelems,MPI_INT,0,MPI_COMM_WORLD);
    stime = omp_get_wtime() - stime;

    /* do things */
    quicksort(sub_array,0,nelems-1);  

    int *sorted = (int *) malloc(SIZE*sizeof(int));
    assert(sorted != NULL);

    double gtime = omp_get_wtime();
    /* gather */
    MPI_Gather(sub_array,nelems,MPI_INT,sorted,nelems,MPI_INT,0,MPI_COMM_WORLD);
    gtime = omp_get_wtime() - gtime;

    MPI_Barrier(MPI_COMM_WORLD);
  	MPI_Finalize();

  	/* merge - sequential version */
    if(taskid == 0) {
    	int *index = (int *) malloc(numtasks*sizeof(int));
    	int min,i,j,b=0;

    	for(i=0; i<numtasks; i++) {
    		index[i] = i*nelems;
    	}

    	for(i=0;i<SIZE;i++) {
    		min = SIZE; // forcing to be the most higher value possible
    		for(j=0;j<numtasks;j++) {
    			// minimum condition && ensures doesn't pass its index limit
    			if((index[j] < j*nelems+nelems) && (sorted[index[j]] < min)) {
    				min = sorted[index[j]];
    				b=j;
    			}
    		}
    		index[b] += 1;
    		array[i] = min;
    	}
    	
    	printf("WallTime = %f\n", omp_get_wtime()-wtime);
    	printf("CommTime = %f\t", stime+gtime);
    	//print(sorted,SIZE);
    	//print(array,SIZE);
   		validate();	
    }

    // clean up
    if(taskid == 0) {
    	free(sorted);
    }
    free(sub_array);

    return 0;
}
