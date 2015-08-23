# This is a tool to read an ascii hex file and generate
# an output binary file

import sys
import string

f_in = open(sys.argv[1], 'r')
f_out = open(sys.argv[2], 'wb')

for line in f_in:
    hex_vals = line.strip().split(' ')
    print hex_vals
    for hex in hex_vals:
        if len(hex)==2:
	    f_out.write(chr(int(hex,16)))
	else:
	    raise Exception()




