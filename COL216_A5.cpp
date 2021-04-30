#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <map>
#include <deque>
#include <iomanip>
#include <set>
#include "parser.hpp"
#include "Memory_request_manager.h"

using namespace std;

vector<map<string, int>> labelNo;
vector<string> words; //stores the entire file as a vector of strings

//each vector element stores register,values for a particular file
set<string> Register_list;
vector<vector<Instruction>> instructs; //stores instructions as structs
map<string, int> operation;
map<int, string> intTostr_operation;
int op_count[11] = {0};
int ins_count[1000000] = {0};
int number_of_files;
int simulation_time;
vector<int> PC;
vector<bool> invalid_files;
Memory_request_manager memReqManager = Memory_request_manager(1, 2);
vector<DRAM_ins> prevMemoryOperation; //remember to initialise this vector !
string ifZero(string temp)
{
    if (temp.size() >= 3 && temp[0] == '$' && temp[1] == 'r' && temp[2] == '0')
    {
        return "$zero" + temp.substr(3, temp.size() - 3);
    }
    return temp;
}

string findInstruction(Instruction current)
{
    int action = operation[current.name];
    switch (action)
    {
    case 1:
        return "add " + ifZero(current.field_1) + ", " + ifZero(current.field_2) + ", " + ifZero(current.field_3);
        break;
    case 2:
        return "sub " + ifZero(current.field_1) + ", " + ifZero(current.field_2) + ", " + ifZero(current.field_3);
        break;
    case 3:
        return "mul " + ifZero(current.field_1) + ", " + ifZero(current.field_2) + ", " + ifZero(current.field_3);
        break;
    case 4:
        return "beq " + ifZero(current.field_1) + ", " + ifZero(current.field_2) + ", " + ifZero(current.field_3);
        break;
    case 5:
        return "bne " + ifZero(current.field_1) + ", " + ifZero(current.field_2) + ", " + ifZero(current.field_3);
        break;
    case 6:
        return "slt " + ifZero(current.field_1) + ", " + ifZero(current.field_2) + ", " + ifZero(current.field_3);
        break;
    case 7:
        return "j " + ifZero(current.field_1);
        break;
    case 9:
        return "lw " + ifZero(current.field_1) + ", " + ifZero(current.field_2) + "(" + ifZero(current.field_3) + ")";
        break;
    case 10:
        return "sw " + ifZero(current.field_1) + ", " + ifZero(current.field_2) + "(" + ifZero(current.field_3) + ")";
        break;
    case 8:
        return "addi " + ifZero(current.field_1) + ", " + ifZero(current.field_2) + ", " + ifZero(current.field_3);
        break;
    }
    return "N.A";
}

void PrintData()
{
    ofstream fout;
    fout.open("output.txt");
    fout << left << setw(15) << "Cycle numbers";
    for (int i = 0; i < number_of_files; i++)
    {

        fout << left << setw(25) << "Instructions CORE" + to_string(i + 1);
        fout << left << setw(25) << "Register CORE" + to_string(i + 1);
    }
    fout << left << setw(100) << "MRM";
    fout << left << setw(50) << "DRAM";
    fout << "\n\n";

    for (int i = 0; i < simulation_time; i++)
    {
        fout << left << setw(15) << i + 1;
        for (int j = 0; j < number_of_files; j++)
        {
            fout << left << setw(25) << memReqManager.coreOpPrint[j][i];
            fout << left << setw(25) << memReqManager.registerPrint[j][i];
        }
        fout << left << setw(100) << memReqManager.mrmPrint[i];
        fout << left << setw(50) << memReqManager.program_dram.dramPrint[i];
        fout << endl;
    }
    fout.close();
}

