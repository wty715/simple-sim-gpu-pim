#include <iostream>
#include <fstream>
#include <cstring>
#include <assert.h>
#include "gpu.h"
#include "pim.h"

using namespace std;

#ifdef RTX2060
    int core_freq = 1680; // MHz
    #ifdef ENPIM
        const int npcu = 8; // per channel
        const int mem_CH = 32;  // 2048 bit
        int mem_bw = 614; // GB/s
    #else
        const int mem_CH = 6;   // 192 bit
        int mem_bw = 336; // GB/s
    #endif
    const int SM_num = 30;
    const int SPinSM = 64;
    const int parallel = 8;
    
#elif defined(RTX3090)
    int core_freq = 1695; // MHz
    #ifdef ENPIM
        const int npcu = 8; // per channel
        const int mem_CH = 64;  // 4096 bit
        int mem_bw = 1228; // GB/s
    #else
        const int mem_CH = 12;  // 384 bit
        int mem_bw = 936; // GB/s
    #endif
    const int SM_num = 82;
    const int SPinSM = 128;
    const int parallel = 8;
#else
    int core_freq;
    int mem_CH;
    int mem_bw;
    int SM_num;
    int SPinSM;
    int parallel;
    int npcu;
#endif

const std::string command_name[int(Command::MAX)] = {
    "ADD", "SUB", "DIV", "MUL", "MOD", "BITOPS", "LOAD"
};
const int Ins_Cycles[int(Command::MAX)-1] = 
{
    4, 4, 120, 16, 140, 4
};
const int PIM_Ins_Amp = 10;

void split(const string& s, vector<string>& tokens, const string& delimiters = " ") {
    string::size_type lastPos = s.find_first_not_of(delimiters, 0);
    string::size_type pos = s.find_first_of(delimiters, lastPos);
    while (string::npos != pos || string::npos != lastPos) {
        tokens.emplace_back(s.substr(lastPos, pos - lastPos));
        lastPos = s.find_first_not_of(delimiters, pos);
        pos = s.find_first_of(delimiters, lastPos);
    }
}

