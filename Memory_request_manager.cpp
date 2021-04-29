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
        registerPrint[program_dram.writeBack.fileNumber].push_back(program_dram.writeBack.reg +" = " + to_string(program_dram.writeBack.value));
        register_values[program_dram.writeBack.fileNumber][program_dram.writeBack.reg] = program_dram.writeBack.value;
        //this means a writeback is being done in current cycle
        program_dram.current_state = 0;
        register_busy[program_dram.writeBack.fileNumber][program_dram.writeBack.reg] = -1;
        return {true, program_dram.writeBack.fileNumber, program_dram.writeBack.reg};
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
        //if just recieved of the core is empty, then prev same instr must be in the buffer
        //else, it must be the last element of just recieved of that core
        if (justReceived[inst.fileNumber].size() == 0)
        {
            //remove the prev same instr from the buffer, if it is not the current instruction
            if (inst.type == 0 && program_dram.DRAMcurrentIns.type == 0 && inst.reg == program_dram.DRAMcurrentIns.reg)
            {
                justReceived[inst.fileNumber].push_back(inst);
                justReceivedSize++;
            }
            else if (inst.type == 1 && program_dram.DRAMcurrentIns.type == 1 && inst.memory_address == program_dram.DRAMcurrentIns.memory_address)
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

int Memory_request_manager::totalBufferSize(){
    int count =0;
    for(int i=0;i<1024;i++)count+=bufferSize[i];
}

void Memory_request_manager::updateMRM()
{
    if (program_dram.start_cycle == program_dram.clock_cycles){
        mrmPrint.push_back("updating pointers"); // updating pointers for last dram instruction sent
        return;
    }
    bool res = program_dram.checkIfRunning();
    if (!res && totalBufferSize()>0)
    {
        //in this case, alot a new instruction
        //row_buffer =-1, or row buffer has an empty row one cycle for combinational logic. else send to DRAM
        if (program_dram.DRAM_PRIORITY_ROW == -1 || bufferSize[program_dram.DRAM_PRIORITY_ROW] == 0)
        {
            //getNewRow();
            for (int i = 0; i < mrmBuffer.size(); i++)
            {
                for (int j = 0; j < mrmBuffer[i].size(); j++)
                {
                    program_dram.DRAM_PRIORITY_ROW = (mrmBuffer[i][j].memory_address) / 1024;
                    mrmPrint.push_back("check new row buffer");
                    // program_dram.setRunning(program_dram.clock_cycles + 1);
                    break;
                }
            }
        }
        else
        {

            //send row buffer instruction to DRAM
            program_dram.setRunning(program_dram.clock_cycles + 1);
            allot_new_instruction(program_dram.DRAM_PRIORITY_ROW);
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
                    bufferSize[temp.memory_address/1024]++;
                }
            }
            mrmPrint.push_back("Changing pointers for just arrived instructions");
        }
        else{
            mrmPrint.push_back("IDLE");
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
                program_dram.DRAMcurrentIns = temp;
                bufferSize[(temp.memory_address) / 1024]--;
                string t1;
                if(temp.type==0){
                    t1 = "lw";
                }
                else{
                    t1 = "sw";
                }
                mrmPrint.push_back("passing new instruction to DRAM: "+ t1 +" " + temp.reg + " " + to_string(temp.memory_address));
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