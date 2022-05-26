#nohup ./main -c configs/RTX2060.conf -t build/trace.txt > rtx2060-gddr6.txt 2>&1 &
#nohup ./main -c configs/RTX2060-hbm.conf -t build/trace.txt > rtx2060-hbm.txt 2>&1 &
nohup ./main-pim -c configs/RTX2060-pim.conf -t build/trace.txt > rtx2060-pim.txt 2>&1 &
nohup ./main-pim-fsm -c configs/RTX2060-pim.conf -t build/trace.txt > rtx2060-pim-fsm.txt 2>&1 &
nohup ./main-pim-fsm-intra -c configs/RTX2060-pim.conf -t build/trace.txt > rtx2060-pim-fsm-intra.txt 2>&1 &

#nohup ./main -c configs/RTX3060.conf -t build/trace.txt > rtx3060-gddr6.txt 2>&1 &
#nohup ./main -c configs/RTX3060-hbm.conf -t build/trace.txt > rtx3060-hbm.txt 2>&1 &
nohup ./main-pim -c configs/RTX3060-pim.conf -t build/trace.txt > rtx3060-pim.txt 2>&1 &
nohup ./main-pim-fsm -c configs/RTX3060-pim.conf -t build/trace.txt > rtx3060-pim-fsm.txt 2>&1 &
nohup ./main-pim-fsm-intra -c configs/RTX3060-pim.conf -t build/trace.txt > rtx3060-pim-fsm-intra.txt 2>&1 &

#nohup ./main -c configs/RTX3090.conf -t build/trace.txt > RTX3090-gddr6.txt 2>&1 &
#nohup ./main -c configs/RTX3090-hbm.conf -t build/trace.txt > RTX3090-hbm.txt 2>&1 &
nohup ./main-pim -c configs/RTX3090-pim.conf -t build/trace.txt > RTX3090-pim.txt 2>&1 &
nohup ./main-pim-fsm -c configs/RTX3090-pim.conf -t build/trace.txt > RTX3090-pim-fsm.txt 2>&1 &
nohup ./main-pim-fsm-intra -c configs/RTX3090-pim.conf -t build/trace.txt > RTX3090-pim-fsm-intra.txt 2>&1 &