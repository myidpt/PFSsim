import random
import sys

blocksize = 4096

random.seed()

if (len(sys.argv) != 4):
  print sys.argv[0], " <number of disks> <number of files> <data file size>"
  sys.exit()

servernum=int(sys.argv[1])
filenum=int(sys.argv[2])
total=int(sys.argv[3]) / blocksize
#extsize=int(sys.argv[4]) # extent size of EXT3
#length = 12 # In EXT3, the first extent length is 12.
print "Generating disk parameters. Server number: ", servernum, " File number: ", filenum, " Data File Size(in 4k block): ", total
name1=str('ext')

total

for n in range (0, servernum):
  if n < 10:
    name = name1 + str('00%(n)d' %{"n":n})
  else:
    name = name1 + str('0%(n)d' %{"n":n})
  f=open(name,'w')
  offset = 1
  for i in range (0, filenum):
    f.write('%(fn)d:\n' %{"fn":i})
    acc = 0;
    while acc < total:
      length = 1024
      f.write('%(acc)d %(off)d %(len)d;\n' %{"acc":acc,"off":offset,"len":length})
      acc += length
      offset += length+1
