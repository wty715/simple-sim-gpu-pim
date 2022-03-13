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
/*
bool PCU::Allocated()
{
    return allocated;
}

void PCU::Allocate(bool val)
{
    allocated = val;
}
*/
int PCU::Get_Cycles()
{
    return total_cycles;
}

CH* PCU::Get_CH()
{
    return attached_CH;
}

int PCU::Execute(int avail_cycles, int& cycles_finish)
{
    if (ins_que.empty()) {
        return 0;
    }
    while (!ins_que.empty()) {
        Instruction ins_tmp = ins_que.front();
        if (ins_tmp.comm_name == "LOAD") {
            ins_que.pop();
#ifdef OPT_INTRA
            // check if intra-channel access
            if (attached_CH->Get_CH_num() != (ins_tmp.op_num>>7)%attached_CH->Get_MC()->Get_GPU()->Get_MC_num()) {
                attached_CH->Get_MC()->Get_GPU()->Add_mem_req(MEMREQ(32*attached_CH->Get_CH_num(), 32, NULL)); // read from current channel
                attached_CH->Get_MC()->Get_GPU()->Add_mem_req(MEMREQ(32*attached_CH->Get_CH_num(), 32, NULL));
                attached_CH->Get_MC()->Get_GPU()->Add_mem_req(MEMREQ(32*attached_CH->Get_CH_num(), 32, NULL));
                attached_CH->Get_MC()->Get_GPU()->Add_mem_req(MEMREQ(32*attached_CH->Get_CH_num(), 32, NULL));
                attached_CH->Get_MC()->Get_GPU()->Add_mem_req(MEMREQ(ins_tmp.op_num, 32, NULL)); // write to corresponding channel
                attached_CH->Get_MC()->Get_GPU()->Add_mem_req(MEMREQ(ins_tmp.op_num, 32, NULL));
                attached_CH->Get_MC()->Get_GPU()->Add_mem_req(MEMREQ(ins_tmp.op_num, 32, NULL));
                attached_CH->Get_MC()->Get_GPU()->Add_mem_req(MEMREQ(ins_tmp.op_num, 32, NULL));
            }
#else
            attached_CH->Get_MC()->Get_GPU()->Add_mem_req(MEMREQ(ins_tmp.op_num, 128, NULL)); // read from 4 channels
            attached_CH->Get_MC()->Get_GPU()->Add_mem_req(MEMREQ(32*attached_CH->Get_CH_num(), 32, NULL)); // write to current channel
            attached_CH->Get_MC()->Get_GPU()->Add_mem_req(MEMREQ(32*attached_CH->Get_CH_num(), 32, NULL));
            attached_CH->Get_MC()->Get_GPU()->Add_mem_req(MEMREQ(32*attached_CH->Get_CH_num(), 32, NULL));
            attached_CH->Get_MC()->Get_GPU()->Add_mem_req(MEMREQ(32*attached_CH->Get_CH_num(), 32, NULL));
#endif
#ifdef ASSERTED
            assert(ins_que.empty() == true); // the last command must be LOAD
#endif
            cycles_finish = total_cycles;
            last_cycles = total_cycles = 0; // reset cycle counter
            return 1; // processed 1 step
        }
        else {
            if (total_cycles+ins_tmp.comm_cycles-last_cycles > avail_cycles) {
                last_cycles = total_cycles;
                return 0; // processed 0 step
            }
            else {
                total_cycles += ins_tmp.comm_cycles;
                ins_que.pop();
            }
        }
    }
#ifdef ASSERTED
    assert(false); // this situation should not appear
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