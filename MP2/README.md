# Real-time Rate Monotonic Scheduler(LKM)
How to run:  
1. ***"make"*** in current source folder, you will get mp2.ko  
2. ***"sudo insmod \*.ko"*** to install the module in your system  
3. ***"./test.sh"*** to run test shell script or you can customize the test case with userapp  
4. ***"./userapp"*** to check the parameters for the test application   

Design Decisions:  
Mostly in the design and implementation, I followed the instruction steps. I started with the
 proc file entry to achieve basic r/w functionalities. Then I implemented structure that contains
 the task\_struct and the slab allocator to handle the frequent allocation of fixed sized memory. 
 Parser in the write callback enables the dispatching requests to different module entries and 
three important function including Register\_entry, Yield\_entry and Deregister\_entry handle each of 
them. Each time user program tries to yield current time period, it triggers the schduler of the 
module, and I implement the priority comparison in the dispatch kernel thread to make it. Lastly, 
I add an admission control with two global variable to avoid any float ops to make it more efficient 
dealing with multiple works. The spin\_lock is used to protect any iteration or operation towards 
the linked list data structure in the module which is used for keeping track of all registered task 
and information stored inside of the special struct container of them.  

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
