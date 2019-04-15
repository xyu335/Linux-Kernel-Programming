# Virtual Memory Page Fault Profiler

## How to run
- ***"make"*** to compile the module in current directory  
- ***"sudo insmod your_module.ko"*** to install the module  
- ***"cat /proc/devices"*** and find the corresponding major number of your character device  
- ***"sudo mknod node c major_number_got 0"*** to create concrete node file in current directory  
- ***"sudo chmod 777 node"*** to add r/w/x access control privlege to all groups  
- ***"./test.sh 1"*** to run case study one and get data files under MP3/data/ directory  
- ***"./test.sh 2 N_start N_end"*** to run case study two with target N_start and N_end, you will get profile_data_N(N_start~N_end).data in the MP3/data/ directory
- You can also create your own test by run ./work with target arguments and run ./monitor to collect results when program ends.  

## Design progress
- Design a lgihtweight tool that can profile page fault rate  
- Learn the kernel-level API like character devices, vmalloc, mmap 
- Test with kernel-level profiler by using a user-level benchmark program by performaing memory allocation/deallocation/local access/ random access operations  
- Analyze, plot and document the profiled data as a function of the workload charateristics


