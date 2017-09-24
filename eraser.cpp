#include <cstdio>
#include <iostream>
#include <fstream>
#include <cstdint>
#include <pthread.h>
#include <map>
#include <bitset>
#include "pin.H"


/* ===================================================================== */
/*      Include Files                                                    */
/* ===================================================================== */

#ifndef INCLUDE_CONSTANT
#include "constant.hpp"
#endif

#ifndef INCLUDE_TYPE
#include "type.hpp"
#endif

#ifndef INCLUDE_LOCKMANAGER
#include "lockmanager.hpp"
#endif

#ifndef INCLUDE_SHADOWWORD
#include "shadowword.hpp"
#endif

#ifndef INCLUDE_LOCKSHELD
#include "locksheld.hpp"
#endif

//#define ERASER_DEBUG

/* ===================================================================== */
/*      Grobal Variable                                                  */
/* ===================================================================== */
LockManager<pthread_mutex_t*,uint32_t> lockmanager;
LocksHeld<THREADID,LockSet> locks_held;

//! {key:検査する変数のアドレス val:変数のシャドーワード}となっているシャドーワード管理用map
std::map<ADDRINT,ShadowWord<LockSet>> candidateLockset; // key : 変数のアドレス , val : 変数のshadow word
PIN_LOCK pinlock;
PIN_LOCK C_lock;

//bool update_Cv = true;
//PIN_LOCK update_Cv_lock;

bool implementOn = false;

PIN_LOCK print_lock;
PIN_LOCK rwlock;

/* ===================================================================== */
/*      Analysis Read and Write access                                   */
/* ===================================================================== */

/*
 * @fn
 * @brief メモリにREADアクセスが発生したときに動作する関数
 * @param ip  insturuction address
 * @param addr readするアドレス
 * @detail  メモリアドレスaddrがREADされたらそのアドレスに対応するシャドーワードを更新し,更新の結果によってはエラーを出力する
 */
// VOID ReadMemAnalysis(VOID * ip, VOID * addr){
VOID ReadMemAnalysis(VOID * ip, ADDRINT addr){/*{{{*/
    //if(!update_Cv or !implementOn) return;
    // スレッドID
    THREADID thread_id = PIN_ThreadId();

    // OSが割り当てるのと同じスレッドIDを使用するなら次のようにする
    //OS_THREAD_ID os_thread_id = PIN_GetTid();

    //PIN_GetLock(&print_lock,thread_id+1);
    //std::cerr << "READ  " << thread_id << " " << std::hex << addr << std::endl;
    //PIN_ReleaseLock(&print_lock);

    //PIN_GetLock(&rwlock,thread_id+1);
    // スレッドの保持するロック集合
    PIN_GetLock(&pinlock,thread_id+1);
    LockSet locks = locks_held.getLocks(thread_id);
    PIN_ReleaseLock(&pinlock);

    // C(v) を更新
    PIN_GetLock(&C_lock,thread_id+1);
    if(!candidateLockset.count(addr)){
        candidateLockset[addr] = ShadowWord<LockSet>(thread_id,addr);
    }{
        candidateLockset[addr].read_access(thread_id,locks);
    }
    PIN_ReleaseLock(&C_lock);
    //PIN_ReleaseLock(&rwlock);
}/*}}}*/

/*
 * @fn
 * @brief メモリにWRITEアクセスが発生したときに動作する関数
 * @param ip  insturuction address
 * @param addr writeするアドレス
 * @detail  メモリアドレスaddrがWRITEされたらそのアドレスに対応するシャドーワードを更新し,更新の結果によってはエラーを出力する
 */
