# A simple Ethereum hashrate simulator for GPU-PIM integrated platform

A simple hashrate simulator that can simulate hashrate for different NVIDIA graphic cards.

All the configurations of different graphic cards can be found in ***/configs***.
Config file format:
+ core_freq: the core frequency (in MHz)
+ mem_CH: the number of memory channels
+ mem_BW: the memory bandwidth (in GB/s)
+ SM_num: the number of SMs
+ SPinSM: the number of SPs in one SM
+ (optional) npcu: the number of PCUs in one memory channel
+ (optional) pcu_freq: the PCU working frequency (in MHz)

---

Firstly, you need to compile with $./make
After compilation, 6 executable files will be generated:
+ main                  (the basic simulator that can only simulate GPU-only approach)
+ main-dbg              (output all the debug information)
+ main-pim              (the simulator that can simulate GPU-PIM platform with task scheduling technique)
+ main-pim-fsm          (the simulator with task scheduling and finite-state-machine techniques)
+ main-pim-fsm-intra    (the fully optimized simulator)
+ main-req              (only output the memory request number that each channel received)

You can download the 15-minute memory access trace from https://drive.google.com/file/d/1aZpdZC_FnDau0QcIDl2j8cc5BjU9hE2c/view?usp=sharing. Then, you need to use ***/build/filterto_trace.py*** to translate raw memory accesses to a trace file named ***trace.txt***.

After trace file generation, you can run all the experiments by $./run.sh
The logs are recorded in the root directory with ***.txt*** suffix. The results will be generated in ***/configs*** with ***(config-file-name).txt***.
Then you can use ***/configs/analyze.sh*** to analyze the results and the statistic information will be written to ***(config-file-name).txt-result.txt***.

---

If you find this tool helpful to your research, please cite the paper:
T Wang, Z Shen, Z Shao. Co-Mining: A Processing-in-Memory Assisted Framework for Memory-Intensive PoW Acceleration[C]. LCTES 2022.