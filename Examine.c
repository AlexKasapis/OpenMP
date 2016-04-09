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


/*
 * ----------------------------1st Week Progress--------------------------------
 * This function receives two timespecs, and calculates their difference and
 * returns it in total nanoseconds. If the value of the argument "pr" is not
 * 0, it also prints the difference in SEC.NSEC format.
 * -----------------------------------------------------------------------------
 */
long calculate_difference(struct timespec start, struct timespec end, int pr) {
	const int DAS_NANO_SECONDS_IN_SEC = 1000000000;
	long timeElapsed_s = end.tv_sec - start.tv_sec;
	long timeElapsed_n = end.tv_nsec - start.tv_nsec;
	if(timeElapsed_n < 0) {
		timeElapsed_n =	DAS_NANO_SECONDS_IN_SEC + timeElapsed_n;
		timeElapsed_s--;
	}
	if(pr != 0)
		printf("time: %ld.%09ld secs \n", timeElapsed_s, timeElapsed_n);
	return timeElapsed_s*DAS_NANO_SECONDS_IN_SEC + timeElapsed_n
;
}

/*
 * ----------------------------1st Week Progress--------------------------------
 * Receives a file pointer, reads the next three float numbers from this file,
 * and if ALL the numbers are within the defined limits, returns 0. Else it
 * returns !0.
 * -----------------------------------------------------------------------------
 */
int process_coords(FILE *file) {
	float x, y, z;
	fread(&x, sizeof(float), 1, file);
	fread(&y, sizeof(float), 1, file);
	fread(&z, sizeof(float), 1, file);
	if(x>=MIN_VALUE && x<=MAX_VALUE && y>=MIN_VALUE && y<=MAX_VALUE &&
	z>=MIN_VALUE &&	z<=MAX_VALUE)
		return(SUCCESS);
	return(FAILURE);
}

/*
 * ----------------------------2nd Week Progress--------------------------------
 * This simple function finds and returns the size of a file in bytes. If the
 * file pointer is invalid it returns -1.
 * -----------------------------------------------------------------------------
 */
off_t fsize(const char *filename) {
    struct stat st;

    if (stat(filename, &st) == 0)
        return st.st_size;

    return -1;
}

/* 
 * ----------------------------2nd Week Progress--------------------------------
 * Find all the valid collisions from the data file given as the third argument.
 * First argument is a limitation on how many sets the function will examine.
 * Second argument is a limitation on how many nanoseconds the function will be
 * running. If the time limit is exceeded, the function will stop examining the
 * file.
 * Fourth argument is the number of threads MPI should utilize.
 * This function uses OMP, but NOT OMPI to go through the examination.
 * -----------------------------------------------------------------------------
 */
int parallel_estimation(long num_coords, int max_nsecs, char *file_name, int threads) {
	// Check if file exists. If not, print a message and return !0.
	if(access(file_name, F_OK) == -1) {
		printf("File was not found!\n");
		return(FAILURE);
	}

	// Create a variable where the total number of valid collisions from all the
	// threads will be added to.
	long sum = 0;
	
	// If there is a limitation on how many threads MPI should use (-1 means all
	// available threads), apply it.
	if(threads >= 1)
		omp_set_num_threads(threads);
		
	// Start using OpenMP.
	#pragma omp parallel shared(file_name)
	{
		// Calculate the total number of coordinates that are stored in the 
		// file.
    	// The number is: NUM = FILE_SIZE / SIZE_OF_A_FLOAT / 3
    	// A coordinate is a set of 3 floats.
    	// The function will check exactly (EXAM_COORDS) collisions from the
    	// file.
		long exam_coords = fsize(file_name) / sizeof(float) / 3;
		
		// If there is a limitation on how many collisions the function should
		// go through, change the value of exam_coords to that limitation.
		if(num_coords >=0 && num_coords < exam_coords)
			exam_coords = num_coords;
			
		// If the limitation exceeds the total number of coordinates stored in the
		// data file, display a message. The total number of collision, the function
		// will go through, remains the total number of coordinates available in the
		// file.	
		if(num_coords > exam_coords)
			printf("You have asked for more lines than the ones available. All the lines are going to be examined.\n");

		// Get the total number of threads the OMP is running.
		int total_threads = omp_get_num_threads();
		
		// Get the ID of this particular thread.
		int tid = omp_get_thread_num();
		
		// Each file opens its own pointer to the data file.
		FILE *file_ptr = fopen(file_name, "rb");
		
		// Set which coords each thread will process.
		long coord_from = (int)exam_coords/total_threads * tid;
		long coord_to = (int)exam_coords/total_threads * (tid+1) - 1;
		if(tid+1 == total_threads)
			coord_to += exam_coords % total_threads;

		// Skip some bytes from the data file, in order to get to the set where
		// the thread must start examining from.
		fseek(file_ptr, 3*coord_from*sizeof(float), SEEK_SET);
		
		long coords_read;
		long valid_collisions=0;

		// The timespecs will keep track of the time, if a limitation has been
		// set.
    	struct timespec start, current;
    
		// Before the start of the examination, get the current time.
		clock_gettime(CLOCK_MONOTONIC, &start);

		// The function will check all the collisions, increasing its sum
		// (valid_collisions) every time a collision is within the limits
		// defined in the start of the code.
		// Every time it goes though one set, if there has been set a limitation
		// on how many nanoseconds the function should run, check the current 
		// time, get the difference from the timestamp when the examination
		// started running and if the time limit has been exceeded, stop the
		// loop.
		for(coords_read=coord_from; coords_read<coord_to+1; coords_read++) {
			if(process_coords(file_ptr)==0)
				valid_collisions++;
			if(max_nsecs!=-1&&calculate_difference(start,current,0)>max_nsecs){
				clock_gettime(CLOCK_MONOTONIC, &current);
				printf("Reached maximum time limit.\n");
				break;
			}
		}

		// Each threads closes its file pointer.
		fclose(file_ptr);
		
		#pragma omp barrier
		
		// Finally, add all the valid collision numbers, each thread has found
		// to the shared variable "sum".
		#pragma omp  for reduction(+:sum)
			for(tid=0;tid<total_threads;tid++)
		  		sum+=valid_collisions;
		
		#pragma omp master	
			printf("Non-MPI Parallel Examine -> Valid collisions: %ld\n", sum);
	}
	return(SUCCESS);
}

