#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define SUCCESS 0
#define FAILURE !SUCCESS

void fill_file(FILE *file_ptr, long num_coords) {
		srand(time(NULL)/2); //Set the seed based on the time
	    long i;
		float *f_array = malloc(3 * num_coords * sizeof(float));
		for(i=0; i<3*num_coords; i++)						
			f_array[i]= (float)34*rand()/(RAND_MAX-1);
		fwrite(f_array, sizeof(float), 3*num_coords, file_ptr);
		fclose(file_ptr);
}

//argv[1] -> name of data file 
//argv[2] -> number of coords (x,y and z)
int main(int argc, char *argv[]) {
	if(argc == 3) {
		FILE *file_ptr = fopen(argv[1], "wb");
		if(!file_ptr) {
			printf("File pointer is NULL.\n");
			return(FAILURE);
		}
		fill_file(file_ptr, atoi(argv[2]));
		return(SUCCESS);
	} else {
		printf("Invalid number of arguements. Exiting.\n");
		return(FAILURE);
	}	
}