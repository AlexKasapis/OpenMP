#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <omp.h>
#include <mpi.h>

#define MIN_VALUE 12
#define MAX_VALUE 30
#define SUCCESS 0
#define FAILURE !SUCCESS

int check_arg_validity(char *argv[]) {
	if(atoi(argv[1]) < -1) {
		printf("First argument value is incorrect.\n");
		return(FAILURE);
	} else if(atoi(argv[2]) < -1) {
		printf("Second argument value is incorrect.\n");
		return(FAILURE);
	} else if(atoi(argv[4]) < -1 || atoi(argv[4]) > 24) {
		printf("Fourth argument value is incorrect.\n");
		return(FAILURE);
	} else if(atoi(argv[5]) < -1) {
		printf("Fifth argument value is incorrect.\n");
		return(FAILURE);
	}
	return(SUCCESS);
}


//Calculated the difference between two moments in seconds and nanosecs, and returns the
//difference in total nanoseconds.
long calculate_difference(struct timespec start, struct timespec end, int print_time) {
	const int DAS_NANO_SECONDS_IN_SEC = 1000000000;
	long timeElapsed_s = end.tv_sec - start.tv_sec;
	long timeElapsed_n = end.tv_nsec - start.tv_nsec;
	if(timeElapsed_n < 0) {
		timeElapsed_n =	DAS_NANO_SECONDS_IN_SEC + timeElapsed_n;
		timeElapsed_s--;
	}
	if(print_time != 0)
		printf("time: %ld.%09ld secs \n", timeElapsed_s, timeElapsed_n);
	return timeElapsed_s*DAS_NANO_SECONDS_IN_SEC + timeElapsed_n
;
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

int parallel_motion_estimation(long num_coords, int max_seconds, char *file_name, int threads) {
	// Check if file exists
	long int sum;
	sum=0;
	if(access(file_name, F_OK) == -1) {
		printf("File was not found!\n");
		return(FAILURE);
	}

	if(threads >= 1)
		omp_set_num_threads(threads);
	#pragma omp parallel shared(file_name)
	{
		long exam_coords = fsize(file_name) / sizeof(float) / 3; //initialization of coordinations we are going to examine= total number
		if(num_coords >=0 && num_coords < exam_coords)
			exam_coords = num_coords;
		if(num_coords > exam_coords)
			printf("You have asked for more lines than the ones available. All the lines are going to be examined.\n");

		int total_threads = omp_get_num_threads();
		int tid = omp_get_thread_num();
		FILE *file_ptr = fopen(file_name, "rb");
        struct timespec start, current;
		
		// Set which coords each thread will process
		long coord_from = (int)exam_coords/total_threads * tid;
		long coord_to = (int)exam_coords/total_threads * (tid+1) - 1;
		if(tid+1 == total_threads)
			coord_to += exam_coords % total_threads;

		fseek(file_ptr, 3*coord_from*sizeof(float), SEEK_SET);
		long coords_read;
		long valid_collisions=0;

		//if(tid==0)
		clock_gettime(CLOCK_MONOTONIC, &start);

		for(coords_read=coord_from; coords_read<coord_to+1; coords_read++) {
			if(process_coords(file_ptr)==0)
				valid_collisions++;
			//if(tid==0 && max_seconds != -1) {
			if(max_seconds != -1) {
				clock_gettime(CLOCK_MONOTONIC, &current);
				if(calculate_difference(start, current, 0) > max_seconds) {
					printf("Reached maximum time limit.\n");
					break;
				}
			}  
		}

		fclose(file_ptr);
		printf("Thread %d, valid collisions %ld\n", tid, valid_collisions);
		#pragma omp barrier
		
		#pragma omp  for reduction(+:sum)
		for(tid=0;tid<total_threads;tid++){
		  					  
		  
		sum+=valid_collisions;} //sums the total collisions of all threads
		
		
		}
	printf("Total valid collisions of all threads: %ld\n",sum);
	}
	return(SUCCESS);
}


