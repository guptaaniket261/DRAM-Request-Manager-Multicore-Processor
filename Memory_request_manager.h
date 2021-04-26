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

struct DRAM_ins
{
    int ins_number;     //stores instruction_number
    int type;           //0 if lw, 1 if sw
    int memory_address; //stores load/store address
    int value = 0;      //stores the value of register in case of sw instruction, as we are already passing it
    int fileNumber;
    string reg; //stores the register of instruction
};

class Memory_request_manager
{
public:
    void push_as_immediate(DRAM_ins, int); //will recieve the details of the instruction here
    void check_immediate(int);             //check if there is an immediate instruction for a file number
    void update();

private:
    queue<DRAM_ins> Immediate_instruction;   //set to -1 (for each file, has some value)
    vector<vector<bool>> instruction_usable; //to check whether a given instruction has to be used or not, for each file
    vector<bool> present_in_imm_queue;       //checks whether a file has some instruction in immediate queue
    pair<int, int> curr_ins = {-1, -1};      //ins number, file name
    DRAM_ins currentInstruction;
    int type_ins; //type can be 0,1,2
    int row_buffer = -1;
    int starting_cycle_num = -1; //stores the starting cycle number for each instruction
    int writeback_row_num = -1;
    int col_access_num = -1;
    vector<deque<DRAM_ins>> DRAM_queues(1024);
    vector<int> actual_queue_size(1024); //to tackle useless instructions, maintains size of each DRAM row
    int ROW_BUFFER[1024] = {0};
    int total_queue_size;
};