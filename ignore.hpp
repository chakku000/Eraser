/**
 * @file ignore.hpp
 * @detail 計装しないかどうかの判定を行う
 */

#include <string>
#include "pin.H"

namespace IGNORE{
    // 無視するときはtrue
    bool isImgIgnore(const string& imgname){
        if(imgname == "/lib64/ld-linux-x86-64.so.2") return true;
        else if(imgname == "/lib/x86_64-linux-gnu/libpthread.so.0") return true;
        else return false;
    }

    // 無視するときはtrue
    bool isInsIgnore(INS ins){
        IMG img = IMG_FindByAddress(INS_Address(ins)); 
        string imgName;
        if(IMG_Valid(img)) imgName = IMG_Name(img);
        else return false;

        return isImgIgnore(imgName);
    }
};
