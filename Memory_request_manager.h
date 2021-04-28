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

//////////////////////////////////

map<int, string> register_numbers;        //maps each number in 0-31 to a register
vector<map<string, int>> register_values; //stores the value of data stored in each register
//each vector element stores register,values for a particular file

vector<map<string, int>> register_busy; //map each register to its lw/sw row number

//////////////////////////////////

class Memory_request_manager
{
public:
    vector<deque<DRAM_ins>> mrmBuffer;
    vector<deque<DRAM_ins>> justReceived;
    int justReceivedSize = 0;
    bool pointer_change_state;

    void allot_new_instruction(int row_buffer);
    int bufferSize[1024] = {};
    tuple<bool, int, string> checkForWriteback();
    void updateMRM();
    void sendToMRM(DRAM_ins inst, int flag);
};