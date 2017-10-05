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
        /**
         * @brief メモリの状態を表す列挙子
         */
        enum State{/*{{{*/
            Virgin,
            Exclusive,
            Shared,
            SharedModified,
            Race,
        };/*}}}*/

        /**
         * @brief enum Stateの状態を文字列でストリームに流す
         */
        friend std::ostream& operator<<(std::ostream& os, const State state){/*{{{*/
            string s;
            if(state == Virgin) s="Virgin";
            else if(state == Exclusive) s="Exclusive";
            else if(state == Shared) s="Shared";
            else if(state == SharedModified) s="SharedModified";
            else if(state == Race) s="Race";
            else s = "UnknownState";
            os << s;
            return os;
        }/*}}}*/

        /**
         * @brief アクセスを保存するクラス
         * @details アクセスのスレッド番号,アクセスしたスレッドの保持するロック,アクセスタイプ,アクセス前と後のメモリ状態
         */
        class Access{/*{{{*/
            public:
                uint32_t threadid;
                Lockset lk;
                string type;
                State st1,st2;  // st1 : アクセス前の状態 , st2 : アクセス後の状態
                Access(uint32_t th , Lockset lks , string t,State st1_,State st2_) : threadid(th) , lk(lks) , type(t), st1(st1_) , st2(st2_){}
        };/*}}}*/

        /**
         * @brief アクセスをostreamに流して出力可能にする
         */
        friend std::ostream& operator<<(std::ostream& os, const Access& ac){/*{{{*/
            os << ac.threadid << " " << ac.lk << " " << ac.type << " " << ac.st1 << "->" << ac.st2;
            return os;
        }/*}}}*/


        Lockset lockset;
        uint32_t th;        // 最初にアクセスしたスレッド番号
        uint64_t addr;      // 監視対象のアドレス
        State state;
        std::vector<Access> history;
    public:
        /*------------------------------------------------------------------------------*/
        /* Constructor                                                                  */
        /* 初期状態をExclusiveにすることで事前にアドレスを保持する必要をなくす          */
        /*------------------------------------------------------------------------------*/

        ShadowWord(){}
        ShadowWord(uint32_t thread_id,uint64_t ad){
            th = thread_id;
            addr = ad;
            state = Exclusive;
            lockset.set();
            // 最初のアクセス
            Access ac(thread_id,lockset,"Init",Virgin,state);
            history.push_back(ac);
        }

        /* -----------------------------------*/
        /* Update state and candidate lockset */
        /* -----------------------------------*/


        /**
         * @brief 読み込みアクセスに対するshadow wordの更新
         * @return データ競合がある場合はtrueを,それ以外はfalseを返す
         */
        bool read_access(uint32_t thread_id , LockSet locksheld){
            if(state==Race) return false;   // 既に競合状態ならば何もしない
            State before = state;
            State after = state;
            bool race = false;              // 競合チェック. 競合がある場合にtrueになる

            // アクセスによる次のStateを決定
            switch(state){
                case Virgin:
                    after = Exclusive;
                    th = thread_id;
                    break;
                case Exclusive:
                    if(th != thread_id) after = Shared;
                    break;
                case Shared:
                    break;
                case SharedModified:
                    break;
                default:
                    break;
            }

            // Stateの更新
            state = after;

            // C(v)の更新
            if(state == Shared or state == SharedModified){
                lockset &= locksheld;
            }

            // 競合チェック
            if(state == SharedModified and !lockset.any()){
                race = true;
                state = after = Race;
            }

            // 履歴に追加
            history.push_back(Access(thread_id,locksheld,"Read",before,after));

            // データ競合通知
            if(race){
                std::cerr << "Datarace found(READ).  " << "Address(" << std::hex << addr << ")" << " ThreadID(" << thread_id << ")" << std::endl;
                for(size_t i=0;i<history.size();i++) std::cerr << history[i] << endl;
            }
            return race;
        }

        /**
         * @brief 書き込みアクセスによるshadow wordの更新
         * @return データ競合がある場合はtrueを,それ以外はfalseを返す
         */
        bool write_access(uint32_t thread_id,LockSet locksheld){
            if(state==Race) return false;   // 既に競合状態ならば何もしない
            State before = state;
            State after = state;
            bool race = false;              // 競合チェック. 競合がある場合にtrueになる

            // アクセスによる次のStateを決定
            switch(state){
                case Virgin:
                    after = Exclusive;
                    th = thread_id;
                    break;
                case Exclusive:
                    if(th != thread_id) after = SharedModified;
                    break;
                case Shared:
                    after = SharedModified;
                    break;
                case SharedModified:
                    break;
                default:
                    break;
            }

            // Stateの更新
            state = after;

            // C(v)の更新
            if(state == Shared or state == SharedModified){
                lockset &= locksheld;
            }

            // 競合チェック
            if(state == SharedModified and !lockset.any()){
                race = true;
                state = after = Race;
            }

            // 履歴に追加
            history.push_back(Access(thread_id,locksheld,"Write",before,after));

            if(race){
                std::cerr << "Datarace found(WRITE). " << "Address(" << std::hex << addr << ")" << " ThreadID(" << thread_id << ")" << std::endl;
                for(size_t i=0;i<history.size();i++) std::cerr << history[i] << std::endl;
            }
            return race;
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

            std::cout << "history" << endl;
            for(size_t i=0;i<history.size();i++){
                Access ac = history[i];
                cerr << ac.threadid << " " << ac.lk << " " << ac.type << endl;
            }
            std::cout << std::endl;
        }
};

