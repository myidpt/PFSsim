#!/bin/sh

#Copy the fstab file on each client and launch them
PATH_EXEC_CLIENT="pvfs-2.8.2/src/apps/kernel/linux"

	#if [ $1 -gt $2 ];
	#then
	#	end=$1
	#	start=$2
	#else
	#	end=$2
	#	start=$1
	#fi
	if [ "$#" -eq 0 ];
	then
		for dest in $(cat mpd.hosts);do
		scp /etc/fstab ${dest}:/etc/
		ssh ${dest} ${PATH_EXEC_CLIENT}/pvfs2-client -p ${PATH_EXEC_CLIENT}/pvfs2-client-core
		done
	
		nb=$(sed -n '$=' mpd.hosts)
		mpdboot -n $nb -f mpd.hosts
		mpdtrace | sort
	fi
	