VOID WriteMemAnalysis(VOID * ip, ADDRINT addr){/*{{{*/
    //if(!update_Cv or !implementOn) return;
    // スレッドID
    THREADID thread_id = PIN_ThreadId();

    // OSが割り当てるのと同じスレッドIDを使用するなら次のようにする
    //OS_THREAD_ID os_thread_id = PIN_GetTid();

    //PIN_GetLock(&print_lock,thread_id+1);
    //std::cerr << "WRITE " << thread_id << " " << std::hex << addr << std::endl;
    //PIN_ReleaseLock(&print_lock);

    //PIN_GetLock(&rwlock,thread_id+1);
    // スレッドの保持するロック集合
    PIN_GetLock(&pinlock,thread_id+1);
    LockSet locks = locks_held.getLocks(thread_id);
    PIN_ReleaseLock(&pinlock);

    // C(v) を更新
    PIN_GetLock(&C_lock,thread_id+1);
    if(!candidateLockset.count(addr)){
        candidateLockset[addr] = ShadowWord<LockSet>(thread_id,addr);
    }{
        candidateLockset[addr].write_access(thread_id,locks);
    }
    PIN_ReleaseLock(&C_lock);
    //PIN_ReleaseLock(&rwlock);
}/*}}}*/

/*
 * @fn
 * @brief mainからreturnされるときに計装を停止する
 */
VOID AnalysisReturnFromMain(){/*{{{*/
    implementOn = false;
    std::cout << "End Implementation" << std::endl;
}/*}}}*/

/* ===================================================================== */
/*      Trace Implement                                                  */
/* ===================================================================== */

VOID Trace(TRACE trace, VOID *v){/*{{{*/
    for(BBL bbl = TRACE_BblHead(trace);BBL_Valid(bbl); bbl = BBL_Next(bbl)){
        for(INS ins = BBL_InsHead(bbl);INS_Valid(ins);ins=INS_Next(ins)){

            UINT32 memOperands = INS_MemoryOperandCount(ins);
            for(UINT32 memOp=0;memOp<memOperands;memOp++){
                if(INS_MemoryOperandIsRead(ins,memOp)){         // Read Access
                    //INS_InsertCall(
                    //bool ret = true;
                    INS_InsertPredicatedCall(
                            ins,IPOINT_BEFORE, (AFUNPTR) ReadMemAnalysis,
                            IARG_INST_PTR,                      // 計装されるinstructionのアドレス
                            IARG_MEMORYOP_EA , memOp,           // メモリオペランドの有効アドレス
                            IARG_END);
                }
                if(INS_MemoryOperandIsWritten(ins,memOp)){      // Write access
                    //INS_InsertCall(
                    //bool ret = true;
                    INS_InsertPredicatedCall(
                            ins,IPOINT_BEFORE, (AFUNPTR) WriteMemAnalysis,
                            IARG_INST_PTR,                      // 計装されるinstructionのアドレス
                            IARG_MEMORYOP_EA , memOp,           // メモリオペランドの有効アドレス
                            IARG_END);
                }
            }

            // 命令がmain関数のreturnのとき,計装を終了する
            if(INS_IsRet(ins)){
                RTN rtn = INS_Rtn(ins);
                if(RTN_Valid(rtn)){
                    std::string rtn_name = RTN_Name(rtn);
                    if(rtn_name == "main"){
                        INS_InsertCall(ins,IPOINT_BEFORE,(AFUNPTR)AnalysisReturnFromMain,IARG_CALL_ORDER,CALL_ORDER_LAST,IARG_END);
                    }
                }
            }
        }
    }
}/*}}}*/


/* ===================================================================== */
/*      Replacement Routine                                              */
/* ===================================================================== */
/*{{{*/
/*
 * pthread_mutex_lockを置換してpthread_mutex_lockを行ったあとに
 * 実行スレッドの保持するlocks_held()に対象のロックを追加する
 */
int Jit_PthreadMutexLock(CONTEXT * context , AFUNPTR orgFuncptr,pthread_mutex_t* mu){
    int ret = 0; // 返り値 いらないか...

    uint32_t thread_id = PIN_ThreadId();

    //if(implementOn) std::cerr << "pthread_mutex_lock replaced. Thread(" << thread_id << ") lock (" << mu << ")" << std::endl;


    /* ------------------------------
     *  pthread_mutex_lock(&mu)を実行
     * ------------------------------ */
    //CALL_APPLICATION_FUNCTION_PARAM  param = CALL_APPLICATION_FUNCTION_PARAM::native;
    PIN_CallApplicationFunction(
            context , PIN_ThreadId(),
            CALLINGSTD_DEFAULT,
            orgFuncptr,
            NULL,                       // デフォルトの挙動
            PIN_PARG(int), &ret,
            PIN_PARG(pthread_mutex_t*), mu,
            PIN_PARG_END());


    /* --------------------------
     * locks_held(t) を更新する
     * --------------------------*/
    PIN_GetLock(&pinlock,thread_id+1);      // スレッドIDはデバッグ用であるが,lockにセットされる値なのでnon-zeroである必要がある
    uint32_t lkid = lockmanager.getLockNumber(mu);  // ロックIDの取得
    locks_held.addLock(thread_id,lkid);
    PIN_ReleaseLock(&pinlock);

    return ret;
}

