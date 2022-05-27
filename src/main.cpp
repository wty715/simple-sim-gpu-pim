#include <iostream>
#include <fstream>
#include <cstring>
#include <assert.h>
#include "gpu.h"
#include "pim.h"

// #define ENPIM 0

using namespace std;

const int parallel = 8;
int core_freq;
int mem_CH;
int mem_BW;
int SM_num;
int SPinSM;
int npcu;
int pcu_freq;

const std::string command_name[int(Command::MAX)] = {
    "ADD", "SUB", "DIV", "MUL", "MOD", "BITOPS", "LOAD"
};
const int Ins_Cycles[int(Command::MAX)-1] = 
{
    4, 4, 120, 16, 140, 4
};

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

    string conf_name;
    configin >> conf_name >> core_freq;
    assert(conf_name == "core_freq");
    configin >> conf_name >> mem_CH;
    assert(conf_name == "mem_CH");
    configin >> conf_name >> mem_BW;
    assert(conf_name == "mem_BW");
    configin >> conf_name >> SM_num;
    assert(conf_name == "SM_num");
    configin >> conf_name >> SPinSM;
    assert(conf_name == "SPinSM");
#ifdef ENPIM
    configin >> conf_name >> npcu;
    assert(conf_name == "npcu");
    configin >> conf_name >> pcu_freq;
    assert(conf_name == "pcu_freq");
    int PIM_ins_amp = core_freq/pcu_freq*2;
#endif

    GPU gpu(SM_num, SPinSM, mem_CH, mem_BW, core_freq);

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

    string outname = argv[2];

#ifdef REQUESTED
    outname += "-req";
#endif
#ifdef OPT_FSM
    outname += "-fsm";
#endif
#ifdef OPT_INTRA
    outname += "-intra";
