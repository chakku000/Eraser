#include <iostream>
#include <cstdint>
#include <algorithm>
#include <vector>
#include <map>
#include <bitset>

using int32 = int32_t;
using uint32 = uint32_t;

namespace Debug{
    void printBit(uint32 bit){
        std::cout << static_cast<std::bitset<32>>(bit) << std::endl;
    }
};

struct ShadowWord{
    private:
        uint32 LocksetIndexMask = (1 << 30) - 1; // 下位30bitを取ってくるマスク
        uint32 TwoBitMask = (1 << 2)  - 1;    // 下位2bitを取ってくるマスク

        uint32 word;    // 下位30bitがロックセットインデックス, 上位2bitが状態

    public:
        ShadowWord(){}
        ShadowWord(uint32 word_) : word(word_){}

        uint32 getLocksetIndex(){   // ロックセットインデックスの取得
            return word & LocksetIndexMask;
        }

        uint32 getState(){          // 状態の取得
            return (word >> 30) & TwoBitMask;
        }

        void updateIndex(uint32 index){
            uint32 state = getState();
            state <<= 30;
            state |= index;
            word = state;
        }

        void updateState(uint32 state){
            uint32 t = word & (3 << 30);    // 上位2bitのマスク
            t |= (state << 30);
            word = t;
        }
};

int main(){
    using namespace Debug;

    uint32 t = 0;
    printBit(t);
    printBit(3<<30);
}
