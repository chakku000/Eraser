/**
 *  @file type.hpp
 *  @brief 型エイリアスについてのファイル
 */
#include <bitset>

#define INCLUDE_TYPE

#ifndef INCLUDE_CONSTANT
#include "constant.hpp"
#endif

//! ロックの集合をビットで表現
using LockSet = std::bitset<max_lock>;      // ロックの最大個数を128とする
