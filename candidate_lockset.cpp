#include <iostream>
#include <cstdint>
#include <algorithm>
#include <vector>
#include <map>
#include <unordered_map>
#include <set>
#include <bitset>

using uint8 = uint8_t;
using int32 = int32_t;
using uint32 = uint32_t;

using Lock = uint32;
using Locks = std::set<uint32>;


/*
 * relate lockset index(uint32) and Lock set(Lock)
 * typename T : type of lockset index
 * typename U : type of lockset
 */
template<typename T,typename U>
struct Map{
    private:
        std::unordered_map<T,U> index_to_locks;
        std::map<U,T> locks_to_index;
        uint32 next_index = 1;

    public:
        inline bool findlock(Lock lk){
            return locks_to_index.count(lk);
        }
        inline bool findindex(uint32 idx){
            return index_to_locks.count(idx);
        }
};

/*
 * shadow word correspond to the memory address
 */
struct ShadowWord{
    uint8 state;
    uint32 lockset_index;
    ShadowWord() : state(0) , lockset_index(0){}
    ShadowWord(uint8 s, uint32 li) : state(s) , lockset_index(li){}
};

/*
 *  meory address -> ShadowWord
 */
std::map<uint32,ShadowWord> shadows;

/*
 *  cache of intersection operation
 */
struct Cache{
    private:
        std::map<uint32,std::map<uint32,uint32>> cache_table;

    public:
        // is exist intersection of a and b ?
        bool exist(uint32 a, uint32 b){
            if(!cache_table.count(a) or !cache_table[a].count(b)) return false;
            else return true;
        }

        // insert the result(c) of intersection(a,b)
        void insert(uint32 a,uint32 b,uint32 c){
            cache_table[a][b]=c;
        }

        uint32 get(uint32 a,uint32 b){
            if(!exist(a,b)) return -1;
            return cache_table[a][b];
        }
};

/*
 *  intersection operation for Lockset and Candidate set
 */
template<typename T>
std::set<T> intersection(std::set<T>& a,std::set<T>& b){
    std::set<T> result;
    std::set_intersection(a.begin(),a.end(),b.begin(),b.end(),std::inserter(result,result.end()));
    return result;
}

int main(){
    // TEST
    std::vector<uint32> addrs{1,2,3,4,5};   // アドレス

    for(size_t i=0;i<addrs.size();i++){
        // 各アドレスに対するロック
        std::vector<Lock> Locks;
        for(int j=0;j<3;j++) Locks.emplace_back(i*j);

        /*
         * Locksに対応するLocksetIndexを検索したい
         */

    }
}
