## MP1 - cpu usage moniter LKM
 
***How to run***
1. change current directory to MP1/
2. build from source code with "make"
3. install LKM with "sudo insmod xy21\_MP1.ko"
4. run demo user space application by "./userapp & ./userapp &" in your terminal

***To register your program and collect information using the LKM***

1. fprintf to the /proc/mp1/status file
2. fread from the /proc/mp1/status file
