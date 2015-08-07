#!/bin/bash

#Launch the different simulation with different parameters
GEN_EXEC=./gen_exe.sh
GEN_INPUT=./gen_input.sh
CMD_BACK_PFSSIM_MASTER=../../
CMD_BACK_PFSSIM_STAGE=../
EXEC_PFSSIM='./PFSsim -u Cmdenv -r 0 omnetpp.ini'
PATH_PFSSIM_MASTER='./PFSsim-master/omnetpp/'
PATH_PFSSIM_STAGE='./PFSsim_stage/'
PATH_TO_RESULT=Results/simulation_traces/
KB=1024
MB=$((${KB}*${KB}))
GB=$((${KB}*${KB}*${KB}))
I=0
SID=1
ITERATION=5
PREFIX=res_sim

if [ $# -ne 0 -a "$1" != "-h" -a "$1" != "--help" ];
then
	#Rebuild the exe first
	cd ${PATH_PFSSIM_MASTER}
	${GEN_EXEC}
	cd $CMD_BACK_PFSSIM_MASTER
	cd ${PATH_PFSSIM_STAGE}
	${GEN_EXEC}
	cd $CMD_BACK_PFSSIM_STAGE
fi

if [ $# -eq 0 ];
then
 echo "Execute launch_simulation --help (-h) to see options"
fi

launchSimulation(){
	if [ $WORD == "-I" ];
	then
		I=0
	elif [ $WORD != "-a" ];
	then
		I=$WORD
	else
		I=$(($I+1))
	fi
	echo ""
	echo $1 #Show the name of the Simulation
	echo ""
	SID=1
	#In order to iterate the test
	if [ "$#" -eq 4 ];
	then
		ITERATION=$4
	fi
	for i in $(seq 1 $ITERATION);
	do
		echo "##Simulation $I launched: Iteration = $SID##"
		cd ${PATH_PFSSIM_MASTER}
		${GEN_INPUT} $2 #Generate the file as layout, traceFile, ext or disk
		echo "##Launch software not modified##"
		$EXEC_PFSSIM $3 > ${CMD_BACK_PFSSIM_MASTER}${PATH_TO_RESULT}${PREFIX}${I}-${SID}_before.txt
		echo "##End of software not modified##"
		cd $CMD_BACK_PFSSIM_MASTER
		cd $PATH_PFSSIM_STAGE
		${GEN_INPUT} $2
		echo "##Launch software modified##"
		$EXEC_PFSSIM  $3 > ${CMD_BACK_PFSSIM_STAGE}${PATH_TO_RESULT}${PREFIX}${I}-${SID}_after.txt
		echo "##End of software modified##"
		cd $CMD_BACK_PFSSIM_STAGE
		SID=$(($SID+1))
		echo ""
	done
}

interactivemode(){
	correct=""
	while [[ $correct != "y" && $correct != "Y" ]];
	do
		correct="y"
		echo "Give the generation of input options (see gen_input --help for valid options):"
		read tmp
		if [ ! -z $tmp ];
		then
			GEN_OPTIONS_D=$tmp
		fi
		echo "Give the name of the configuration (see .ini files for valid name):"
		read tmp
		if [ ! -z $tmp ];
		then
			CONFIG_OPTIONS_D="-c $tmp"
		fi
		echo "Give the number of iteration:[5]"
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
		echo "gen_input Options : "${GEN_OPTIONS_D}
		echo "Config Options : "${CONFIG_OPTIONS_D}
		echo "Number of iteration: "${ITERATION}
		echo "Name of result file: "${PREFIX}
		echo "Is it ok?[Y/n]"
		read tmp
		if [ ! -z $tmp ];
		then
			correct=$tmp
		fi
	done
	echo "${GEN_INPUT} ${GEN_OPTIONS_D}"
	echo "${EXEC_PFSSIM} ${CONFIG_OPTIONS_D}"
	launchSimulation "###Simulation with custom options###" "${GEN_OPTIONS_D}" "${CONFIG_OPTIONS_D}" $ITERATION
}


for WORD in "$@";
do
	case $WORD in
		--help | -h)
			echo "Options:"
			echo "-a : # Execute all simulations"
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
			echo "10 : # Unique Shared File"
			echo "11 : # With file-size = 1 GB"
			echo "12 : # With file-size = 512 MB"
			echo "13 : # With Write and Uniq Shared File"
			echo "14 : # With FIFO Scheduler Algorithm"
			echo "15 : # With SFQ Scheduler Algorithm [Doesn't work in command line. Use GUI]"
			echo "16 : # Unique Shared File and Read-ahead disabled"
			exit ;;
		1)	
			#With default parameters client = 32, server=6, file-size=2GB 
			#stripe-size=64K, req-size=1M, read=1
			launchSimulation "###Simulation with default paramaters###"
			shift ;;
		2)
			#With client = 64
			launchSimulation "###Simulation with 64 clients###" --client=64 "-c Client64"
			shift ;;
		3)
			#With ODIRECT=1
			launchSimulation "###Simulation with ODIRECT=1###" "" "-c Odirect"
			shift ;;
		4)	
			#With Write (read=0)
			launchSimulation "###Simulation write files###" --read=0 "-c Write"
			shift ;;
		5)
			#With Stripe-Size = 32K
			launchSimulation "###Simulation Requier Size = 32 KB###" --req-size=$(echo "32*$KB" | bc) "-c LittleReqSize"
			shift ;;
		6)	
			#With Stripe-Size = 128K
			launchSimulation "###Simulation Requier Size = 128 KB###" --req-size=$(echo "128*$KB" | bc) "-c LittleReqSize"
			shift ;;
		7)
			#With req-size = 256K
			launchSimulation "###Simulation Requier Size = 256 K###" --req-size=$(echo "256*$KB" | bc) "-c LittleReqSize"
			shift ;;
		8)
			#With req-size=10MB
			launchSimulation "###Simulation Requier Size = 8 MB###" --req-size=$(echo "8*$MB" | bc)
			shift;;
		9)	
			#With req-size=50MB
			launchSimulation "###Simulation Requier Size = 64 MB###" --req-size=$(echo "64*$MB" | bc)
			shift;;
		10)
			#Uniq Shared File
			launchSimulation "###Simulation Unique Shared File###" --uniq-shared-file
			shift;;
		11) 
			# With file-size = 1 GB
			launchSimulation "###Simulation with size of file = 1 GB###" --file-size=$GB
			shift ;;
		12)	
			# With file-size = 512 MB
			launchSimulation "###Simulation with size of file = 512 MB###" --file-size=$(echo "512*$MB" | bc)
			shift ;;
		13)
			#With Write and Uniq Shared File
			launchSimulation "###Simulation write files and Unique Shared File###" "--read=0 --uniq-shared-file" "-c Write"
			shift;;
		14)
			#With FIFO Scheduler Algorithm
			launchSimulation "###Simulation FIFO Scheduler Algorithm###" "" "-c FIFO"
			shift;;
		15)
			#With SFQ Scheduler Algorithm
			launchSimulation "###Simulation SFQ Scheduler Algorithm###" "" "-c SFQ"
			shift;;
		16)
			#Unique Shared File and Read-ahead disabled
			launchSimulation "###Simulation Shared File and Read-ahead disabled###" "--uniq-shared-file" "-c NoRA"
			shift;;
		-a) #All Simulations
			launchSimulation "###Simulation with default paramaters###"
			launchSimulation "###Simulation with 64 clients###" --client=64 "-c Client64"
			launchSimulation "###Simulation with ODIRECT=1###" "" "-c Odirect"
			launchSimulation "###Simulation write files###" --read=0 "-c Write"
			launchSimulation "###Simulation Requier Size = 32 KB###" --req-size=$(echo "32*$KB" | bc) "-c LittleReqSize"
			launchSimulation "###Simulation Requier Size = 128 KB###" --req-size=$(echo "128*$KB" | bc) "-c LittleReqSize"
			launchSimulation "###Simulation Requier Size = 256 KB###" --req-size=$(echo "256*$KB" | bc) "-c LittleReqSize"
			launchSimulation "###Simulation Require Size = 8MB###" --req-size=$(echo "8*$MB" | bc)
			launchSimulation "###Simulation Require Size = 64MB###" --req-size=$(echo "64*$MB" | bc)
			launchSimulation "###Simulation Unique Shared File" --uniq-shared-file
			launchSimulation "###Simulation with size of file = 1 GB" --file-size=$GB
			launchSimulation "###Simulation with size of file = 512 MB" --file-size=$(echo "512*$MB" | bc)
			launchSimulation "###Simulation write files and Unique Shared File###" "--read=0 --uniq-shared-file" "-c Write"
			launchSimulation "###Simulation FIFO Scheduler Algorithm###" "" "-c FIFO"
			launchSimulation "###Simulation Shared File and Read-ahead disabled###" "--uniq-shared-file" "-c NoRA"
			#launchSimulation "###Simulation SFQ Scheduler Algorithm###" "" "-c SFQ"
			exit ;;
		-I)#Launch Test with custom options
			echo "###Interactive mode###"
			interactivemode
			echo "###END###"
			exit;;
		
		*)
			echo "Invalid Numero of Simulation type -h or --help"
			exit ;;
	esac
done
