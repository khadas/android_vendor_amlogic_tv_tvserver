//
//
//  amlogic 2013
//
//  @ Project : tv
//  @ File Name : CTvGroup.h
//  @ Date : 2013-11
//  @ Author :
//
//


#if !defined(_CTVGROUP_H)
#define _CTVGROUP_H

#include <utils/Vector.h>
using namespace android;
// Group对应DTV中的一个节目分组
class CTvGroup {
public:
    CTvGroup();
    ~CTvGroup();
    // 取得所有节目分组信息
    static Vector<CTvGroup> selectByGroup();
    static void addGroup();
    static void deleteGroup();
    static void editGroup();
};

#endif  //_CTVGROUP_H