void map_register_numbers()
{
    //maps each register to a unique number between 0-31 inclusive
    memReqManager.register_numbers[0] = "$r0";
    memReqManager.register_numbers[1] = "$at";
    memReqManager.register_numbers[2] = "$v0";
    memReqManager.register_numbers[3] = "$v1";
    for (int i = 4; i <= 7; i++)
    {
        memReqManager.register_numbers[i] = "$a" + to_string(i - 4);
    }
    for (int i = 8; i <= 15; i++)
    {
        memReqManager.register_numbers[i] = "$t" + to_string(i - 8);
    }
    for (int i = 16; i <= 23; i++)
    {
        memReqManager.register_numbers[i] = "$s" + to_string(i - 16);
    }
    memReqManager.register_numbers[24] = "$t8";
    memReqManager.register_numbers[25] = "$t9";
    memReqManager.register_numbers[26] = "$k0";
    memReqManager.register_numbers[27] = "$k1";
    memReqManager.register_numbers[28] = "$gp";
    memReqManager.register_numbers[29] = "$sp";
    memReqManager.register_numbers[30] = "$s8";
    memReqManager.register_numbers[31] = "$ra";
    for (int i = 0; i < 32; i++)
    {
        Register_list.insert(memReqManager.register_numbers[i]);
    }
}
void map_operations()
{
    operation["add"] = 1;
    operation["sub"] = 2;
    operation["mul"] = 3;
    operation["beq"] = 4;
    operation["bne"] = 5;
    operation["slt"] = 6;
    operation["j"] = 7;
    operation["addi"] = 8;
    operation["lw"] = 9;
    operation["sw"] = 10;

    intTostr_operation[1] = "add";
    intTostr_operation[2] = "sub";
    intTostr_operation[3] = "mul";
    intTostr_operation[4] = "beq";
    intTostr_operation[5] = "bne";
    intTostr_operation[6] = "slt";
    intTostr_operation[7] = "j";
    intTostr_operation[8] = "addi";
    intTostr_operation[9] = "lw";
    intTostr_operation[10] = "sw";
}

void initialise_Registers(int file_number)
{
    //initialises all the registers
    for (int i = 0; i < 32; i++)
    {
        memReqManager.register_values[file_number][memReqManager.register_numbers[i]] = 0;
        memReqManager.register_busy[file_number][memReqManager.register_numbers[i]] = -1;
    }
}

void add(int file_num)
{
    struct Instruction current = instructs[file_num][PC[file_num]];
    if (current.field_1 == "$r0")
    {
        memReqManager.registerPrint[file_num].push_back("IDLE");
        return;
    }
    else if (is_integer(current.field_3))
    {
        memReqManager.register_values[file_num][current.field_1] = memReqManager.register_values[file_num][current.field_2] + stoi(current.field_3);
        memReqManager.registerPrint[file_num].push_back(current.field_1 + " = " + to_string(memReqManager.register_values[file_num][current.field_1]));
    }
    else
    {
        memReqManager.register_values[file_num][current.field_1] = memReqManager.register_values[file_num][current.field_2] + memReqManager.register_values[file_num][current.field_3];
        memReqManager.registerPrint[file_num].push_back(current.field_1 + " = " + to_string(memReqManager.register_values[file_num][current.field_1]));
    }
}

void sub(int file_num)
{
    struct Instruction current = instructs[file_num][PC[file_num]];
    if (current.field_1 == "$r0")
    {
        memReqManager.registerPrint[file_num].push_back("IDLE");
        return;
    }
    else if (is_integer(current.field_3))
    {
        memReqManager.register_values[file_num][current.field_1] = memReqManager.register_values[file_num][current.field_2] - stoi(current.field_3);
        memReqManager.registerPrint[file_num].push_back(current.field_1 + " = " + to_string(memReqManager.register_values[file_num][current.field_1]));
    }
    else
    {
        memReqManager.register_values[file_num][current.field_1] = memReqManager.register_values[file_num][current.field_2] - memReqManager.register_values[file_num][current.field_3];
        memReqManager.registerPrint[file_num].push_back(current.field_1 + " = " + to_string(memReqManager.register_values[file_num][current.field_1]));
    }
}

void mul(int file_num)
{
    struct Instruction current = instructs[file_num][PC[file_num]];
    if (current.field_1 == "$r0")
    {
        memReqManager.registerPrint[file_num].push_back("IDLE");
        return;
    }
    else if (is_integer(current.field_3))
    {
        memReqManager.register_values[file_num][current.field_1] = memReqManager.register_values[file_num][current.field_2] * stoi(current.field_3);
        memReqManager.registerPrint[file_num].push_back(current.field_1 + " = " + to_string(memReqManager.register_values[file_num][current.field_1]));
    }
    else
    {
        memReqManager.register_values[file_num][current.field_1] = memReqManager.register_values[file_num][current.field_2] * memReqManager.register_values[file_num][current.field_3];
        memReqManager.registerPrint[file_num].push_back(current.field_1 + " = " + to_string(memReqManager.register_values[file_num][current.field_1]));
    }
}

void beq(int file_num)
{
    struct Instruction current = instructs[file_num][PC[file_num]];
    if (memReqManager.register_values[file_num][current.field_1] == memReqManager.register_values[file_num][current.field_2])
    {
        PC[file_num] = labelNo[file_num][current.field_3] - 1;
    }
    else
    {
        PC[file_num]++;
    }
    memReqManager.registerPrint[file_num].push_back("IDLE");
}

