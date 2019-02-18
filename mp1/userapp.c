#include "userapp.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h> // both for getpid
#include <sys/time.h>

char * filename;

int main_1(int argc, char * argv[])
{
	// test
	// filename = "tmpfile";
	filename = argv[1];
	char buf[2048];
	FILE * fp = fopen(filename, "r");
	int readBytes = fread(buf, 2048, 1, fp);
	printf("string got: %s \n", buf);
	
}

int main(int argc, char* argv[])
{

	// get pid
	int mypid = getpid();
	filename = "/proc/mp1/status";

	char cmd[256];
	sprintf(cmd, "echo %d > %s", mypid, filename);
	int ret = system(cmd);
	printf("system call:%s\n", cmd);
	printf("system call return: %d\n", ret);
	
	int times = 5000;
	double duration = 15.0;
	//FILE * fp = fopen(filename, 'r');
	//kernel_cat();
	workfunc1(duration);
	kernel_cat();
	
	//fclose(fp);
	return 0;
}

void workfunc1(double dur)
{

	struct timeval t1,t2;
	gettimeofday(&t1, NULL);
	while (1)
	{
		gettimeofday(&t2,NULL);
		if ((double)(t2.tv_sec - t1.tv_sec) > dur) break;
	}
}

void workfunc(int times)
{ 
	while (times > 0)
	{
		/* fibonacci */
		int i = 0;
		int j = 1;
		int result = 0;	
		while (result > 0)
		{
			result = i + j;
			i = j;
			j = result;	
		}
		times--;
	}
}

void kernel_cat()
{
	printf("start read the proc/mp1/status\n");
	execlp("cat", "cat", "/proc/mp1/status", (char*)NULL);
}

void kernel_syscall(FILE * fp)
{
	/* read from the proc */
	char buf[2048];
	
	printf("Start reading from proc\n");
	int readval = fread(buf, 2048, 1, fp);
	// int readval = fgets(buf, 2048, fp);
	//TODO read fails, the stdout should be return to the buf. 
	if (readval < 1) 
	{
		printf("no char read from the proc\n");
		return 1;
	}
	buf[readval] = '\x00';
	printf("%d bytes read, the result is shown as below: \n%s ", readval, buf);
	
	return;	
}


