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

map<string, int> labelNo;
bool f = false;
int ROW_ACCESS_DELAY, COL_ACCESS_DELAY;
int row_buffer_updates = 0;
struct toPrint
{
    int startingCycle;
    int endingCycle;
    string instruction;
    string RegisterChanged;
    string DRAMoperation;
    string DRAMchanges;
    string queueOp = "N.A.";
    string address = "N.A"; //stores the address of the completed instruction, if so
};

vector<toPrint> prints;
DRAM_ins currentInstruction;
int number_of_files;
int simulation_time;
vector<int> PC;
vector<string> words;                     //stores the entire file as a vector of strings
map<int, string> register_numbers;        //maps each number in 0-31 to a register
vector<map<string, int>> register_values; //stores the value of data stored in each register
//each vector element stores register,values for a particular file
set<string> Register_list;
vector<vector<Instruction>> instructs; //stores instructions as structs
int memory[(1 << 20)] = {0};           //memory used to store the data
int clock_cycles = 0;
map<string, int> operation;
map<int, string> intTostr_operation;
vector<map<string, int>> register_busy; //map each register to its lw/sw row number
pair<int, int> curr_ins = {-1, -1};     //ins number, file name
int type_ins;
/*instruction can be of three types 
type 0: only col_access
type 1: activation + col_acess
type 2: writeback, activation, col_access
*/

Memory_request_manager memReqManager;
int op_count[11] = {0};
int ins_count[1000000] = {0};
//bool completed_just =false;
//stores that if an instruction was just completed in the current cycle
//for printing, we will store the starting cycle, ending cycle


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
    case 8:
        return "lw " + ifZero(current.field_1) + ", " + ifZero(current.field_2) + "(" + ifZero(current.field_3) + ")";
        break;
    case 9:
        return "sw " + ifZero(current.field_1) + ", " + ifZero(current.field_2) + "(" + ifZero(current.field_3) + ")";
        break;
    case 10:
        return "addi " + ifZero(current.field_1) + ", " + ifZero(current.field_2) + ", " + ifZero(current.field_3);
        break;
    }
    return "N.A";
}
void reset_instruction()
{
    /*ins_register[currentInstruction]=-1*/
    curr_ins = {-1, -1};
    type_ins = -1;
    starting_cycle_num = -1;
    writeback_row_num = -1;
    col_access_num = -1;
}

