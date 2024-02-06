#include <iostream>
#include <queue>
using namespace std;

typedef unsigned long int uint32;

typedef struct Command_Line{
    uint32 rob_size;
    uint32 iq_size;
    uint32 width;
}Command_Line;

typedef struct AVAIL{
    bool DE;
    bool RN;
    bool RR;
    bool DI;
}AVAIL;

typedef struct INST{
    int     seq;
    int     op;    
    int     dest;
    int     src1;
    int     src2;
    uint32  b_FE; //Begin cycle
    uint32  b_DE;
    uint32  b_RN;
    uint32  b_RR;
    uint32  b_DI;
    uint32  b_IS;
    uint32  b_EX;
    uint32  b_WB;
    uint32  b_RT;
    int  ROB_tag; //for Issue Queue
    bool    src1_rdy;
    bool    src2_rdy;    
}INST;

// Rename Map Table, RMT
typedef struct RMTable{
    bool    v;
    int     ROB_tag;
}RMTable;

// Reorder Buffer, ROB
typedef struct RoBuffer{    
    int         dest;       // -1: no dst, 0-66: reg tag
    bool        rdy;
    int         seq;
    INST        inst;
}RoBuffer;

//Issue Queue
typedef struct vINST{    
    bool    v;
    INST    inst;
}vINST;

//Execution List
typedef struct ExeList{   
    bool    v;
    INST    inst;
    uint32  exitCycle;
}ExeList;

class Scheduler{
public:
    bool            finished = false;   
    bool            end_file = false;
    int          lastSeq = 0;
    uint32          cycle = 0;      // clock cycle starting 0
    Command_Line    params;
    uint32          ARF_Number = 67;
    uint32          IQAvailTail; //keep record of IQ availability
    uint32          ROB_head = 0, ROB_tail = 0; //ROB
    bool            ROBEmpty = true;
    queue <uint32>  q; //keep tracing of ROB FIFO


    //Pipeline Registers
    INST*           FE;
    INST*           DE;
    INST*           RN;
    INST*           RR;
    INST*           DI;    
    vINST*          IQ; //inst with v
    ExeList*        EX; //inst with v and exitCycle
    vINST*          WB;
    INST*           RT;    
    AVAIL*          avail;
    RMTable*        RMT;
    RoBuffer*       ROB;
    
    Scheduler(Command_Line p);
    void Retire();
    void Writeback();
    void Execute();
    void Issue();
    void Dispatch();
    void RegRead();
    void Rename();
    void Decode();
    void Fetch();
    void Advance_Cycle();
    void Print(INST, int);
    void DebugPrint();
    void ROBPrint();
    void ResultPrint();
};
