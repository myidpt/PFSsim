import sys

if (len(sys.argv) != 4):
  print sys.argv[0], "<number of servers> <number of files> <stripe size>"
  sys.exit()

print "Generating PFS layout file. Number of servers: ", sys.argv[1], " number of files: ", sys.argv[2], " stripe size: ", sys.argv[3]
snum = int(sys.argv[1])
fnum = int(sys.argv[2])
ssize = sys.argv[3]

name = str("even-dist")
f=open(name,'w')
for n in range (0, fnum):
  remain = n % snum
  line = str(n)
  for i in range (0, snum):
    sr = remain + i
    if sr >= snum:
      sr -= snum
    line += str(" [%(sr)d " %{"sr":sr}) + ssize + str("]")
  line += str("\n")
  f.write(line)
f.close()