void bne(int file_num)
{
    struct Instruction current = instructs[file_num][PC[file_num]];
    if (memReqManager.register_values[file_num][current.field_1] != memReqManager.register_values[file_num][current.field_2])
    {
        PC[file_num] = labelNo[file_num][current.field_3] - 1;
    }
    else
    {
        PC[file_num]++;
    }
    memReqManager.registerPrint[file_num].push_back("IDLE");
}

void slt(int file_num)
{
    struct Instruction current = instructs[file_num][PC[file_num]];
    if (current.field_1 == "$r0")
    {
        memReqManager.registerPrint[file_num].push_back("IDLE");
        return;
    }
    else if (is_integer(current.field_3))
    {
        if (stoi(current.field_3) > memReqManager.register_values[file_num][current.field_2])
            memReqManager.register_values[file_num][current.field_1] = 1;
        else
            memReqManager.register_values[file_num][current.field_1] = 0;
        memReqManager.registerPrint[file_num].push_back(current.field_1 + " = " + to_string(memReqManager.register_values[file_num][current.field_1]));
    }
    else
    {
        if (memReqManager.register_values[file_num][current.field_3] > memReqManager.register_values[file_num][current.field_2])
            memReqManager.register_values[file_num][current.field_1] = 1;
        else
            memReqManager.register_values[file_num][current.field_1] = 0;
        memReqManager.registerPrint[file_num].push_back(current.field_1 + " = " + to_string(memReqManager.register_values[file_num][current.field_1]));
    }
}

void addi(int file_number)
{
    struct Instruction current = instructs[file_number][PC[file_number]];
    if (current.field_1 == "$r0")
    {
        return;
        memReqManager.registerPrint[file_number].push_back("IDLE");
    }
    else
    {
        memReqManager.register_values[file_number][current.field_1] = memReqManager.register_values[file_number][current.field_2] + stoi(current.field_3);
        memReqManager.registerPrint[file_number].push_back(current.field_1 + " = " + to_string(memReqManager.register_values[file_number][current.field_1]));
    }
}

void j(int file_number)
{
    struct Instruction current = instructs[file_number][PC[file_number]];
    PC[file_number] = labelNo[file_number][current.field_1] - 1;
    memReqManager.registerPrint[file_number].push_back("IDLE");
}
//handle labelNo for each file separately

void callFunction(int name, int file_num)
{
    switch (name)
    {
    case 1:
        add(file_num);
        break;
    case 2:
        sub(file_num);
        break;
    case 3:
        mul(file_num);
        break;
    case 4:
        beq(file_num);
        break;
    case 5:
        bne(file_num);
        break;
    case 6:
        slt(file_num);
        break;
    case 7:
        j(file_num);
        break;
    case 8:
        addi(file_num);
        break;
    }
}

