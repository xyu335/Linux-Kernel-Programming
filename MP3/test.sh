#!/bin/bash

# test thrashing and locality
CMD1="nice ./work 1024 R 50000 & nice ./work 1024 R 10000 & "

# test thrashing and locality
CMD2="nice ./work 1024 R 50000 & nice ./work 1024 L 10000 & "

# test multiprogramming 
# nice ./work 200 R 10000  & nice ./work 200 R 10000 

# multiprogramming 
echo "argument's number: $#"

if  [$# -ne 2] && [$# -ne 3] 
then
	echo "usage: ./exe expID <Nstart> <Nend> <memory=200MB> <locality=Random> <uaccess=10000>"
	echo "or ./exe expID <case study ID>"
	echo "expID: \t 2 - N multiprocessing expriment \n \t 1 - case study"
	exit
fi

if [ $# -eq 2 ]
then 
	csID=$2
	if [ $csID -eq 1 ]
	then
		$CMD1	 
	else 
		$CMD2
	fi
	exit
fi
N_start=$2
N_end=$3
memory=200
locality='R'
naccess=10000

# index for single work, global
index=0
CMD="nice ./work $memory $locality $naccess > /dev/null &"
SAVE_FILE_PREFIX="data/profile_N"
echo $CMD



# iterate throught different N 
for ((N=$N_start;N<$N_end;N++)); do
	echo "START: N $N work is running in parallel"
	ITR=0
	INCREMENT=1
	while [ $ITR -lt $N ]
	do	
		ITR=` expr $ITR + $INCREMENT `
	# iterate the cmd
		$CMD
		pids[$index]=$!
	done

	echo "WAIT: N==$N subprocess to finish"
	# wait for all pids
	
	for pid in ${pids[*]}; do
    	wait $pid
	done

	echo "WRITE TO FILE"
	#write to file
	CMD_MONITOR="./monitor > $SAVE_FILE_PREFIX$N.data "

done


exit

echo "Parallel processes have started";
	PID_LIST+=" $pid"
# trap 
trap "kill $PID_LIST" SIGINT
echo "Parallel processes have started";

wait $PID_LIST

echo
echo "All processes have completed";

