import sys

out = open(sys.argv[1]+"-result.txt", "w")

out.write(sys.argv[1]+"\n")
overall_bw = 0
ava_bw = 0
bw_limit = 0
throughput = 0
pim_through = 0
pcu_thread = 0
times = 0

with open(sys.argv[1], "r", encoding='utf-8') as infile:
    for line in infile:
        x = line.split()
        if (x == []):
            continue
        if (x[0] == "Bandwidth"):
            overall_bw += int(x[3])
            if ("pim" in sys.argv[1]):
                ava_bw += int(x[5])
                bw_limit += int(x[7])
            else:
                bw_limit += int(x[5])
                ava_bw = bw_limit
        elif (x[0] == "Throughput"):
            throughput += int(x[3])
        elif (x[0] == "PIM"):
            pim_through += int(x[3])
        elif (x[0] == "Overall"):
            if (int(x[1]) != throughput+pim_through):
                out.write("Error!\n")
        elif (x[0] == "Total"):
            if (pcu_thread != int(x[3])):
                out.write("PCU = " + str(pcu_thread) + " :\n")
                out.write("GPU bandwidth: " + str(float(overall_bw)/times) + " Bytes\n")
                out.write("GPU BW percentage: " + str(float(overall_bw)/float(bw_limit)*100) + " %\n")
                #if ("pim" in sys.argv[1]):
                #    out.write("Available bandwidth: " + str(float(ava_bw)/times) + " Bytes\n")
                out.write("Bandwidth limit: " + str(float(bw_limit)/times) + " Bytes\n")
                if ("pim" in sys.argv[1]):
                    out.write("PIM bandwidth: " + str(float(bw_limit-ava_bw)/times) + " Bytes\n")
                    out.write("PIM BW percentage: " + str(float(bw_limit-ava_bw)/float(bw_limit)*100) + " %\n")
                    out.write("Overall bandwidth: " + str(float(overall_bw+bw_limit-ava_bw)/times) + " Bytes\n")
                    out.write("Overall BW percentage: " + str(float(overall_bw+bw_limit-ava_bw)/float(bw_limit)*100) + " %\n")
                out.write("GPU throughput: " + str(float(throughput)/times) + " KH/s\n")
                if ("pim" in sys.argv[1]):
                    out.write("PIM throughput: " + str(float(pim_through)/times) + " KH/s\n")
                    out.write("Overall throughput: " + str(float(throughput+pim_through)/times) + " KH/s\n\n")

                pcu_thread = int(x[3])
                overall_bw = 0
                ava_bw = 0
                bw_limit = 0
                throughput = 0
                pim_through = 0
                times = 0
            else:
                times += 1
        else:
            continue

out.write("PCU = " + str(pcu_thread) + " :\n")
out.write("GPU bandwidth: " + str(float(overall_bw)/times) + " Bytes\n")
out.write("GPU BW percentage: " + str(float(overall_bw)/float(bw_limit)*100) + " %\n")
#if ("pim" in sys.argv[1]):
#    out.write("Available bandwidth: " + str(float(ava_bw)/times) + " Bytes\n")
out.write("Bandwidth limit: " + str(float(bw_limit)/times) + " Bytes\n")
if ("pim" in sys.argv[1]):
    out.write("PIM bandwidth: " + str(float(bw_limit-ava_bw)/times) + " Bytes\n")
    out.write("PIM BW percentage: " + str(float(bw_limit-ava_bw)/float(bw_limit)*100) + " %\n")
    out.write("Overall bandwidth: " + str(float(overall_bw+bw_limit-ava_bw)/times) + " Bytes\n")
    out.write("Overall BW percentage: " + str(float(overall_bw+bw_limit-ava_bw)/float(bw_limit)*100) + " %\n")
out.write("GPU throughput: " + str(float(throughput)/times) + " KH/s\n")
if ("pim" in sys.argv[1]):
    out.write("PIM throughput: " + str(float(pim_through)/times) + " KH/s\n")
    out.write("Overall throughput: " + str(float(throughput+pim_through)/times) + " KH/s\n\n")