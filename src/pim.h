#ifndef _PIM_H_
#define _PIM_H_

#include <queue>
#include "gpu.h"

class PCU;
class CH;

class PCU
{
    public:
        PCU() {}
        PCU(CH* ch) : attached_CH(ch), total_cycles(0) {}

        void Add_Ins(Instruction);
        int Get_pending_ins_num();
        int Get_Cycles();
        CH* Get_CH();
        int Execute();

    private:
        CH* attached_CH;
        std::queue<Instruction> ins_que;
        int total_cycles;
};

class CH
{
    public:
        CH() {}
        CH(MC* mc, int Npcu, int ch) : attached_MC(mc), PCU_num(Npcu), CH_num(ch) {
            PCUs = (PCU*)operator new(sizeof(PCU)*PCU_num);
            for (int i=0; i<PCU_num; ++i) {
                new(PCUs+i) PCU(this);
            }
        }

        PCU* Availiable();
        MC* Get_MC();
        int Get_CH_num();
        int Get_PCU_num();
    
        PCU* PCUs;

        ~CH() {
            delete PCUs;
        }
        
    private:
        MC* attached_MC;
        int PCU_num;
        int CH_num;
};

#endif