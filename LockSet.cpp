#include <iostream>
#include <bitset>
#include <map>
#include <cstdint>
#include <string>


constexpr uint32_t max_lock = 10;
using Locks = std::bitset<max_lock>;      // ロックの最大個数を128とする

template<typename Key,typename Val>
struct LockManager{
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
};


struct LocksHeld{
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
};

struct ShadowWord{
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
};

LockManager<pthread_mutex_t*,uint32_t> lockmanager;
LocksHeld locks_held;
std::map<uint32_t,ShadowWord> candidateLockset;

int main(){

    Locks lock1,lock2,lock3;
    for(int i=0;i<7;i++) if(i%2==0) lock1 |= (1<<i);
    std::cout << "thread1 = "<< lock1 << std::endl;

    for(int i=1;i<=5;i++) lock2 |= (1<<i);
    std::cout << "thread2 = " << lock2 << std::endl;

    std::cout << std::endl;

    std::cout << "thread1 read" << std::endl;
    candidateLockset[1].read_access(1,lock1);
    candidateLockset[1].print();


    std::cout << "thread1にlock8を追加" << std::endl;
    lock1 |= (1<<8);

    std::cout << "thread1 = "<< lock1 << std::endl;

    std::cout << "thread1 read" << std::endl;
    candidateLockset[1].read_access(1,lock1);
    candidateLockset[1].print();

    std::cout << "thread1 write" << std::endl;
    candidateLockset[1].write_access(1,lock1);
    candidateLockset[1].print();

    std::cout << "thread2 read" << std::endl;
    candidateLockset[1].read_access(2,lock2);
    candidateLockset[1].print();

    std::cout << "thread1 read" << std::endl;
    candidateLockset[1].read_access(1,lock1);
    candidateLockset[1].print();

    std::cout << "thread2 からロック2を除去" << std::endl;
    lock2 &= ~(1<<2);
    std::cout << "lock2 = " << lock2 << std::endl;

    std::cout << "thread2 write" << std::endl;
    candidateLockset[1].write_access(2,lock2);
    candidateLockset[1].print();


    std::cout << "thread3 はロック1だけをもつ" << std::endl;
    lock3 = (1<<1);
    
    std::cout << "thread3がwrite access" << std::endl;
    candidateLockset[1].write_access(3,lock3);
    candidateLockset[1].print();
}