void map_register_numbers()
{
    //maps each register to a unique number between 0-31 inclusive
    register_numbers[0] = "$r0";
    register_numbers[1] = "$at";
    register_numbers[2] = "$v0";
    register_numbers[3] = "$v1";
    for (int i = 4; i <= 7; i++)
    {
        register_numbers[i] = "$a" + to_string(i - 4);
    }
    for (int i = 8; i <= 15; i++)
    {
        register_numbers[i] = "$t" + to_string(i - 8);
    }
    for (int i = 16; i <= 23; i++)
    {
        register_numbers[i] = "$s" + to_string(i - 16);
    }
    register_numbers[24] = "$t8";
    register_numbers[25] = "$t9";
    register_numbers[26] = "$k0";
    register_numbers[27] = "$k1";
    register_numbers[28] = "$gp";
    register_numbers[29] = "$sp";
    register_numbers[30] = "$s8";
    register_numbers[31] = "$ra";
    for (int i = 0; i < 32; i++)
    {
        Register_list.insert(register_numbers[i]);
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
    operation["lw"] = 8;
    operation["sw"] = 9;
    operation["addi"] = 10;

    intTostr_operation[1] = "add";
    intTostr_operation[2] = "sub";
    intTostr_operation[3] = "mul";
    intTostr_operation[4] = "beq";
    intTostr_operation[5] = "bne";
    intTostr_operation[6] = "slt";
    intTostr_operation[7] = "j";
    intTostr_operation[8] = "lw";
    intTostr_operation[9] = "sw";
    intTostr_operation[10] = "addi";
}
void initialise_memory()
{
    for (int i = 0; i < (1 << 20); i++)
    {
        memory[i] = 0;
    }
    for (int i = 0; i < 1024; i++)
    {
        ROW_BUFFER[i] = 0;
    }
}
void initialise_Registers(int file_number)
{
    //initialises all the registers
    for (int i = 0; i < 32; i++)
    {
        register_values[file_number][register_numbers[i]] = 0;
        register_busy[file_number][register_numbers[i]] = -1;
    }
}
void writeBack(int num_row)
{
    for (int i = num_row * 1024; i < num_row * 1024 + 1024; i++)
    {
        memory[i] = ROW_BUFFER[i - num_row * 1024];
    }
}

void activateRow(int num_row)
{
    for (long i = num_row * 1024; i < num_row * 1024 + 1024; i++)
    {
        ROW_BUFFER[i - num_row * 1024] = memory[i];
    }
}

void optimizeSw(int numRow)
{
    deque<DRAM_ins> temp = DRAM_queues[numRow];
    vector<DRAM_ins> temp1;
    int mem_last = -1;
    DRAM_ins lastsw;
    for (auto u : temp)
    {
        DRAM_ins curr = u;
        if (u.type == 1)
        { //sw instruction, see if consecutive present ahead
            if (mem_last == -1)
            {
                mem_last = curr.memory_address;
                lastsw = u;
            }
            else
            {
                if (u.memory_address == mem_last)
                {
                    lastsw = u;
                }
                else
                {
                    temp1.push_back(lastsw);
                    lastsw = u;
                    mem_last = u.memory_address;
                }
            }
        }
        else
        {
            if (mem_last != -1)
            {
                temp1.push_back(lastsw);
            }
            temp1.push_back(u);
            mem_last = -1;
        }
    }
    if (mem_last != -1)
    {
        temp1.push_back(lastsw);
    }
    DRAM_queues[numRow] = {};
    for (int i = 0; i < temp1.size(); i++)
    {
        DRAM_queues[numRow].push_back(temp1[i]);
    }
}

void Complete_dependent(string register_name, int file_number)
{
    if (curr_ins.first == -1 || !(currentInstruction.reg == register_name && currentInstruction.type == 0)) //if current instruction is already being completed
    {
        memReqManager.push_as_immediate(register_name, file_number);
    }
    else
    {
    }
}

bool freeRegister(int file_number)
{
    struct Instruction current = instructs[file_number][PC[file_number]];
    if (valid_register(current.field_1, Register_list) && register_busy[file_number][current.field_1] != -1 && register_busy[file_number][current.field_1] == row_buffer)
    {
        Complete_dependent(current.field_1, file_number);
        return true;
    }
    if (valid_register(current.field_2, Register_list) && register_busy[file_number][current.field_2] != -1 && register_busy[file_number][current.field_2] == row_buffer)
    {
        Complete_dependent(current.field_2, file_number);
        return true;
    }
    if (valid_register(current.field_3, Register_list) && register_busy[file_number][current.field_3] != -1 && register_busy[file_number][current.field_3] == row_buffer)
    {
        Complete_dependent(current.field_3, file_number);
        return true;
    }
    vector<pair<int, string>> row_order;
    if (valid_register(current.field_1, Register_list) && register_busy[file_number][current.field_1] != -1)
        row_order.push_back({register_busy[file_number][current.field_1], current.field_1});
    if (valid_register(current.field_2, Register_list) && register_busy[file_number][current.field_2] != -1)
        row_order.push_back({register_busy[file_number][current.field_2], current.field_2});
    if (valid_register(current.field_3, Register_list) && register_busy[file_number][current.field_3] != -1)
        row_order.push_back({register_busy[file_number][current.field_3], current.field_3});
    int i = 0;
    sort(row_order.begin(), row_order.end());
    if (row_order.size())
    {
        Complete_dependent(row_order[0].second);
        return true;
    }
    return false;
}

void add(int file_number)
{
    struct Instruction current = instructs[file_number][PC[file_number]];
    bool wait_for_completion = freeRegister(file_number);
    //here make an instruction to be completed immediately.
    //toPrint temp = print_normal_operation();
    if (wait_for_completion)
    {
        return;
    }
    else
    {
        if (current.field_1 == "$r0")
        {
        }
        else if (is_integer(current.field_3))
        {
            register_values[file_number][current.field_1] = register_values[file_number][current.field_2] + stoi(current.field_3);
        }
        else
        {
            register_values[file_number][current.field_1] = register_values[file_number][current.field_2] + register_values[file_number][current.field_3];
        }
        PC[file_number]++;
    }
}

void sub(int file_number)
{
    struct Instruction current = instructs[file_number][PC[file_number]];
    bool wait_for_completion = freeRegister();
    //here make an instruction to be completed immediately.
    //toPrint temp = print_normal_operation();
    if (wait_for_completion)
    {
        return;
    }
    else
    {
        if (current.field_1 == "$r0")
        {
        }
        else if (is_integer(current.field_3))
        {
            register_values[file_number][current.field_1] = register_values[file_number][current.field_2] - stoi(current.field_3);
        }
        else
        {
            register_values[file_number][current.field_1] = register_values[file_number][current.field_2] - register_values[file_number][current.field_3];
        }
        PC[file_number]++;
    }
}

void mul(int file_number)
{
    struct Instruction current = instructs[file_number][PC[file_number]];
    bool wait_for_completion = freeRegister();
    //here make an instruction to be completed immediately.
    //toPrint temp = print_normal_operation();
    if (wait_for_completion)
    {
        return;
    }
    else
    {
        if (current.field_1 == "$r0")
        {
        }
        else if (is_integer(current.field_3))
        {
            register_values[file_number][current.field_1] = register_values[file_number][current.field_2] * stoi(current.field_3);
        }
        else
        {
            register_values[file_number][current.field_1] = register_values[file_number][current.field_2] * register_values[file_number][current.field_3];
        }
        PC[file_number]++;
    }
}

// void addi(int file_number)
// {
//     struct Instruction current = instructs[file_number][PC[file_number]];
//     bool wait_for_completion = freeRegister();
//     freeRegister();
//     clock_cycles++;
//     if (current.field_1 == "$r0")
//     {
//     }
//     else
//     {
//         register_values[file_number][current.field_1] = register_values[file_number][current.field_2] + stoi(current.field_3);
//     }
//     PC[file_number]++;
// }

void process()
{
    while (true)
    {
        for (int i = 0; i < number_of_files; i++)
        {
            struct Instruction current = instructs[i][PC[i]];
            int action = operation[current.name];
            if (action != 8 && action != 9)
                op_count[action]++;
            switch (action)
            {
            case 1:
                add(i);
                break;
            case 2:
                sub(i);
                break;
            case 3:
                mul(i);
                break;
            case 4:
                beq(i);
                break;
            case 5:
                bne(i);
                break;
            case 6:
                slt(i);
                break;
            case 7:
                j(i);
                break;
            case 8:
                lw(i);
                break;
            case 9:
                sw(i);
                break;
            case 10:
                addi(i);
                break;
            }
            //all clock cycles are incremented inside the functions
        }
        if (total_queue_size > 0 || curr_ins_num != -1)
        {
            complete_remaining(); //this function simply completes the remaining instructions starting from the current cycle
        }
        if (row_buffer != -1)
        {
            writeBack(row_buffer);
        }
    }
}

int main(int argc, char *argv[])
{
    memReqManager = new Memory_request_manager();
    validFile = true;
    if (argc < 5)
    {
        cout << "Invalid arguments\n";
        return -1;
    }
    //Taking file name, row and column access delays from the command line
    number_of_files = stoi(argv[1]);
    simulation_time = stoi(argv[2]);
    ROW_ACCESS_DELAY = stoi(argv[3]);
    COL_ACCESS_DELAY = stoi(argv[4]);
    if (ROW_ACCESS_DELAY < 1 || COL_ACCESS_DELAY < 1)
    {
        cout << "Invalid arguments\n";
        return -1;
    }
    map_register_numbers();
    map_operations();
    initialise_memory();
    register_values.resize(number_of_files);
    instructs.resize(number_of_files);
    register_busy.resize(number_of_files);
    for (int i = 0; i < number_of_files; i++)
    {
        ifstream file("t" + to_string(i + 1) + ".txt");
        string current_line;
        initialise_Registers(i);
        validFile = true;
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
                validFile = false;
            }
        }
        if (!validFile)
        {
            //cout << "ok";
            cout << "Invalid MIPS program" << endl;
            return -1;
        }
    }
    labelNo = getLabels();
    //check for invalid lw/sw addresses
    for (int i = 0; i < number_of_files; i++) //initializing program counter for each of the files
    {
        PC.push_back(0);
    }
    clock_cycles = 0;
    process();
    // PrintData();
    return 0;
}
