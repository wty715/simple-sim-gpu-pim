out = open("filtered.log", "w")

with open("eth_output.log", "r", encoding='utf-8') as infile:
    lastnum = 0
    for line in infile:
        if (line[26:32] != "mining"):
            print(line)
            continue
        strrr = line[84:-1]
        num = int(strrr)
        if (num != lastnum+1):
            out.write(str(num) + '\n')
        lastnum = num;