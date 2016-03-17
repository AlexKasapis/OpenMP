#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <omp.h>

#define MIN_VALUE 12
#define MAX_VALUE 30
#define SUCCESS 0
#define FAILURE !SUCCESS
#define NUMSIZE 4 //size of each float in the file

long calculate_difference(struct timespec start, struct timespec end) {
	const int DAS_NANO_SECONDS_IN_SEC = 1000000000;
	long timeElapsed_s = end.tv_sec - start.tv_sec;
	long timeElapsed_n = end.tv_nsec - start.tv_nsec;
	if(timeElapsed_n < 0) {
		timeElapsed_n =	DAS_NANO_SECONDS_IN_SEC + timeElapsed_n;
		timeElapsed_s--;
	}
	printf("Time: %ld.%09ld secs \n", timeElapsed_s, timeElapsed_n);
	return timeElapsed_s;
}

//Checks if the coords are within the valid limits
int process_coords(FILE *file) {
	float x, y, z;
	fread(&x, sizeof(float), 1, file);
	fread(&y, sizeof(float), 1, file);
	fread(&z, sizeof(float), 1, file);
	//printf("x:%f y:%f z:%f\n", x, y, z);
	if(x>=MIN_VALUE && x<=MAX_VALUE && y>=MIN_VALUE && y<=MAX_VALUE && z>=MIN_VALUE &&
	z<=MAX_VALUE)
		return(SUCCESS);
	return(FAILURE);
}

//Finds and returns the size of a file
off_t fsize(const char *filename) {
    struct stat st;

    if (stat(filename, &st) == 0)
        return st.st_size;

    return(FAILURE);
}

int parallel_motion_estimation(long num_coords, int seconds, char *file_name, size_t threads) {
	// Check if file exists
	if(access(file_name, F_OK) == -1) {
		printf("File was not found!\n");
		return(FAILURE);
	}
	#pragma omp parallel shared(file_name)
	{
		long exam_coords = fsize(file_name) / NUMSIZE / 3; //initialization of coordinations we are going to examine= total number
		if(num_coords >=0 && num_coords<exam_coords){
		exam_coords=num_coords;				
	    }
		if(num_coords>total_coords){
		printf("You have asked for more lines than the ones available. All the lines are going to be examined.");
	}
		int total_threads = omp_get_num_threads();
		int tid = omp_get_thread_num();
		FILE *file_ptr = fopen(file_name, "rb");
        struct timespec start, current;
		
		// Set which coords each thread will process
		long coord_from = (int)exam_coords_coords/total_threads * tid;
		long coord_to = (int)exam_coords_coords/total_threads * (tid+1) - 1;
		if(tid+1 == total_threads)
			coord_to += total_coords % total_threads;

		fseek(file_ptr, 3*coord_from*sizeof(float), SEEK_SET);
		long coords_read;
		long valid_collisions=0;
		if(tid==0){
			
			clock_gettime(CLOCK_MONOTONIC, &start);			
		}
		for(coords_read=coord_from; coords_read<coord_to+1; coords_read++) {
			if(process_coords(file_ptr)==0)
				valid_collisions++;
			if(tid==0){
				clock_gettime(CLOCK_MONOTONIC, &current);
				if(calculate_difference(start,current)>max_seconds){
			    printf("Reached maximum time limit.");
			    break;
				}
		  		
			}
			     
		}
		fclose(file_ptr);
		printf("Thread %d, valid collisions %ld\n", tid, valid_collisions);
	}
	return(SUCCESS);
}

int linear_motion_estimation(long num_coords, int max_seconds, char *file_name) {
	FILE *file_ptr = fopen(file_name, "rb");
	if(!file_ptr) {
		printf("Unable to open file!");
		return(FAILURE);
	}
    struct timespec start, current;
	long exam_coords = fsize(file_name) / NUMSIZE / 3; //initialization of coordinations we are going to examine= total number
	long curr_coord;
	long valid_collisions=0;
	if(num_coords >=0 && num_coords<total_coords){
		exam_coords=num_coords;				
	}
	if(num_coords>total_coords){
		printf("You have asked for more lines than the ones available. All the lines are going to be examined.");
	}
	if(max_seconds <= -1)
		max_seconds=1000000;
	clock_gettime(CLOCK_MONOTONIC, &start);
	for(curr_coord=0; curr_coord<exam_coords; curr_coord++) {
		if(process_coords(file_ptr)==0)
			valid_collisions++;
		clock_gettime(CLOCK_MONOTONIC, &current);
		if(calculate_difference(start,current)>max_seconds){
			printf("Reached maximum time limit.");
			break;
		}
	}
	fclose(file_ptr);
	printf("Linear Examine -> Valid collisions: %ld\n", valid_collisions);
	return(SUCCESS);
}

/*
 * argv[1] : number of collisions to examine
 * argv[2] : maximum running time
 * argv[3] : name of the data file
 * argv[4] : how many threads should openmp use (-1 = all available threads)
 * argv[5] : how many procs should openmpi use (-1 = all available procs)
 */
int main(int argc, char *argv[]) {
	if(argc == 6) {
		struct timespec start, end;
		clock_gettime(CLOCK_MONOTONIC, &start);
		linear_motion_estimation(atoi(argv[1]), atoi(argv[2]), argv[3]);
		parallel_motion_estimation(atoi(argv[1]), atoi(argv[2]), argv[3], atoi(argv[4]));
		clock_gettime(CLOCK_MONOTONIC, &end);
		calculate_difference(start, end);
		return 0;
	} else {
		printf("Invalid number of arguements. Exiting.\n");
		return -1;
	}
}