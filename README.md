# CS 423 Operating System

- MP1 system tool - cpu usage helper
Implement a system tool as kernel module to achieve cpu usage for certain PID. User program can register itself by write to the corresponding proc_file created by this module. And cpu related information can be retrieval by reading from the same proc_file.   
In detail, I use linux kernel list_head struct to maintain a linked list of registered processes, and by utilizing kernel timer library, the timer interrup handler is able to load the timer_callback function to generate a work to update the cpu_usage for every 5 seconds.   
The work is pushed onto the kernel thread workqueue every 5 seconds, and each time it deals with modification of the linked list, lock will be required beforehead, so that all the information the process retrieved by calling read to the proc_file is guaranteed to be acquired at the sametime.  
Using linux timer api, list api.


-MP2 system tool - real-time work scheduler
Implement a system tool as kernel module to achieve schedule periodical tasks for multiple application. 

Using linux scheduler api, timer api, list api. 
