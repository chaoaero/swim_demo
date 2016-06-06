/*==================================================================
*   Copyright (C) 2016 All rights reserved.
*   
*   filename:     swim_server.cc
*   author:       Meng Weichao
*   created:      2016/05/12
*   description:  
*
================================================================*/
#include <stdio.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <string>
#include <map>
#include <set>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "swim_cpp/swim.pb.h"
#include "poppy/rpc_server.h"
#include "common/system/net/ip_address.h"
#include "common/system/time/timestamp.h"
#include "common/system/timer/timer_manager.h"

#include "common/base/string/string_number.h"
#include "common/base/string/algorithm.h"

#include "swim_server_impl.h"

// includes from thirdparty
#include "gflags/gflags.h"
#include "glog/logging.h"

using namespace common;
using namespace std;

DEFINE_string(server_address, "0.0.0.0:9999", "the address server listen on.");
DEFINE_int32(tcp_timeout, 10, "the default timeout for tcp connection");
DEFINE_int32(num_indirect_checks, 3, "number of indirect checks for failure detection");
DEFINE_int32(probe_interval, 1, "interval seconds between two probe action");
DEFINE_int32(probe_timeout, 500, "milliseconds for judging a failed node");

int main(int argc, char** argv) {

    google::ParseCommandLineFlags(&argc, &argv, true);
    //google::InitGoogleLogging(argv[0]);

    // Define an rpc server.
    poppy::RpcServerOptions options;
    options.set_tos(148);
    poppy::RpcServer rpc_server(0, NULL, options);
    // get localhost address
    char hname[128];
    gethostname(hname, sizeof(hname));
    IPAddress address;
    IPAddress::GetFirstLocalAddress(&address);
    string host_name;

    vector<string> localhost_port;
    SplitString(FLAGS_server_address, ":", &localhost_port);
    //string localhost_addr(address.ToString() + ":" + localhost_port[1]);
    string local_host(address.ToString());

    SWIMServerImpl* service = new SWIMServerImpl(local_host, localhost_port[1]);

    if(!service->Init()) {
        LOG(WARNING) << "Failed to init server.";
        return EXIT_FAILURE;
    }

    rpc_server.RegisterService(service);

    if(!rpc_server.Start(FLAGS_server_address)) {
        LOG(WARNING) << "Failed to start server.";
        return EXIT_FAILURE;
    }

    return rpc_server.Run();

}
