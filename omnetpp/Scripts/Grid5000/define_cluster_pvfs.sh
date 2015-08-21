#!/bin/bash

NCLUSTER="graphene-"
rm -f mpd.hosts
rm -f server.hosts
rm -f /etc/pvfs2-fs.conf

#Clust1[1-39]
#Clust2[40-74]
#Clust3[75-104]
#Clust4[105-144]
CLIENT=1
SERVER=2
INSUFFICIENT=3

NUM_CLUSTER=4

CLUSTER[1]=$INSUFFICIENT;
CLUSTER[2]=$INSUFFICIENT;
CLUSTER[3]=$INSUFFICIENT;
CLUSTER[4]=$INSUFFICIENT;
if [ "$#" -eq 2 ];
then
#Count how many nodes in each cluster is allocated
for dest in $(sort <${OAR_NODEFILE} | uniq);
do

NUM=${dest:9:$((${#dest}-27))}

if [[ $NUM -ge 1 && $NUM -le 39 ]];
then 
	Clust1[$NUM]=${NCLUSTER}${NUM}
elif [[ $NUM -ge 40 && $NUM -le 74 ]];
then 
	Clust2[$NUM]=${NCLUSTER}${NUM}
elif [[ $NUM -ge 75 && $NUM -le 104 ]];
then 
	Clust3[$NUM]=${NCLUSTER}${NUM}
elif [[ $NUM -ge 105 && $NUM -le 144 ]];
then 
	Clust4[$NUM]=${NCLUSTER}${NUM}
fi
done

tab[1]=${#Clust1[@]}
tab[2]=${#Clust2[@]}
tab[3]=${#Clust3[@]}
tab[4]=${#Clust4[@]}
#~ TODO : Test this lines to prevent user if there are enough nodes
#~ t=$(($tab[1]+$tab[2]+$tab[3]+$tab[4]))
#~ n=$(($1+$2))
#~ if [[ $t -lt $n ]];
#~ then
	#~ echo "Not enough nodes are allocated for $1 clients and $2 servers"
	#~ exit
#~ fi

#sort the number of nodes on each cluster
sort_table=($(printf "%s\n" ${tab[*]} | sort -n))

#choose the first wich is higher than the number of server
k=0
while [ ${sort_table[${k}]} -lt $2 ];do
	k=$(($k+1))
done

#search wich cluster has the minimum calculate previously
if [ ${sort_table[${k}]} -eq ${#Clust1[@]} ];
then
	CLUSTER[1]=$SERVER
elif [ ${sort_table[${k}]} -eq ${#Clust2[@]} ];
then
	CLUSTER[2]=$SERVER
elif [ ${sort_table[${k}]} -eq ${#Clust3[@]} ];
then
	CLUSTER[3]=$SERVER
elif [ ${sort_table[${k}]} -eq ${#Clust4[@]} ];
then
	CLUSTER[4]=$SERVER
fi

#sort the number of nodes on each cluster
sort_table=($(printf "%s\n" ${tab[*]} | sort -n -r))
#Choose the cluster if the number of nodes is sufficient
TOTAL_NODE=0
j=0
#Assume that there are enough nodes for servers and clients
while [ $TOTAL_NODE -lt $1 -o $j -eq $NUM_CLUSTER ];
do	
	if [ ${#Clust1[@]} -eq ${sort_table[j]} -a ${CLUSTER[1]} -eq $INSUFFICIENT ];
	then
		CLUSTER[1]=$CLIENT
		TOTAL_NODE=$(($TOTAL_NODE+${#Clust1[@]}))
	else
		if [ ${#Clust2[@]} -eq ${sort_table[j]} -a ${CLUSTER[2]} -eq $INSUFFICIENT ];
		then
			CLUSTER[2]=$CLIENT
			TOTAL_NODE=$(($TOTAL_NODE+${#Clust2[@]}))
		else
			if [ ${#Clust3[@]} -eq ${sort_table[j]} -a ${CLUSTER[3]} -eq $INSUFFICIENT ];
			then
				CLUSTER[3]=$CLIENT
				TOTAL_NODE=$(($TOTAL_NODE+${#Clust3[@]}))

			elif [ ${CLUSTER[4]} -eq $INSUFFICIENT ];
			then
				CLUSTER[4]=$CLIENT
				TOTAL_NODE=$(($TOTAL_NODE+${#Clust4[@]}))
			fi
		fi
	fi
	j=$(($j+1))
done;

echo ${#Clust1[@]} ${#Clust2[@]} ${#Clust3[@]} ${#Clust4[@]}
echo ${CLUSTER[1]} ${CLUSTER[2]} ${CLUSTER[3]} ${CLUSTER[4]}

t=1
for state in ${CLUSTER[@]};
do
	m=0
	eval var=Clust${t}[@]
	case $state in
	$CLIENT)
		for node in ${!var};do
			echo $node >> mpd.hosts
			m=$(($m+1))
		done
		echo "Cluster $t is reserved for clients : $m nodes"
		cnode="${cnode}${t}, "
		nnode=$(($nnode + $m))
		;;
	$SERVER)
		for node in ${!var};do
			echo $node >> server.hosts
			m=$(($m+1))
		done
		echo "Cluster $t is reserved for servers : $m nodes"
		snode="${snode}${t}, "
		snnode=$(($snnode + $m))
		;;
	esac
	t=$(($t+1))
done

echo "Summary : "
echo "Cluster(s) ${cnode:0:$((${#cnode}-2))} is(are) used for clients : ${nnode} nodes"
echo "$(sed -n '1p' mpd.hosts) is the first client node of the list"
echo "Cluster(s) ${snode:0:$((${#snode}-2))} is(are) used for servers : ${snnode} nodes"
echo "Server List :"
for destse in $(head -n $2 server.hosts);do
echo $destse
done

pvfs2-genconfig /etc/pvfs2-fs.conf

if [ $? -eq 0 ];
then

./launch_servers_pvfs2.sh $2

./launch_client_pvfs2.sh

./mount_pvfs2_server.sh $2
fi

else
	echo "Too few arguments : define_cluster_pvfs \$NUMBER_CLIENT \$NUMBER_SERVER"
fi
