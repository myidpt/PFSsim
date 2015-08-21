#!/usr/bin/env python

import re
import csv
import glob
import os

r = re.compile(ur'^read[ A-Za-z0-9\.]+(?!.*^read[ A-Za-z0-9\.]+)', re.MULTILINE | re.DOTALL)
w = re.compile(ur'^write[ A-Za-z0-9\.]+(?!.*^write[ A-Za-z0-9\.]+)', re.MULTILINE | re.DOTALL)
prefix="result_test"
suffix=".txt"
pathToResults="../../Results/"

def getReadResult(lines):
	"""
	Get the time result for a read test with the benchmark IOR
	fil: file with results of the test
	"""
	read=False
	res = r.findall(lines)
	time=0
	if len(res) != 0:
		spl = res[0].split()
		time = spl[5]
		read=True
	return time, read

def getWriteResult(lines):
	"""
	Get the time result for a write test with the benchmark IOR
	fil: file with results of the test
	"""
	write=False
	res = w.findall(lines)
	time=0
	if len(res) != 0:
		spl = res[0].split()
		time = spl[5]
		write=True
	return time, write

nbfile=len(glob.glob(prefix+"*-1"+suffix))+1
i=1
k=1
while i < nbfile:	
	#Get the result from the files
	nbsubfile=len(glob.glob(prefix+str(k)+"-*"+suffix))+1
	resr=[]
	resw=[]
	if os.path.isfile(prefix+str(k)+"-"+"1"+suffix):
		for j in range(1,nbsubfile):
			with open(prefix+str(k)+"-"+str(j)+suffix) as fil:
				lines = fil.read()
				timer, read=getReadResult(lines)
				if read:
					resr.append(float(timer))
				
				timew, write=getWriteResult(lines)
				if write:
					resw.append(float(timew))
					
		#Calcul the average of the test number i
		if read:
			averR=(sum(resr)/float(len(resr)))
		if write:
			averW=(sum(resw)/float(len(resw)))
		
		#Some simulation has to be compare to the same real result
		if k == 1:
			l=[1,14,15]
		elif k == 10:
			l=[10,16]
		else:
			l=[k]

		for m in l: #Take the same result for the test 1, 14, 15
			if os.path.isfile(pathToResults+"result_"+str(m)+".csv"):
				#Write on the existant files of results
				with open(pathToResults+"result_"+str(m)+".csv","rb") as fil:
					rcs = csv.reader(fil)
					datas=list(rcs)
					if len(datas[0]) == 5:
						if read:
							datas[0][2]="Result for read from real system"
							datas[1][2]=averR
						if write and not read:
							datas[0][2]="Result for write from real system"
							datas[1][2]=averW
						elif write and read:
							datas[0].insert(3,"Result for write from real system")
							datas[1].insert(3,averW)
					elif len(datas[0]) == 6:
						if read:
								datas[1][2]=averR
						if write:
								datas[1][3]=averW
					elif len(datas[0]) == 4:
						if read:
							datas[0].insert(2,"Result for read from real system")
							datas[1].insert(2,averR)
						if write and not read:
							datas[0].insert(2,"Result for write from real system")
							datas[1].insert(2,averW)
						elif write and read:
							datas[0].insert(3,"Result for write from real system")
							datas[1].insert(3,averW)

				with open(pathToResults+"result_"+str(m)+".csv","wb") as fil:
					c = csv.writer(fil)
					c.writerows(datas)
		
		print "File", k, "done"
		i+=1
	k+=1
		

