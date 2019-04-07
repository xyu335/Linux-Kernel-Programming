EXTRA_CFLAGS +=
APP_EXTRA_FLAGS:= -O2 -ansi -pedantic
KERNEL_SRC:= /lib/modules/$(shell uname -r)/build
SUBDIR= $(PWD)
GCC:=gcc
RM:=rm

.PHONY : clean

all: clean modules app1

# obj-m:= xy21_MP3.o
# xy21_MP2-y:=mp3.o
obj-m:=mp3.o
# compiled with mp3 

modules:
	$(MAKE) -C $(KERNEL_SRC) M=$(SUBDIR) modules

#app: userapp.c userapp.h
#	$(GCC) -o userapp userapp.c
# app for work and monitor program

app1: monitor.c
	$(GCC) -o monitor monitor.c

app2: work.c
	$(GCC) -o work work.c

clean:
	$(RM) -f userapp *~ *.o *.mod.c Module.symvers modules.order