int main(int argc, char* argv[])
{
    GPU gpu(SM_num, SPinSM, mem_CH, mem_bw, core_freq);
    if (argc < 5) {
        cerr << "Usage: main -c [config file] -t [trace file]" << endl;
        return 1;
    }
    assert(strcmp(argv[1], "-c") == 0 && strcmp(argv[3], "-t") == 0);

    ifstream configin(argv[2], ios::in), tracein(argv[4], ios::in);
    if (!configin.is_open() || !tracein.is_open()) {
        cerr << "Cannot find config file or trace file" << endl;
        return 1;
    }

#ifdef ENPIM
    // link gpu with pim
    CH *chs = (CH*)operator new(sizeof(CH)*gpu.Get_MC_num());
    for (int i=0; i<gpu.Get_MC_num(); ++i) {
        new(chs+i) CH(&gpu.MCs[i], npcu, i);
#ifdef ASSERTED
        assert(gpu.MCs[i].Get_CH_num() == chs[i].Get_CH_num());
#endif
    }
#endif

    string outname;
#ifdef RTX2060
    outname = "result2060";
#elif defined(RTX3090)
    outname = "result3090";
#endif
#ifdef ENPIM
    outname += "-pim";
#endif
    outname += ".txt";
    ofstream res_out(outname, ios::out);
    string line;
    vector<string> lineitems;
    int linenum = 0, paraindex = 0;
    vector<Instruction> ins_set[32/parallel];
    int last_cycles = 0;
    while(getline(tracein, line)) {
        ++linenum;
        lineitems.clear();
        split(line, lineitems);
        if (lineitems[0] == command_name[int(Command::ADD)]) {
            ins_set[paraindex].emplace_back(Instruction(Command::ADD, command_name[int(Command::ADD)], stoi(lineitems[1]), Ins_Cycles[int(Command::ADD)]));
        }
        else if (lineitems[0] == command_name[int(Command::SUB)]) {
            ins_set[paraindex].emplace_back(Instruction(Command::SUB, command_name[int(Command::SUB)], stoi(lineitems[1]), Ins_Cycles[int(Command::SUB)]));
        }
        else if (lineitems[0] == command_name[int(Command::DIV)]) {
            ins_set[paraindex].emplace_back(Instruction(Command::DIV, command_name[int(Command::DIV)], stoi(lineitems[1]), Ins_Cycles[int(Command::DIV)]));
        }
        else if (lineitems[0] == command_name[int(Command::MUL)]) {
            ins_set[paraindex].emplace_back(Instruction(Command::MUL, command_name[int(Command::MUL)], stoi(lineitems[1]), Ins_Cycles[int(Command::MUL)]));
        }
        else if (lineitems[0] == command_name[int(Command::MOD)]) {
            ins_set[paraindex].emplace_back(Instruction(Command::MOD, command_name[int(Command::MOD)], stoi(lineitems[1]), Ins_Cycles[int(Command::MOD)]));
        }
        else if (lineitems[0] == command_name[int(Command::BIT)]) {
            ins_set[paraindex].emplace_back(Instruction(Command::BIT, command_name[int(Command::BIT)], stoi(lineitems[1]), Ins_Cycles[int(Command::BIT)]));
        }
        else if (lineitems[0] == command_name[int(Command::LOAD)]) {
            ins_set[paraindex].emplace_back(Instruction(Command::LOAD, command_name[int(Command::LOAD)], stoi(lineitems[1]), 0));
#ifdef ENPIM
            // check PIM first
            int involved_CH = (ins_set[paraindex].back().op_num>>7)%gpu.Get_MC_num();
            PCU* selected = chs[involved_CH].Availiable();
            // PIM task must already finished
            if (selected && chs[involved_CH].PCUs[0].Get_Cycles() <= gpu.SMs[0].Get_Cycles()) {
                for (auto ins: ins_set[paraindex]) {
                    // amplified instruction cycle
                    selected->Add_Ins(Instruction(ins.comm, ins.comm_name, ins.op_num, ins.comm_cycles*PIM_Ins_Amp));
                }
                ins_set[paraindex].clear();
                continue;
            }
#endif
            ++paraindex;
            if (paraindex == 32/parallel) {
                paraindex = 0;
                int i;
                for (i=0; i<gpu.Get_SM_num(); ++i) {
                    if (gpu.SMs[i].Schedule(ins_set, parallel)) {
                        break;
                    }
                }
                if (i == gpu.Get_SM_num()) {
                    // gpu execution
                    for (int j=0; j<gpu.Get_SM_num(); ++j) {
#ifdef DEBUGGING
                        cout << "SM " << j << " executing..." << endl;
#endif
                        while (gpu.SMs[j].Execute());
                    }
                    // check cycles
                    int tmp_cycles = gpu.SMs[0].Get_Cycles();
                    res_out << endl << "Now instruction cycles: " << tmp_cycles << endl;
                    if (tmp_cycles % 110400 == 0) {
                        cout << "Total " << tmp_cycles << " cycles executed." << endl;
                    }
                    // throughput
                    int overall_throughput = 0;
                    // consume memory bandwidth according to limitation
                    for (int j=0; j<gpu.Get_MC_num(); ++j) {
#ifdef REQUESTED
                        res_out << "Channel " << j << " receive 32B reqs: " << gpu.MCs[j].Execute() << endl;
#else
#ifdef ENPIM
                        // PIM execution
                        for (int k=0; k<chs[j].Get_PCU_num(); ++k) {
                            chs[j].PCUs[k].Execute();
                        }
#endif
                        int processed_reqs = gpu.MCs[j].Execute(tmp_cycles - last_cycles);
                        res_out << "Channel " << j << " processed 32B reqs: " << processed_reqs << endl;
                        int ava_mem_bw = mem_bw*1000*(tmp_cycles-last_cycles)/gpu.Get_MC_num()/core_freq; // in bytes
                        res_out << "Bandwidth " << j << " utilization: " << processed_reqs*32*1000/ava_mem_bw << "%。" << endl;
                        // 4 = 32*1024/64/128
                        res_out << "Throughput " << j << ": " << processed_reqs*4*core_freq/(tmp_cycles-last_cycles) << "KH/s" << endl;
                        overall_throughput += processed_reqs*4*core_freq/(tmp_cycles-last_cycles);
#endif
                    }
                    res_out << "Overall: " << overall_throughput << "KH/s" << endl;
                    // record cycles
                    last_cycles = tmp_cycles;
                    assert(gpu.SMs[0].Schedule(ins_set, parallel) == true);
                }
                for (int i=0; i<32/parallel; ++i) {
                    ins_set[i].clear();
                }
            }
        }
        else {
            cout << "Command not defined at line" << linenum << ": " << line << endl;
            cout << "Terminated." << endl;
            break;
        }
    }
    cout << "Trace file executed." << endl;

    return 0;
}