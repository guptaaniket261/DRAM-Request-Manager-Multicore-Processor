#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <map>
#include <deque>
#include <iomanip>
#include <set>
#include <queue>
using namespace std;

int clock_cycles = 0;
bool running = false;
int memory[(1 << 20)] = {0}; //memory used to store the data
int ROW_BUFFER[1024] = {};
int start_cycle;
int cycle_type;
int writeback_row_number;
int current_state = 0;
struct DRAM_ins
{
    int ins_number;     //stores instruction_number
    int type;           //0 if lw, 1 if sw
    int memory_address; //stores load/store address
    int value = 0;      //stores the value of register in case of sw instruction, as we are already passing it
    int fileNumber;
    string reg; //stores the register of instruction
};
DRAM_ins writeBack;
int DRAM_PRIORITY_ROW;
DRAM_ins DRAMcurrentIns;
int DRAM_ROW_BUFFER;
int ROW_ACCESS_DELAY, COL_ACCESS_DELAY;
bool checkIfRunning()
{
    return running;
}

void setRunning(int starting_cycle)
{
    running = true;
    start_cycle = starting_cycle;
    if (DRAM_ROW_BUFFER == -1)
    {
        cycle_type = 1; //ROW ACTIVATION +COLUMN aCCESS
    }
    else if (DRAM_ROW_BUFFER == DRAM_PRIORITY_ROW)
    {
        cycle_type = 0; //COLUMN ACCESS
    }
    else
    {
        writeback_row_number = DRAM_ROW_BUFFER;
        DRAM_ROW_BUFFER = DRAM_PRIORITY_ROW;
        cycle_type = 2; //WRITEBACK, ACTIVATION, COL ACCESS
    }
}

void activateRow(int row_num)
{
    for (int i = 0; i < 1024; i++)
    {
        ROW_BUFFER[i] = memory[1024 * row_num + i];
    }
}
void writeBackRow()
{
    for (int i = 0; i < 1024; i++)
    {
        memory[1024 * writeback_row_number + i] = ROW_BUFFER[i];
    }
}
void updateMemory()
{
    ROW_BUFFER[DRAMcurrentIns.memory_address % 1024] = DRAMcurrentIns.value;
}
void sendToWrite_mrm()
{
    current_state = 1;
    writeBack = DRAMcurrentIns;
    writeBack.value = ROW_BUFFER[DRAMcurrentIns.memory_address % 1024];
}
void simulateDRAM()
{
    if (!running)
    {
        return;
    }
    else
    {
        if (cycle_type == 0)
        {
            //print column access number, and the corresponding instruction etc.
        }
        else if (cycle_type == 1)
        {
            if (start_cycle + ROW_ACCESS_DELAY > clock_cycles)
            {
                //activation
            }
            else
            {
                //print column access no. etc
            }
        }
        else
        {
            if (start_cycle + ROW_ACCESS_DELAY > clock_cycles)
            {
                //writeback the row
            }
            else if (start_cycle + 2 * ROW_ACCESS_DELAY > clock_cycles)
            {
                //activation
            }
            else
            {
                //column access
            }
        }
        if (start_cycle + cycle_type * ROW_ACCESS_DELAY + COL_ACCESS_DELAY - 1 == clock_cycles)
        {
            running = false;
            if (cycle_type == 2)
            {
                writeBackRow();
                activateRow(DRAM_ROW_BUFFER);
            }
            else if (cycle_type == 1)
            {
                activateRow(DRAM_ROW_BUFFER);
            }
            if (DRAMcurrentIns.type == 1)
            {
                updateMemory();
            }
            else
            {
                sendToWrite_mrm();
            }
        }
    }
}
