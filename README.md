# A simple Ethereum hashrate simulator for GPU-PIM integrated platform

A simple hashrate simulator that can simulate hashrate for different NVIDIA graphic cards.

All the configurations of different graphic cards can be found in ***configs***.

Config file format:
+ core_freq: the core frequency (in MHz)
+ mem_CH: the number of memory channels
+ mem_BW: the memory bandwidth (in GB/s)
+ SM_num: the number of SMs
+ SPinSM: the number of SPs in one SM
+ (optional) npcu: the number of PCUs in one memory channel
+ (optional) pcu_freq: the PCU working frequency (in MHz)

---

### Getting started guide:

We only need GCC compiler and MAKE tools for simulation.  
We use python3 for result analysis.

Firstly, you need to compile with **`$make`**.  
After compilation, 4 executable files will be generated:
+ main                  (the basic simulator that can only simulate GPU-only approach)
+ main-pim              (the simulator that can simulate GPU-PIM platform with task scheduling technique)
+ main-pim-fsm          (the simulator with task scheduling and finite-state-machine techniques)
+ main-pim-fsm-intra    (the fully optimized simulator)

If you want to read some debug information, you can compile debug version by **`$make debug`**.  
One more executable file will be generated:
+ main-dbg              (output all the debug information)

If you only want to check the memory channel overload, you can compile with **`$make req`**.  
One more executable file will be generated:
+ main-req              (only output the memory request number that each channel received)

### Step-by-step instructions:

You can download the 15-minute memory access trace from https://drive.google.com/file/d/1aZpdZC_FnDau0QcIDl2j8cc5BjU9hE2c/view?usp=sharing.  
The downloaded ***filtered.log*** should be moved to ***build*** directory: **`$mv filtered.log ./build/`**.  
Then, you need to enter ***build*** directory **`$cd build/`** and use **`$python3 filterto_trace.py`** to translate raw memory accesses to a trace file named ***trace.txt***.  
***NOTE:*** The trace file may consume ~5GB disk space. Make sure that you have enough disk space.

After trace file generation, you can run all the experiments by **`$./run.sh`**.  
Please make sure that ***trace.txt*** exists in the ***build*** directory.  
Note that all experiments may take more than 1 hours.  
*For your reference: I7-10700K@5GHz with DDR4_32G@3000MHz finishes all experiments in* ***30min***.

***NOTE:*** If you want to run an individual experiment without ***run.sh***, you can use **`./[exec file] -c [config file] -t [trace file]`** to generate results.

The logs are recorded in the root directory with ***.txt*** suffix. The results will be generated in ***configs*** with ***(config-file-name).txt***.  
Then you can use **`$./analyze.sh`** in ***configs*** directory to analyze the results and the statistic information will be written to ***(config-file-name).txt-result.txt***.

***TIPS:*** if you want to get all the results faster, you may generate a shrunken trace file by **`$python3 build/filterto_trace.py small`**.  
It only generates a half trace file (~2.5GB), but the results may not be exactly the same with what in papers.

### Results explanation:

For example: (the results in **RTX2060-pim.conf.txt-result.txt**)

```c
PCU = 146 :                                 // number of PCU cores activated
GPU bandwidth: 33897.5 Bytes                // GPU bandwidth consumption during 1 calculation step
GPU BW percentage: 35.68251796099897 %      // GPU bandwidth consumption percentage (GPU_BW/bw_limit)%
Bandwidth limit: 94997.5 Bytes              // Bandwidth limit during 1 calculation step
PIM bandwidth: 18073.65625 Bytes            // PIM bandwidth consumption during 1 calculation step
PIM BW percentage: 19.025401984262743 %     // PIM bandwidth consumption percentage (PIM_BW/bw_limit)%
Overall bandwidth: 51971.15625 Bytes        // Total bandwidth consumption during 1 calculation step (GPU_BW+PIM_BW)
Overall BW percentage: 54.70791994526172 %  // Total bandwidth consumption percentage ((GPU_BW+PIM_BW)/bw_limit)%
GPU throughput: 25581.125 KH/s              // GPU hash calculation throughput (mining hashrate)
PIM throughput: 1574.546875 KH/s            // PIM hash calculation throughput (mining hashrate)
Overall throughput: 27155.671875 KH/s       // The overall mining hashrate (GPU_throughput + PIM_throughput)
```

Note that items `(PIM_BW, PIM_BW_%, Overall_BW, Overall_BW_%, PIM_throughput, Overall_throughput)` only appears when PIM-related parameters are set in config file.

We also need to transfer bandwidth consumption percentage into **(GB/s)**, which is calculated by ***BW_consumption_percentage * mem_BW*** in config file. For example, in the above result, we equip RTX2060 with **614GB/s** HBM-PIM, which means it finally consumes **54.7% * 614GB/s = 335.858GB/s** bandwidth in total.

---

If you find this tool helpful for your research, please cite the paper:  
T Wang, Z Shen, Z Shao. Co-Mining: A Processing-in-Memory Assisted Framework for Memory-Intensive PoW Acceleration[C]. LCTES 2022.
