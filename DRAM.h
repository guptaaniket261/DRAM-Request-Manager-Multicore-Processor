#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <map>
#include <deque>
#include <iomanip>
#include <set>
#include <queue>
#include "print.hpp"
using namespace std;

struct DRAM_ins
{
    int ins_number;     //stores instruction_number
    int type;           //0 if lw, 1 if sw
    int memory_address; //stores load/store address
    int value = 0;      //stores the value of register in case of sw instruction, as we are already passing it
    int fileNumber;
    int memInsNumber;
    string reg; //stores the register of instruction
};
class DRAM
{
public:
    int dramCycle = 0;
    int buffer_updates = 0;
    DRAM(int, int); //row access, col access
    bool checkIfRunning();
    void setRunning(int);
    void setDRAM(int, int);
    void activateRow(int);
    void writeBackRow();
    void update_DRAM();
    void updateMemory();
    void sendToWrite_mrm();
    void simulateDRAM();
    deque<DRAM_ins> writeBack;
    int DRAM_PRIORITY_ROW;
    int row_buffer_dirty;
    DRAM_ins DRAMcurrentIns;
    int DRAM_ROW_BUFFER;
    int ROW_ACCESS_DELAY, COL_ACCESS_DELAY;
    int clock_cycles = 0;
    bool running = false;
    int memory[(1 << 20)]; // = {0}; //memory used to store the data
    int ROW_BUFFER[1024];
    int start_cycle;
    int cycle_type;
    int writeback_row_number;
    int current_state = 0;
    vector<int> instructions_per_core;
    vector<string> dramPrint;
};