/* 
 * ----------------------------4th Week Progress--------------------------------
 * Find all the valid collisions from the data file given as the third argument.
 * First argument is a limitation on how many sets the function will examine.
 * Second argument is a limitation on how many nanoseconds the function will be
 * running. If the time limit is exceeded, the function will stop examining the
 * file.
 * Fourth argument is the number of threads MPI should utilize.
 * This function uses OMP AND OMPI to go through the examination.
 * -----------------------------------------------------------------------------
 */
int ompi_parallel_estimation(long num_coords, int max_nsecs, char *file_name, int threads) {
	// Check if file exists. If not, print a message and return !0.
	if(access(file_name, F_OK) == -1) {
		printf("File was not found!\n");
		return(FAILURE);
	}
	
	// Initialize the MPI. After this line, multiple procs run at the same time.
	MPI_Init(NULL, NULL);

    // Get the total number of processes that are running.
    int total_procs;
    MPI_Comm_size(MPI_COMM_WORLD, &total_procs);    

    // Get the rank of the process
    int proc_num;
    MPI_Comm_rank(MPI_COMM_WORLD, &proc_num);
	if(proc_num>total_procs)
	{
		MPI_Finalize();
	}
    
    long *succesful_col;
	if(proc_num == 0)
		 succesful_col = malloc(sizeof(long) * total_procs); 

    // Calculate the total number of coordinates that are stored in the file.
    // The number is: NUM = FILE_SIZE / SIZE_OF_A_FLOAT / 3
    // A coordinate is a set of 3 floats.
    // The function will check exactly (EXAM_COORDS) collisions from the file.
    long exam_coords = fsize(file_name) / sizeof(float) / 3;
    
    // If there is a limitation on how many collisions the function should go
	// through, change the value of exam_coords to that limitation.
    if(num_coords >= 0 && num_coords < exam_coords)
        exam_coords = num_coords;
        
    // If the limitation exceeds the total number of coordinates stored in the
	// data file, display a message. The total number of collision, the function
	// will go through, remains the total number of coordinates available in the
	// file.	
    if(num_coords > exam_coords)
        printf("You have asked for more lines than the ones available. All the lines are going to be examined.\n");
    
    // Set which coords each PROCESS will go through. We will repeat the same
    // divison, when we distribute these coords to every THREAD that is running
    // within ever process. 
    long coord_from = (int)exam_coords/total_procs * proc_num;
    long coord_to = (int)exam_coords/total_procs * (proc_num+1) - 1;
    if(proc_num+1 == total_procs)
        coord_to += exam_coords % total_procs;

	// Create a variable where the total number of valid collisions from all the
	// threads will be added to.
	long sum = 0;
	
	// If there is a limitation on how many threads MPI should use (-1 means all
	// available threads), apply it.
	if(threads >= 1)
		omp_set_num_threads(threads);
		
	// Start using OpenMP.
	#pragma omp parallel shared(file_name)
	{
		// Get the total number of threads the OMP is running.
		int total_threads = omp_get_num_threads();
		
		// Get the ID of this particular thread.
		int tid = omp_get_thread_num();
		
		// Each file opens its own pointer to the data file.
		FILE *file_ptr = fopen(file_name, "rb");
		
		// Same when we divided the sets and distributed them to each process,
		// but this time we are doing it for ever thread within EACH process.
		long coll_from = (int)(coord_to-coord_from+1)/total_threads * tid;
		long coll_to = (int)(coord_to-coord_from+1)/total_threads * (tid+1) - 1;
		if(tid+1 == total_threads)
			coll_to += (coord_to-coord_from+1) % total_threads;

		// Skip some bytes from the data file, in order to get to the set where
		// the thread must start examining from
		fseek(file_ptr, 3*(coord_from+coll_from)*sizeof(float), SEEK_SET);
		
		long coords_read;
		long valid_collisions=0;

		// The timespecs will keep track of the time, if a limitation has been
		// set.
    	struct timespec start, current;
    
		// Before the start of the examination, get the current time.
		clock_gettime(CLOCK_MONOTONIC, &start);

		// The function will check all the collisions, increasing its sum
		// (valid_collisions) every time a collision is within the limits
		// defined in the start of the code.
		// Every time it goes though one set, if there has been set a limitation
		// on how many nanoseconds the function should run, check the current 
		// time, get the difference from the timestamp when the examination
		// started running and if the time limit has been exceeded, stop the
		// loop.
		for(coords_read=coll_from; coords_read<coll_to+1; coords_read++) {
			if(process_coords(file_ptr)==0)
				valid_collisions++;
			if(max_nsecs!=-1&&calculate_difference(start,current,0)>max_nsecs){
				clock_gettime(CLOCK_MONOTONIC, &current);
				printf("Reached maximum time limit.\n");
				break;
			}
		}

		// Each threads closes its file pointer.
		fclose(file_ptr);

		#pragma omp barrier
		
		// Finally, add all the valid collision numbers, each thread has found
		// to the shared variable "sum".
		#pragma omp  for reduction(+:sum)
			for(tid=0;tid<total_threads;tid++)
		  		sum += valid_collisions; //sums the total collisions of all threads
	}
	
	// After each process has calculated how many valid collisions there are in
	// its own section of data, the MPI adds all the different result into one
	// shared variable (final_count) with the help of MPI_Gather.
	MPI_Gather(&sum, 1, MPI_LONG, succesful_col, 1, MPI_LONG, 0, MPI_COMM_WORLD);
	if(proc_num == 0){
		long final_count = 0;
		for(int i=0; i<total_procs; i++){
			final_count+=succesful_col[i];
		}
		printf("MPI Parallel Examine -> Valid collisions: %ld", final_count); 
	}

	MPI_Finalize();
	
	return(SUCCESS);
}

