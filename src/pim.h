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
        PCU(CH* ch) : attached_CH(ch), total_cycles(0), last_cycles(0) {}

        void Add_Ins(Instruction);
        int Get_pending_ins_num();
        /*
        bool Allocated();
        void Allocate(bool);
        */
        int Get_Cycles();
        CH* Get_CH();
        int Execute(int, int&);

    private:
        CH* attached_CH;
        std::queue<Instruction> ins_que;
        int total_cycles, last_cycles;
        // bool allocated;
};

class CH
{
    public:
        CH() {}
        CH(MC* mc, int Npcu, int ch) : attached_MC(mc), PCU_num(Npcu), CH_num(ch), working_PCUs(0) {
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
        int working_PCUs;

        ~CH() {
            delete PCUs;
        }
        
    private:
        MC* attached_MC;
        int PCU_num;
        int CH_num;
};

#endif