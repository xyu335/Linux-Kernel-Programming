#!/bin/bash

# test thrashing and locality
CMD1="nice ./work 1024 R 50000 & nice ./work 1024 R 10000 & "

# test thrashing and locality
CMD2="nice ./work 1024 R 50000 & nice ./work 1024 L 10000 & "

# test multiprogramming 
# nice ./work 200 R 10000  & nice ./work 200 R 10000 

# multiprogramming 
echo "argument's number: $#"

if  [ $# -ne 2 ] && [ $# -ne 3 ] 
then
	echo "usage: ./exe expID <Nstart> <Nend> <memory=200MB> <locality=Random> <uaccess=10000>"
	echo "or ./exe expID <case study ID>"
	echo "expID: \t 2 - N multiprocessing expriment \n \t 1 - case study"
	exit
fi

if [ $# -eq 2 ]
then
	echo "2 arguments, experiment 1 starts" 
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
((N_end=$N_end+1))
memory=200
locality='R'
naccess=10000
CWD=$PWD
#TARGET="/home/xy21/CS423/MP3/"
echo "parameters: N_start $N_start, N_end $N_end, current dir: $CWD"

# index for single work, global
index=0
CMD="nice ./work $memory $locality $naccess > /dev/null" # not sure if the /dev/null will cause inorder of cmd
# CMD="nice ./work $memory $locality $naccess"
SAVE_FILE_PREFIX="./data/profile_N"
# $CMD


# iterate throught different N 
for ((N=$N_start;N<N_end;N++)); do
	echo "START: N $N work is running in parallel"
	ITR=0
	INCREMENT=1
	while [ $ITR -lt $N ]
	do	
		ITR=` expr $ITR + $INCREMENT `
	# iterate the cmd
		$CMD & 
		echo "N==$N INR==$ITR"
		pids[$index]=$!
	done

	echo "WAIT: N==$N subprocess to finish"
	# wait for all pids
	
	for pid in ${pids[*]}; do
    	wait $pid
	done

	echo "WRITE TO FILE FOR N $N"
	#write to file
	FILE_NAME="$SAVE_FILE_PREFIX$N.data"
	CMD_MONITOR="./monitor"
	echo "save command: $CMD_MONITOR > $FILE_NAME"
	$CMD_MONITOR > $FILE_NAME
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

