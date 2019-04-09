#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h> 

int main()
{
	/*
	char * file = "node";
	int fd = open(file, O_RDONLY|O_SYNC);
	
	printf("open file, get fd: %d \n", fd);
	
	if (fd > 0) close(fd);
	*/
	
	test();
	return 0;
}


void test()
{
	unsigned long var = 10L;
	char * tmp = &var;	

	unsigned long * ptr = (unsigned long *) tmp + 4;
	
	printf("var pointer:  %p,  ptr pointer: %p \n", tmp, ptr);
	
	printf("varibale: +1 %p, -4 %p \n", ptr + 1, ptr - 4);

	
	printf("unsigned long size: %d \n", sizeof(unsigned long));
}
