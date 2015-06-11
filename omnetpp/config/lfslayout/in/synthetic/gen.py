import random
import sys
from collections import namedtuple
import re

ServerInfo = namedtuple("ServerInfo", "IDServer StripeSize Unit")
mat = lambda n, m: [[0 for j in range(0,m)] for i in range(0,n)]

blocksize = 4096

random.seed()

if (len(sys.argv) != 4):
  print sys.argv[0], " <number of disks> <number of files> <data file size>"
  sys.exit()

servernum=int(sys.argv[1])
filenum=int(sys.argv[2])
totalfilesize=int(sys.argv[3])
total=totalfilesize / blocksize

StripeSize=mat(filenum,servernum)
Total=mat(filenum,servernum)

size=''
KiloUnitConvertion=1024
#extsize=int(sys.argv[4]) # extent size of EXT3
#length = 12 # In EXT3, the first extent length is 12.
print "Generating disk parameters. Server number: ", servernum, " File number: ", filenum, " Data File Size(in 4k block): ", total
name1=str('ext')

total

fil=open("../../../pfslayout/synthetic/even-dist/even-dist","r")

#Parse the file even-dist to take the layout
#if the format of the file changes, change this
for i in range(0,filenum):
	line = fil.readline().rstrip('\n\r')#erase the end of line marker
	h=0
			#take the information inside the '[ ]'
	for infoserver in re.findall(r"\[([^\]]*)\]*", line): 
		index = 1
	
		IDServer=int(infoserver[0])#the first element is the IDServer
		
		#obtain the digits that compose the stripeSize
		while (infoserver[index] != 'K') and (infoserver[index]!='k') and infoserver[index] != 'm' and infoserver[index] != 'M': 
			size += infoserver[index]
			index+=1
			
		unit=KiloUnitConvertion #To convert KB in B
		
		if(infoserver[index] == 'm' or infoserver[index] == 'M'):
			unit*=KiloUnitConvertion #To convert MB in B
		
		#if size is empty, there is no stripesize for this file in this server
		if size != ' ': 
			info=ServerInfo(IDServer,int(size),unit)
			StripeSize[i][h]=info #push for this file the informations
		else:
			StripeSize[i][h]=0
		size=''#reset the variable
		h+=1

fil.close()

#For each file calcul the total data on each server
for i in range (0, filenum):
	k=0;
	totalfile=totalfilesize;
	while totalfile > 0:
		info=StripeSize[i][k] #Information for the file and the kth server
		if info !=0 :
			Total[i][info[0]] += info[1]
			totalfile -=(info[1]*info[2])
		k+=1
		if k >= servernum:
			k=0;
			
	for t in range (0,servernum):
		info_unit=StripeSize[i][t]
		if info_unit!=0:
			Total[i][info_unit[0]] *=info_unit[2]#Convert in B the total size
	#The last server have too many informations
	if totalfile < 0:
		Total[i][info[0]] +=totalfile

for n in range (0, servernum):
  if n < 10:
    name = name1 + str('00%(n)d' %{"n":n})
  else:
    name = name1 + str('0%(n)d' %{"n":n})
  f=open(name,'w')
  offset = 1
  for i in range (0, filenum):
	total = Total[i][n]/blocksize
	#If there are datas on the server we write the number of file
	if total != 0: 
		f.write('%(fn)d:\n' %{"fn":i})
	acc = 0;
	while acc < total:
		length = 1024
		f.write('%(acc)d %(off)d %(len)d;\n' %{"acc":acc,"off":offset,"len":length})
		acc += length
		offset += length+1