/* 
 * ----------------------------1st Week Progress--------------------------------
 * Find all the valid collisions from the data file given as the third argument.
 * First argument is a limitation on how many sets the function will examine.
 * Second argument is a limitation on how many nanoseconds the function will be
 * running. If the time limit is exceeded, the function will stop examining the
 * file.
 * This function does NOT use OMP or OMPI to go through the examination.
 * -----------------------------------------------------------------------------
 */
int linear_estimation(long num_coords, int max_nsecs, char *file_name) {
	FILE *file_ptr = fopen(file_name, "rb");
	if(!file_ptr) {
		printf("Unable to open file!\n");
		return(FAILURE);
	}
    
    // Calculate the total number of coordinates that are stored in the file.
    // The number is: NUM = FILE_SIZE / SIZE_OF_A_FLOAT / 3
    // A coordinate is a set of 3 floats.
    // The function will check exactly (EXAM_COORDS) collisions from the file.
	long exam_coords = fsize(file_name) / sizeof(float) / 3;
	
	long curr_coord;
	long valid_collisions=0;
	
	// If there is a limitation on how many collisions the function should go
	// through, change the value of exam_coords to that limitation.
	if(num_coords>=0 && num_coords<exam_coords)
		exam_coords=num_coords;
		
	// If the limitation exceeds the total number of coordinates stored in the
	// data file, display a message. The total number of collision, the function
	// will go through, remains the total number of coordinates available in the
	// file.	
	if(num_coords > exam_coords)
		printf("You have asked for more lines than the ones available. All the lines are going to be examined.\n");
	
	// The timespecs will keep track of the time, if a limitation has been set.
    struct timespec start, current;
    
	// Before the start of the examination, get the current time.
	clock_gettime(CLOCK_MONOTONIC, &start);
	
	// The function will check all the collisions, increasing its sum
	// (valid_collisions) every time a collision is within the limits defined in
	// the start of the code.
	// Every time it goes though one set, if there has been set a limitation on
	// how many nanoseconds the function should run, check the current time, get
	// the difference from the timestamp when the examination started running
	// and if the time limit has been exceeded, stop the loop.
	for(curr_coord=0; curr_coord<exam_coords; curr_coord++) {
		if(process_coords(file_ptr)==0)
			valid_collisions++;
		if(max_nsecs!=-1 && calculate_difference(start, current, 0)>max_nsecs){
			clock_gettime(CLOCK_MONOTONIC, &current);
			printf("Reached maximum time limit.\n");
			break;
		}
	}
	
	fclose(file_ptr);
	
	printf("Non-MPI Linear Examine -> Valid collisions: %ld\n", valid_collisions);
    
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
	if(w_rank>w_size)
	{
		MPI_Finalize();
	}
	long *succesful_col;
	if(w_rank==0){

		 succesful_col= malloc(sizeof(long)*w_size); 
		
		
	}
	
	//initialization of coordinations we are going to examine= total number	
	long offset= w_rank * (fsize(file_name)/w_size);
	long chunk_size= fsize(file_name)/w_size;
    FILE *file = fopen(file_name,"rb");
	fseek(file,offset, SEEK_SET);
	 long counter=0;
	 long bytes_read;
	for( bytes_read=0; bytes_read<chunk_size; bytes_read+=(3*sizeof(float))){
		 if(process_coords(file)==SUCCESS){
			counter++;
		 }		
	}
	
	MPI_Gather(&counter,1,MPI_LONG, succesful_col,1, MPI_LONG,0,MPI_COMM_WORLD);
	if(w_rank==0){
		long final_count=0;
		int i;
		for( i=0;i<w_size;i++){
			final_count+=succesful_col[i];
		}
		printf("number of collisions: %ld",final_count); 
	}



	MPI_Finalize();
	return SUCCESS;
}

