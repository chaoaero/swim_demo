/*==================================================================
*   Copyright (C) 2016 All rights reserved.
*   
*   filename:     swim_server_impl.h
*   author:       Meng Weichao
*   created:      2016/05/12
*   description:  
*
================================================================*/
#ifndef __SWIM_SERVER_IMPL_H__
#define __SWIM_SERVER_IMPL_H__
#include <stdio.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <string>
#include <map>
#include <set>
#include <vector>
#include <queue>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "poppy/rpc_server.h"
#include "common/system/time/timestamp.h"
#include "common/system/timer/timer_manager.h"
#include "common/system/concurrency/mutex.h"
#include "common/base/string/algorithm.h"
#include "gflags/gflags.h"
#include "glog/logging.h"
#include "common/base/stdext/shared_ptr.h"

#include "swim_cpp/swim.pb.h"
#include "swim_cpp/common/thread_pool.h"
#include <boost/atomic.hpp>
#include "swim_client_msg.h"

using namespace common;
using namespace std;

class SWIMServerImpl : public SWIMServer {

public:
    SWIMServerImpl(string& server_address, string& port);
    ~SWIMServerImpl();
    virtual void Join(google::protobuf::RpcController* controller, const JoinReq* request, JoinResp* response, google::protobuf::Closure* done) {
        done->Run();
    }
    virtual void Members(google::protobuf::RpcController* controller, const poppy::EmptyRequest* request, MembersResp* response, google::protobuf::Closure* done) {
        done->Run();
    }
    bool Init();
    void UDPServerListen(int port);
    int recv_data(int sock_fd, char *buf, ClientReq& req, struct sockaddr_in& addr);
    void handle_receive(ClientReq& req, struct sockaddr_in& addr);
    void handle_payload(ClientReq& req, struct sockaddr_in& addr);
    void print_client_req(ClientReq& req);
    void get_payload(Payload& payload, bool *has_payload); 
    void construct_resp(MessageType message_type, uint32_t sequence_no, bool has_payload, Payload& payload, uint32_t suspect_host, uint32_t suspect_port, char buf[], uint32_t *resp_len);
    void get_random_target(Node& node, uint32_t suspect_host, uint32_t suspect_port);
    void probe(uint64_t timer_id);
    void check_status(Node& node);
    void SWIMServerImpl::enqueue_payload(StatusType status_type, Node& node); 

public:
    struct Payload {
        uint32_t send_times;
        uint32_t status_type;
        uint32_t source_host;
        uint32_t source_port;
        uint32_t target_host;
        uint32_t target_port;
        Payload() : send_times(0) {};
        bool operator<(const Payload& item) const {
            return send_times < item.send_times;
        }
    };
    typedef struct Payload Payload;
    typedef std::priority_queue<Payload> PayloadQueue;
    struct Node {
        uint32_t host;
        uint32_t port;
    };
    typedef struct Node Node;

private:
    TimerManager timer_manager_;

    uint32_t local_host_;// local IP address
    uint32_t local_port_;
    Node self_node_;
    my_common::ThreadPool suspicion_checker_;
    
    PayloadQueue payload_queue_;
    Mutex endpoint_map_mutex_;
    Mutex payload_queue_mutex_;
    std::map<Node, stdext::shared_ptr<NodeStatus> > node_map_;
    int cli_sock_fd_;

    boost::atomic<int> seq_no_;
    boost::atomic<int> incarnation_;

}; 


#endif //__SWIM_SERVER_IMPL_H__
