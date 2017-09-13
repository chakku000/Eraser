/*!
 * @file locksheld.hpp
 * @brief locksheldの構造体
 */

#include <map>
#include <cstdint>

#ifndef INCLUDE_LODKSHELD
#define INCLUDE_LODKSHELD
#endif

/*!
 * @struct LocksHeld
 * @brief 各スレッドとそのスレッドのもつロック集合を対応付ける
 */
template<typename ThreadID , typename Lockset>
struct LocksHeld{
    private:
        std::map<ThreadID,Lockset> locks_held;
    public:
        /*
         * スレッドIDがthread_idのスレッドのlocks_held(thread_id)を返す
         */
        Lockset getLocks(const ThreadID thread_id){
            return locks_held[thread_id];
        }

        /*
         * arg0 thread_id : スレッドid
         * arg1 lkid      : ロックID
         * thread_idのスレッドのlocks_held(thread_id)にロックlkidを追加
         */
        void addLock(const ThreadID thread_id ,const ThreadID lkid){
            Lockset lk = locks_held.count(thread_id) ? locks_held[thread_id] : 0;
            lk |= (1 << lkid);
            locks_held[thread_id] = lk;
        }

        /*
         * arg0 thread_id : スレッドid
         * arg1 lkid      : ロックID
         * thread_idのスレッドのlocks_held(thread_id)からロックlkidを除去
         */
        void deleteLock(const ThreadID thread_id , const ThreadID lkid){
            Lockset mask = ~(1 << lkid);
            locks_held[thread_id] &= mask;
        }
};
