/*==================================================================
*   Copyright (C) 2016 All rights reserved.
*   
*   filename:     udp_client.h
*   author:       Meng Weichao
*   created:      2016/05/13
*   description:  
*
================================================================*/
#ifndef __UDP_CLIENT_H__
#define __UDP_CLIENT_H__
#include <iostream>
#include <boost/array.hpp>
#include <boost/asio.hpp>
#include <boost/shared_ptr.hpp>
#include <deque>
#include "thirdparty/jsoncpp/json.h"

#include "common/system/net/ip_address.h"
#include "swim_client_msg.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <stdio.h>
#include <sys/time.h>

#define MEGA 1024*1024

#define MAX_BUF_LEN 4096
#define MAX_WORD_LEN 64
#define MAX_RESULT_COUNT 20


int64_t get_micros() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return static_cast<int64_t>(tv.tv_sec) * 1000 + tv.tv_usec / 1000;
}

using namespace std;

/*
using boost::asio::ip::udp;

class UDPClient
{
public:
    UDPClient(
        boost::asio::io_service& io_service, 
        const std::string& host, 
        const std::string& port
    ) : io_service_(io_service), socket_(io_service, udp::endpoint(udp::v4(), 0)) {
        udp::resolver resolver(io_service_);
        udp::resolver::query query(udp::v4(), host, port);
        udp::resolver::iterator iter = resolver.resolve(query);
        endpoint_ = *iter;
    }

    ~UDPClient()
    {
        socket_.close();
    }

    void send(const std::string& msg) {
        socket_.send_to(boost::asio::buffer(msg.c_str(), msg.size()), endpoint_);
        // used for test the show result
        boost::array<char, 1024> nim;
        size_t len = socket_.receive_from(boost::asio::buffer(nim), endpoint_);
        std::cout.write(nim.data(), len);
        std::cout<<std::endl;
    }

private:
    boost::asio::io_service& io_service_;
    udp::socket socket_;
    udp::endpoint endpoint_;
};

*/

struct Ping {
    uint32_t msg_len;
    uint32_t  seqno;
    char *buf;
};

struct IndirectPingReq {
    uint32_t msg_len;
    uint32_t seqno;
    uint32_t target_len;
    char *buf;
};

/*
 *
http://stackoverflow.com/questions/1680365/integer-to-ip-address-c
https://en.wikipedia.org/wiki/Lamport_timestamps


 */



int main( int argc, char* argv[])  {

    struct sockaddr_in cliaddr;
    int sockfd, len = 0;
    int addr_len = sizeof(struct sockaddr_in);
    if((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
        perror("socket create failed");
        exit(1);
    }

    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 1000000;
    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(tv)) < 0) {
        perror("Error");
    }

    bzero(&cliaddr, sizeof(cliaddr));
    cliaddr.sin_family = AF_INET;
    cliaddr.sin_addr.s_addr = inet_addr(argv[1]);
    cliaddr.sin_port = htons(atoi(argv[2]));

    string msg;
    char buffer[1024];
    char *p = buffer;
    int num;

    IPAddress address;
    IPAddress::GetFirstLocalAddress(&address);
    string host(address.ToString());
    cout<<"host is "<<host<<endl;
    struct sockaddr_in ip_add;
    inet_aton(host.c_str(), &ip_add.sin_addr);
    cout<<"number is"<<(uint32_t)(ip_add.sin_addr.s_addr)<<endl;

    uint32_t ip = (uint32_t)(ip_add.sin_addr.s_addr);
    struct in_addr ip_addr;
    ip_addr.s_addr = ip;
    printf("The IP address is %s\n", inet_ntoa(ip_addr));
    cout<<"ip value is "<<ip<<endl;

    while(cin>>msg) { 
        num = 1024;
        int64_t begin = get_micros();
        bzero(buffer,sizeof(buffer));
        char ff[1024];
        p = ff;
        *(uint32_t *)p = htonl(12);
        p += 4;
        *(uint32_t *)p = htonl(PING);
        p += 4;
        *(uint32_t *)p = htonl(ip);
        //p += 4;
        cout<<"the len of ff is"<<strlen(ff)<<endl;
        //int s_l = sendto(sockfd, msg.c_str() , msg.size(), 0,(struct sockaddr*)&cliaddr,  addr_len);
        int s_l = sendto(sockfd, ff, 12, 0,(struct sockaddr*)&cliaddr,  addr_len);
        cout<<"the size sent "<<s_l<<endl;
        int r_l = recvfrom(sockfd, buffer, 1024, 0, (struct sockaddr*)&cliaddr, (socklen_t *)&addr_len);
        char *q = buffer;
        int r_num = *(int *)q;
        q += 4;
        cout<<"test val is "<<ntohl(r_num)<<endl;
        cout<<"the size receive"<<r_l<<endl;
        cout<<q<<endl; 
        cout<<"cost time:"<< get_micros() - begin<<endl;
    }

    /*
    boost::asio::io_service io_service;
    //UDPClient client(io_service, "localhost", "9999");
    UDPClient client(io_service, argv[1], argv[2]);
    std::cout<<argv[1]<<argv[2]<<std::endl;

    std::string msg;
    while(std::cin>>msg) {
        client.send(msg);
    }
    */
    return 0;
}
#endif //__UDP_CLIENT_H__