void process()
{

    while (true)
    {
        memReqManager.increment_cycles();
        int curr_cycles = memReqManager.get_clock_cycles();
        if (curr_cycles == simulation_time + 1)
        {
            break;
        }
        tuple<bool, int, string> result = memReqManager.checkForWriteback();
        //boolean, file number, register
        memReqManager.simulate_DRAM();
        memReqManager.updateMRM();
        for (int i = 0; i < number_of_files; i++)
        {
            if (invalid_files[i] || PC[i] >= instructs[i].size())
            {
                memReqManager.coreOpPrint[i].push_back("IDLE");
                if (get<1>(result) != i)
                {
                    memReqManager.registerPrint[i].push_back("IDLE");
                }
                continue;
            }
            else
            {
                Instruction current_instr = instructs[i][PC[i]];
                int current_instr_name = operation[current_instr.name];

                if (current_instr_name <= 8)
                {
                    if (current_instr_name != 4 && current_instr_name != 5 && current_instr_name != 7)
                    {
                        //add, sub, mul, addi, slt

                        // @TODO: can we read in the same cycle in which it is written ??
                        if (get<0>(result) && i == get<1>(result) && current_instr.field_1 == get<2>(result))
                        {
                            memReqManager.coreOpPrint[i].push_back("IDLE");
                            continue;
                        }
                        else
                        {
                            //Register_list, //initialise register_usy to minus -1
                            if (Register_list.find(current_instr.field_1) != Register_list.end() && memReqManager.register_busy[i][current_instr.field_1] != -1)
                            {
                                memReqManager.coreOpPrint[i].push_back("IDLE");
                                memReqManager.registerPrint[i].push_back("IDLE");
                                continue;
                            }
                            if (Register_list.find(current_instr.field_2) != Register_list.end() && memReqManager.register_busy[i][current_instr.field_2] != -1)
                            {
                                memReqManager.coreOpPrint[i].push_back("IDLE");
                                memReqManager.registerPrint[i].push_back("IDLE");
                                continue;
                            }
                            if (Register_list.find(current_instr.field_3) != Register_list.end() && memReqManager.register_busy[i][current_instr.field_3] != -1)
                            {
                                memReqManager.coreOpPrint[i].push_back("IDLE");
                                memReqManager.registerPrint[i].push_back("IDLE");
                                continue;
                            }
                            memReqManager.coreOpPrint[i].push_back(findInstruction(current_instr));
                            callFunction(current_instr_name, i);
                            PC[i]++;
                        }
                    }
                    else
                    {
                        //j,beq,bne
                        if (current_instr_name == 7)
                        {
                            callFunction(current_instr_name, i);
                            memReqManager.coreOpPrint[i].push_back(findInstruction(current_instr));
                        }
                        else
                        {
                            if (Register_list.find(current_instr.field_1) != Register_list.end() && memReqManager.register_busy[i][current_instr.field_1] != -1)
                            {
                                memReqManager.registerPrint[i].push_back("IDLE");
                                memReqManager.coreOpPrint[i].push_back("IDLE");
                                continue;
                            }
                            if (Register_list.find(current_instr.field_2) != Register_list.end() && memReqManager.register_busy[i][current_instr.field_2] != -1)
                            {
                                memReqManager.registerPrint[i].push_back("IDLE");
                                memReqManager.coreOpPrint[i].push_back("IDLE");
                                continue;
                            }
                            memReqManager.registerPrint[i].push_back("IDLE");
                            memReqManager.coreOpPrint[i].push_back(findInstruction(current_instr));
                            callFunction(current_instr_name, i);
                        }
                    }
                }
                else if (current_instr_name == 9)
                {
                    //PC[i]++;
                    DRAM_ins temp;
                    int address = memReqManager.register_values[i][current_instr.field_3] + stoi(current_instr.field_2);
                    temp.ins_number = PC[i];
                    temp.type = 0;
                    temp.memory_address = address;
                    temp.reg = current_instr.field_1;
                    temp.fileNumber = i;
                    if (get<0>(result) && i == get<1>(result) && current_instr.field_3 == get<2>(result))
                    { //offset register is being written on
                        memReqManager.coreOpPrint[i].push_back("IDLE");
                        continue;
                    }
                    if (Register_list.find(current_instr.field_3) != Register_list.end() && memReqManager.register_busy[i][current_instr.field_3] != -1)
                    {
                        memReqManager.registerPrint[i].push_back("IDLE");
                        memReqManager.coreOpPrint[i].push_back("IDLE");
                        continue;
                    }
                    if (Register_list.find(current_instr.field_1) != Register_list.end() && memReqManager.register_busy[i][current_instr.field_1] != -1)
                    {
                        //&& !(memReqManager.program_dram.DRAMcurrentIns.type == 0 && memReqManager.program_dram.DRAMcurrentIns.reg == current_instr.field_1)
                        if (prevMemoryOperation[i].type == 0 && prevMemoryOperation[i].reg == current_instr.field_1)
                        {

                            if (memReqManager.mrmBuffer[i].size() + memReqManager.justReceived[i].size() < 64 / number_of_files)
                            {
                                prevMemoryOperation[i] = temp;
                                memReqManager.sendToMRM(temp, 1);
                                memReqManager.registerPrint[i].push_back("IDLE");
                                memReqManager.register_busy[i][temp.reg] = address / 1024;
                                memReqManager.coreOpPrint[i].push_back(findInstruction(current_instr));
                                PC[i]++;
                                continue;
                            }
                            else
                            {
                                memReqManager.registerPrint[i].push_back("IDLE");
                                memReqManager.coreOpPrint[i].push_back("IDLE");
                                continue;
                            }
                        }
                        else
                        {
                            memReqManager.registerPrint[i].push_back("IDLE");
                            memReqManager.coreOpPrint[i].push_back("IDLE");
                            continue;
                        }
                    }

                    if (memReqManager.mrmBuffer[i].size() + memReqManager.justReceived[i].size() < 64 / number_of_files)
                    {
                        memReqManager.registerPrint[i].push_back("IDLE");
                        memReqManager.coreOpPrint[i].push_back(findInstruction(current_instr));
                        prevMemoryOperation[i] = temp;
                        memReqManager.register_busy[i][temp.reg] = address / 1024;
                        memReqManager.sendToMRM(temp, 0);
                        PC[i]++;
                        continue;
                    }
                    else
                    {
                        memReqManager.registerPrint[i].push_back("IDLE");
                        memReqManager.coreOpPrint[i].push_back("IDLE");
                        continue;
                    }
                }
                else
                {
                    DRAM_ins temp;
                    int address = memReqManager.register_values[i][current_instr.field_3] + stoi(current_instr.field_2);
                    temp.ins_number = PC[i];
                    temp.type = 1;
                    temp.memory_address = address;
                    temp.reg = current_instr.field_1;
                    temp.value = memReqManager.register_values[i][current_instr.field_1];
                    temp.fileNumber = i;
                    if (Register_list.find(current_instr.field_1) != Register_list.end() && memReqManager.register_busy[i][current_instr.field_1] != -1)
                    {
                        memReqManager.registerPrint[i].push_back("IDLE");
                        memReqManager.coreOpPrint[i].push_back("IDLE");
                        continue;
                    }
                    if (Register_list.find(current_instr.field_3) != Register_list.end() && memReqManager.register_busy[i][current_instr.field_3] != -1)
                    {
                        memReqManager.registerPrint[i].push_back("IDLE");
                        memReqManager.coreOpPrint[i].push_back("IDLE");
                        continue;
                    }
                    if (prevMemoryOperation[i].type == 1 && prevMemoryOperation[i].memory_address == address)
                    {
                        if (memReqManager.mrmBuffer[i].size() + memReqManager.justReceived[i].size() < 64 / number_of_files)
                        {
                            memReqManager.registerPrint[i].push_back("IDLE");
                            memReqManager.coreOpPrint[i].push_back(findInstruction(current_instr));
                            prevMemoryOperation[i] = temp;
                            memReqManager.sendToMRM(temp, 1);
                            PC[i]++;
                            continue;
                        }
                        else
                        {
                            memReqManager.registerPrint[i].push_back("IDLE");
                            memReqManager.coreOpPrint[i].push_back("IDLE");
                            continue;
                        }
                    }
                    if (memReqManager.mrmBuffer[i].size() + memReqManager.justReceived[i].size() < 64 / number_of_files)
                    {
                        prevMemoryOperation[i] = temp;
                        memReqManager.sendToMRM(temp, 0);
                        memReqManager.registerPrint[i].push_back("IDLE");
                        memReqManager.coreOpPrint[i].push_back(findInstruction(current_instr));
                        PC[i]++;
                        continue;
                    }
                    else
                    {
                        memReqManager.registerPrint[i].push_back("IDLE");
                        memReqManager.coreOpPrint[i].push_back("IDLE");
                        continue;
                    }
                }
            }
        }
    }
}

