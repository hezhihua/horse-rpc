#ifndef _TC_ENUM_H_
#define _TC_ENUM_H_    
    
    
    enum PACKET_TYPE
    {
        PACKET_LESS = 0,
        PACKET_FULL = 1,
        PACKET_FULL_CLOSE = 2,
        PACKET_ERR  = -1,
    };

    enum AUTH_TYPE
    {
        AUTH_TYPENONE =0,

    };
    enum AUTH_STATE
    {
        AUTH_SUCC =0,
        AUTH_INIT =-127,
    };



 #endif