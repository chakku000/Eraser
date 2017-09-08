#include <iostream>
#include <fstream>
#include <cstdint>
#include <pthread.h>
#include <cstdio>
#include "pin.H"

/*
VOID Trace(TRACE trace, VOID *v){
    for(BBL bbl = TRACE_BblHead(trace);BBL_Valid(bbl); bbl = BBL_Next(bbl)){
        for(INS ins = BBL_InsHead(bbl);INS_Valid(ins);ins=INS_Next(ins)){
             * AFUNPTR functionに与える引数
             * arg0 : IARG_THREAD_ID(スレッドID)
            INS_InsertCall(ins,IPOINT_BEFORE,(AFUNPTR),IARG_THREAD_ID,IARG_END);
        }
    }
}
*/



/* ===================================================================== */
/*      Replacement Routine                                              */
/* ===================================================================== */

/*
 * pthread_mutex_lockを置換してpthread_mutex_lockを行ったあとに
 * 実行スレッドの保持するlocks_held()に対象のロックを追加する
 */
int Jit_PthreadMutexLock(CONTEXT * context , AFUNPTR orgFuncptr,pthread_mutex_t* mu){
    int ret = 0; // 返り値 いらないか...

    uint32_t thread_id = PIN_ThreadId();

    std::cerr << "pthread_mutex_lock replaced..." << std::endl;
    std::cerr << "Thread( " << thread_id << " ) try get lock (" <<  mu << ")" << std::endl;

    // pthread_mutex_lockを実行
    PIN_CallApplicationFunction(
            context , PIN_ThreadId(),
            CALLINGSTD_DEFAULT,
            orgFuncptr,
            NULL,                       // デフォルトの挙動
            PIN_PARG(int), &ret,
            PIN_PARG(pthread_mutex_t*), mu,
            PIN_PARG_END());

    return ret;
}



/* ===================================================================== */
/* プログラム実行前にimageを検査して,ロックの解放,獲得の関数を置き換える */
/* ===================================================================== */


/*
 *  pthread_mutex_lock() 関数を置き換える
 */
VOID ImageLoad(IMG img,VOID *v){

    // replace pthread_mutex_lock
    // Define a function prototype that describes the application routine that will be replaced
    {
        PROTO proto_pthread_mutex_lock = PROTO_Allocate(PIN_PARG(int),    // 返り値の型
                CALLINGSTD_DEFAULT, // 推奨値のまま
                "pthread_mutex_lock",   // 関数名
                PIN_PARG(pthread_mutex_t*),  // 引数の型
                PIN_PARG_END());

        // See if pthread_mutex_lock() is present in the image
        RTN rtn = RTN_FindByName(img,"pthread_mutex_lock");
        if(RTN_Valid(rtn)){
            // replace pthread_mutex_lock
            // 参考に ToolUnitTest/replace_malloc_init.cpp
            RTN_ReplaceSignature(
                    rtn,AFUNPTR(Jit_PthreadMutexLock),  // 置換ルーチンと置換された結果の関数ポインタ
                    IARG_PROTOTYPE, proto_pthread_mutex_lock,   // 置換対象の情報
                    IARG_CONTEXT,
                    IARG_ORIG_FUNCPTR,
                    IARG_FUNCARG_ENTRYPOINT_VALUE,  // pthred_mutex_lockに対する引数を渡すという宣言
                    0,                              // 0番目の引数を渡す
                    IARG_END);
        }
    }

    // replace pthread_mutex_unlock
    {
    }
}


VOID Fini(INT32 code,VOID *v){}

/* ===================================================================== */
/* Print Help Message                                                    */
/* ===================================================================== */

int main(int argc,char* argv[]){
    // Initialie pin
    PIN_InitSymbols();
    PIN_Init(argc,argv);

    // Imageを静的に検査して
    IMG_AddInstrumentFunction(ImageLoad,0);

    // Register Instruction to be called to instrument instruction
    //TRACE_AddInstrumentFunction(Trace,0);

    // Register Fini to be called when the application exist
    PIN_AddFiniFunction(Fini,0);


    std::cout << "pass" << std::endl;
    // Start the Program never returns;
    PIN_StartProgram();
    return 0;
}
