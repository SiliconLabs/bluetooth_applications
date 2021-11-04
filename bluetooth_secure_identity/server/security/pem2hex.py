import sys
import asn1
tags = []
values = []
if(len(sys.argv) < 2):
    print("usage: python <script> <der file>")
else:
    decoder = asn1.Decoder()
    with open(sys.argv[1]) as input:
        private_key = input.read()
        decoder.start(private_key)
        for tag in tags, value in values:
            tag,value = decoder.read()
        print(tags, values)
