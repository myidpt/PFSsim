import random

time = 0
offset = 0
size = 300000
read = 1
fid = 1
app = 1
sync = 0

random.seed()
for i in range(0, 32):
	if i < 10:
		name = str('trace000%d' %i)
	else:
		name = str('trace00%d' %i)
	f = open(name, 'w')
	time = random.random()
	sync = 0
	for i in range(0, 20):
		f.write('%(t)lf %(f)d %(o)ld %(s)ld %(r)d %(a)d %(s2)d\n' %{"t":time,"f":fid,"o":offset,"s":size,"r":read,"a":app,"s2":sync})
		offset += 300000
		sync = 1
		time = 0
