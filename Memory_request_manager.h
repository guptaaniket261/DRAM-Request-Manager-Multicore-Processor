#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <map>
#include <deque>
#include <iomanip>
#include <set>
#include <queue>
#include "DRAM.h"

using namespace std;

class Memory_request_manager
{
public:
    vector<vector<string>> coreOpPrint;
    vector<vector<string>> registerPrint;
    vector<string> mrmPrint;
    int totalBufferSize();
    Memory_request_manager(int, int);
    vector<deque<DRAM_ins>> mrmBuffer;
    vector<deque<DRAM_ins>> justReceived;
    int justReceivedSize = 0;
    bool pointer_change_state;
    string allot_new_instruction(int);
    int bufferSize[1024] = {};
    tuple<bool, int, string> checkForWriteback();
    void updateMRM();
    void increment_cycles();
    void simulate_DRAM();
    void sendToMRM(DRAM_ins, int);
    void set(int, int);
    int get_clock_cycles();
    map<int, string> register_numbers;        //maps each number in 0-31 to a register
    vector<map<string, int>> register_values; //stores the value of data stored in each register
    //each vector element stores register,values for a particular file
    DRAM program_dram = DRAM(1, 2);
    vector<map<string, int>> register_busy; //map each register to its lw/sw row number
};