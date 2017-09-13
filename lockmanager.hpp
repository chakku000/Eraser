/*!
 * @file lockmanager.hpp
 * @brief ロックのアドレスと番号を対応付ける
 */

#include <map>
#include <cstdint>

#ifndef INCLUDE_LOCKMANAGER
#define INCLUDE_LOCKMANAGER
#endif

/*!
 * @struct LockManager
 * @brief ロックに対して新しいロック番号を割り当てて対応付ける
 */

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
