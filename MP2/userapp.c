#include "userapp.h"
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <sys/types.h>
#include <unistd.h> // used for getpid

#define PROC_ENTRY "/proc/mp2/status"
//#define FACTORIAL_N INT_MAX
#define FACTORIAL_N 20000000
// INTMAX = 213748364847
//#define DEBUG 1

/* 
	in this program, we are going to test the functionality of the RTS
	we start by register one program at the same time. 

	we input with {period, computation, times for the program. } 

	then we will try to register several at the same time. 
*/

struct timeval tv1, tv2; 
pid_t curr_pid;
FILE * fp;
int period, computation, times;

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

static void yield(void)
{
	fprintf(fp, "Y,%d\n", curr_pid);
	fflush(fp);
	return;
}

/* deregister to the kernel */
static void deregister(void)
{
	fprintf(fp, "D,%d\n", curr_pid);
	fflush(fp);
	printf("deregister finished... \n");
	return;
}

/* get the current pid, and pass the period, computation to the kernel module */
static void regist(void)
{
	fp = fopen(PROC_ENTRY, "r+");	
	fprintf(fp, "R,%d,%d,%d\n", curr_pid, period, computation);
	fflush(fp);

	printf("register finished...\n");
	return;	
}

static void loop(int set_times)
{
	printf("entering loop func...\n");
	int time = 0;
	// clock_t clk1 = clock();
	
	yield();
	// after register, yield to get timer activated
	while (time < set_times)
	{
		gettimeofday(&tv1, NULL); // vsys_call, not a systm_call but the data on that page is maintained by the kernel
		printf("%dth loop wakeup at %lu\n", time+1, tv1.tv_sec * 1000);
		
		factorial();
		gettimeofday(&tv1, NULL);
		printf("%dth loop ready to sleep at %lu\n", time+1, tv1.tv_sec * 1000);// second precision
		++time;
		if (time >= set_times) return;
		// yield => work => yield
		yield();
	}
	return;
}

static double helper()
{
	clock_t clk1, clk2;
	clk1 = clock();
	printf("start of one round: %fsec\n", (double)clk1 / CLOCKS_PER_SEC);
	factorial();
	clk2 = clock();
	double cost = (double) (clk2-clk1) / CLOCKS_PER_SEC;
	printf("end of one round: %fsec\n", cost);
	return cost;
}

/*
	in the main function, we arrange periodic task by invoking register()
	in each round of execution, we yield once we return from the task
	when the run times defined by the input is reached, we send request by calling deregister()
*/
#ifndef DEBUG
int main(int argc, char ** argv)
{
	double comp = helper();
	unsigned int time_unit = comp * 1000;
	if (argc != 4)
	{
		printf("please enter parameters in format of: ./userapp <PEPRIOD> <COMPUTATION_TIMES> <TIMES>, computation_cost for each round will be %d * <COMPUTATION_TIME> * 1000 ms\n",time_unit);
		return 1;
	}
	period = atoi(argv[1]); // msec unit
	int computation_times = atoi(argv[2]);	
	unsigned int computation = comp * computation_times * 1000; // msecs
	times = atoi(argv[3]); // 1 unit
	curr_pid = getpid();
	printf("input params: pid %d, period %d, computation %d times %d \n", curr_pid, period, computation, times);
	regist();
	loop(times);
	deregister();
	printf("finishing deregister...\n");
	fclose(fp);
	return 0;
}
#else
int main(int argc, char **argv)
{
	helper();
	return 0;
}
#endif