/*
 * pthread_mutex_unlockを置換してpthread_mutex_unlockのコール時に
 * 実行スレッドの保持するlocks_held()から対象のロックを削除する
 */
int Jit_PthreadMutexUnlock(CONTEXT *context , AFUNPTR orgFuncptr , pthread_mutex_t* mu){
    int ret = 0;

    uint32_t thread_id = PIN_ThreadId();

    //if(implementOn) std::cerr << "pthread_mutex_unlock replaced. Thread(" << thread_id << ") unlock (" << mu << ")" << std::endl;

    /* ------------------------------
     * locks_held(t) を更新
     * ------------------------------*/
    PIN_GetLock(&pinlock,thread_id+1);
    uint32_t lkid = lockmanager.getLockNumber(mu);  // ロックIDの取得
    locks_held.deleteLock(thread_id,lkid);
    PIN_ReleaseLock(&pinlock);

    /* ------------------------------
     *  pthread_mutex_unlock(&mu)を実行
     * ------------------------------ */
    PIN_CallApplicationFunction(
            context , thread_id,
            CALLINGSTD_DEFAULT,
            orgFuncptr,
            NULL,
            PIN_PARG(int),&ret,
            PIN_PARG(pthread_mutex_t*),mu,
            PIN_PARG_END());
    return ret;
}

/*}}}*/

/**
 * @fn
 * pthread_createをこの関数で置換する
 */
int Jit_PthreadCreate(CONTEXT * context , AFUNPTR orgFuncptr , pthread_t * th , pthread_attr_t * attr, void* fun_ptr , void* args){
    int ret = 0;
    // pthread_create関数を計装しない
    CALL_APPLICATION_FUNCTION_PARAM  param;
    param.native=1;
    PIN_CallApplicationFunction(
            context,PIN_ThreadId(),
            CALLINGSTD_DEFAULT,
            orgFuncptr,
            &param,
            PIN_PARG(int), &ret,
            PIN_PARG(pthread_t*), th,
            PIN_PARG(pthread_attr_t*), attr,
            PIN_PARG(void*), fun_ptr,
            PIN_PARG(void*), args,
            PIN_PARG_END());
    return ret;
}

/**
 * @fn
 * pthread_joinをこの関数で置換する
 */
int Jit_PthreadJoin(CONTEXT * context, AFUNPTR orgFuncptr, pthread_t th, void** thread_return){
    int ret = 0;
    // pthread_join関数を計装しないようにする
    CALL_APPLICATION_FUNCTION_PARAM  param;
    param.native=1;
    PIN_CallApplicationFunction(
            context,PIN_ThreadId(),
            CALLINGSTD_DEFAULT,
            orgFuncptr,
            &param,
            PIN_PARG(int),&ret,
            PIN_PARG(pthread_t), th,
            PIN_PARG(void**), thread_return,
            PIN_PARG_END());
    return ret;
}

VOID AnalysisMainEntrance(){
    std::cerr << "start main" << std::endl;
    implementOn = true;
}

/* ===================================================================== */
/* プログラム実行前にimageを検査して,ロックの解放,獲得の関数を置き換える */
/* ===================================================================== */

/*{{{*/
/*
 *  pthread_mutex_lock() 関数を置き換える
 */
