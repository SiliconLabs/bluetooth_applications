import sys
term = 0x00
with open(sys.argv[1], "ab") as input:
     input.write('\0')
     input.close()