/*
 * -----------------------------------------------------------------------------
 * argv[1] : Maximum number of collisions to examine (-1 == all available colls)
 * argv[2] : Maximum running time in nanoseconds (-1 == until end of file)
 * argv[3] : Name of the binary data file
 * argv[4] : How many threads OMP is going to use (-1 == all available threads)
 * argv[5] : How many procs OMPI is going to use (-1 = all available procs)
 *
 * NOTE: It is not possible to Initialize the MPI after it has been Finalized.
 * If you want to test an MPI section in comparison with a non-MPI one, just
 * uncomment the part in the main, where we run the non-MPI code.
 * If you want to test both MPI parts, you have to run the program two times.
 * -----------------------------------------------------------------------------
 */
int main(int argc, char *argv[]) {
	if(argc == 6) {
		if(check_arg_validity(argv) != SUCCESS)
			return(FAILURE);
		struct timespec start, end;
		
		
		// Run the linear (non-MPI)
		clock_gettime(CLOCK_MONOTONIC, &start);
		linear_estimation(atoi(argv[1]), atoi(argv[2]), argv[3]);
		clock_gettime(CLOCK_MONOTONIC, &end);
		printf("Non-MPI Linear motion estimation total ");
		calculate_difference(start, end, 1);

		printf("\n");
		/*
		// Run the linear (MPI)
		clock_gettime(CLOCK_MONOTONIC, &start);
		ompi_linear_estimation(atoi(argv[1]), atoi(argv[2]), argv[3]);
		clock_gettime(CLOCK_MONOTONIC, &end);
		printf("MPI Linear motion estimation total ");
		calculate_difference(start, end, 1);
		
		printf("\n");
		*/
		// Run the parallel (Non-MPI)
		clock_gettime(CLOCK_MONOTONIC, &start);
		parallel_estimation(atoi(argv[1]), atoi(argv[2]), argv[3], atoi(argv[4]));
		clock_gettime(CLOCK_MONOTONIC, &end);
		printf("Non-MPI Parallel motion estimation total ");
		calculate_difference(start, end, 1);
		/*
		printf("\n");
		
		// Run the parallel (MPI)
		clock_gettime(CLOCK_MONOTONIC, &start);
		ompi_parallel_estimation(atoi(argv[1]), atoi(argv[2]), argv[3], atoi(argv[4]));
		clock_gettime(CLOCK_MONOTONIC, &end);
		printf("MPI Parallel motion estimation total ");
		calculate_difference(start, end, 1);
		*/
		
		return(SUCCESS);
	} else {
		printf("Invalid number of arguements. Exiting.\n");
		return(FAILURE);
	}
}