#endif
    outname += ".txt";
    ofstream res_out(outname, ios::out);
    string line;
    vector<string> lineitems;
    int linenum = 0, paraindex = 0;
    vector<Instruction> ins_set[32/parallel];
    int last_cycles = 0, last_throughput = 0;
    int overall_throughput = 0;
    int PCU_threads = 0;
    int maintain_times = 0;
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
            if (overall_throughput != last_throughput) { // pim will have benefit
                if (++maintain_times >= 50) {
                    ++PCU_threads;
                    maintain_times = 0;
                }
                last_throughput = overall_throughput;
                if (PCU_threads > npcu*gpu.Get_MC_num()) {
                    PCU_threads = npcu*gpu.Get_MC_num();
                }
            }
            
            int total_working_PCUs = 0;
            for (int i=0; i<gpu.Get_MC_num(); ++i) {
                total_working_PCUs += chs[i].working_PCUs;
            }
            if (total_working_PCUs < PCU_threads) {
                int selected_CH = -1;
                // select channel with least pending requests
                int min_pending_req = (1<<30);
                for (int i=0; i<gpu.Get_MC_num(); ++i) {
                    if (gpu.MCs[i].Get_pending_req() < min_pending_req && chs[i].Availiable()) {
                        selected_CH = i;
                        min_pending_req = gpu.MCs[i].Get_pending_req();
                    }
                }
                if (selected_CH != -1) {
                    // check PIM
                    PCU* selected = chs[selected_CH].Availiable(); // find availiable PCU
                    if (selected) {
                        ++chs[selected_CH].working_PCUs;
// #ifdef ASSERTED
//                         assert(selected->Get_Cycles() <= gpu.SMs[0].Get_Cycles()); // PIM task should be finished
// #endif
                        for (auto ins: ins_set[paraindex]) {
                            // amplified instruction cycle
                            if (ins.comm != Command::LOAD) {
                                for (int j=0; j<PIM_ins_amp; ++j) {
                                    selected->Add_Ins(ins);
                                }
                            }
                            else {
                                selected->Add_Ins(ins); // only 1 LOAD command
                            }
                        }
                        // this task is offloaded to PIM, not need to execute on GPU
                        ins_set[paraindex].clear();
                        continue;
                    }
                }
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
                        while (gpu.SMs[j].Execute());
                    }
                    // check cycles
                    int tmp_cycles = gpu.SMs[0].Get_Cycles();
                    res_out << endl << "Now instruction cycles: " << tmp_cycles << endl;
                    if (tmp_cycles % 110400 == 0) {
                        cout << "Total " << tmp_cycles << " cycles executed." << endl;
                    }
                    // throughput
                    overall_throughput = 0;
                    // consume memory bandwidth according to limitation
                    for (int j=0; j<gpu.Get_MC_num(); ++j) {
#ifdef REQUESTED
                        res_out << "Channel " << j << " receive 32B reqs: " << gpu.MCs[j].Execute() << endl;
#else
#ifdef ENPIM
                        // PIM execution
                        int cycles_this_time = PIM_ins_amp * (tmp_cycles-last_cycles-4); // estimated PCU cycles
                        int pim_executed = 0;
                        for (int k=0; k<chs[j].Get_PCU_num(); ++k) {
#ifdef DEBUGGING
                            if (chs[j].PCUs[k].Get_pending_ins_num() != 0) {
                                res_out << "Channel " << j << "'s PCU " << k << " pending instructions: " << chs[j].PCUs[k].Get_pending_ins_num() << endl;
                            }
#endif
                            int pcu_cycles = 0;
                            pim_executed += chs[j].PCUs[k].Execute(tmp_cycles-last_cycles, pcu_cycles);
                            if (pcu_cycles) { // PCU finishes
                                cycles_this_time = pcu_cycles; // record the last finished PCU's actual cycles
                            }
                        }
#endif
                        int ava_mem_BW;
#ifdef ENPIM
                        int processed_reqs = gpu.MCs[j].Execute(tmp_cycles-last_cycles, chs[j].working_PCUs, ava_mem_BW);
#else
                        int processed_reqs = gpu.MCs[j].Execute(tmp_cycles-last_cycles, 0, ava_mem_BW);
#endif
#ifdef DEBUGGING
                        res_out << "Channel " << j << " processed 32B reqs: " << processed_reqs << endl;
#endif
                        int total_mem_BW = mem_BW*1000*(tmp_cycles-last_cycles)/gpu.Get_MC_num()/core_freq; // in bytes
#ifdef ENPIM
                        res_out << "Bandwidth " << j << " : " << gpu.MCs[j].Get_Consumed_BW() << " / " << ava_mem_BW << " / " << total_mem_BW << " Bytes" << endl;
#else
                        res_out << "Bandwidth " << j << " : " << gpu.MCs[j].Get_Consumed_BW() << " / " << total_mem_BW << " Bytes" << endl;
#endif
                        // 4 = 32B*1024(M->K)/64(steps)/128(B)
                        int throughput = processed_reqs*4*core_freq/(tmp_cycles-last_cycles);
                        res_out << "Throughput " << j << " : " << throughput << " KH/s" << endl;
#ifdef ENPIM
                        int pim_throughput = chs[j].working_PCUs*1024/64*core_freq/cycles_this_time;
                        res_out << "Channel " << j << " Working PCU number: " << chs[j].working_PCUs << endl;
                        res_out << "PIM " << j << " throughput: " << pim_throughput << " KH/s" << endl;
                        throughput += pim_throughput;
                        // these PCUs finished their calculations
                        chs[j].working_PCUs -= pim_executed;
#endif
                        overall_throughput += throughput;
#endif
                    }
                    res_out << "Overall: " << overall_throughput << " KH/s" << endl;
                    res_out << "Total PCU threads: " << PCU_threads << endl;
                    // record cycles
                    last_cycles = tmp_cycles;
                    assert(gpu.SMs[0].Schedule(ins_set, parallel) == true);
                }
                for (i=0; i<32/parallel; ++i) {
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