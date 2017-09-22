#include <cstdio>
#include <iostream>
#include <fstream>
#include <cstdint>
#include <pthread.h>
#include <map>
#include <bitset>
#include "pin.H"

// ip : instructionのアドレス
// addr  : readするアドレス
VOID ReadMemAnalysis(VOID * ip, VOID * addr){
    printf("Mem Read(Dynamic) : %p\n",addr);
    fflush(stdout);
}

// ip : instructionのアドレス
// addr : writeのアドレス
VOID WriteMemAnalysis(VOID * ip, VOID * addr){
    printf("Mem write(Dynamic) : %p\n",addr);
    fflush(stdout);
}


// ip : instructionのアドレス
// addr  : readするアドレス
VOID Static_ReadMemAnalysis(VOID * ip, VOID * addr){
    printf("Mem Read(static) : %p\n",addr);
    fflush(stdout);
}

// ip : instructionのアドレス
// addr : writeのアドレス
VOID Static_WriteMemAnalysis(VOID * ip, VOID * addr){
    printf("Mem write(static) : %p\n",addr);
    fflush(stdout);
}

/* ===================================================================== */
/*      Trace Implement                                                  */
/* ===================================================================== */

VOID Trace(TRACE trace, VOID *v){
    for(BBL bbl = TRACE_BblHead(trace);BBL_Valid(bbl); bbl = BBL_Next(bbl)){
        for(INS ins = BBL_InsHead(bbl);INS_Valid(ins);ins=INS_Next(ins)){

            UINT32 memOperands = INS_MemoryOperandCount(ins);
            for(UINT32 memOp=0;memOp<memOperands;memOp++){
                if(INS_MemoryOperandIsRead(ins,memOp)){         // Read Access
                    INS_InsertPredicatedCall(
                            ins,IPOINT_BEFORE, (AFUNPTR) ReadMemAnalysis,
                            IARG_INST_PTR,                      // 計装されるinstructionのアドレス
                            IARG_MEMORYOP_EA , memOp,           // メモリオペランドの有効アドレス
                            IARG_END);
                }
                if(INS_MemoryOperandIsWritten(ins,memOp)){      // Write access
                    INS_InsertPredicatedCall(
                            ins,IPOINT_BEFORE, (AFUNPTR) WriteMemAnalysis,
                            IARG_INST_PTR,                      // 計装されるinstructionのアドレス
                            IARG_MEMORYOP_EA , memOp,           // メモリオペランドの有効アドレス
                            IARG_END);
                }
            }
        }
    }
}

VOID Instruction(INS ins,VOID *v){
    UINT32 memOperands = INS_MemoryOperandCount(ins);

    for (UINT32 memOp = 0; memOp < memOperands; memOp++)
    {
        if (INS_MemoryOperandIsRead(ins, memOp))
        {
            INS_InsertPredicatedCall(
                ins, IPOINT_BEFORE, (AFUNPTR)Static_ReadMemAnalysis,
                IARG_INST_PTR,
                IARG_MEMORYOP_EA, memOp,
                IARG_END);
        }
        if (INS_MemoryOperandIsWritten(ins, memOp))
        {
            INS_InsertPredicatedCall(
                ins, IPOINT_BEFORE, (AFUNPTR)Static_WriteMemAnalysis,
                IARG_INST_PTR,
                IARG_MEMORYOP_EA, memOp,
                IARG_END);
        }
    }
}

VOID Fini(INT32 code,VOID *v){}

/* ===================================================================== */
/* Print Help Message                                                    */
/* ===================================================================== */

int main(int argc,char* argv[]){
    PIN_InitSymbols();
    PIN_Init(argc,argv);

    // Try Analysis read and write static
    INS_AddInstrumentFunction(Instruction, 0);

    // Try Analysis read and write dynamic
    //TRACE_AddInstrumentFunction(Trace,0);

    // Register Fini to be called when the application exist
    PIN_AddFiniFunction(Fini,0);

    // Start the Program never returns;
    PIN_StartProgram();
    return 0;
}
