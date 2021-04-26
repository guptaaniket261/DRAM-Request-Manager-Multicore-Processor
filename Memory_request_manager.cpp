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
Memory_request_manager::int findType(int row_number)
{
    if (row_buffer == -1)
    {
        //this is type 1, activation + COl_access
        return 1;
    }
    else
    {
        if (row_number == row_buffer)
        {
            return 0;
            //this is type 0, only col_access
        }
        else
        {
            return 2; //writeback also required
        }
    }
}
Memory::request_manager::void assign_new_instruction(DRAM_ins instruction)
{
    current_instruction = curr;
    type_ins = findType(instruction.memory_address / 1024);
    if (type_ins == 2)
    {
        writeback_row_num = row_buffer;
    }
    col_access_num = instruction.memory_address % 1024;
    row_buffer = memory_address / 1024;
    starting_cycle_num = clock_cycles;
}
Memory_request_manager::void update()
{
    if (curr_ins.first != -1)
    {
        //running lw or sw instriuction
        //execute one cycle of the instruction
        execute_one_cycle(current_instruction);
    }
    else
    {
        if (Immediate_instruction.size() != 0)
        {
            //execute one cycle of instruction from this queue, remember this instruction
            //decrease the size of corresponding DRAM_queue by 1, and mark it as useless
            //decrease the size of correspondind DRAM row also
            DRAM_ins curr = Immediate_instruction.front();
            Immediate_instruction.pop_front();
            present_in_imm_queue[curr.fileNumber] = false;                //since this is not present in immediate queue anymore
            instruction_usable[curr.fileNumber][curr.ins_number] = false; //to avoid repeated instruction
            actual_queue_size[curr.memory_address / 1024]--;              //decreasing queue size for the row
            total_queue_size--;
            assign_new_instruction(curr);
        }
        else
        {
            if (total_queue_size == 0)
            {
                return;
            }
            else
            {
                if (row_buffer == -1)
                {
                    //select first non zero row from DRAM_queues
                    for (int i = 0; i < 1024; i++)
                    {
                        if (DRAM_queues[i].size() > 0)
                        {
                            DRAM_ins curr = DRAM_queues[i].front();
                            DRAM_queues[i].pop_front();
                            actual_queue_size[i]--;
                            total_queue_size--;
                            assign_new_instruction(curr);
                            break;
                        }
                    }
                }
                else
                {
                    if (DRAM_queues[row_buffer].size() == 0)
                    {
                        //select first non zero row from DRAM_queues
                        for (int i = 0; i < 1024; i++)
                        {
                            if (DRAM_queues[i].size() > 0)
                            {
                                DRAM_ins curr = DRAM_queues[i].front();
                                DRAM_queues[i].pop_front();
                                actual_queue_size[i]--;
                                total_queue_size--;
                                assign_new_instruction(curr);
                                break;
                            }
                        }
                    }
                    else
                    {
                        //select row = row_buffer
                        DRAM_ins curr = DRAM_queues[row_buffer].front();
                        DRAM_queues[row_buffer].pop_front();
                        actual_queue_size[row_buffer]--;
                        total_queue_size--;
                        assign_new_instruction(curr);
                    }
                }
            }
        }
    }
}
