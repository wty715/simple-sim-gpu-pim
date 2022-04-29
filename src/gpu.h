#ifndef _GPU_H_
#define _GPU_H_

#include <queue>
#include <string>
#include <vector>

class MEMREQ;
class Instruction;
class SP;
class Thread;
class WARP;
class SM;
class MC;
class GPU;

class MEMREQ
{
    public:
        MEMREQ() {}
        MEMREQ(int a, int b, WARP* c) : addr(a), size(b), attached_WARP(c) {}
        int addr;
        int size;
        WARP* attached_WARP;
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
    ADD, SUB, DIV, MUL, MOD, BIT, LOAD, MAX
};

class Instruction
{
    public:
        Instruction(Command c, std::string s, int o, int n) : comm(c), comm_name(s), op_num(o), comm_cycles(n) {}
        Instruction(const Instruction& ins) :comm(ins.comm), comm_name(ins.comm_name), op_num(ins.op_num), comm_cycles(ins.comm_cycles) {}

        Command comm;
        std::string comm_name;
        int op_num;
        int comm_cycles;
};

class SP
{
    public:
        SP() : total_cycles(0), attached_warp(NULL), thr(NULL) {}
        
        void Set_Warp(WARP*);
        void Set_Thread(Thread*);
        WARP* Get_Warp();
        bool Execute();
        int Get_Cycles();

    private:
        int total_cycles;
        WARP* attached_warp;
        Thread* thr;
};

class Thread
{
    public:
        Thread() : st(States::HALT), allocated_SP(NULL) {}

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
        WARP() : st(States::HALT), attached_SM(NULL) {}

        void Set_SM(SM*);
        void Notified(Thread*, Instruction);
        void Add_Ins(Instruction, int, int);
        int Get_pending_ins_num();
        States Get_State();
        void Set_State(States);
        void Set_SP(int, SP*);

    private:
        void Aggregate();

        States st;
        Thread Thr[32]; // 1 warp = 32 threads
        MEMREQ mem_reqs[32];
        SM* attached_SM;
};

class SM
{
    public:
        SM() {}
        SM(int spnum, GPU* gpu) : attached_GPU(gpu) {
            SPs = new SP[spnum];
        }

        bool Schedule(std::vector<Instruction>*, int);
        GPU* Get_GPU();
        bool Execute();
        int Get_Cycles();

        ~SM() {
            delete SPs;
        }

    private:
        void Allocate_Warp(WARP*, std::vector<Instruction>*, int);

        std::vector<WARP*> warps;
        GPU* attached_GPU;
        SP* SPs;
};

class MC
{
    public:
        MC() {}
        MC(int ch, GPU* gpu) : CH_num(ch), consumed_BW(0), attached_GPU(gpu), last_req_num(0), thres_req_num(0), avg_times(0) {}

        void Add_Queue(MEMREQ);
        int Execute(); // return request num and empty queue
        int Execute(int, int, int&); // return processed num and bw consumption
        GPU* Get_GPU();
        int Get_CH_num();
        int Get_Consumed_BW();
        int Get_pending_req();

    private:
        void Clear();

        int CH_num;
        int consumed_BW;
        GPU* attached_GPU;
        std::queue<MEMREQ> req_que;

        int last_req_num;
        int thres_req_num;
        int avg_times;
};

class GPU
{
    public:
        GPU() {}
        GPU(int sm, int sp, int mc, int bw, int fre) : SM_num(sm), SP_per_SM(sp), mem_ctrller_num(mc), mem_bandwidth(bw), Core_freq(fre) {
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
            delete SMs;
            delete MCs;
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

#endif