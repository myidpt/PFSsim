#!/bin/bash

#Launch the different tests with IOR with different parameters
ID=0
SID=1
PATH_TO_RESULT=Results/
MPI_EXEC="mpirun"
MPI_OPTIONS_D="-machinefile mpd.hosts "
IOR_EXEC="ior"
IOR_OPTIONS_D="-Z -X 34 -k -a POSIX -i 1 -o /mnt/pvfs2/testFile "
PREFIX="result_test"
SUFFIX=".txt"
ITERATION=5
NUM_SERVER=7

flushCache(){
	for dest in $(head -n $NUM_SERVER server.hosts);
	do
		ssh ${dest} sync
		sleep 1
		ssh ${dest} "echo 3 | tee /proc/sys/vm/drop_caches" >/dev/null 2>&1
	done
}

launchSimulation(){
	
	if [ $WORD == "-I" ];
	then
		ID=0
	elif [ $WORD != "-a" ];
	then
		ID=$WORD
	else
		ID=$(($ID+1))
	fi
	SID=1
	#In order to iterate the test
	if [ "$#" -eq 4 ];
	then
		ITE=$4
	else
		ITE=1
	fi
	
	echo ""
	echo $1
	echo ""
	MPI_OPTIONS=${MPI_OPTIONS_D}$2
	IOR_OPTIONS=${IOR_OPTIONS_D}$3
	echo "Initialization of data on file system start"
	${MPI_EXEC} ${MPI_OPTIONS} ${IOR_EXEC} ${IOR_OPTIONS} -w >/dev/null 2>&1
	echo "Initialization of data on file system end"
	VERBOSE="-v -v -v"
	for i in $(seq 1 $ITE);
	do
		echo "##Benchmark $ID launched: Iteration = $SID##"
		flushCache
		${MPI_EXEC} ${MPI_OPTIONS} ${IOR_EXEC} ${IOR_OPTIONS} ${VERBOSE} > ${PATH_TO_RESULT}${PREFIX}${ID}-${SID}${SUFFIX}
		echo "##Benchmark $ID finished: Iteration = $SID##"
		SID=$(($SID+1))
		VERBOSE="-v -v"
	done
	ssh $(head -n 1 mpd.hosts) rm /mnt/pvfs2/testFile*
}

interactivemode(){
	correct=""
	while [[ $correct != "y" && $correct != "Y" ]];
		do
		correct="y"
		echo "Give the MPICH options (see man mpiexec for valid options):[ ${MPI_OPTIONS_D}]"
		read tmp
		if [ ! -z $tmp ];
		then
			MPI_OPTIONS_D=$tmp
		fi
		echo "Give the IOR options (see man ior for valid options):[ ${IOR_OPTIONS_D}]"
		read tmp
		if [ ! -z $tmp ];
		then
			IOR_OPTIONS_D=$tmp
		fi
		echo "Give the number of iteration:[${ITERATION}]"
		read tmp
		if [ ! -z $tmp ];
		then
			ITERATION=$tmp
		fi
		echo "Give the name of the file wich takes the results: [ ${PREFIX} ]"
		read tmp
		if [ ! -z $tmp ];
		then
			PREFIX=$tmp
		fi
		echo "MPICH Options : "${MPI_OPTIONS_D}
		echo "IOR Options : "${IOR_OPTIONS_D}
		echo "Number of iteration: "${ITERATION}
		echo "Name of result file: "${PREFIX}
		echo "Is it ok?[Y/n]"
		read tmp
		if [ ! -z $tmp ];
		then
			correct=$tmp
		fi
	done
	echo "${MPI_EXEC} ${MPI_OPTIONS} ${IOR_EXEC} ${IOR_OPTIONS}"
	launchSimulation "###Test with custom options###" "" "" $ITERATION
}

