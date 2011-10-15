#!/usr/bin/python

import random

# We consider more of a real-world workload, the group1 and group2 starttime will be fully "randomized", so group2 will not be as bursty as the burst_var workload.
time = 0
offset = 0
read = 1
fid = 0
app = 0
sync = 0

random.seed()
for i in range(0, 128):
	if i < 10:
		name = str('trace000%d' %i)
	elif i < 100:
		name = str('trace00%d' %i)
	else:
		name = str('trace0%d' %i)
	f = open(name, 'w')
	time = random.random() * 10
	sync = 0
	offset = 0
	if i >= 64: # checkpointing
		app = 1
		size = 1048576 * 4 # 4M
		for j in range(0,100):
			f.write('%(t)lf %(f)d %(o)ld %(s)ld %(r)d %(a)d %(s2)d\n' %{"t":time,"f":fid,"o":offset,"s":size,"r":read,"a":app,"s2":sync})
			offset += size
			time = 0
			sync = 1
	else: # real-time
		app = 0
		size = 1048576 # 1M
		for j in range(0, 100):
			if j % 10 == 0 and sync == 1:
				time = 10
			f.write('%(t)lf %(f)d %(o)ld %(s)ld %(r)d %(a)d %(s2)d\n' %{"t":time,"f":fid,"o":offset,"s":size,"r":read,"a":app,"s2":sync})
			offset += size
			time = 0
			sync = 1
	fid += 1