int linear_motion_estimation(long num_coords, int max_seconds, char *file_name) {

	
   

	FILE *file_ptr = fopen(file_name, "rb");
	if(!file_ptr) {
		printf("Unable to open file!\n");
		return(FAILURE);
	}
	

    struct timespec start, current;
	long exam_coords = fsize(file_name) / sizeof(float) / 3; //initialization of coordinations we are going to examine= total number
	long curr_coord;
	long valid_collisions=0;
	if(num_coords>=0 && num_coords<exam_coords)
		exam_coords=num_coords;		
	if(num_coords > exam_coords)
		printf("You have asked for more lines than the ones available. All the lines are going to be examined.\n");

	clock_gettime(CLOCK_MONOTONIC, &start);
	for(curr_coord=0; curr_coord<exam_coords; curr_coord++) {
		if(process_coords(file_ptr)==0)
			valid_collisions++;
		clock_gettime(CLOCK_MONOTONIC, &current);
		if(max_seconds != -1 && calculate_difference(start, current, 0)>max_seconds){
			printf("Reached maximum time limit.\n");
			break;
		}
	}
	fclose(file_ptr);
	printf("Linear Examine -> Valid collisions: %ld\n", valid_collisions);
    
	return(SUCCESS);
	
}
//3rd week progress
int ompi_linear_estimation(long num_coords, int max_seconds, char *file_name){
	// Check if file exists
	if(access(file_name, F_OK) == -1) {
		printf("File was not found!\n");
		return(FAILURE);
	}

	MPI_Init(0,NULL);
	int w_rank;//process rank
	MPI_Comm_rank(MPI_COMM_WORLD, &w_rank);
	int w_size;//number of processes
	MPI_Comm_size(MPI_COMM_WORLD,&w_size);
	long *succesful_col;
	if(w_rank==0){

		 succesful_col= malloc(sizeof(long)*w_size); 
		
		
	}
	//initialization of coordinations we are going to examine= total number
	long exam_coords = fsize(file_name) / sizeof(float) / 3;
	
	long offset= w_rank * (fsize(file_name)/w_size);
	long chunk_size= fsize(file_name)/w_size;
    FILE *file = fopen(file_name,"rb");
	fseek(file,offset, SEEK_SET);
	 long counter=0;
	for(long bytes_read=0; bytes_read<chunk_size; bytes_read+=(3*sizeof(float))){
		 if(process_coords(file)==SUCCESS){
			counter++;
		 }		
	}
	
	MPI_Gather(&counter,1,MPI_LONG, succesful_col,1, MPI_LONG,0,MPI_COMM_WORLD);
	if(w_rank==0){
		long final_count=0;
		for(int i=0;i<w_size;i++){
			final_count+=succesful_col[i];
		}
		printf("number of collisions: %ld",final_count); 
	}



	MPI_Finalize();
	return SUCCESS;
}

/*
 * argv[1] : Maximum number of collisions to examine (-1 == all available collisions)
 * argv[2] : Maximum running time in nanoseconds (-1 == until all collisions are checked)
 * argv[3] : Name of the binary data file
 * argv[4] : How many threads OMP is going to use (-1 == all available threads)
 * argv[5] : How many procs OMPI is going to use (-1 = all available procs)
 */
int main(int argc, char *argv[]) {
	if(argc == 6) {
		if(check_arg_validity(argv) != SUCCESS)
			return(FAILURE);
		struct timespec start, end;

		clock_gettime(CLOCK_MONOTONIC, &start);
		linear_motion_estimation(atoi(argv[1]), atoi(argv[2]), argv[3]);
		clock_gettime(CLOCK_MONOTONIC, &end);
		printf("Linear motion estimation total ");
		calculate_difference(start, end, 1);

		printf("\n");

		clock_gettime(CLOCK_MONOTONIC, &start);
		parallel_motion_estimation(atoi(argv[1]), atoi(argv[2]), argv[3], atoi(argv[4]));
		clock_gettime(CLOCK_MONOTONIC, &end);
		printf("Parallel motion estimation total ");
		calculate_difference(start, end, 1);

		return(SUCCESS);
	} else {
		printf("Invalid number of arguements. Exiting.\n");
		return(FAILURE);
	}
}

