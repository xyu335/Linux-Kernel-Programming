#include "userapp.h"
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <sys/types.h>
#include <unistd.h> // used for getpid

// entry name
#define PROC_ENTRY "/proc/mp2/status"
// factorial parameter
#define FACTORIAL_N 20000000

/* 
	in this program, we are going to test the functionality of the RTS
	we start by register one program at the same time. 

	we input with {period, computation, times for the program. } 

	then we will try to register several at the same time. 
*/

// timestamp for 
struct timeval tv1, tv2; 
pid_t curr_pid;
FILE * fp;
unsigned int period, computation, times;

/* basic computation function */
static void factorial(void)
{
	int out = 0;
	int i = 0;
	for (; i < FACTORIAL_N; ++i)
	{
		out *= i++;
	}
	return;
}

/* write call to kernel */
static void yield(void)
{
	fprintf(fp, "Y,%d\n", curr_pid);
	fflush(fp);
	return;
}

/* deregister to the kernel */
static void deregister(void)
{
	printf("pid:%d De-register entered... \n", curr_pid);
	fprintf(fp, "D,%d\n", curr_pid);
	fflush(fp);
	return;
}

/* get the current pid, and pass the period, computation to the kernel module */
static void regist(void)
{
	printf("pid:%d Register entered..\n", curr_pid);
	fp = fopen(PROC_ENTRY, "r+");	
	fprintf(fp, "R,%d,%d,%d\n", curr_pid, period, computation);
	fflush(fp);
	return;	
}

/* read from kernel */
static int check(void)
{
	FILE * rfp = fopen(PROC_ENTRY, "r");
	char line[256] = {};
	int pid = 0;
	int size = 0;
	fgets(line, 256, rfp);
	sscanf(line, "%d", &size);
	printf("pid:%d Check: there are %d task registered...\n", curr_pid, size);
	while (fgets(line, 256, rfp))
	{
		sscanf(line, "%d", &pid);
		if (pid == curr_pid) 
		{
			fclose(rfp);
			return 0;
		}
	}
	fclose(rfp);
	return 1;
}

/* loop for set_times */
static void loop(int set_times, int computation_times)
{
	printf("pid: %d Entering loop\n", curr_pid);
	int time = 0;
	yield();
	// after register, yield to get timer activated
	while (time < set_times)
	{
		gettimeofday(&tv1, NULL); // vsys_call, not a systm_call but the data on that page is maintained by the kernel
		printf("pid: %d, %dth task start at %ld ms\n", curr_pid, time+1, tv1.tv_usec / 1000);
		int i = 0;
		for (; i < computation_times; ++i) factorial();
		gettimeofday(&tv1, NULL);
		printf("pid: %d, %dth task done at %ld ms\n", curr_pid, time+1, tv1.tv_usec / 1000);// second precision
		++time;
		if (time >= set_times) return;
		yield();
	}
	return;
}

/* helper function to get the time unit of one round of factorial computation */
static double helper(void)
{
	clock_t clk1, clk2;
	clk1 = clock();
	factorial();
	clk2 = clock();
	double cost = (double) (clk2-clk1) / CLOCKS_PER_SEC;
	printf("Single computation task time cost on cpu: %fsec\n", cost);
	return cost;
}

/*
	in the main function, we arrange periodic task by invoking register()
	in each round of execution, we yield once we return from the task
	when the run times defined by the input is reached, we send request by calling deregister()
*/
int main(int argc, char ** argv)
{
	double comp = helper();
	unsigned int time_unit = comp * 1000;
	if (argc != 4)
	{
		printf("Please enter parameters in format of: ./userapp <PEPRIOD> <COMPUTATION_TIMES> <TIMES>, computation_cost for each round will be %d * <COMPUTATION_TIME> ms\n",time_unit);
		return 1;
	}
	period = atoi(argv[1]); // msec unit
	int computation_times = atoi(argv[2]);
	computation = comp * computation_times * 1000; // msecs
	times = atoi(argv[3]); // 1 unit
	curr_pid = getpid();
	printf("pid:%d Input params: period %d, computation %d times %d \n", curr_pid, period, computation, times);
	
	regist();
	if (check())
	{
		printf("pid:%d This pid failed registering in the module...\n", curr_pid);
		fclose(fp);
		return 1;
	}
	loop(times, computation_times);
	deregister();
	printf("pid:%d Finished userapp\n", curr_pid);
	fclose(fp);
	return 0;
}
