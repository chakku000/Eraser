#include <bitset>

#define INCLUDE_TYPE

#ifndef INCLUDE_CONSTANT
#include "constant.hpp"
#endif

using Locks = std::bitset<max_lock>;      // ロックの最大個数を128とする
