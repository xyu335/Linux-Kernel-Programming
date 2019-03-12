# Real-time Rate Monotonic Scheduler
Progress:  
- Initialization and Exit  
- Proc file entry  
- Data structure and slab allocator  
- Write callback function, Read call back function   
- Register and deregister  
- Timer callback 

Todo:   
- Dispatch kernel thread(save current thread, pick a highest priority thread to context switch)   
- Yield entry to giveup current period, call the dispatch kernel thread  
- // think over how to decide if a kernel is still in its period...
- Admission control function  
- Test case 
