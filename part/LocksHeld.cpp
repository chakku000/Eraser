#include <cstdint>
#include <map>
#include <bitset>

#include <iostream>
#include <vector>
#include <string>


using namespace std;

using Locks = bitset<128>;      // ロックの最大個数を128とする

struct LockManager{
    public:
        uint32_t index=0;   // 次のロックに割り振る値
        map<string,uint32_t> addressToIndex;  // ロックのアドレス -> ロックのインデックス のテーブル

    public:
        /*
         * ロックのアドレスを与えて,そのロックの番号を返す
         */
        uint32_t getLockNumber(string addr){
            if(addressToIndex.count(addr)) return addressToIndex[addr];
            else return addressToIndex[addr] = index++;
        }
};

struct LocksHeld{
    private:
        std::map<uint32_t,locks> locks_held;
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

int main(){
    LockManager lm;

    vector<string> vs{"shiratsuyu","murasame","shiugre","yuudachi","harusame"};

    for(auto s : vs){
        cout << lm.getLockNumber(s) << endl;
    }


    for(auto p : lm.addressToIndex){
        cout << p.first << " " << p.second << endl;
    }
}
