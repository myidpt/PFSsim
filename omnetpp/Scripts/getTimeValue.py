#!/usr/bin/env python
# -*- coding: utf-8 -*-
#
#  GetTimeValue.py
#  
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2 of the License, or
#  (at your option) any later version.
#  
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#  
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software
#  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
#  MA 02110-1301, USA.
#  
#  
import re
import csv
import os
import glob

p = re.compile(r"t=[0-9]+\.[0-9]+")
elapse = re.compile(ur'Elapsed: [0-9]+\.[0-9]+(?!.*Elapsed: [0-9]+\.[0-9]+)', re.DOTALL)
prefix = "res_sim";
asuffix="_after";
bsuffix="_before";
extension = ".txt";
pathToResults="simulation_traces/"

nbfile=len(glob.glob(pathToResults+prefix+"*-1"+asuffix+extension))+1
i=1
k=1
while i < nbfile:
	if os.path.isfile(pathToResults+prefix+str(k)+"-1"+asuffix+extension) and os.path.isfile(pathToResults+prefix+str(k)+"-1"+bsuffix+extension):
		#Get the result from the files
		nbsubfile=len(glob.glob(pathToResults+prefix+str(k)+"-*"+asuffix+extension))+1
		resSa=[]
		resSb=[]
		resTa=[]
		resTb=[]
		for j in range(1,nbsubfile):
			with open(pathToResults+prefix+str(k)+"-"+str(j)+asuffix+extension) as fil:
				lines=fil.read();
				resa=p.findall(lines);
				resSa.append(float(resa[0][2:]))
				timea=elapse.findall(lines);
				resTa.append(float(timea[0][9:]))

			with open(pathToResults+prefix+str(k)+"-"+str(j)+bsuffix+extension) as fil:
				lines=fil.read();
				resb=p.findall(lines);
				resSb.append(float(resb[0][2:]))
				timeb=elapse.findall(lines);
				resTb.append(float(timeb[0][9:]))
		
		datas=[]
		averSA=(sum(resSa)/float(len(resSa)))
		averSB=(sum(resSb)/float(len(resSb)))
		averTA=(sum(resTa)/float(len(resTa)))
		averTB=(sum(resTb)/float(len(resTb)))
		
		#Get the previous values if file exist
		if os.path.isfile("result_"+str(k)+".csv"):
			with open("result_"+str(k)+".csv","rb") as fil:
				r = csv.reader(fil)
				datas=list(r)
			
		with open("result_"+str(k)+".csv","wb") as fil:
			c = csv.writer(fil)
			if not datas or len(datas[0]) == 4:
				#if file did not exist or the real system has not send its results, update the datas
				c.writerow(["Simulation time before","Simulation time after","Execution Time before", "Execution Time after"])
				c.writerow([averSB,averSA,averTB,averTA])
			elif len(datas[0]) == 5:
				#Update the datas in the correct columns
				datas[1][0]=averSB
				datas[1][1]=averSA
				datas[1][3]=averTB
				datas[1][4]=averTA
				c.writerows(datas)
			elif len(datas[0]) == 6:
				#Update the datas in the correct columns
				datas[1][0]=averSB
				datas[1][1]=averSA
				datas[1][4]=averTB
				datas[1][5]=averTA
				c.writerows(datas)
		
		print "File",k,"done"
		i+=1
	k+=1
