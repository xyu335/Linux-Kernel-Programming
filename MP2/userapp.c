#include "userapp.h"
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h> // used for getpid

#define PROC_ENTRY "/proc/mp2/status"
#define FACTORIAL_N INT_MAX

struct timeval tv1, tv2; 
pid_t curr_pid;
FILE * fp;
int period, computation;

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
	fprintf(fp, "Y,%d", curr_pid);
	fflush(fp);
	return;
}

/* deregister to the kernel */
static void deregister(void)
{
	fprintf(fp, "D,%d", curr_pid);
	fflush(fp);
	printf("deregister finished... \n");
	return;
}

/* get the current pid, and pass the period, computation to the kernel module */
static void regist(void)
{
	fp = fopen(PROC_ENTRY, "r+");	
	fprintf(fp, "R,%d,%d,%d", curr_pid, period, computation);
	fflush(fp);
	printf("register finished...\n");
	return;	
}

static void loop(int times)
{
	printf("entering loop func...\n");
	int time = 0;
	while (time < times)
	{
		yield();
		gettimeofday(&tv1, NULL); // vsys_call, not a systm_call but the data on that page is maintained by the kernel
		printf("%dth loop start at %f", time+1, tv1.tv_sec);
		factorial();
		gettimeofday(&tv1, NULL);
		printf("%dth loop end at %f", time+1, tv1.tv_sec);// second precision
		++time;

	}
	return;
}

/*
	in the main function, we arrange periodic task by invoking register()
	in each round of execution, we yield once we return from the task
	when the run times defined by the input is reached, we send request by calling deregister()
*/
int main(int argc, char ** argv)
{
	gettimeofday(&tv1, NULL);
	gettimeofday(&tv2, NULL);
	printf("time started from: %f and %f\n", tv1.tv_sec, tv2.tv_sec);
	period = atoi(argv[1]);
	computation = atoi(argv[2]);	
	int times = atoi(argv[3]);
	curr_pid = getpid();
	printf("input params: pid %d, period %d, computation %d times %d \n", curr_pid, period, computation, times);
	regist();
	loop(times);
	deregister();
	printf("finishing deregister...");
	fclose(fp);
	return 0;
}
