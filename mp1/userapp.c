#include "userapp.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h> // both for getpid


char * filename;

int main(int argc, char * argv[])
{
	// test
	filename = "tmpfile";
	char buf[2048];
	FILE * fp = fopen(filename, "r");
	int readBytes = fread(buf, 2048, fp);
	printf("string got: %s \n", buf);
	
}

int main_1(int argc, char* argv[])
{

	// get pid
	int mypid = getpid();
	filename = "/proc/mp1/status";

	char cmd[256];
	sprintf(cmd, "echo %d > %s", mypid, filename);
	int ret = system(cmd);
	printf("system call return: %d\n", ret);
	
	/* read from the proc */
	char buf[2048];
	
	FILE *fp = fopen(filename, "r");

	int readval = fgets(buf, 2048, fp);
	//TODO read fails, the stdout should be return to the buf. 
	if (readval < 1) 
	{
		printf("no char read from the proc\n");
		return 1;
	}
	buf[readval] = '\x00';
	printf("the result is shown as below: \n%s ", buf);
	
	fclose(fp);
	return 0;
}


void workfunc()
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
}

void kernel_syscall()
{
	/* get the infomation from the kernel*/
	FILE * fp = fopen(filename, "r");
	
	// system call the kernel :wq
	char buf[2048];
	// proc file reading 
	int read_bytes = fgets(buf, sizeof(buf), fp);
	
}


