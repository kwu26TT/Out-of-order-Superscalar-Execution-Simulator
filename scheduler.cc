#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Scheduler::Scheduler(Command_Line p){
    params = p;
    DE = new INST[params.width]();    
    RN = new INST[params.width]();
    RR = new INST[params.width]();
    DI = new INST[params.width]();
    IQ = new vINST[params.iq_size]();    
    EX = new ExeList[params.width * 5]();
    WB = new vINST[params.width * 5]();
    RT = new INST[params.rob_size](); //1-1 to ROB entry
    avail = new AVAIL[1]();
    RMT = new RMTable[67](); //ISA 0 -66
    ROB = new RoBuffer[params.rob_size]();

    for (uint32 i=0; i < params.width; i++){ //take care of head/tail non code
        DE[i].seq = -1;
        RN[i].seq = -1;
        RR[i].seq = -1;
        DI[i].seq = -1;
    }
    for (uint32 i=0; i < params.width * 5; i++) //take care of head/tail non code
        RT[i].seq = -1;      
}
void Scheduler::Fetch(){
    if (avail->DE|| DE[0].seq == -1) {
        for (uint32 i=0; i<params.width; i++){
            FE[i].b_FE = cycle;
            FE[i].b_DE = cycle + 1;
        }
        DE = FE;        
        if (end_file){ //get rid of the instructions after eof
            INST* inst_set = new INST[params.width];
            for (uint32 i=0; i<params.width; i++)
                inst_set[i] = (INST){.seq=-1, .op=0, .dest=-1, .src1=-1, .src2=-1};
            FE = inst_set;
        }              
        // for (int i=0; i<params.width; i++)
        //     printf("In Fetch 2, Seq: %d Fet: %d\n", DE[i].seq, DE[i].b_FE);
    }
}
void Scheduler::Decode(){   
    if (avail->RN || RN[0].seq == -1) {
        for (uint32 i=0; i<params.width; i++){
            DE[i].b_RN = cycle + 1;            
        }
        RN = DE;
        avail->DE = true;
    } else
        avail->DE = false;
}
void Scheduler::Rename(){     
    if (avail->RR || RR[0].seq == -1){     
        //if the ROB does not have enough free entries to accept the entire rename bundle, then do nothing.
        if (q.size() + params.width <= params.rob_size){
            for (uint32 i=0; i<params.width; i++){
                if (RN[i].seq >= 0){ //avoid head/tail non code
                    RN[i].b_RR = cycle + 1;
                    //check source before destionation to avoid deadlock
                    //check source1 and source2
                    if (RN[i].src1 >= 0)
                        if (RMT[RN[i].src1].v) {
                            RN[i].src1_rdy = false;
                            RN[i].src1 = RMT[RN[i].src1].ROB_tag;
                        } else
                            RN[i].src1_rdy = true;
                    else
                        RN[i].src1_rdy = true;
                    if (RN[i].src2 >= 0)
                        if (RMT[RN[i].src2].v){
                            RN[i].src2_rdy = false;
                            RN[i].src2 = RMT[RN[i].src2].ROB_tag;
                        } else
                            RN[i].src2_rdy = true;
                    else
                        RN[i].src2_rdy = true;

                    //for destination, reserve one ROB slot even if dest register = -1               
                    ROB[ROB_tail] = (RoBuffer){.dest = RN[i].dest, .rdy = false, .seq = RN[i].seq};
                    if (RN[i].dest >= 0){//skip -1 for non register
                        RMT[RN[i].dest].v = true;
                        RMT[RN[i].dest].ROB_tag = ROB_tail; 
                    }                                           
                    RN[i].ROB_tag = ROB_tail; //Do I need it?                                    
                    q.push(ROB_tail);
                    if (++ROB_tail == params.rob_size)
                        ROB_tail = 0;    
                } //avoid non code
            } //for each item in RN bundle            
            RR = RN;            
            // for (uint32 j=0; j<params.width; j++)                
            //     RN[j].seq = -1; //to avoid RN twice, only after RR is updated
            avail->RN = true;   
        } else { //not enough ROB entries
            avail->RN = false;
        }               
    } else //RR not available
        avail->RN = false;    
}
void Scheduler::RegRead(){   
    //values are not modeled; otherwise, take values from ARF here
    if (avail->DI || DI[0].seq == -1) {
        for (uint32 i=0; i<params.width; i++){
            if (RR[i].seq >= 0 ){//avoid head/tail non code                
                RR[i].b_DI = cycle + 1;            
                // printf("Inside RR, Cycle = %d, seq = %d, src = %d, %d\n",
                    // cycle, RR[i].seq, RR[i].src1, RR[i].src2);
            }
        }
        DI = RR;
        INST* inst_set = new INST[params.width];
        RR =  inst_set;
        for (uint32 i=0; i<params.width; i++)
            RR[i].seq = -1;
        avail->RR = true;
    } else
        avail->RR = false;    
}
void Scheduler::Dispatch(){   
    //IQ is sorted by seq in EXE
    IQAvailTail = 0;
    while (IQ[IQAvailTail].v)
        IQAvailTail++;
    if (IQAvailTail + params.width <= params.iq_size){
        for (uint32 i=0; i<params.width; i++){
            if (DI[i].seq >= 0 ){ //avoid head/tail non code
                IQ[IQAvailTail] = (vINST) {.v = true, .inst = DI[i]};
                IQ[IQAvailTail++].inst.b_IS = cycle +1;                
                DI[i].seq = -1; //avoid reading DI twice, only after insturction is put in IQ   
            }
        }  
        avail->DI = true;
    } else //not enough IQ entries
        avail->DI = false;        
}
void Scheduler::Issue(){      
    uint32 exePos = 0, iqPos = 0, toExeLimit = 0;
    bool iqFound = false;
    uint32 latency;
    
    // printf("Issue, exePos = %d\n", exePos);
    while (iqPos < params.iq_size && exePos < params.width * 5 && toExeLimit < params.width){
        //find issuable in IQ
        // printf("before while, iqPos = %d\n", iqPos);
        while (iqPos < params.iq_size && !(IQ[iqPos].v && IQ[iqPos].inst.src1_rdy &&
         IQ[iqPos].inst.src2_rdy))            
            iqPos++;
        if (iqPos < params.iq_size)
            iqFound = true;
        else
            break; //no code available to execute
        //find available basket in Execution list
        //width FU with 5 latency is equivalent to width*5 basket
        while (exePos < params.width * 5 && EX[exePos].v){
            exePos++;
        }
        if (exePos >= params.width * 5) {
            printf("Fatal error, should not happen!");
            break;
        }
        if (iqFound) {            
            // printf("seq = %d, cycle = %d, iqPos = %d, exePos = %d\n", IQ[iqPos].inst.seq, cycle, iqPos, exePos);
            IQ[iqPos].v = false;
            if (IQ[iqPos].inst.op == 0) latency = 1;
            if (IQ[iqPos].inst.op == 1) latency = 2;
            if (IQ[iqPos].inst.op == 2) latency = 5;
            EX[exePos] = (ExeList) {.v = true, .inst = IQ[iqPos].inst, .exitCycle = cycle + latency};
            EX[exePos].inst.b_EX = cycle + 1;
            toExeLimit++;
        }
    }
    //sort IQ to squeeze bubble out
    for (uint32 i=0; i<params.iq_size; i++){
        uint32 j, k;
        if (!IQ[i].v){            
            j = i;
            k = j + 1;
            //find bubble                
            while (k < params.iq_size){
                if (IQ[k].v)
                    break;
                k++;
            }
            //squeeze
            while (k < params.iq_size){
                IQ[j] = IQ[k];
                IQ[k].v = false;
                j++;
                k++;
            }
        }
    }
}
void Scheduler::Execute(){
    for (uint32 i=0; i<params.width*5; i++){      
        if (EX[i].v && EX[i].exitCycle <= cycle){
            //Remove from the execute list
            EX[i].v = false;
            //Add to WB
            uint32 j = 0;
            while (WB[j].v)    
                j++;
            WB[j] = (vINST) {.v = true, .inst = EX[i].inst};
            //Wake up dependent, IQ
            for (j=0; j<params.iq_size; j++){
                if (IQ[j].v) { //look for ROB dependent
                    if (IQ[j].inst.src1 == EX[i].inst.ROB_tag && !IQ[j].inst.src1_rdy){                            
                        IQ[j].inst.src1_rdy = true; //And change the value here
                        IQ[j].inst.src1 = ROB[EX[i].inst.ROB_tag].dest;
                    }
                    if (IQ[j].inst.src2 == EX[i].inst.ROB_tag && !IQ[j].inst.src2_rdy){
                        // printf("Inside IQ, IQ[%d] src2 = %d seq = %d, EX[%d] ROB = %d\n",
                        //  j, IQ[j].inst.src2, IQ[j].inst.seq, i, EX[i].inst.ROB_tag);
                        IQ[j].inst.src2_rdy = true; //And change the value here                        
                        IQ[j].inst.src2 = ROB[EX[i].inst.ROB_tag].dest;
                    }
                }
            }
            //Wake up dispatch, DI
            for (j=0; j<params.width; j++){
                if (DI[j].src1 == EX[i].inst.ROB_tag && !DI[j].src1_rdy){
                    DI[j].src1_rdy = true; //And change the value here     
                     DI[j].src1 = ROB[EX[i].inst.ROB_tag].dest;
                }
                if (DI[j].src2 == EX[i].inst.ROB_tag && !DI[j].src2_rdy){
                        DI[j].src2_rdy = true; //And change the value here                        
                        DI[j].src2 = ROB[EX[i].inst.ROB_tag].dest;
                    }
            }            
            //Wake up register read, RR
            for (j=0; j<params.width; j++){
                if (RR[j].src1 == EX[i].inst.ROB_tag && !RR[j].src1_rdy){
                    RR[j].src1_rdy = true; //And change the value here     
                    RR[j].src1 = ROB[EX[i].inst.ROB_tag].dest;
                }                   
                if (RR[j].src2 == EX[i].inst.ROB_tag && !RR[j].src2_rdy){
                        RR[j].src2_rdy = true; //And change the value here                        
                        RR[j].src2 = ROB[EX[i].inst.ROB_tag].dest;
                    }
            }             
            if(RMT[EX[i].inst.dest].ROB_tag == EX[i].inst.ROB_tag) //Reset RMT only for the latest
                RMT[EX[i].inst.dest].v = false;
            // printf("Inside Execute, Cycle = %d, seq = %d, dest = %d\n",
            // cycle, EX[i].inst.seq, EX[i].inst.dest);
        }
    }
}
void Scheduler::Writeback(){    
    for (uint32 i=0; i<params.width*5; i++){
        if(WB[i].v){ //reset ROB
            WB[i].v = false;            
            WB[i].inst.b_WB = cycle;                                    
            ROB[WB[i].inst.ROB_tag].rdy = true;
            ROB[WB[i].inst.ROB_tag].inst = WB[i].inst; //need to pass the info to next stages
        }
    }
}
void Scheduler::Retire(){
    uint32 rtLimit = 0;
    while (rtLimit < params.width && !q.empty()) {       
        if (ROB[ROB_head].rdy){            
            rtLimit++;
            Print(ROB[ROB_head].inst, cycle);
            if (ROB[ROB_head].seq == lastSeq)
                finished = true;
            q.pop();            
            ROB_head = q.front();

        } else
            rtLimit = params.width; //way to force quitting retire, because oldest not ready yet
    }
}
void Scheduler::Advance_Cycle(){
    cycle++;
    // if (cycle == 100) finished = true;
};
void Scheduler::Print(INST inst, int cy){
    printf("%d fu{%d} src{%d,%d} dst{%d} "
        "FE{%lu,%lu} DE{%lu,%lu} RN{%lu,%lu} RR{%lu,%lu} DI{%lu,%lu} IS{%lu,%lu} "
        "EX{%lu,%lu} WB{%lu,%d} RT{%lu,%lu}\n",
        inst.seq, inst.op, inst.src1, inst.src2, inst.dest,
        inst.b_FE, inst.b_DE-inst.b_FE,
        inst.b_DE, inst.b_RN-inst.b_DE,
        inst.b_RN, inst.b_RR-inst.b_RN,
        inst.b_RR, inst.b_DI-inst.b_RR,
        inst.b_DI, inst.b_IS-inst.b_DI,
        inst.b_IS, inst.b_EX-inst.b_IS,
        inst.b_EX, inst.b_WB-inst.b_EX,
        inst.b_WB, 1,
        inst.b_WB + 1, cy - inst.b_WB);
}
void Scheduler::ROBPrint(){    
    uint32 tt = 0;
    if (q.size() > 0)
        tt = q.back();
    printf("ROB H = %lu, T = %lu, size = %lu %s, front = %lu, back = %lu\n", ROB_head, ROB_tail, q.size(),
    q.size() + params.width > params.rob_size ? "Full":"", q.front(), tt);
    
    uint32 i = ROB_head;
    while (i != tt) {    
        printf("ROB[%lu] rdy = %s seq = %d, dest = %d, ", i, ROB[i].rdy ? "true":"false", ROB[i].seq, ROB[i].dest);
        i++;
        if (i == params.rob_size)
            i = 0;        
        }
    if (q.size() > 0) //print the last one
         printf("ROB[%lu] rdy = %s seq = %d, dest = %d\n", i, ROB[i].rdy ? "true":"false", ROB[i].seq, ROB[i].dest);        
    printf("\n");    
}
void Scheduler::DebugPrint(){  
    
    printf("avail->DE a= %s, RN a= %s, RR a= %s, DI a= %s\n",
     avail-> DE ? "true":"false", avail-> RN ? "true":"false", avail-> RR ? "true":"false",avail-> DI ? "true":"false");
    for (uint32 i=0; i<params.width; i++) //Fetch
        printf("FE Seq = %d, op = %d, dest = %d, src1 = %d, src2 = %d\n",
            FE[i].seq, FE[i].op, FE[i].dest, FE[i].src1, FE[i].src2);          
    printf("\n");
    for (uint32 i=0; i<params.width; i++) //Decode
        printf("DE Seq = %d, op = %d, dest = %d, src1 = %d, src2 = %d\n",
            DE[i].seq, DE[i].op, DE[i].dest, DE[i].src1, DE[i].src2);          
    printf("\n");    
    for (uint32 i=0; i<params.width; i++) //Rename
        printf("RN Seq = %d, dest = %d, src1 = %d %s, "
            "src2 = %d %s, ROB = %d\n",
            RN[i].seq, RN[i].dest, RN[i].src1, RN[i].src1_rdy ? "true":"false",           
            RN[i].src2, RN[i].src2_rdy ? "true":"false", RN[i].ROB_tag);           
    printf("\n");
    for (uint32 i=0; i<params.width; i++) //Regread
        printf("RR Seq = %d, RR dest = %d, src1 = %d %s, "
            "src2 = %d %s, ROB = %d\n",
            RR[i].seq, RR[i].dest, RR[i].src1, RR[i].src1_rdy ? "true":"false",
            RR[i].src2, RR[i].src2_rdy ? "true":"false", RR[i].ROB_tag);           
    printf("\n");
    for (uint32 i=0; i<params.width; i++) //dispatch
        printf("DI Seq = %d, DI dest[%d], src1 = %d %s, src2 = %d %s\n",
            DI[i].seq, DI[i].dest, DI[i].src1, DI[i].src1_rdy ? "true":"false",
            DI[i].src2, DI[i].src2_rdy ? "true":"false");        
    printf("\n");
    // for (uint32 i=0; i<67; i++) //RMT
    //     if(RMT[i].v)
    //         printf("RMT[%d] ROB = %d, ", i, RMT[i].ROB_tag);
    // printf("\n");
    ROBPrint();
    printf("IQ tail = %lu %s\n", IQAvailTail, IQAvailTail + params.width >= params.iq_size  ? "Full":""); 
    for (uint32 i=0; i<params.iq_size; i++) //IQ
        if (IQ[i].v)
            printf("IQ[%lu], dst = %d, s1 = %d %s, s2 = %d %s, "
            "ROB = %d, RN cycle = %lu, RR = %lu, DI cycle = %lu, IQ cycle = %lu, seq = %d\n",
            i, IQ[i].inst.dest, IQ[i].inst.src1, IQ[i].inst.src1_rdy ? "true":"false",  
            IQ[i].inst.src2, IQ[i].inst.src2_rdy ? "true":"false", 
            IQ[i].inst.ROB_tag, IQ[i].inst.b_RN, IQ[i].inst.b_RR, 
            IQ[i].inst.b_DI, IQ[i].inst.b_IS, IQ[i].inst.seq);
    printf("\n");
    for (uint32 i=0; i<params.width * 5; i++) //Execute        
        if (EX[i].v)
            printf("EX[%lu], dest = %d, EX cycle = %lu, exitCycle = %lu, ROB tag = %d, seq = %d\n",
            i,  EX[i].inst.dest, EX[i].inst.b_EX, EX[i].exitCycle,
            EX[i].inst.ROB_tag, EX[i].inst.seq);    
    for (uint32 i=0; i<params.width * 5; i++) //WB        
        printf("WB[%lu].v = %s, seq = %d, ROB_tag = %d\n", i, WB[i].v ? "true":"false",
            WB[i].inst.seq, WB[i].inst.ROB_tag);  
    printf("\n-----------------Last Seq = %d, End of Cycle %lu------------------------\n\n\n", lastSeq, cycle);
}

void Scheduler::ResultPrint(){
    float IPC = float(lastSeq) / float(cycle);
    cout << "# === Simulation Results ========\n";
    cout << "# Dynamic Instruction Count    = " << lastSeq+1 << endl;
    cout << "# Cycles                       = " << cycle << endl;
    //cout << "# Instructions Per Cycle (IPC) = " << fixed << setprecision(2) << IPC << endl;
    printf("# Instructions Per Cycle (IPC) = %.2f\n", IPC );
    
}