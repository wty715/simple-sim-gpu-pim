# nohup ./main -c build/RTX2060.txt -t build/trace.txt > rtx2060-gddr6.txt 2>&1 &
# nohup ./main -c build/RTX2060-hbm.txt -t build/trace.txt > rtx2060-hbm.txt 2>&1 &
nohup ./main-pim -c build/RTX2060-pim.txt -t build/trace.txt > rtx2060-pim.txt 2>&1 &
nohup ./main-pim-fsm -c build/RTX2060-pim.txt -t build/trace.txt > rtx2060-pim-fsm.txt 2>&1 &
nohup ./main-pim-fsm-intra -c build/RTX2060-pim.txt -t build/trace.txt > rtx2060-pim-fsm-intra.txt 2>&1 &

# nohup ./main -c build/RTX3060.txt -t build/trace.txt > rtx3060-gddr6.txt 2>&1 &
# nohup ./main -c build/RTX3060-hbm.txt -t build/trace.txt > rtx3060-hbm.txt 2>&1 &
nohup ./main-pim -c build/RTX3060-pim.txt -t build/trace.txt > rtx3060-pim.txt 2>&1 &
nohup ./main-pim-fsm -c build/RTX3060-pim.txt -t build/trace.txt > rtx3060-pim-fsm.txt 2>&1 &
nohup ./main-pim-fsm-intra -c build/RTX3060-pim.txt -t build/trace.txt > rtx3060-pim-fsm-intra.txt 2>&1 &

# nohup ./main -c build/RTX3090.txt -t build/trace.txt > RTX3090-gddr6.txt 2>&1 &
# nohup ./main -c build/RTX3090-hbm.txt -t build/trace.txt > RTX3090-hbm.txt 2>&1 &
nohup ./main-pim -c build/RTX3090-pim.txt -t build/trace.txt > RTX3090-pim.txt 2>&1 &
nohup ./main-pim-fsm -c build/RTX3090-pim.txt -t build/trace.txt > RTX3090-pim-fsm.txt 2>&1 &
nohup ./main-pim-fsm-intra -c build/RTX3090-pim.txt -t build/trace.txt > RTX3090-pim-fsm-intra.txt 2>&1 &