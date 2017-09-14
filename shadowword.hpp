/*!
 * @file shadowword.hpp
 * @brief ShadowWordの定義
 */

#include <iostream>
#include <cstdio>
#include <cstdint>
#include <vector>

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
        class Access{
            public:
                uint32_t threadid;
                Lockset lk;
                string type;
                Access(uint32_t th , Lockset lks , string t) : threadid(th) , lk(lks) , type(t){}
        };
        Lockset lockset;
        uint32_t th;
        State state;
        std::vector<Access> history;
    public:
        /*------------------------------------------------------------------------------*/
        /* Constructor                                                                  */
        /* 初期状態をExclusiveにすることで事前にアドレスを保持する必要をなくす          */
        /*------------------------------------------------------------------------------*/
        //ShadowWord(){
        //    fprintf(stderr,"new shadow word\n");
        //    state = Exclusive;
        //    lockset.set();
        //}
        ShadowWord(){}
        ShadowWord(uint32_t thread_id){
            th = thread_id;
            state = Exclusive;
            lockset.set();

            history.push_back(Access(thread_id,lockset,"init"));
        }

        /* -----------------------------------*/
        /* Update state and candidate lockset */
        /* -----------------------------------*/
        void read_access(uint32_t thread_id , LockSet locksheld){
            history.push_back(Access(thread_id,locksheld,"read"));
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
                    fprintf(stderr,"Datarace Found (READ). ThreadID (%d)\n",thread_id);
                    for(size_t i=0;i<history.size();i++){
                        Access ac = history[i];
                        cerr << ac.threadid << " " << ac.lk << " " << ac.type << endl;
                    }
                }
            }
        }

        void write_access(uint32_t thread_id,LockSet locksheld){
            history.push_back(Access(thread_id,locksheld,"write"));
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
                    fprintf(stderr,"Datarace Found (WRITE). ThreadID (%d)\n",thread_id);
                    for(size_t i=0;i<history.size();i++){
                        Access ac = history[i];
                        cerr << ac.threadid << " " << ac.lk << " " << ac.type << endl;
                    }
                }
            }
        }

        /**
         *  @brief デバッグ用
         */
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

