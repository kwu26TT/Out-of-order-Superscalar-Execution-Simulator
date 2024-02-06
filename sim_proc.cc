#include <stdio.h>
#include <stdlib.h>
#include "sim_proc.h"
#include "scheduler.cc"

#include <string.h>
#include <iostream>

int main (int argc, char* argv[]){
    FILE *FP;               
    char *trace_file;       // Variable that holds trace file name;
    Command_Line params;    // sim 256 32 4 gcc_trace.txt ROB IQ Width file
    uint32 pc;
    int op_type, dest, src1, src2;

    if (argc != 5){
        printf("Error: Wrong number of inputs:%d\n", argc-1);
        printf("%s %s %s\n", argv[0], argv[1], argv[2]);
        exit(EXIT_FAILURE);
    }

    params.rob_size     = strtoul(argv[1], NULL, 10);
    params.iq_size      = strtoul(argv[2], NULL, 10);
    params.width        = strtoul(argv[3], NULL, 10);
    trace_file          = argv[4];

    
    // Open trace_file in read mode
    FP = fopen(trace_file, "r");
    if(FP == NULL){
        // Throw error and exit if fopen() failed
        printf("Error: Unable to open file %s\n", trace_file);
        exit(EXIT_FAILURE);
    }        
    
    // params.rob_size     = 16;
    // params.iq_size      = 8;
    // params.width        = 1;
    // trace_file          = "val_trace_gcc1";

    Scheduler* sch = new Scheduler(params);

    int seq = 0; //seq starting at 0 
    sch->avail->DE = true; //for cycle 0

    while(!sch->finished){        
        INST* inst_set = new INST[params.width];
        // printf("End file = %s\n", sch->end_file ? "true":"false");                       
        
        if (!sch->end_file && sch->avail->DE)
            for(int i=0; i < (int)params.width ; i++){
                if(fscanf(FP, "%lx %d %d %d %d", &pc, &op_type, &dest, &src1, &src2) != EOF){
                    inst_set[i] = (INST){.seq=seq, .op=op_type, .dest=dest,.src1=src1, .src2=src2};                    
                    sch->lastSeq = seq;
                } else {
                    inst_set[i].seq = -1;                
                    sch->end_file = true;
                }                
                sch->FE = inst_set;
                seq++;
            }        

        sch->Retire();        
        // printf("After Retire\n");
        sch->Writeback();
        
        // printf("After writeback\n");
        sch->Execute();
        // printf("After execute\n");
        sch->Issue();
        // printf("After Issue\n");
        sch->Dispatch();
        sch->RegRead();
        sch->Rename();
        sch->Decode();
        sch->Fetch();
        // sch->DebugPrint();
        sch->Advance_Cycle();    
    }
    cout << "# === Simulator Command =========\n";
    cout << "# " << argv[0] << " " << argv[1] << " " << argv[2] << " " << argv[3] << " " << argv[4] << endl;
     cout << "# === Processor Configuration ===" << endl;
    cout << "# ROB_SIZE = " << params.rob_size  << endl;
    cout << "# IQ_SIZE  = " << params.iq_size  << endl;
    cout << "# WIDTH    = " << params.width << endl;
    sch->ResultPrint();
}