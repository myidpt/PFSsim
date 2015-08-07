#!/bin/sh

	#if [ $1 -gt $2 ];
	#then
	#	end=$1
	#	start=$2
	#else
	#	end=$2
	#	start=$1
	#fi
	for destcl in $(cat mpd.hosts);do
		for destse in $(head -n $1 server.hosts);do
			ssh ${destcl} mount -t pvfs2 tcp://${destse}:3334/pvfs2-fs /mnt/pvfs2
		done
	done
	ssh $(head -n 1 mpd.hosts) mount | grep pvfs2

