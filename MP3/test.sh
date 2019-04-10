#!/bin/bash

# test thrashing and locality
#  nice ./work 1024 R 50000 & nice ./work 1024 R 10000  

# test thrashing and locality
# nice ./work 1024 R 50000 & nice ./work 1024 L 10000 & 

# test multiprogramming 
# nice ./work 200 R 10000  & nice ./work 200 R 10000 

# multiprogramming 
echo "argument's number: $#"
args=4
if [ $# -ne $args ]
then
echo "usage: ./exe <N> <memory> <locality> <uaccess>"
exit
fi

N=$1
memory=$2
locality=$3
naccess=$4

echo "this command will be run for $ROUND times in parallel"
# echo "$@" # pass the argument in a block

CMD="nice ./work $memory $locality $naccess"

echo $CMD


ITR=0
INCREMENT=1

while [ $ITR -lt $N ]
do	
	ITR=` expr $ITR + $INCREMENT `
	# iterate the cmd
	$CMD & pid=$! & 
	PID_LIST+=" $pid" 	
done

echo "Parallel processes have started";
exit


# trap 
trap "kill $PID_LIST" SIGINT
echo "Parallel processes have started";

wait $PID_LIST

echo
echo "All processes have completed";

