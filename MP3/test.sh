#!/bin/bash

# test thrashing and locality
#  nice ./work 1024 R 50000 & nice ./work 1024 R 10000  

# test thrashing and locality
# nice ./work 1024 R 50000 & nice ./work 1024 L 10000 & 

# test multiprogramming 
nice ./work 200 R 10000  & nice ./work 200 R 10000 
