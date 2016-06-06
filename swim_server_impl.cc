/*==================================================================
*   Copyright (C) 2016 All rights reserved.
*   
*   filename:     swim_server_impl.cc
*   author:       Meng Weichao
*   created:      2016/05/12
*   description:  
*
================================================================*/
#include "swim_server_impl.h"
#include "common/system/concurrency/thread.h"
#include "common/system/concurrency/this_thread.h"
#include "common/system/net/ip_address.h"
#include "common/base/string/string_number.h"
#include <iostream>
#include <memory>
#include <vector>
#include<cstdlib>
#include <algorithm>
#include <cmath>
#include<ctime>
#include "thirdparty/gflags/gflags.h"
#include "thirdparty/glog/logging.h"
#include "thirdparty/jsoncpp/json.h"

#include "poppy/rpc_channel.h"
#include "poppy/rpc_client.h"
#include "common/base/cxx11.h"
#include "common/base/closure.h"

#include "swim_client_msg.h"

#define MAXLINE 1024
#define MAX_BUF_LEN 4096

using namespace common;
using namespace std;
using google::protobuf::RepeatedPtrField;

SWIMServerImpl::SWIMServerImpl(string& host, string& port): seq_no_(0), incarnation_(0){
    StringToNumber(host, &local_host_);
    StringToNumber(port, &local_port_);
    self_node_.host = local_host_;
    self_node_.port = local_port_;
    timer_manager_.AddPeriodTimer(5000, Bind(&SWIMServerImpl::probe, this));
    Thread thread(Bind(&SWIMServerImpl::UDPServerListen, this, local_port_));
    thread.Start();
    thread.Join();

    return; 
}

SWIMServerImpl::~SWIMServerImpl() {
    timer_manager_.Stop();
}

