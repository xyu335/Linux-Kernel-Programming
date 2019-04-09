#include <stdio.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <stdio.h> // f related 
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <time.h>
#include <string.h> 

#define NPAGES (128)   // The size of profiler buffer (Unit: memory page)
#define BUFD_MAX 48000 // The max number of profiled samples stored in the profiler buffer
// SAMPLES => 

static int buf_fd = -1;
static int buf_len;
static char * output_file; 

// This function opens a character device (which is pointed by a file named as fname) and performs the mmap() operation. If the operations are successful, the base address of memory mapped buffer is returned. Otherwise, a NULL pointer is returned.
void *buf_init(char *fname)
{
  unsigned int *kadr;

  if(buf_fd == -1){
    buf_len = NPAGES * getpagesize();
  
  // SYNC flag for open
  printf("try to open the char dev - node file..\n");
  if ((buf_fd=open(fname, O_RDWR|O_SYNC))<0){
  // if ((buf_fd=open(fname, O_RDONLY|O_SYNC))<0){
        printf("file open error. %s\n", fname);
        return NULL;
    }
  }

  //void *mmap(void *addr, size_t lengthint " prot ", int " flags , int fd, off_t offset); 
  kadr = mmap(0, buf_len, PROT_READ|PROT_WRITE, MAP_SHARED, buf_fd, 0);
  if (kadr == MAP_FAILED){
      close(buf_fd);
      printf("buf file open error.\n");
      return NULL;
  }

  return kadr;
}

// This function closes the opened character device file.
void buf_exit()
{
  if(buf_fd != -1){
    close(buf_fd);
    buf_fd = -1;
  }
}

int main(int argc, char* argv[])
{
  long *buf;
  int index = 0;
  int i;

  char buff[2048] = {0};
  char * str = "output-";
  strncpy(buff, str, strlen(str));
  time_t tm;
  time(&tm);
  
  char * timeinfo = ctime(&tm);
  
  char * time = strtok(timeinfo, " ");
  while (time) 
  {
	// printf("%s \n", time);
	if (strlen(time) > 6) break;
	time = strtok(NULL, " ");
  }

  output_file = strncat(buff,time, 40);
  printf("output_filename is %s. Check that after the program finished.\n", output_file);

  // Open the char device and mmap(), device file name = node 
  buf = buf_init("node");
  if(!buf)
    return -1;
  

  // read four chunks of long data for all the samples
  // so there are exactly maximal 12000 integral samples 
	// default value should be -1, to end the iteration through buffer 
	// index = the first element that is the first element that is not -1. 

  // Read and print profiled data
  for(index=0; index<BUFD_MAX; index++)
    if(buf[index] != -1) break;

  i = 0;
  while(buf[index] != -1){
    printf("%ld ", buf[index]);
    buf[index++] = -1;
    if(index >= BUFD_MAX)
      index = 0;

    printf("%ld ", buf[index]);
    buf[index++] = -1;
    if(index >= BUFD_MAX)
      index = 0;

    printf("%ld ", buf[index]);
    buf[index++] = -1;
    if(index >= BUFD_MAX)
      index = 0;

    printf("%ld\n", buf[index]);
    buf[index++] = -1;
    if(index >= BUFD_MAX)
      index = 0;
    i++;
  }
  printf("read %d profiled data\n", i);

  // Close the char device
  buf_exit();
}

