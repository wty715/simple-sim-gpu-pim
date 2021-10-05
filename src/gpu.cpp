#include "gpu.h"
#include <assert.h>

Thread::Thread()
{

}

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

void Thread::Set_State(States st)
{
    this->st = st;
}

SP::SP()
{
    
}

void SP::Execute()
{
    Instruction ins = thr->Get_Ins();
    if (ins.comm == Command::LOAD)
    {
        thr->Set_State(States::WAIT);
        attached_warp->Notified(thr, ins);
    }
    else
    {
        total_cycles += ins.comm_cycles;
    }
}

WARP::WARP()
{
    for (int i=0; i<32; ++i)
    {
        Thr[i] = new Thread;
        st[i] = States::HALT;
    }
}

void WARP::Notified(Thread* thr, Instruction ins)
{
    int thr_id = -1;
    int i;
    for (int i=0; i<32; ++i)
    {
        if (Thr[i] == thr)
        {
            thr_id = i;
            break;
        }
    }
    assert(thr_id != -1); // make sure this threads is in this warp
    st[thr_id] = States::WAIT;

    // check if all threads are waiting for memory request
    for (i=0; i<32; ++i)
    {
        if (st[i] == States::EXEC)
        {
            break;
        }
    }
    if (i == 32)
    {
        // add mem access req to the queue
        mem_reqs.push(MEMREQ(ins.op_num, 128));
    }
}