void SWIMServerImpl::probe(uint64_t timer_id) {
    bool has_payload = false;
    Payload payload;
    get_payload(payload, &has_payload);
    char buffer[MAX_BUF_LEN];
    uint32_t message_length = 0;
    {
        MutexLocker lock(&endpoint_map_mutex_);
        seq_no_ += 1;
        Node target, indirect;
        get_random_target(target, 0, 0);
        construct_resp(PING, seq_no_, has_payload, payload, 0, 0,buffer, &message_length);

        struct sockaddr_in cliaddr;
        bzero(&cliaddr, sizeof(cliaddr));
        cliaddr.sin_family = AF_INET;
        cliaddr.sin_addr.s_addr = target.host;
        cliaddr.sin_port = htons(target.port);
        uint32_t send_len = sendto(cli_sock_fd_, buffer, message_length , 0, (struct sockaddr*)&cliaddr, sizeof(cliaddr) );
        // cannot connet the target node
        if(send_len == -1) {
            get_random_target(indirect, target.host, target.port); 
            construct_resp(PING_REQ, seq_no_, has_payload, payload, 0, 0,buffer, &message_length);
        bzero(&cliaddr, sizeof(cliaddr));
        bzero(buffer, sizeof(buffer));
        cliaddr.sin_family = AF_INET;
        cliaddr.sin_addr.s_addr = indirect.host;
        cliaddr.sin_port = htons(indirect.port);
        uint32_t send_len = sendto(cli_sock_fd_, buffer, message_length , 0, (struct sockaddr*)&cliaddr, sizeof(cliaddr) );
        if(send_len == -1) // suspicious
        {
            stdext::shared_ptr<NodeStatus> ptr = endpoint_map_mutex_[target]->second;
            ptr->set_status(SUSPICIOUS);
            enqueue_payload(SUSPICIOUS, target); 
            // and the timeout func for amending the node status
            suspicion_checker_.DelayTask(1000, boost::bind(&SWIMServerImpl::check_status, this, target));
        }

    }

}

void SWIMServerImpl::check_status(Node& node) {
    MutexLocker lock(&endpoint_map_mutex_);
    stdext::shared_ptr<NodeStatus> ptr = endpoint_map_mutex_[target]->second;
    if(ptr->status() == SUSPICIOUS) {
        ptr->set_status(DEAD);
        enqueue_payload(DEAD, node);
    }

}

void SWIMServerImpl::enqueue_payload(StatusType status_type, Node& node) {
    MutexLocker ll(&payload_queue_mutex_);
    payload payload;
    payload.status_type = status_type;
    payload.source_host = local_host_;
    payload.source_port = local_port_;
    payload.target_host = node.host;
    payload.target_port = node.port;
    payload_queue_.push(payload); 
}

void SWIMServerImpl::UDPServerListen(int port) {
    try
    {
        int sockfd;
        struct sockaddr_in serveraddr , cliaddr;
        sockfd = socket(AF_INET, SOCK_DGRAM, 0); 
        int recv_len;
        int ret; 
        int c_len = sizeof(struct sockaddr_in);
        char msg[MAXLINE];
        if(sockfd == -1) {
            LOG(ERROR)<<"create udp sock failed";
            exit(-1);
        }
        bzero(&serveraddr, sizeof(serveraddr));
        serveraddr.sin_family = AF_INET;
        serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
        serveraddr.sin_port = htons(port);
        if(bind(sockfd, (struct sockaddr*)&serveraddr, sizeof(serveraddr)) < 0) {
            LOG(ERROR)<<"bind udp sock server failed";
            exit(-1);
        }
        while(1) {
            bzero(msg, sizeof(msg));
            ClientReq req;
            ret = recv_data(sockfd,msg, req, cliaddr );            
            if(ret == E_RECV_ERROR) {
                continue; 
            }
            if(ret < 0) {
                LOG(ERROR)<<"receive message data illegal";
                continue;
            }
            print_client_req(req);
            handle_receive(req, cliaddr);
            
            if(sendto(sockfd, msg, 1024, 0, (struct sockaddr*)&cliaddr, c_len ) == -1) {
                LOG(ERROR)<<"send udp data failed";
                exit(-1);
            }
        }
        close(sockfd);
    }
    catch (std::exception& e)
    {
        std::cerr << e.what() << std::endl;
    }

}

void SWIMServerImpl::print_client_req(ClientReq& req) {
    LOG(INFO)<<"the data receive is "<<req.total_len<<" message type "<<req.message_type<<" sequence_no"<<req.sequence_no<<" has payload: "<<req.has_payload;
}

int SWIMServerImpl::recv_data(int sock_fd, char *buf, ClientReq& req, struct sockaddr_in& addr) {
    int recv_len;
    char *p;
    uint32_t tmp;
    socklen_t addr_len;
    addr_len = sizeof(addr);
    recv_len = recvfrom(sock_fd, buf, MAX_BUF_LEN, 0, (struct sockaddr*)&addr, &addr_len );
    if(recv_len <= 0 || recv_len >= MAX_BUF_LEN - 1) {
        return E_RECV_ERROR;
    }
    buf[recv_len] = '\0';
    p = buf;
    tmp = *(uint32_t*)p;
    req.total_len = ntohl(tmp);
    p += 4;
    if(req.total_len != recv_len ) {
        return E_MSG_ILLEGAL;
    }

    tmp = *(uint32_t *)p;
    req.message_type = (MessageType)(ntohl(tmp));
    p += 4;

    tmp = *(uint32_t*)p;
    req.sequence_no = ntohl(tmp); 
    p += 4;
    
    tmp = *(uint32_t *)p;
    req.suspect_host = ntohl(tmp); 
    p += 4;

    tmp = *(uint32_t *)p;
    req.suspect_port = ntohl(tmp);
    p += 4;

    tmp = *(uint32_t *)p;
    req.has_payload = ntohl(tmp);
    if(req.has_payload == 0) {
        LOG(INFO)<<"this message has not payload";
        return 0;     
    }
    p += 4;
    // status_type
    tmp = *(uint32_t *)p;
    req.status_type = (StatusType)(ntohl(tmp));
    p += 4;
    // incarnation
    tmp = *(uint32_t *)p;
    req.incarnation = ntohl(tmp);
    p += 4;
    // source_host
    tmp = *(uint32_t *)p;
    req.source_host = ntohl(tmp);
    p += 4;
    // source_port
    tmp = *(uint32_t *)p;
    req.source_port = ntohl(tmp);
    p += 4;
    // target_host
    tmp = *(uint32_t *)p;
    req.target_host = ntohl(tmp);
    p += 4;
    //target_port
    req.target_port = ntohl(tmp);
    return 0;
}

void SWIMServerImpl::construct_resp(MessageType message_type, uint32_t sequence_no, bool has_payload, Payload& payload, uint32_t suspect_host, uint32_t suspect_port, char buf[], uint32_t *resp_len) {

    char *p = buf;
    *resp_len = (has_payload ? 48: 24);
    *(uint32_t *)p = htonl(*resp_len);
    p += 4;
    *(uint32_t *)p = htonl(message_type);
    p += 4;
    *(uint32_t *)p = htonl(suspect_host);
    p += 4;
    *(uint32_t *)p = htonl(suspect_port);
    p += 4;
    *(uint32_t *)p = htonl(has_payload);
    if(has_payload) {
        p += 4;
        *(uint32_t *)p = htonl(payload.status_type);
        p += 4;
        *(uint32_t *)p = htonl(payload.source_host);
        p += 4;
        *(uint32_t *)p = htonl(payload.source_port);
        p += 4;
        *(uint32_t *)p = htonl(payload.target_host);
        p += 4;
        *(uint32_t *)p = htonl(payload.target_port);

    }
}

void SWIMServerImpl::handle_receive(ClientReq& req, struct sockaddr_in& addr) {
    bool has_payload = false;
    Payload payload;
    get_payload(payload, &has_payload);
    char buffer[MAX_BUF_LEN];
    uint32_t message_length = 0;

    switch(req.message_type) {
        case PING: 
            {
                uint32_t sequence_no = req.sequence_no;
                construct_resp(ACK, sequence_no, has_payload, payload, 0, 0,buffer, &message_length);
                sendto(cli_sock_fd_, buffer, message_length , 0, (struct sockaddr*)&addr, sizeof(addr) );
            }
            break;
        case PING_REQ:
            {
                struct sockaddr_in cliaddr;
                uint32_t suspect_host = req.suspect_host;
                uint32_t suspect_port = req.suspect_port;
                uint32_t sequence_no = req.sequence_no;
                // log the target host
                //struct in_addr ip_addr;
                //ip_addr.s_addr = suspect_host;
                //LOG(INFO)<<"now send ping to node"<<inet_ntoa(ip_addr)<<" : "<<suspect_port;
                // begin to send message
                bzero(&cliaddr, sizeof(cliaddr));
                cliaddr.sin_family = AF_INET;
                cliaddr.sin_addr.s_addr = suspect_host;
                cliaddr.sin_port = htons(suspect_port);
                construct_resp(PING, sequence_no, has_payload, payload, 0, 0,buffer, &message_length);
                uint32_t send_len = sendto(cli_sock_fd_, buffer, message_length, 0, (struct sockaddr*)&cliaddr, sizeof(cliaddr));
                if(send_len == -1) {
                    LOG(ERROR)<<"cannot connect the suspicious node with host "<<suspect_host<<" and port"<<suspect_port;
                } else {
                    char recv_buf[MAX_BUF_LEN];
                    ClientReq req;
                    recv_data(cli_sock_fd_, recv_buf, req, cliaddr );                  
                    // receive the suspicous node
                    if(req.message_type == ACK) {
                        construct_resp(ACK, sequence_no, has_payload, payload, 0, 0,buffer, &message_length);
                        sendto(cli_sock_fd_, buffer, message_length , 0, (struct sockaddr*)&addr, sizeof(addr) );
                    }
                }
            }
            break;
        default:
            break;
    }
    if(req.has_payload) {
        handle_payload(req, addr);
    }
    return; 
}

void SWIMServerImpl::get_payload(Payload& payload, bool *has_payload) {
    MutexLocker locker(payload_queue_mutex_);
    if(payload_queue_.empty()) {
        *has_payload = 0;
    } else {
        *has_payload = 1;
        payload = payload_queue_.top();
        payload_queue_.pop();
        payload.send_times = payload.send_times + 1;
        payload_queue_.push(payload);
    }
}

void SWIMServerImpl::handle_payload(ClientReq& req, struct sockaddr_in& addr) {
    Node tmp_node;
    tmp_node.host = req.target_host;
    tmp_node.port = req.target_port;
    MutexLocker lock(&endpoint_map_mutex_);
    stdext::shared_ptr<NodeStatus> ptr = endpoint_map_mutex_[tmp_node]->second; 
    ptr->set_status(req.status_type);
}

void SWIMServerImpl::get_random_target(Node& node, uint32_t suspect_host, uint32_t suspect_port) {
    MutexLocker lock(&endpoint_map_mutex_);
    for(map<Node, stdext::shared_ptr<NodeStatus> >::const_iterator iter = node_map_.begin(); iter != node_map_.end(); iter++) {
        if(*iter == self_node_)
            continue;
        if(iter->first->host == suspect_host && iter->first->port == suspect_port)
            continue;
        double r = ((double) rand() / (RAND_MAX));
        if(r > 0.5 ) {
            if(iter->second->status() == ALIVE || iter->second->status() == SUSPICIOUS) {
                node = iter->first;
                return;
            }
        }
    }
    return;
}

bool SWIMServerImpl::Init() {
    // initialize the socket client
    struct sockaddr_in cliaddr;
    int addr_len = sizeof(struct sockaddr_in);
    if((cli_sock_fd_ = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        LOG(ERROR)<<"client socket create failed!";
        return false;
    }
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 1000000;
    // set timeout option for client socket fd
    if (setsockopt(cli_sock_fd_ , SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(tv)) < 0) {
        LOG(ERROR)<<"client socket option timeout option set failed";
        return false;
    }
    
    return true;
} 
