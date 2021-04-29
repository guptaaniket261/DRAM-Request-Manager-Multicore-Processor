#include "DRAM.h"
DRAM::DRAM(int r, int c)
{
    ROW_ACCESS_DELAY = r;
    COL_ACCESS_DELAY = c;
    for (int i = 0; i < (1 << 20); i++)
    {
        memory[i] = 0;
    }
    for (int i = 0; i < 1024; i++)
    {
        ROW_BUFFER[i] = 0;
    }
    clock_cycles = 0;
    DRAM_PRIORITY_ROW = -1;
    DRAM_ROW_BUFFER = -1;

}
bool DRAM::checkIfRunning()
{
    return running;
}
void DRAM::setDRAM(int r, int c)
{
    ROW_ACCESS_DELAY = r;
    COL_ACCESS_DELAY = c;
}
void DRAM::setRunning(int starting_cycle)
{
    running = true;
    start_cycle = starting_cycle;
    if (DRAM_ROW_BUFFER == -1)
    {
        DRAM_ROW_BUFFER = DRAM_PRIORITY_ROW;
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

void DRAM::activateRow(int row_num)
{
    for (int i = 0; i < 1024; i++)
    {
        ROW_BUFFER[i] = memory[1024 * row_num + i];
    }
}
void DRAM::writeBackRow()
{
    for (int i = 0; i < 1024; i++)
    {
        memory[1024 * writeback_row_number + i] = ROW_BUFFER[i];
    }
}
void DRAM::updateMemory()
{
    ROW_BUFFER[DRAMcurrentIns.memory_address % 1024] = DRAMcurrentIns.value;
}
void DRAM::sendToWrite_mrm()
{
    current_state = 1;
    writeBack = DRAMcurrentIns;
    writeBack.value = ROW_BUFFER[DRAMcurrentIns.memory_address % 1024];
}
void DRAM::update_DRAM()
{
    if (!running)
    {
        dramPrint.push_back("IDLE");
        return;
    }
    else
    {
        if (cycle_type == 0)
        {
            //print column access number, and the corresponding instruction etc.
            if (start_cycle + cycle_type * ROW_ACCESS_DELAY + COL_ACCESS_DELAY - 1 == clock_cycles && DRAMcurrentIns.type==1){
                dramPrint.push_back(to_string(DRAMcurrentIns.memory_address)+"-"+to_string(DRAMcurrentIns.memory_address+3)+"="+to_string(DRAMcurrentIns.value));
            }
            else{
                dramPrint.push_back("Column Access "+to_string(DRAMcurrentIns.memory_address%1024));
            }
        }
        else if (cycle_type == 1)
        {
            if (start_cycle + ROW_ACCESS_DELAY > clock_cycles)
            {
                //activation
                dramPrint.push_back("Activate row "+to_string(DRAMcurrentIns.memory_address/1024));
            }
            else
            {
                //print column access no. etc
                if (start_cycle + cycle_type * ROW_ACCESS_DELAY + COL_ACCESS_DELAY - 1 == clock_cycles && DRAMcurrentIns.type==1){
                    dramPrint.push_back(to_string(DRAMcurrentIns.memory_address)+"-"+to_string(DRAMcurrentIns.memory_address+3)+"="+to_string(DRAMcurrentIns.value));
                }
                else{
                    dramPrint.push_back("Column Access "+to_string(DRAMcurrentIns.memory_address%1024));
                }
            }
        }
        else
        {
            if (start_cycle + ROW_ACCESS_DELAY > clock_cycles)
            {
                //writeback the row
                dramPrint.push_back("Writeback row "+to_string(writeback_row_number));
            }
            else if (start_cycle + 2 * ROW_ACCESS_DELAY > clock_cycles)
            {
                dramPrint.push_back("Activate row "+to_string(DRAMcurrentIns.memory_address/1024));
            }
            else
            {
                //column access
                if (start_cycle + cycle_type * ROW_ACCESS_DELAY + COL_ACCESS_DELAY - 1 == clock_cycles && DRAMcurrentIns.type==1){
                    dramPrint.push_back(to_string(DRAMcurrentIns.memory_address)+"-"+to_string(DRAMcurrentIns.memory_address+3)+"="+to_string(DRAMcurrentIns.value));
                }
                else{
                    dramPrint.push_back("Column Access "+to_string(DRAMcurrentIns.memory_address%1024));
                }
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