VOID ImageLoad(IMG img,VOID *v){
    // find main
    {
        RTN rtn = RTN_FindByName(img,"main");
        if(RTN_Valid(rtn)){
            RTN_Open(rtn);

            RTN_InsertCall(rtn,IPOINT_BEFORE,(AFUNPTR)AnalysisMainEntrance,IARG_CALL_ORDER,CALL_ORDER_FIRST,IARG_END);

            RTN_Close(rtn);
        }
    }


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
        PROTO proto_pthread_mutex_unlock = PROTO_Allocate(PIN_PARG(int),    // 戻り値の型情報
                                                          CALLINGSTD_DEFAULT, // 推奨値
                                                          "pthread_mutex_unlock",   // 置換対象の関数名
                                                          PIN_PARG(pthread_mutex_t*),   // 引数の型情報
                                                          PIN_PARG_END());

        // See if pthread_mutex_unlock() is present in the image
        RTN rtn = RTN_FindByName(img,"pthread_mutex_unlock");

        // replace pthread_mutex_unlock
        if(RTN_Valid(rtn)){
            RTN_ReplaceSignature(
                    rtn,AFUNPTR(Jit_PthreadMutexUnlock),
                    IARG_PROTOTYPE, proto_pthread_mutex_unlock,
                    IARG_CONTEXT,
                    IARG_ORIG_FUNCPTR,
                    IARG_FUNCARG_ENTRYPOINT_VALUE,
                    0,
                    IARG_END);
        }
    }

    // replace pthread_thread_create
    {
        PROTO proto_pthread_create = PROTO_Allocate(
                                        PIN_PARG(int),      // 戻り値の型情報
                                        CALLINGSTD_DEFAULT,
                                        "pthread_create",
                                        PIN_PARG(pthread_t*),
                                        PIN_PARG(pthread_attr_t*),
                                        PIN_PARG(void*),
                                        PIN_PARG(void*),
                                        PIN_PARG_END());

        RTN rtn = RTN_FindByName(img,"pthread_create");

        if(RTN_Valid(rtn)){
            RTN_ReplaceSignature(
                    rtn,AFUNPTR(Jit_PthreadCreate),
                    IARG_PROTOTYPE,proto_pthread_create,
                    IARG_CONTEXT,
                    IARG_ORIG_FUNCPTR,
                    IARG_FUNCARG_ENTRYPOINT_VALUE, 0,   // pthread_t*
                    IARG_FUNCARG_ENTRYPOINT_VALUE, 1,   // pthread_attr_t*
                    IARG_FUNCARG_ENTRYPOINT_VALUE, 2,   // void *
                    IARG_FUNCARG_ENTRYPOINT_VALUE, 3,   // void *
                    IARG_END);
        }
    }

    // replace pthread_join
    {
        PROTO proto_pthread_join = PROTO_Allocate(
                                        PIN_PARG(int),  // pthread_joinの戻り値
                                        CALLINGSTD_DEFAULT,
                                        "pthread_join",
                                        PIN_PARG(pthread_t),
                                        PIN_PARG(void**),
                                        PIN_PARG_END());
        RTN rtn = RTN_FindByName(img,"pthread_join");

        if(RTN_Valid(rtn)){
            RTN_ReplaceSignature(
                    rtn,AFUNPTR(Jit_PthreadJoin),
                    IARG_PROTOTYPE,proto_pthread_join,
                    IARG_CONTEXT,
                    IARG_ORIG_FUNCPTR,
                    IARG_FUNCARG_ENTRYPOINT_VALUE,0,    // pthread_t
                    IARG_FUNCARG_ENTRYPOINT_VALUE,1,    // void**
                    IARG_END);
        }
    }
}


VOID Fini(INT32 code,VOID *v){}
///*}}}*/

/* ===================================================================== */
/* Print Help Message                                                    */
/* ===================================================================== */

int main(int argc,char* argv[]){
    // Initialize the pin Lock
    //PIN_InitLock(&pinlock);
    // Initialie pin
    PIN_InitSymbols();
    PIN_Init(argc,argv);

    // Imageを静的に検査して
    IMG_AddInstrumentFunction(ImageLoad,0);

    // Register Instruction to be called to instrument instruction
    TRACE_AddInstrumentFunction(Trace,0);

    // Register Fini to be called when the application exist
    PIN_AddFiniFunction(Fini,0);


    // Start the Program never returns;
    PIN_StartProgram();

    return 0;
}
