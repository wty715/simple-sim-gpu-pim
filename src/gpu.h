#ifndef _GPU_H_
#define _GPU_H_

#include <queue>
#include <string>
#include <vector>

class MEMREQ
{
    public:
        MEMREQ() {}
        MEMREQ(long long a, int b) : addr(a), size(b) {}
        long long addr;
        int size;
        WARP* attached_warp;
};

/* EXEC: executing commands
   WAIT: wait for memory access
   HALT: finished */
enum class States : int
{
    EXEC, WAIT, HALT, MAX
};

enum class Command : int
{
    ADD, SUB, DIV, MUL, MOD, LOAD, MAX
};

std::string command_name[int(Command::MAX)] = {
    "ADD", "SUB", "DIV", "MUL", "MOD", "LOAD"
};

const int Ins_Cycles[int(Command::MAX)-1] = 
{
    4, 4, 120, 16, 160
};

class Instruction
{
    public:
        Command comm;
        std::string comm_name;
        int op_num;
        int comm_cycles;
};

class SP
{
    public:
        SP() : total_cycles(0) {}
        
        void Set_Warp(WARP*);
        void Set_Thread(Thread*);
        WARP* Get_Warp();
        void Execute();

    private:
        int total_cycles;
        WARP* attached_warp;
        Thread* thr;
};

class Thread
{
    public:
        Thread() : st(States::HALT) {}

        void Add_Ins(Instruction);
        int Get_pending_ins_num();
        Instruction Get_Ins();
        States Get_State();
        void Set_State(States);
        void Set_SP(SP*);
        SP* Get_SP();

    private:
        SP* allocated_SP;
        std::queue<Instruction> ins_que;
        States st;
};

class WARP
{
    public:
        WARP() : st(States::HALT) {}

        void Set_SM(SM*);
        void Notified(Thread*, Instruction);
        void Add_Ins(Instruction);
        States Get_State();
        void Set_State(States);
        void Set_SP(int, SP*);

    private:
        void Aggregate();

        Thread Thr[32]; // 1 warp = 32 threads
        MEMREQ mem_reqs[32];
        SM* attached_SM;
        States st;
};

class SM
{
    public:
        SM() {}
        SM(int spnum, GPU* gpu) : attached_GPU(gpu) {
            SPs = new SP[spnum];
        }

        void Schedule(Instruction*);
        GPU* Get_GPU();
        void Execute();

    private:
        void Allocate_Warp(Instruction*);

        std::vector<WARP> warps;
        GPU* attached_GPU;
        SP* SPs;
};

class GPU
{
    public:
        GPU() {}
        GPU(int sm, int sp, int mc, int bw, int freq) : SM_num(sm), SP_per_SM(sp), mem_ctrller_num(mc), mem_bandwidth(bw), Core_freq(freq) {
            SMs = (SM*)operator new(sizeof(SM)*SM_num);
            for (int i=0; i<SM_num; ++i) {
                new(SMs+i) SM(SP_per_SM, this);
            }
            MCs = (MC*)operator new(sizeof(MC)*mem_ctrller_num);
            for (int i=0; i<mem_ctrller_num; ++i) {
                new(MCs+i) MC(i, this);
            }
        }
        int Get_SM_num();
        int Get_SP_num();
        int Get_MC_num();
        int Get_BandWidth();
        int Get_Core_freq();

        ~GPU() {
            for (int i=0; i<SM_num; ++i) {
                delete &SMs[i];
            }
            for (int i=0; i<mem_ctrller_num; ++i) {
                delete &MCs[i];
            }
        }
        
        void Add_mem_req(MEMREQ);
        
        SM* SMs;
        MC* MCs;

    private:
        int SM_num;
        int mem_ctrller_num;
        int mem_bandwidth; // in GB/s
        int SP_per_SM;
        int Core_freq; // in MHz
};

class MC
{
    public:
        MC() {}
        MC(int ch, GPU* gpu) : CH_num(ch), attached_GPU(gpu) {}

        void Add_Queue(MEMREQ);
        int Execute(); // return cycles for each mem req

    private:
        int CH_num;
        GPU* attached_GPU;
        std::queue<MEMREQ> req_que;
};

#endif