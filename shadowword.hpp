/*!
 * @file shadowword.hpp
 * @brief ShadowWordの定義
 */

#include <iostream>
#include <cstdio>
#include <cstdint>

#ifndef INCLUDE_TYPE
#include "type.hpp"
#endif

#ifndef INCLUDE_SHADOWWORD
#define INCLUDE_SHADOWWORD
#endif

template<class Lockset>
struct ShadowWord{
    private:
        enum State{
            Virgin,
            Exclusive,
            Shared,
            SharedModified,
        };
        Lockset lockset;
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
        void read_access(uint32_t thread_id , LockSet locksheld){
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
                    std::cerr << "\x1b[41mCandidate Lock Set is empty\x1b[0m" << std::endl;
                }
            }
        }

        void write_access(uint32_t thread_id,LockSet locksheld){
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
                    std::cerr << "\x1b[41mCandidate Lock Set is empty\x1b[0m" << std::endl;
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

