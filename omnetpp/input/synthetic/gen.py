import random
import sys

if (len(sys.argv) != 6):
  print sys.argv[0], " <number of clients> <number of traces per client> <total size> <request size> <read(1/0)>"
  sys.exit()

read = int(sys.argv[5])
if (read != 1 and read != 0):
  print "read value should be 1 or 0"
  sys.exit()

print "Generating input files. Number of clients:", sys.argv[1], " number of traces per client: ", sys.argv[2], " total size: ", sys.argv[3], " request size:", sys.argv[4], " read: ", sys.argv[5]

nclient = int(sys.argv[1])
ntrace = int(sys.argv[2])
tsize = long(sys.argv[3])
size = long(sys.argv[4])
nline = int(tsize / size)

time = 0
offset = 0
fid = 0
sync = 0

random.seed()
for i in range(0, nclient):
	if i >= 100:
		name1 = str('client%d' %i)
	elif i >= 10:
		name1 = str('client0%d' %i)
	else:
		name1 = str('client00%d' %i)
	for j in range(0, ntrace):
		if j < 10:
			name = name1 + str('trace0%d' %j)
		else:
			name = name1 + str('trace%d' %j)
		f = open(name, 'w')
		time = random.random() * 0.1
		sync = 0
		offset = 0
		for k in range(0, nline):
			f.write('%(t)lf %(f)d %(o)ld %(s)ld %(r)d %(a)d %(s2)d\n' %{"t":time,"f":fid,"o":offset,"s":size,"r":read,"a":j,"s2":sync})
			offset += size
			sync = 1
			time = 0
		fid += 1
		f.close()
