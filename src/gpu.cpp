#include "gpu.h"
#include <assert.h>

std::string command_name[int(Command::MAX)] = {
    "ADD", "SUB", "DIV", "MUL", "MOD", "LOAD"
};

const int Ins_Cycles[int(Command::MAX)-1] = 
{
    4, 4, 120, 16, 160
};

void Thread::Add_Ins(Instruction ins)
{
    ins_que.push(ins);
}

int Thread::Get_pending_ins_num()
{
    return ins_que.size();
}

Instruction Thread::Get_Ins()
{
    Instruction tmp = ins_que.front();
    ins_que.pop();
    return tmp;
}

States Thread::Get_State()
{
    return st;
}

void Thread::Set_State(States st)
{
    this->st = st;
}

void Thread::Set_SP(SP* sp)
{
    allocated_SP = sp;
}

SP* Thread::Get_SP()
{
    return allocated_SP;
}

void SP::Set_Warp(WARP* warp)
{
    attached_warp = warp;
}

void SP::Set_Thread(Thread* th)
{
    thr = th;
}

WARP* SP::Get_Warp()
{
    return attached_warp;
}

void SP::Execute()
{
    Instruction ins = thr->Get_Ins();
    if (ins.comm == Command::LOAD)
    {
        thr->Set_State(States::WAIT);
        attached_warp->Notified(thr, ins);
        total_cycles += 4; // 4 cycles for context switch
    }
    else
    {
        total_cycles += ins.comm_cycles;
    }
}

void WARP::Set_SM(SM* sm)
{
    attached_SM = sm;
}

void WARP::Aggregate()
{
    for (int i=0; i<32; ++i) {
        for (int j=i+1; j<32; ++j) {
            if (mem_reqs[i].addr > mem_reqs[j].addr) {
                std::swap(mem_reqs[i], mem_reqs[j]);
            }
        }
    }
    for (int i=0; i<31; ++i) {
        if (mem_reqs[i].addr == 0) continue;
        if (mem_reqs[i].addr + mem_reqs[i].size >= mem_reqs[i+1].addr) {
            mem_reqs[i].size = mem_reqs[i+1].addr + mem_reqs[i+1].size - mem_reqs[i].addr;
            mem_reqs[i+1].addr = mem_reqs[i+1].size = 0;
            std::swap(mem_reqs[i], mem_reqs[i+1]);
        }
    }
    for (int i=0; i<32; ++i) {
        if (mem_reqs[i].addr != 0) {
            GPU* gpu = attached_SM->Get_GPU();
            gpu->Add_mem_req(mem_reqs[i]);
        }
    }
}

void WARP::Notified(Thread* thr, Instruction ins)
{
    int thr_id = -1;
    int i;
    for (i=0; i<32; ++i)
    {
        if (&Thr[i] == thr)
        {
            thr_id = i;
            break;
        }
    }
    assert(thr_id != -1); // make sure this threads is in this warp

    // add mem access req to the queue
    mem_reqs[thr_id].addr = ins.op_num;
    mem_reqs[thr_id].size = 128;

    // check if all threads are waiting for memory request
    for (i=0; i<32; ++i)
    {
        if (Thr[i].Get_State() == States::EXEC) break;
    }
    if (i == 32)
    {
        // aggregate all mem reqs
        Aggregate();

        // give up SP resources
        st = States::WAIT;
        for (int j=0; j<32; ++j) {
            SP* tmp = Thr[j].Get_SP();
            Thr[j].Set_SP(NULL);
            tmp->Set_Warp(NULL);
            tmp->Set_Thread(NULL);
        }
    }
}

void WARP::Add_Ins(Instruction ins)
{
    for (int i=0; i<32; ++i) {
        Thr[i].Add_Ins(ins);
    }
}

States WARP::Get_State()
{
    return st;
}

void WARP::Set_State(States next_st)
{
    st = next_st;
    for (int i=0; i<32; ++i) {
        Thr[i].Set_State(st);
    }
}

void WARP::Set_SP(int thr_id, SP* sp)
{
    Thr[thr_id].Set_SP(sp);
    sp->Set_Thread(&Thr[thr_id]);
    sp->Set_Warp(this);
}

void SM::Allocate_Warp(Instruction* ins_set)
{
    warps.push_back(WARP());
    for (int i=0; i<sizeof(ins_set); ++i) {
        warps.back().Add_Ins(ins_set[i]);
    }
}

GPU* SM::Get_GPU()
{
    return attached_GPU;
}

void SM::Schedule(Instruction* ins_set)
{
    for (int i=0; i<(attached_GPU->Get_SP_num()>>5); i+=32) {
        if (SPs[i].Get_Warp() == NULL) {
            bool found_exec = false;
            for (std::vector<WARP>::iterator it = warps.begin(); it!=warps.end(); it++) {
                if (it->Get_State() == States::HALT) {
                    it->Set_State(States::EXEC);
                    it->Set_SM(this);
                    for (int j=0; j<32; ++j) {
                        it->Set_SP(j, &SPs[i+j]);
                    }
                    found_exec = true;
                    break;
                }
            }
            if (!found_exec) {
                Allocate_Warp(ins_set);
                assert(warps.end()->Get_State() == States::HALT);
                warps.end()->Set_State(States::EXEC);
                warps.end()->Set_SM(this);
                for (int j=0; j<32; ++j) {
                    warps.end()->Set_SP(j, &SPs[i+j]);
                }
            }
            break;
        }
    }
}

void SM::Execute()
{
    for (int i=0; i<attached_GPU->Get_SP_num(); ++i) {
        SPs[i].Execute();
    }
}

int GPU::Get_MC_num()
{
    return mem_ctrller_num;
}

int GPU::Get_SM_num()
{
    return SM_num;
}

int GPU::Get_SP_num()
{
    return SP_per_SM;
}

int GPU::Get_BandWidth()
{
    return mem_bandwidth;
}

int GPU::Get_Core_freq()
{
    return Core_freq;
}

void GPU::Add_mem_req(MEMREQ mem_req)
{
    // 32B per channel
    int base = (mem_req.addr>>5)%mem_ctrller_num;
    for (int i=0; i<((mem_req.size+31)>>5); ++i) {
        MCs[(base+i)%mem_ctrller_num].Add_Queue(MEMREQ((((mem_req.addr>>5)/mem_ctrller_num)<<5)+(mem_req.addr&0x1f), 32));
    }
}

void MC::Add_Queue(MEMREQ mem_req)
{
    req_que.push(mem_req);
}

int MC::Execute()
{
    double time_in_ns = (double)req_que.front().size / ((double)attached_GPU->Get_BandWidth() / attached_GPU->Get_MC_num());
    double cycles = (double)attached_GPU->Get_Core_freq()/1000 * time_in_ns;
    return (cycles+0.984375);
}