for WORD in "$@";
do
	case $WORD in
		--help | -h)
			echo "Options:"
			echo "-a : # Execute all tests"
			echo "-I : # Interactive Mode, Custom your test"
			echo "1 : # Default parameters : client = 32, server = 6, file-size = 2GB, stripe-size=64K, req-size=1MB, read=1, Odirect=0, File-per-proc"
			echo "2 : # With client = 64"
			echo "3 : # With ODIRECT = 1"
			echo "4 : # With Write (read=0)"
			echo "5 : # With req-size = 32 KB"
			echo "6 : # With req-size = 128 KB"
			echo "7 : # With req-size = 256 KB"
			echo "8 : # With req-size = 8 MB"
			echo "9 : # With req-size = 64 MB"
			echo "10 : # Uniq Shared File"
			echo "11 : # With file-size = 1 GB"
			echo "12 : # With file-size = 512 MB"
			echo "13 : # With Write and Uniq Shared File"
			exit ;;
		1)	
			#With default parameters client = 32, server=6, file-size=2GB 
			#stripe-size=64K, req-size=1M, read=1
			launchSimulation "###Test with default paramaters###" "-n 32" "-N 32 -t 1m -b 2g -F -r" $ITERATION
			shift ;;
		2)
			#With client = 64
			launchSimulation "###Test with 64 clients###" "-n 64" "-N 64 -t 1m -b 2g -F -r" $ITERATION
			shift ;;
		3)
			#With ODIRECT=1
			launchSimulation "###Test with ODIRECT=1###" "-n 32" "-N 32 -t 1m -b 2g -F -r -B" $ITERATION
			shift ;;
		4)	
			#With Write (read=0)
			launchSimulation "###Test write files###" "-n 32" "-N 32 -t 1m -b 2g -F -w" $ITERATION
			shift ;;
		5)
			#With Stripe-Size = 32K
			launchSimulation "###Test Requier Size = 32 KB###" "-n 32" "-N 32 -t 32k -b 2g -F -r" $ITERATION
			shift ;;
		6)	
			#With Stripe-Size = 128K
			launchSimulation "###Test Requier Size = 128 KB###" "-n 32" "-N 32 -t 128k -b 2g -F -r" $ITERATION
			shift ;;
		7)
			#With req-size = 256K
			launchSimulation "###Test Requier Size = 256 K###" "-n 32" "-N 32 -t 256k -b 2g -F -r" $ITERATION
			shift ;;
		8)
			#With req-size=8MB
			launchSimulation "###Test Requier Size = 8 MB###" "-n 32" "-N 32 -t 8m -b 2g -F -r" $ITERATION
			shift;;
		9)	
			#With req-size=64MB
			launchSimulation "###Test Requier Size = 64 MB###" "-n 32" "-N 32 -t 64m -b 2g -F -r" $ITERATION
			shift;;
		10)
			#Uniq Shared File
			launchSimulation "###Test Uniq Shared File###" "-n 32" "-N 32 -t 1m -b 2g -r" $ITERATION
			shift;;
		11) 
			# With file-size = 1 GB
			launchSimulation "###Test with size of file = 1 GB###" "-n 32" "-N 32 -t 1m -b 1g -F -r" $ITERATION
			shift ;;
		12)	
			# With file-size = 512 MB
			launchSimulation "###Test with size of file = 512 MB###" "-n 32" "-N 32 -t 1m -b 512m -F -r" $ITERATION
			shift ;;
		13)
			#With Write and Uniq Shared File
			launchSimulation "###Test write files and Uniq Shared File###" "-n 32" "-N 32 -t 1m -b 2g -w" $ITERATION
			shift;;
		-a) #All Simulations
			#1
			launchSimulation "###Test with default paramaters###" "-n 32" "-N 32 -t 1m -b 2g -F -r" $ITERATION
			#2
			launchSimulation "###Test with 64 clients###" "-n 64" "-N 64 -t 1m -b 2g -F -r" $ITERATION
			#3
			launchSimulation "###Test with ODIRECT=1###" "-n 32" "-N 32 -t 1m -b 2g -F -r -B" $ITERATION
			#4
			launchSimulation "###Test write files###" "-n 32" "-N 32 -t 1m -b 2g -F -w" $ITERATION
			#5
			launchSimulation "###Test Requier Size = 32 KB###" "-n 32" "-N 32 -t 32k -b 2g -F -r" $ITERATION
			#6
			launchSimulation "###Test Requier Size = 128 KB###" "-n 32" "-N 32 -t 128k -b 2g -F -r" $ITERATION
			#7
			launchSimulation "###Test Requier Size = 256 K###" "-n 32" "-N 32 -t 256k -b 2g -F -r" $ITERATION
			#8
			launchSimulation "###Test Requier Size = 8 MB###" "-n 32" "-N 32 -t 8m -b 2g -F -r" $ITERATION
			#9
			launchSimulation "###Test Requier Size = 64 MB###" "-n 32" "-N 32 -t 64m -b 2g -F -r" $ITERATION
			#10
			launchSimulation "###Test Uniq Shared File###" "-n 32" "-N 32 -t 1m -b 2g -r" $ITERATION
			#11
			launchSimulation "###Test with size of file = 1 GB###" "-n 32" "-N 32 -t 1m -b 1g -F -r" $ITERATION
			#12
			launchSimulation "###Test with size of file = 512 MB###" "-n 32" "-N 32 -t 1m -b 512m -F -r" $ITERATION
			#13
			launchSimulation "###Test write files and Uniq Shared File###" "-n 32" "-N 32 -t 1m -b 2g -w" $ITERATION
			exit ;;
		-I)
			#Launch Test with custom options
			echo "###Interactive mode###"
			interactivemode
			echo "###END###"
			exit;;
		*)
			echo "Invalid Numero of Test type -h or --help"
			exit ;;
	esac
done
