#ifndef _GPU_H_
#define _GPU_H_

#include <queue>
#include <string>

class MEMREQ
{
    public:
        MEMREQ() {}
        MEMREQ(int a, int b) : req_addr(a), req_size(b) {}
        int req_addr;
        int req_size;
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

const int Ins_Cycles[int(Command::MAX)] = 
{

};

class Instruction
{
    public:
        Command comm;
        std::string comm_name;
        int op_num;
        int comm_cycles;
};


class Thread
{
    public:
        Thread();

        void Add_Ins(Instruction);
        int Get_pending_ins_num();
        Instruction Get_Ins();
        void Set_State(States);

    private:
        SP* allocated_SP;
        std::queue<Instruction> ins_que;
        States st;
};

class SP
{
    public:
        SP();

        void Execute();

    private:
        int total_cycles;
        WARP* attached_warp;
        Thread* thr;
        MEMREQ mem_req;
};

class WARP
{
    public:
        WARP();

        void Aggregate();
        void Notified(Thread*, Instruction);

    private:
        Thread* Thr[32]; // 1 warp = 32 threads
        std::queue<MEMREQ> mem_reqs;
        States st[32];
};

class SM
{
    public:
        SM();

        void Schedule();

    private:
        int SP_per_SM;
        std::queue<WARP> warps;
        SP* SPs;
};

class GPU
{
    public:
        GPU();
        

    private:
        int SM_num;
        SM* SMs;
        int mem_ctrller_num;
        MC* MCs;

};

class MC
{
    public:
        MC();

        void Aggregate();

    private:
        int CH_num;
};

#endif