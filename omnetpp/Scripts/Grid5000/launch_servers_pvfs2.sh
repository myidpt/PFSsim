#!/bin/sh

#Copy the config file on each server and launch them

	#if [ $1 -gt $2 ];
	#then
	#	end=$1
	#	start=$2
	#else
	#	end=$2
	#	start=$1
	#fi
for dest in $(head -n $1 server.hosts);do
	scp /etc/pvfs2-fs.conf ${dest}:/etc/
	ssh ${dest} pvfs2-server /etc/pvfs2-fs.conf -f
	ssh ${dest} pvfs2-server /etc/pvfs2-fs.conf
	echo "tcp://${dest}:3334/pvfs2-fs /mnt/pvfs2 pvfs2 defaults,noauto 0 0" >> /etc/fstab
done
	
pvfs2-ping -m /mnt/pvfs2


