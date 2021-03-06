import sys

out = open("trace.txt", "w")

i = 0
max=40600000

with open("filtered.log", "r", encoding='utf-8') as infile:
    for line in infile:
        i += 1
        
        num = int(line)
        num *= 64  # index *64 -> bytes
        
        # calculate index
        out.write("BITOPS 1\n") # 2 cycles
        out.write("MUL 1\n")    # 5 cycles
        out.write("BITOPS 2\n") # 2 cycles
        out.write("BITOPS 3\n") # 2 cycles
        out.write("MOD 1\n")    # 200 cycles
        out.write("BITOPS 4\n") # 2 cycles
        
        # do fnv hashing
        out.write("MUL 2\n")
        out.write("BITOPS 5\n")
        out.write("MUL 3\n")
        out.write("BITOPS 6\n")
        out.write("MUL 4\n")
        out.write("BITOPS 7\n")
        out.write("MUL 5\n")
        out.write("BITOPS 8\n")
        
        # address
        out.write("LOAD " + str(num) + '\n')
        
        if (i%406000 == 0):
            tmp = i//406000
            print("\r{}% lines finished.".format(tmp), "#"*(tmp//2), end="")
            if (len(sys.argv) > 1 and sys.argv[1] == "small"):
                if (tmp == 50):
                    break
print("\n")