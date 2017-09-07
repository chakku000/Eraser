#include <iostream>
#include <fstream>
#include <cstdint>
//#include <cpthread>
#include "pin.H"

VOID Trace(TRACE trace, VOID *v){
    for(BBL bbl = TRACE_BblHead(trace);BBL_Valid(bbl); bbl = BBL_Next(bbl)){
        for(INS ins = BBL_InsHead(bbl);INS_Valid(ins);ins=INS_Next(ins)){
            /*
             * AFUNPTR functionに与える引数
             * arg0 : IARG_THREAD_ID(スレッドID)
             */
            INS_InsertCall(ins,IPOINT_BEFORE,(AFUNPTR),IARG_THREAD_ID,IARG_END);
        }
    }
}



/* ===================================================================== */
/*      Replacement Routine                                              */
/* ===================================================================== */

/*
 * pthread_mutex_lockを置換してpthread_mutex_lockを行ったあとに
 * 実行スレッドの保持するlocks_held()に対象のロックを追加する
 */
VOID* Jit_PthreadMutexLock(CONTEXT * context , AFUNPTR orgFuncptr,pthread_mutex_lock* mu){
    VOID * ret; // 返り値 いらないか...

    uint32_t thread_id = PIN_ThreadID();

    std::cerr << "pthread_mutex_lock replaced..." << std::endl;
    std::cerr << "Thread( " << thread_id << " ) try get lock (" <<  mu << ")" << std::endl;

    // pthread_mutex_lockを実行
    PIN_CallApplicationFunction(
            context , PIN_ThreadID(),
            CALLINGSTD_DEFAULT,
            orgFuncptr,
            NULL,                       // デフォルトの挙動
            PIN_PARG(void*), &ret,
            PIN_PARG(size_t), mu,
            PIN_PARG_END());
}



/* ===================================================================== */
/* プログラム実行前にimageを検査して,ロックの解放,獲得の関数を置き換える */
/* ===================================================================== */

/*
 *  pthread_mutex_lock() 関数を置き換える
 */
VOID ImageLoad(IMG img,VOID *v){

    // replace pthread_mutex_lock
    {
        // dEFINE A FUNCTION PROTOTYPE THAT DESCRIBES THE APPLICATION ROUTINE THAT WILL BE REPLACED
        proto PROTO_PTHREAD_MUTEX_LOCK = proto_aLLOCATE(pin_parg(VOID*),    // 返り値の型
                callingstd_default, // 推奨値のまま
                "PTHREAD_MUTEX_LOCK",   // 関数名
                pin_parg(PTHREAD_MUTEX_LOCK*),  // 引数の型
                pin_parg_end());

        // sEE IF PTHREAD_MUTEX_LOCK() IS PRESENT IN THE IMAGE
        RTN rtn = rtn_fINDbYnAME(IMG,"PTHREAD_MUTEX_LOCK");
        IF(rtn_vALID(RTN)){
            // REPLACE PTHREAD_MUTEX_LOCK
            // 参考に tOOLuNITtEST/REPLACE_MALLOC_INIT.CPP
            rtn_rEPLACEsIGNATURE(
                    RTN,afunptr(),  // 置換ルーチンと置換された結果の関数ポインタ
                    iarg_prototype, PROTO_PTHREAD_MUTEX_LOCK,   // 置換対象の情報
                    iarg_context,
                    iarg_orig_funcptr,
                    iarg_funcarg_entrypoint_value,  // PTHRED_MUTEX_LOCKに対する引数を渡すという宣言
                    0,                              // 0番目の引数を渡す
                    iarg_end);
        }
    }
}

KNOB<string> KnobOutputFile(KNOB_MODE_WRITEONCE, "pintool", "o", "inscount.out", "specify output file name");

VOID Fini(INT32 code,VOID *v){}

/* ===================================================================== */
/* Print Help Message                                                    */
/* ===================================================================== */

INT32 Usage()
{
    cerr << "This tool counts the number of dynamic instructions executed" << endl;
    cerr << endl << KNOB_BASE::StringKnobSummary() << endl;
    return -1;
}

int main(int argc,char* argv[]){
    // Initialie pin
    if(PIN_Init(argc,argv)) return Usage();

    OutFile.open(KnobOutputFile.Value().c_str());

    // Imageを静的に検査して
    IMG_AddInstrumentFunction(ImageLoad_Pthread_mutex_lock,0);

    // Register Instruction to be called to instrument instruction
    TRACE_AddInstrumentFunction(Trace,0);

    // Register Fini to be called when the application exist
    PIN_AddFiniFunction(Fini,0);

    // Start the Program never returns;
    PIN_StartProgram();
    return 0;
}
