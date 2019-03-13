# Real-time Rate Monotonic Scheduler
Progress:  
- Initialization and Exit  
- Proc file entry  
- Data structure and slab allocator  
- Write callback function, Read call back function   
- Register and deregister  
- Timer callback 
- Dispatch kernel thread(save current thread, pick a highest priority thread to context switch)   
- Yield entry to giveup current period, call the dispatch kernel thread  

Todo:   
- Admission control function
- Parameterized test program with input of ***priod_duration period_times computation_time*** 
