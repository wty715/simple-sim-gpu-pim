#include "pim.h"
#ifdef ASSERTED
    #include <assert.h>
#endif

extern int SM_num;
extern int SPinSM;
extern int mem_CH;

void PCU::Add_Ins(Instruction ins)
{
    ins_que.push(ins);
}

int PCU::Get_pending_ins_num()
{
    return ins_que.size();
}

int PCU::Get_Cycles()
{
    return total_cycles;
}

CH* PCU::Get_CH()
{
    return attached_CH;
}

int PCU::Execute()
{
    if (ins_que.empty()) {
        return 0;
    }
    while (!ins_que.empty()) {
        Instruction ins_tmp = ins_que.front();
        ins_que.pop();
        if (ins_tmp.comm_name == "LOAD") {
            // check if intra-channel access
            if (attached_CH->Get_CH_num() != (ins_tmp.op_num>>7)%attached_CH->Get_MC()->Get_GPU()->Get_MC_num()) {
                attached_CH->Get_MC()->Get_GPU()->Add_mem_req(MEMREQ(ins_tmp.op_num, 128, NULL));
                attached_CH->Get_MC()->Get_GPU()->Add_mem_req(MEMREQ(ins_tmp.op_num, 128, NULL));
            }
#ifdef NO_OPT
            // read
            attached_CH->Get_MC()->Get_GPU()->Add_mem_req(MEMREQ(ins_tmp.op_num, 128, NULL));
            // write
            attached_CH->Get_MC()->Get_GPU()->Add_mem_req(MEMREQ(ins_tmp.op_num, 128, NULL));
            // blocked
            for (int i=0; i<SM_num*SPinSM*2/mem_CH; ++i) {
                attached_CH->Get_MC()->Get_GPU()->Add_mem_req(MEMREQ(attached_CH->Get_CH_num()*32, 32, NULL));
            }
#endif
#ifdef ASSERTED
            assert(ins_que.empty() == true); // this scenario should not appear
#endif
            return 1;
        }
        else {
            total_cycles += ins_tmp.comm_cycles;
        }
    }
#ifdef ASSERTED
    assert(false); // the last command must be LOAD
#endif
}

PCU* CH::Availiable()
{
    for (int i=0; i<PCU_num; ++i) {
        if (PCUs[i].Get_pending_ins_num() == 0) {
            return &PCUs[i];
        }
    }
    return NULL;
}

MC* CH::Get_MC()
{
    return attached_MC;
}

int CH::Get_CH_num()
{
    return CH_num;
}

int CH::Get_PCU_num()
{
    return PCU_num;
}