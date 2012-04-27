import random
import sys

random.seed()

if (len(sys.argv) != 3):
  print sys.argv[0], " <number of disks> <number of files>"
  sys.exit()

disknum=int(sys.argv[1])
filenum=int(sys.argv[2])
print "Generating disk parameters. Disk number: ", disknum, " File number: ", filenum
total = 16 * 1024 # 256MB, or 16K pages
extsize = 1024 # typical extent size of EXT3
#length = 12 # In EXT3, the first extent length is 12.
name1=str('ext')

for n in range (0, disknum):
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
