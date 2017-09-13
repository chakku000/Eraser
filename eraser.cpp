#include <cstdio>
#include <iostream>
#include <fstream>
#include <cstdint>
#include <pthread.h>
#include <map>
#include <bitset>
#include "pin.H"


/* ===================================================================== */
/*      Const Value                                                      */
/* ===================================================================== */
constexpr uint32_t max_lock = 128;

/* ===================================================================== */
/*      Type Alias                                                       */
/* ===================================================================== */
using Locks = bitset<max_lock>;      // ロックの最大個数を128とする

/* ===================================================================== */
/*      Data Structure                                                   */
/* ===================================================================== */

/* ------------------------------------------------------------*
 * ロックのアドレスとロック番号を対応付ける構造体
 * ------------------------------------------------------------*/
template<typename Key,typename Val> struct LockManager{/*{{{*/
    private:
        uint32_t index=1;   // 次のロックに割り振る値
        std::map<Key,Val> addressToIndex;  // ロックのアドレス -> ロックのインデックス のテーブル

    public:
        /*
         * ロックのアドレスを与えて,そのロックの番号を返す
         */
        Val getLockNumber(Key addr){
            if(addressToIndex.count(addr)) return addressToIndex[addr];
            else return addressToIndex[addr] = index++;
        }
};/*}}}*/

/* ------------------------------------------------------------*
 * 各スレッドの持つロックを管理
 * ------------------------------------------------------------*/
struct LocksHeld{/*{{{*/
    private:
        std::map<uint32_t,Locks> locks_held;
    public:
        /*
         * スレッドIDがthread_idのスレッドのlocks_held(thread_id)を返す
         */
        Locks getLocks(uint32_t thread_id){
            return locks_held[thread_id];
        }

        /*
         * arg0 thread_id : スレッドid
         * arg1 lkid      : ロックID
         * thread_idのスレッドのlocks_held(thread_id)にロックlkidを追加
         */
        void addLock(const uint32_t thread_id ,const uint32_t lkid){
            Locks lk = locks_held.count(thread_id) ? locks_held[thread_id] : 0;
            lk |= (1 << lkid);
            locks_held[thread_id] = lk;
        }

        /*
         * arg0 thread_id : スレッドid
         * arg1 lkid      : ロックID
         * thread_idのスレッドのlocks_held(thread_id)からロックlkidを除去
         */
        void deleteLock(const uint32_t thread_id , const uint32_t lkid){
            Locks mask = ~(1 << lkid);
            locks_held[thread_id] &= mask;
        }
};/*}}}*/

/* ------------------------------------------------------------*
 * shadow word (詳細は論文)
 * ------------------------------------------------------------*/
struct ShadowWord{/*{{{*/
    private:
        enum State{
            Virgin,
            Exclusive,
            Shared,
            SharedModified,
        };
        Locks lockset;
        uint32_t th;
        State state;
    public:
        /*------------------------------------------------------------------------------*/
        /* Constructor                                                                  */
        /* 初期状態をExclusiveにすることで事前にアドレスを保持する必要をなくす          */
        /*------------------------------------------------------------------------------*/
        ShadowWord(){
            state = Exclusive;
            lockset.set();
        }
        ShadowWord(uint32_t thread_id){
            th = thread_id;
            state = Exclusive;
            lockset.set();
        }

        /* -----------------------------------*/
        /* Update state and candidate lockset */
        /* -----------------------------------*/
        void read_access(uint32_t thread_id , Locks locksheld){
            if(state == Virgin){
                state = Exclusive;
                th = thread_id;
            }else if(state == Exclusive){
                if(th != thread_id){
                    state = Shared;
                    lockset &= locksheld;
                }
            }else if(state == Shared){
                lockset &= locksheld;
            }else if(state == SharedModified){
                lockset &= locksheld;
                if(!lockset.any()){
                    // 警告
                    std::cerr << "Candidate Lock Set is empty" << std::endl;
                }
            }
        }

        void write_access(uint32_t thread_id,Locks locksheld){
            if(state == Virgin){
                state = Exclusive;
                th = thread_id;
            }else if(state == Exclusive){
                if(th != thread_id){
                    state = SharedModified;
                    lockset &= locksheld;
                }
            }else if(state == Shared){
                state = SharedModified;
                lockset &= locksheld;
            }else if(state == SharedModified){
                lockset &= locksheld;
                if(!lockset.any()){
                    // 警告
                    std::cerr << "Candidate Lock Set is empty" << std::endl;
                }
            }
        }

        void print(){
            std::string s;
            if(state == Virgin) s = "Virgin";
            else if(state == Exclusive) s = "Exclusive";
            else if(state == Shared) s = "Shared";
            else if(state == SharedModified) s = "SharedModified";

            std::cout << "state = " << s << std::endl;
            std::cout << "C(v) = " << lockset << std::endl;
            if(state == Exclusive) std::cout << "thread = " << th << std::endl;

            std::cout << std::endl;
        }
};/*}}}*/


/* ===================================================================== */
/*      Grobal Variable                                                  */
/* ===================================================================== */
LockManager<pthread_mutex_t*,uint32_t> lockmanager;
LocksHeld locks_held;

//! {key:検査する変数のアドレス val:変数のシャドーワード}となっているシャドーワード管理用map
std::map<uint32_t,ShadowWord> candidateLockset; // key : 変数のアドレス , val : 変数のshadow word
PIN_LOCK pinlock;


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
VOID ReadMemAnalysis(VOID * ip, ADDRINT addr){
    // スレッドID
    THREADID thread_id = PIN_ThreadId();

    // OSが割り当てるのと同じスレッドIDを使用するなら次のようにする
    //OS_THREAD_ID os_thread_id = PIN_GetTid();

    // スレッドの保持するロック集合
    Locks locks = locks_held.getLocks(thread_id);

    // C(v) を更新
    candidateLockset[addr].read_access(thread_id,locks);
}

/*
 * @fn
 * @brief メモリにWRITEアクセスが発生したときに動作する関数
 * @param ip  insturuction address
 * @param addr writeするアドレス
 * @detail  メモリアドレスaddrがWRITEされたらそのアドレスに対応するシャドーワードを更新し,更新の結果によってはエラーを出力する
 */
VOID WriteMemAnalysis(VOID * ip, ADDRINT addr){
    // スレッドID
    THREADID thread_id = PIN_ThreadId();

    // OSが割り当てるのと同じスレッドIDを使用するなら次のようにする
    //OS_THREAD_ID os_thread_id = PIN_GetTid();

    // スレッドの保持するロック集合
    Locks locks = locks_held.getLocks(thread_id);

    // C(v) を更新
    candidateLockset[addr].write_access(thread_id,locks);
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

    std::cerr << "pthread_mutex_lock replaced. Thread(" << thread_id << ") lock (" << mu << ")" << std::endl;


    /* ------------------------------
     *  pthread_mutex_lock(&mu)を実行
     * ------------------------------ */
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

    std::cerr << "pthread_mutex_unlock replaced. Thread(" << thread_id << ") unlock (" << mu << ")" << std::endl;

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

/* ===================================================================== */
/* プログラム実行前にimageを検査して,ロックの解放,獲得の関数を置き換える */
/* ===================================================================== */

/*{{{*/
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
