/*==================================================================
*   Copyright (C) 2016 All rights reserved.
*   
*   filename:     gossip_msg.h
*   author:       Meng Weichao
*   created:      2016/05/26
*   description:  
*
================================================================*/
#ifndef __GOSSIP_MSG_H__
#define __GOSSIP_MSG_H__

enum MessageType {
    UNKOWN = 1000,
    PING = 1001,
    PING_REQ = 1002,
    ACK = 1003
};

enum StatusType {
    ALIVE = 2001,
    SUSPICIOUS = 2002,
    DEAD = 2003,
    JOIN = 2004
};

enum eErrorMsg
{
    E_RECV_ERROR = -1,      //接收错误
    E_MSG_ILLEGAL = -2,     //通信包非法
};

struct ClientReq {
    uint32_t total_len;
    MessageType message_type;
    uint32_t sequence_no;
    uint32_t suspect_host; // used for indirect req
    uint32_t suspect_port; // used for indirect req
    uint32_t has_payload;
    uint32_t incarnation;
    StatusType status_type;
    uint32_t source_host;
    uint32_t source_port;
    uint32_t target_host;
    uint32_t target_port;
    ClientReq(): sequence_no(0), has_payload(1), target_host(0), target_port(0) {}
};

typedef struct ClientReq ClientReq;


#endif //__GOSSIP_MSG_H__
