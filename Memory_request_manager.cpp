#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <map>
#include <deque>
#include <iomanip>
#include <set>
#include "Memory_request_manager.h"
using namespace std;
Memory_request_manager::Memory_request_manager(int r, int c)
{
    program_dram = DRAM(r, c);
}
tuple<bool, int, string> Memory_request_manager::checkForWriteback()
{
    if (program_dram.current_state == 1)
    {
        registerPrint[program_dram.writeBack.front().fileNumber].push_back(program_dram.writeBack.front().reg + " = " + to_string(program_dram.writeBack.front().value));
        register_values[program_dram.writeBack.front().fileNumber][program_dram.writeBack.front().reg] = program_dram.writeBack.front().value;
        program_dram.dramCycle = max(program_dram.dramCycle, program_dram.clock_cycles);
        //this means a writeback is being done in current cycle
        //mrmBuffer, justrecieved
        bool found = false;
        for (int i = 0; i < mrmBuffer.size(); i++)
        {
            for (int j = 0; j < mrmBuffer[i].size(); j++)
            {
                DRAM_ins temp = mrmBuffer[i][j];
                if (temp.type == 0 && temp.fileNumber == program_dram.writeBack.front().fileNumber && temp.reg == program_dram.writeBack.front().reg)
                {
                    found = true;
                    break;
                }
            }
        }
        for (int i = 0; i < justReceived.size(); i++)
        {
            for (int j = 0; j < justReceived[i].size(); j++)
            {
                DRAM_ins temp = justReceived[i][j];
                if (temp.type == 0 && temp.fileNumber == program_dram.writeBack.front().fileNumber && temp.reg == program_dram.writeBack.front().reg)
                {
                    found = true;
                    break;
                }
            }
        }
        if (!found)
        {
            register_busy[program_dram.writeBack.front().fileNumber][program_dram.writeBack.front().reg] = -1;
        } ////@error
        program_dram.instructions_per_core[program_dram.writeBack.front().fileNumber]++;
        int file_num = program_dram.writeBack.front().fileNumber;
        string reg1 = program_dram.writeBack.front().reg;
        program_dram.writeBack.pop_front();
        if (program_dram.writeBack.size() == 0)
        {
            program_dram.current_state = 0;
        }
        return {true, file_num, reg1};
    }
    else
    {
        return {false, -1, "-"};
    }
}
void Memory_request_manager::increment_cycles()
{
    program_dram.clock_cycles++;
}
void Memory_request_manager::simulate_DRAM()
{
    program_dram.update_DRAM();
}
int Memory_request_manager::get_clock_cycles()
{
    return program_dram.clock_cycles;
}
void Memory_request_manager::set(int r, int c)
{
    program_dram.setDRAM(r, c);
}

void Memory_request_manager::sendToMRM(DRAM_ins inst, int flag)
{
    if (flag == 0)
    {
        justReceived[inst.fileNumber].push_back(inst);
        justReceivedSize++;
    }
    else
    {
        //cout << "flag 1" << endl;
        //if just recieved of the core is empty, then prev same instr must be in the buffer, or being executed
        //else, it must be the last element of just recieved of that core
        //cout << inst.memory_address << endl;
        if (justReceived[inst.fileNumber].size() == 0)
        {
            //cout << inst.memory_address << endl;
            //remove the prev same instr from the buffer, if it is not the current instruction
            // if (inst.type == 0 && program_dram.checkIfRunning() && program_dram.DRAMcurrentIns.type == 0 && inst.reg == program_dram.DRAMcurrentIns.reg)
            // {
            //     //cout << "okay1" << endl;
            //     //actually, the running instructions is not the just previous instruction in thus case !
            //     justReceived[inst.fileNumber].push_back(inst);
            //     justReceivedSize++;
            // }
            // else if (inst.type == 1 && program_dram.checkIfRunning() && program_dram.DRAMcurrentIns.type == 1 && inst.memory_address == program_dram.DRAMcurrentIns.memory_address)
            // {
            //     justReceived[inst.fileNumber].push_back(inst);
            //     justReceivedSize++;
            // }
            if (mrmBuffer[inst.fileNumber].size() > 0 && mrmBuffer[inst.fileNumber].back().memInsNumber == inst.memInsNumber-1 &&
                mrmBuffer[inst.fileNumber].back().type == inst.type && 
                ((inst.type == 1 && inst.memory_address==mrmBuffer[inst.fileNumber].back().memory_address) || (inst.type == 0 && inst.reg == mrmBuffer[inst.fileNumber].back().reg)))
            {
                //the last ins will either be the correct prev lw, nothing, or an sw instruction from which forwading has been done
                program_dram.instructions_per_core[inst.fileNumber]++;
                bufferSize[mrmBuffer[inst.fileNumber].back().memory_address / 1024]--;
                mrmBuffer[inst.fileNumber].pop_back();
                //cout << mrmBuffer[inst.fileNumber].size() << endl;
                justReceived[inst.fileNumber].push_back(inst);
                justReceivedSize++;
            }
            else
            {
                justReceived[inst.fileNumber].push_back(inst);
                justReceivedSize++;
            }
        }
        else
        {
            program_dram.instructions_per_core[inst.fileNumber]++;
            // cout << "Popped first" << endl;
            justReceived[inst.fileNumber].pop_back();
            justReceived[inst.fileNumber].push_back(inst);
        }
    }
}

int Memory_request_manager::totalBufferSize()
{
    int count = 0;
    for (int i = 0; i < 1024; i++)
        count += bufferSize[i];
    return count;
}

