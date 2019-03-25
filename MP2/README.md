# Real-time Rate Monotonic Scheduler(LKM)
How to run:  
1. "make" in current source folder, you will get mp2.ko  
2. "sudo insmod \*.ko" to install the module in your system  
3. "./test.sh" to run test shell script or you can customize the test case with userapp  
4. "./userapp" to check the parameters for the test application   


Progress:  
- Initialization and Exit  
- Proc file entry  
- Data structure and slab allocator  
- Write callback function, Read call back function   
- Register and deregister  
- Timer callback 
- Dispatch kernel thread(save current thread, pick a highest priority thread to context switch)   
- Yield entry to giveup current period, call the dispatch kernel thread  
- Admission control function
- Parameterized test program with input of ***priod_duration period_times computation_time*** 
