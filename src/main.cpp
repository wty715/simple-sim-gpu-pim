#include <iostream>
#include <fstream>
#include "gpu.h"
#define RTX2060 0

using namespace std;

#ifdef RTX2060
    const int core_freq = 1680; // MHz
    const int mem_CH = 6;   // 192 bit
    const int mem_bw = 336; // GB/s
    const int SM_num = 30;
    const int SPinSM = 64;
#endif

#ifdef RTX3090
    const int core_freq = 1695; // MHz
    const int mem_CH = 12;  // 384 bit
    const int mem_bw = 936; // GB/s
    const int SM_num = 82;
    const int SPinSM = 128;
#endif

int main()
{
    GPU gpu(SM_num, SPinSM, mem_CH, mem_bw, core_freq);
    
    return 0;
}