bool Memory_request_manager::forwardable(int file_num)
{

    return (justReceived[file_num].front().type == 0 && mrmBuffer[file_num].size() > 0 && mrmBuffer[file_num].back().type == 1 && justReceived[file_num].front().memory_address == mrmBuffer[file_num].back().memory_address && justReceived[file_num].front().memInsNumber == mrmBuffer[file_num].back().memInsNumber+1);
}

void Memory_request_manager::updateMRM()
{
    if (program_dram.start_cycle == program_dram.clock_cycles)
    {
        //cout << program_dram.start_cycle << endl;
        mrmPrint.push_back("Updating pointers of MRM buffer after sending latest DRAM ins"); // updating pointers for last dram instruction sent
        return;
    }
    bool res = program_dram.checkIfRunning();
    string forPrint = "";
    if (justReceivedSize > 0)
    {
        string forwarding = "";
        for (int i = 0; i < justReceived.size(); i++)
        {
            if (justReceived[i].size() > 0)
            {
                //cout << "file no. " << i << endl;
                if (forwardable(i))
                {
                    //check if the front instruction of just recieved has a matching sw as the last elem of mrmBuffer[i]
                    string t1, ans;
                    DRAM_ins temp = justReceived[i].front();
                    if (temp.type == 0)
                    {
                        t1 = "lw";
                    }
                    else
                    {
                        t1 = "sw";
                    }
                    ans = (t1 + " " + temp.reg + " " + to_string(temp.memory_address));
                    if (forwarding.size() != 0)
                    {
                        forwarding = forwarding + ", ";
                    }
                    forwarding += "(" + ans + " core :" + to_string(i + 1) + ")";

                    justReceived[i].pop_front();
                    justReceivedSize--;
                    temp.value = mrmBuffer[i].back().value;
                    program_dram.writeBack.push_back(temp);
                    program_dram.current_state = 1;
                }
                else
                {
                    DRAM_ins temp = justReceived[i].front();
                    justReceived[i].pop_front();
                    justReceivedSize--;
                    mrmBuffer[i].push_back(temp);
                    bufferSize[temp.memory_address / 1024]++;
                }
            }
        }
        forwarded_data[get_clock_cycles()] = forwarding;
        forPrint += "Pushing into MRM Buffer"; //
        //mrmPrint.push_back("Changing pointers for just arrived instructions");
    }
    else
    {
        forPrint += "";
    }
    if (!res && totalBufferSize() > 0)
    {
        //cout << "FFFFFFFF" << endl;
        //in this case, alot a new instruction
        //row_buffer =-1, or row buffer has an empty row one cycle for combinational logic. else send to DRAM
        if (program_dram.DRAM_PRIORITY_ROW == -1 || bufferSize[program_dram.DRAM_PRIORITY_ROW] == 0)
        {
            //getNewRow();
            bool found = false;
            for (int i = 0; i < mrmBuffer.size(); i++)
            {
                if (found)
                {
                    break;
                }
                for (int j = 0; j < mrmBuffer[i].size(); j++)
                {
                    program_dram.DRAM_PRIORITY_ROW = (mrmBuffer[i][j].memory_address) / 1024;
                    //mrmPrint.push_back("check new row buffer");
                    if (forPrint.size() > 0)
                    {
                        forPrint += " and ";
                    }
                    found = true;
                    forPrint += "Finding a new row for DRAM"; //reading operation of half - cycle
                    // program_dram.setRunning(program_dram.clock_cycles + 1);
                    break;
                }
            }
        }
        else
        {
            //cout << "bs" << bufferSize[program_dram.DRAM_PRIORITY_ROW] << endl;
            //cout << "FFF" << endl;
            //send row buffer instruction to DRAM
            if (forPrint.size() > 0)
            {
                forPrint += " and ";
            }
            program_dram.setRunning(program_dram.clock_cycles + 1);
            forPrint += allot_new_instruction(program_dram.DRAM_PRIORITY_ROW);
        }
    }
    else
    {
        if (forPrint == "")
            forPrint = "IDLE";
    }
    mrmPrint.push_back(forPrint);
    //we are taking one cycle to assign a new instruction (by popping from buffer) due to pointer updations in ll and hashmap
    //MRM changes pointers of queue buffer - 3rd Priority
    //MRM finds first non zero instruction (combinational logic) - 1st Priority
    //MRM sends an instruction to DRAM for execution -2nd priority
    // Mem_req_manager() algorithm: (in order)
    // 1.) if there is a running instruction, execute one cycle of it (done by DRAM)
    // 2.) if writing a value into register or not doing anything, then pick apt row and start executing along with it.
    // 		if row_buffer is -1, choose first non zero row, takes one cycle as done by combinational logic
    // 4.) else just continue
}

string Memory_request_manager::allot_new_instruction(int row_number)
{
    //first find the core for the row
    string ans = "";
    DRAM_ins temp;
    bool found = false;
    for (int i = 0; i < mrmBuffer.size(); i++)
    {
        deque<DRAM_ins> temp_deque;
        for (int j = 0; j < mrmBuffer[i].size(); j++)
        {
            DRAM_ins temp = mrmBuffer[i][j];
            if (((temp.memory_address) / 1024 == row_number) && !found)
            {
                program_dram.DRAMcurrentIns = temp;
                bufferSize[(temp.memory_address) / 1024]--;
                string t1;
                if (temp.type == 0)
                {
                    t1 = "lw";
                }
                else
                {
                    t1 = "sw";
                }
                ans = ("passing new instruction to DRAM: " + t1 + " " + temp.reg + " " + to_string(temp.memory_address));
                found = true;
            }
            else
            {
                temp_deque.push_back(temp);
            }
        }
        mrmBuffer[i] = temp_deque;
        if (found)
        {
            break;
        }
    }
    return ans;
}