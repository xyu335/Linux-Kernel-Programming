#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h> 

int main()
{
	char * file = "node";
	int fd = open(file, O_RDONLY|O_SYNC);
	
	printf("open file, get fd: %d \n", fd);
	
	if (fd > 0) close(fd);
	return 0;
}
