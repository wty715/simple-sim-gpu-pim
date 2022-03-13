#include "gpu.h"
#ifdef ASSERTED
    #include <assert.h>
#endif

extern int core_freq;
extern int mem_BW;

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
    if (ins_que.empty()) {
        return Instruction(Command::MAX, "", 0, 0);
    }
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

bool SP::Execute() // return false: cannot execute further
{
#ifdef ASSERTED
    assert(thr != NULL && thr->Get_State() == States::EXEC);
#endif
    Instruction ins = thr->Get_Ins();
    if (ins.comm == Command::MAX) {
        thr->Set_State(States::HALT);
        attached_warp->Notified(thr, ins);
        total_cycles += 4;
#ifdef ASSERTED
        assert(false); // this scenario should not appear
#endif
    }
    else if (ins.comm == Command::LOAD) {
        thr->Set_State(States::WAIT);
        attached_warp->Notified(thr, ins);
        total_cycles += 4; // 4 cycles for context switch
        return false;
    }
    else {
        total_cycles += ins.comm_cycles;
        return true;
    }
}

int SP::Get_Cycles()
{
    return total_cycles;
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
            attached_SM->Get_GPU()->Add_mem_req(mem_reqs[i]);
        }
    }
}

void WARP::Notified(Thread* thr, Instruction ins)
{
    int thr_id = -1;
    int i;
    for (i=0; i<32; ++i) {
        if (&Thr[i] == thr) {
            thr_id = i;
            break;
        }
    }
#ifdef ASSERTED
    assert(thr_id != -1); // make sure this threads is in this warp
#endif

    if (ins.comm == Command::LOAD) {
        // add mem access req to the queue
        mem_reqs[thr_id].addr = ins.op_num;
        mem_reqs[thr_id].size = 128;
        mem_reqs[thr_id].attached_WARP = this;
    }
#ifdef ASSERTED
    else {
        assert(ins.comm == Command::MAX);
    }
#endif

    // check if all threads are waiting for memory request
    for (i=0; i<32; ++i) {
        if (Thr[i].Get_State() == States::EXEC) break;
    }
    if (i == 32) {
        // aggregate all mem reqs
        Aggregate();

        // give up SP resources
        st = States::WAIT;
        for (int j=0; j<32; ++j) {
            Thr[j].Get_SP()->Set_Warp(NULL);
            Thr[j].Get_SP()->Set_Thread(NULL);
            Thr[j].Set_SP(NULL);
        }

        // make sure no further instructions
#ifdef ASSERTED
        assert(Get_pending_ins_num() == 0);
#endif
    }
}

void WARP::Add_Ins(Instruction ins, int parallel, int index)
{
    for (int i=0; i<parallel; ++i) {
        Thr[index*parallel+i].Add_Ins(ins);
    }
}

int WARP::Get_pending_ins_num()
{
    return Thr[0].Get_pending_ins_num();
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

void SM::Allocate_Warp(WARP* cur, std::vector<Instruction>* ins_set, int parallel)
{
    if (cur == NULL) {
        warps.emplace_back(new WARP());
        for (int i=0; i<32/parallel; ++i) {
            for (auto ins: ins_set[i]) {
                warps.back()->Add_Ins(ins, parallel, i);
            }
        }
    }
    else {
        for (int i=0; i<32/parallel; ++i) {
            for (auto ins: ins_set[i]) {
                cur->Add_Ins(ins, parallel, i);
            }
        }
    }
}

GPU* SM::Get_GPU()
{
    return attached_GPU;
}

bool SM::Schedule(std::vector<Instruction>* ins_set, int parallel)
{
    for (int i=0; i<attached_GPU->Get_SP_num(); i+=32) {
        if (SPs[i].Get_Warp() == NULL) {
            bool found_exec = false;
            for (auto warp : warps) {
                if (warp->Get_State() == States::HALT || warp->Get_State() == States::WAIT) {
#ifdef ASSERTED
                    assert(warp->Get_pending_ins_num() == 0); // must have no pending ins
#endif
                    Allocate_Warp(warp, ins_set, parallel); // add ins to existing warp
                    warp->Set_State(States::EXEC);
                    warp->Set_SM(this);
                    for (int j=0; j<32; ++j) {
                        warp->Set_SP(j, &SPs[i+j]);
                    }
                    found_exec = true;
                    break;
                }
            }
            if (!found_exec) {
                Allocate_Warp(NULL, ins_set, parallel); // add ins to new warp
#ifdef ASSERTED
                assert(warps.back()->Get_State() == States::HALT);
#endif
                warps.back()->Set_State(States::EXEC);
                warps.back()->Set_SM(this);
                for (int j=0; j<32; ++j) {
                    warps.back()->Set_SP(j, &SPs[i+j]);
                }
            }
            return true;
        }
    }
    return false;
}

bool SM::Execute()
{
    bool ret = false;
    for (int i=0; i<attached_GPU->Get_SP_num(); ++i) {
        ret = (ret | SPs[i].Execute());
    }
    return ret;
}

int SM::Get_Cycles()
{
    return SPs[0].Get_Cycles();
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
#ifdef ASSERTED
    assert(mem_req.addr%128 == 0 || mem_req.attached_WARP == NULL && mem_req.addr%32 == 0);
#endif
    int base = (mem_req.addr>>5)%mem_ctrller_num;
    for (int i=0; i<((mem_req.size+31)>>5); ++i) {
        MCs[(base+i)%mem_ctrller_num].Add_Queue(MEMREQ((((mem_req.addr>>5)/mem_ctrller_num)<<5)+(mem_req.addr&0x1f), 32, mem_req.attached_WARP));
    }
}

void MC::Add_Queue(MEMREQ mem_req)
{
    req_que.push(mem_req);
}

int MC::Execute()
{
    int reqs = req_que.size();
    Clear();
    return (reqs);
}

int MC::Execute(int cycles, int working_PCUs)
{
    // each PCU will occupy 2 banks (8MB per bank) / total 32 banks (16 banks with 8M, 16 banks with 16M) per channel (48*8M)
    // with FSM, each PCU can only occupy 1 bank (EVEN or ODD banks)
    int ava_mem_BW = mem_BW*1000*cycles/attached_GPU->Get_MC_num()/core_freq*(48-working_PCUs)/48; // in bytes
    int processed_reqs = 0;
    consumed_BW = 0;
    while(!req_que.empty()) {
        if (consumed_BW + req_que.front().size > ava_mem_BW) {
            break;
        }
        consumed_BW += req_que.front().size;
        if (req_que.front().attached_WARP != NULL) {
            ++processed_reqs;
        }
        req_que.pop();
    }
    return processed_reqs;
}

GPU* MC::Get_GPU()
{
    return attached_GPU;
}

int MC::Get_CH_num()
{
    return CH_num;
}

int MC::Get_Consumed_BW()
{
    return consumed_BW;
}

int MC::Get_pending_req()
{
    return req_que.size();
}

void MC::Clear()
{
    std::queue<MEMREQ> empty;
    std::swap(empty, req_que);
}