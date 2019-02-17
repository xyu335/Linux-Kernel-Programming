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

	FILE * fp = fopen(filename, "w+");
	if (fp == NULL) 
	{
		perror("file open failed");
		return 1; // file open error
	}

	/* register it self */
	fprintf(fp, "%d", mypid); // null byte add? 
	
	/* read from the proc */
	char buf[2048];
	
	int readval = fread(buf, 2048, 1, fp);
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
