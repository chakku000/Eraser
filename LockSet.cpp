#include <iostream>
#include <bitset>
#include <map>
#include <cstdint>


constexpr uint32_t max_lock = 128;
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
        ShadowWord(){
            state = Virgin;
            lockset.set();
        }
        ShadowWord(uint32_t thread_id){
            th = thread_id;
            state = Virgin;
            lockset.set();
        }

        void read_access(uint32_t thread_id , Locks locksheld){
            if(state == Virgin){
                state = Exclusive;
            }else if(state == Exclusive){
                if(th != thread_id){
                    state = Shared;
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
            }else if(state == Exclusive){
                if(th != thread_id){
                    state = SharedModified;
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
};

LockManager<pthread_mutex_t*,uint32_t> lockmanager;
LocksHeld locks_held;

int main(){

}
