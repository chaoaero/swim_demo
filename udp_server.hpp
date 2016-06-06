/*==================================================================
 *   Copyright (C) 2016 All rights reserved.
 *   
 *   filename:     boost_test.cc
 *   author:       Meng Weichao
 *   created:      2016/05/16
 *   description:  
 *
 ================================================================*/
#include <ctime>
#include <iostream>
#include <string>
#include <boost/array.hpp>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/asio.hpp>

using boost::asio::ip::udp;
//using boost::asio::ip::tcp;

#define MAX_LENGTH 1024

class udp_server
{
    public:
        udp_server(boost::asio::io_service& io_service, short port):io_service_(io_service), socket_(io_service, udp::endpoint(udp::v4(), port))
        {
            start_receive();
        }

        ~udp_server() {
            socket_.close();
        }

    private:
        void start_receive()
        {
            socket_.async_receive_from(
                    boost::asio::buffer(data_, MAX_LENGTH), remote_endpoint_,
                    boost::bind(&udp_server::handle_receive, this,
                        boost::asio::placeholders::error,
                        boost::asio::placeholders::bytes_transferred));
        }

        void handle_receive(const boost::system::error_code& error,
                std::size_t bytes_recvd)
        {
            if (!error && bytes_recvd > 0) {
                socket_.async_send_to(boost::asio::buffer(data_, bytes_recvd), remote_endpoint_, boost::bind(&udp_server::handle_send, this,  
                            boost::asio::placeholders::error,
                            boost::asio::placeholders::bytes_transferred));
            } 
            start_receive();
        }

        void handle_send( const boost::system::error_code& error,
                std::size_t bytes_transferred)
        {
            start_receive();
        }

        boost::asio::io_service& io_service_;
        udp::socket socket_;
        udp::endpoint remote_endpoint_;
        char data_[MAX_LENGTH];
};

/*
int main()
{
    try
    {
        boost::asio::io_service io_service;
        udp_server server(io_service,  9999);
        io_service.run();
    }
    catch (std::exception& e)
    {
        std::cerr << e.what() << std::endl;
    }

    return 0;
} */
