# CS 423 Operating System

***MP1 Cpu usage retrieve***
- Implement a system tool as a kernel module to achieve cpu usage retrieving for certain process, allowing user to retrieve any cpu runtime of a process by registering the process ID.
- Using linux proc file, timer, list, workqueue, spin lock api.  

***MP2 Real-time work scheduler***
- Implement a system tool as kernel module to achieve the scheduling for periodical tasks for multiple application, providing rate monotonic scheduling policy and admission control to ensure the correstness and efficiency of scheduler. 
- Using linux scheduler, timer, list, slab, spin lock api.  

***MP3 Virtual memory page-fault profiler***
- Implement a system tool as kernel moduel to work as a page-fault profiler, allowing user program to get the statistics about page-faults and cpu execution correlating to its timestamp of its lifespan. 
- Write work and monitor program to test the correctness of the module and provide interface to user. 
- Write scripts to automate testing process to collect enough data to analyze characteristics of program.  
- Using linux character device, vmalloc, slab, spin lock, mmap api.  

***MP4 Linux Security Module - Mandatory Access Control***
- Implement a Linux Security Module(LSM) to provide specific mandatory access control, achieving directory and file access control over READ/WRITE/APPEND/ACCESS/EXEC for target and non-target program and files.  
- Write scripts to get Least Privilege Policy for passwd program and succeed in providing least security priviledges.  
- Using Linux security, dacache, fs, xattr api.
