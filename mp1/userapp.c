#include "userapp.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h> // both for getpid


int main(int argc, char* argv[])
{

	// get pid
	int mypid = getpid();
	char * filename = "/proc/mp1/status";

	char cmd[256];
	sprintf(cmd, "echo %d > %s", mypid, filename);
	int ret = system(cmd);
	printf("system call return: %d\n", ret);
	
	/* read from the proc */
	char buf[2048];
	
	FILE *fp = fopen(filename, "r");

	int readval = fread(buf, 2048, 1, fp);
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
