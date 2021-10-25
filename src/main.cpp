#include <iostream>
#include <fstream>
#include <cstring>
#include <assert.h>
#include "gpu.h"
#define RTX3090 0

using namespace std;

#ifdef RTX2060
    const int core_freq = 1680; // MHz
    const int mem_CH = 6;   // 192 bit
    const int mem_bw = 336; // GB/s
    const int SM_num = 30;
    const int SPinSM = 64;
    const int parallel = 8;
#elif defined(RTX3090)
    const int core_freq = 1695; // MHz
    const int mem_CH = 12;  // 384 bit
    const int mem_bw = 936; // GB/s
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
#endif

const std::string command_name[int(Command::MAX)] = {
    "ADD", "SUB", "DIV", "MUL", "MOD", "BITOPS", "LOAD"
};
const int Ins_Cycles[int(Command::MAX)-1] = 
{
    4, 4, 120, 16, 160, 4
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

    ofstream res_out("result3090.txt", ios::out);
    string line;
    vector<string> lineitems;
    int linenum = 0, paraindex = 0;
    vector<Instruction> ins_set[32/parallel];
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
                    for (int j=0; j<gpu.Get_SM_num(); ++j) {
                        // cout << "SM " << j << " executing..." << endl;
                        while (gpu.SMs[j].Execute());
                    }
                    for (int j=0; j<gpu.Get_MC_num(); ++j) {
                        res_out << "Channel " << j << " receive 32B reqs: " << gpu.MCs[j].Execute() << endl;
                    }
                    long long tmp_cycles = gpu.SMs[0].Get_Cycles();
                    res_out << "Now instruction cycles: " << tmp_cycles << endl << endl;
                    if (tmp_cycles % 110400 == 0) {
                        cout << "Total " << tmp_cycles << " cycles executed." << endl;
                    }
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