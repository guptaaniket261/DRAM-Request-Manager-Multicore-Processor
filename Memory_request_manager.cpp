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
Memory_request_manager::Memory_request_manager() {}
tuple<bool, int, string> Memory_request_manager::checkForWriteback()
{
    if (current_state == 1)
    {
        register_values[writeBack.fileNumber][writeBack.reg] = writeBack.value;
        //this means a writeback is being done in current cycle
        current_state = 0;
        return {true, writeBack.fileNumber, writeBack.reg};
    }
    else
    {
        return {false, -1, "-"};
    }
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
        //if just recieved of the core is empty, then prev same instr must be in the buffer
        //else, it must be the last element of just recieved of that core
        if (justReceived[inst.fileNumber].size() == 0)
        {
            //remove the prev same instr from the buffer, if it is not the current instruction
            if (inst.type == 0 && DRAMcurrentIns.type == 0 && inst.reg == DRAMcurrentIns.reg)
            {
                justReceived[inst.fileNumber].push_back(inst);
                justReceivedSize++;
            }
            else if (inst.type == 1 && DRAMcurrentIns.type == 1 && inst.memory_address == DRAMcurrentIns.memory_address)
            {
                justReceived[inst.fileNumber].push_back(inst);
                justReceivedSize++;
            }
            else
            {
                //find in buffer the previous lw/sw instruction and remove it
                //we will store for buffer address for last memory instruction pushed
                // for replacing the just previous instruction:
                // for sw - just replace the previous instruction from the last instruction's address
                // for lw - clear the address of the last instruction and push the current address as usual
                mrmBuffer[inst.fileNumber].pop_back();
                justReceived[inst.fileNumber].push_back(inst);
                justReceivedSize++;
            }
        }
        else
        {
            justReceived[inst.fileNumber].pop_back();
            justReceived[inst.fileNumber].push_back(inst);
        }
    }
}
void Memory_request_manager::updateMRM()
{
    if (start_cycle == clock_cycles)
        return;
    bool res = checkIfRunning();
    if (!res)
    {
        //in this case, alot a new instruction
        //row_buffer =-1, or row buffer has an empty row one cycle for combinational logic. else send to DRAM
        if (DRAM_PRIORITY_ROW == -1 || bufferSize[DRAM_PRIORITY_ROW] == 0)
        {
            //getNewRow();
            for (int i = 0; i < mrmBuffer.size(); i++)
            {
                for (int j = 0; j < mrmBuffer[i].size(); j++)
                {
                    DRAM_PRIORITY_ROW = (mrmBuffer[i][j].memory_address) / 1024;
                    setRunning(clock_cycles + 1);
                    break;
                }
            }
        }
        else
        {

            //send row buffer instruction to DRAM
            setRunning(clock_cycles + 1);
            allot_new_instruction(DRAM_PRIORITY_ROW);
        }
    }
    else
    {
        if (justReceivedSize > 0)
        {
            for (int i = 0; i < justReceived.size(); i++)
            {
                if (justReceived[i].size() > 0)
                {
                    DRAM_ins temp = justReceived[i].front();
                    justReceived[i].pop_front();
                    justReceivedSize--;
                    mrmBuffer[i].push_back(temp);
                }
            }
        }
    }
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

void Memory_request_manager::allot_new_instruction(int row_number)
{
    //first find the core for the row
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
                DRAMcurrentIns = temp;
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
}