int main(int argc, char *argv[])
{

    if (argc < 5)
    {
        cout << "Invalid arguments\n";
        return -1;
    }

    //Taking file name, row and column access delays from the command line
    int r, c;
    number_of_files = stoi(argv[1]);
    simulation_time = stoi(argv[2]);
    r = stoi(argv[3]);
    c = stoi(argv[4]);
    if (r < 1 || c < 1 || number_of_files <= 0 || simulation_time <= 0)
    {
        cout << "Invalid arguments\n";
        return -1;
    }

    memReqManager.set(r, c);

    invalid_files.resize(number_of_files);
    for (int i = 0; i < number_of_files; i++)
    {
        invalid_files[i] = false;
    }
    map_register_numbers();
    map_operations();
    memReqManager.mrmBuffer.resize(number_of_files);
    memReqManager.justReceived.resize(number_of_files);
    memReqManager.coreOpPrint.resize(number_of_files);
    memReqManager.registerPrint.resize(number_of_files);
    memReqManager.register_values.resize(number_of_files);
    instructs.resize(number_of_files);
    memReqManager.register_busy.resize(number_of_files);
    prevMemoryOperation.resize(number_of_files);

    for (int i = 0; i < number_of_files; i++)
    {
        string filename = "t" + to_string(i + 1) + ".txt";
        ifstream file(filename);
        string current_line;
        initialise_Registers(i);
        while (getline(file, current_line))
        {

            if (ifEmpty(current_line))
                continue;
            pair<int, Instruction> temp = Create_structs(current_line, Register_list, instructs[i].size());
            if (temp.first == 1)
            {
                temp.second.file_name = i;
                instructs[i].push_back(temp.second);
                //cout<<temp.second.name<<" "<<instructs.size()<<endl;
            }
            else if (temp.first == 2)
            {
                continue;
            }
            else
            {

                invalid_files[i] = true;
            }
        }
        labelNo.push_back(getLabels());
    }
    //check for invalid lw/sw addresses
    for (int i = 0; i < number_of_files; i++) //initializing program counter for each of the files
    {
        PC.push_back(0);
    }
    process();

    PrintData();
    return 0;
}