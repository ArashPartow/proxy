//
// tcpproxy_server.cpp
// ~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2007 Arash Partow (http://www.partow.net)
// URL: http://www.partow.net/programming/tcpproxy/index.html
//
// Distributed under the Boost Software License, Version 1.0.
//

#include <cstdlib>
#include <cstddef>
#include <iostream>
#include <string>

#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/asio.hpp>

namespace tcp_proxy
{
   namespace ip = boost::asio::ip;

   class tcp_bridge : public boost::enable_shared_from_this<tcp_bridge>
   {
   private:

      typedef boost::function <void (const boost::system::error_code&, const size_t& )> handler_type;
      typedef ip::tcp::socket socket_type;

   public:

      typedef tcp_bridge type;
      typedef boost::shared_ptr<type> ptr_type;

      tcp_bridge(boost::asio::io_service& ios)
      : client_socket_(ios),
        remote_socket_(ios)
      {}

      socket_type& socket()
      {
         return client_socket_;
      }

      void start(const std::string& remote_host, unsigned short remote_port)
      {
         remote_socket_.async_connect(
               ip::tcp::endpoint(
                     boost::asio::ip::address::from_string(remote_host),
                     remote_port),
               boost::bind(&type::handle_remote_connect,
                     shared_from_this(),
                     boost::asio::placeholders::error));
      }

      void handle_remote_connect(const boost::system::error_code& error)
      {
         if (!error)
         {
            remote_write_ = boost::bind(&type::handle_remote_write,
                  shared_from_this(),
                  boost::asio::placeholders::error);

            client_write_ = boost::bind(&type::handle_client_write,
                  shared_from_this(),
                  boost::asio::placeholders::error);

            remote_read_ = boost::bind(&type::handle_remote_read,
                  shared_from_this(),
                  boost::asio::placeholders::error,
                  boost::asio::placeholders::bytes_transferred);

            client_read_ = boost::bind(&type::handle_client_read,
                  shared_from_this(),
                  boost::asio::placeholders::error,
                  boost::asio::placeholders::bytes_transferred);

            remote_socket_.async_read_some(
                  boost::asio::buffer(remote_data_, max_data_length),
                  remote_read_);

            client_socket_.async_read_some(
                  boost::asio::buffer(client_data_, max_data_length),
                  client_read_);
         }
         else
            close();
      }

   private:

      void handle_client_write(const boost::system::error_code& error)
      {
         if (!error)
         {
            remote_socket_.async_read_some(
                  boost::asio::buffer(remote_data_, max_data_length),
                  remote_read_);
         }
         else
            close();
      }

      void handle_client_read(const boost::system::error_code& error,
                              const size_t& bytes_transferred)
      {
         if (!error)
         {
            async_write(remote_socket_,
                  boost::asio::buffer(client_data_,bytes_transferred),
                  remote_write_);
         }
         else
            close();
      }

      void handle_remote_write(const boost::system::error_code& error)
      {
         if (!error)
         {
            client_socket_.async_read_some(
                  boost::asio::buffer(client_data_, max_data_length),
                  client_read_);
         }
         else
            close();
      }

      void handle_remote_read(const boost::system::error_code& error,
                              const size_t& bytes_transferred)
      {
         if (!error)
         {
            async_write(client_socket_,
                  boost::asio::buffer(remote_data_, bytes_transferred),
                  client_write_);
         }
         else
            close();
      }

      void close()
      {
         if (client_socket_.is_open()) client_socket_.close();
         if (remote_socket_.is_open()) remote_socket_.close();
      }

      socket_type client_socket_;
      socket_type remote_socket_;

      enum { max_data_length = 4096 };
      char client_data_[max_data_length];
      char remote_data_[max_data_length];

      handler_type client_write_;
      handler_type remote_write_;
      handler_type client_read_;
      handler_type remote_read_;

   public:

      static inline ptr_type create(boost::asio::io_service& ios)
      {
         return ptr_type(new type(ios));
      }

      class acceptor
      {
      private:
         typedef boost::function<void (const boost::system::error_code&)> handler_type;

      public:

         acceptor(boost::asio::io_service& ios,
                  unsigned short local_port,
                  unsigned short remote_port,
                  const std::string& local_host,
                  const std::string& remote_host)
         : io_service_(ios),
           localhost_address(boost::asio::ip::address_v4::from_string(local_host)),
           acceptor_(ios,ip::tcp::endpoint(localhost_address,local_port)),
           remote_port_(remote_port),
           remote_host_(remote_host)
         {}

         bool accept_connections()
         {
            try
            {
               session_ = type::create(io_service_);
               acceptor_.async_accept(session_->socket(),
                     boost::bind(&acceptor::handle_accept,this,
                           boost::asio::placeholders::error));
            }
            catch(std::exception& e)
            {
               std::cerr << "acceptor exception: " << e.what() << std::endl;
               return false;
            }
            return true;
         }

      private:

         void handle_accept(const boost::system::error_code& error)
         {
            if (!error)
            {
               session_->start(remote_host_,remote_port_);
               if (!accept_connections())
               {
                  std::cerr << "Failed to accept." << std::endl;
               }
            }
            else
            {
               std::cerr << error.message() << std::endl;
            }
         }

         boost::asio::io_service& io_service_;
         ip::address_v4 localhost_address;
         ip::tcp::acceptor acceptor_;
         ptr_type session_;
         handler_type handle_;
         unsigned short remote_port_;
         std::string remote_host_;
      };

   };
}

int main(int argc, char* argv[])
{
   if (argc != 5)
   {
      std::cerr << "usage: tcpproxy_server <local host ip> <local port> <remote host ip> <remote port>" << std::endl;
      return 1;
   }

   const unsigned short local_port  = static_cast<unsigned short>(::atoi(argv[2]));
   const unsigned short remote_port = static_cast<unsigned short>(::atoi(argv[4]));
   const std::string local_host     = argv[1];
   const std::string remote_host    = argv[3];

   boost::asio::io_service ios;

   try
   {
      tcp_proxy::tcp_bridge::acceptor acceptor(ios,
                                               local_port,remote_port,
                                               local_host,remote_host);
      acceptor.accept_connections();
      ios.run();
   }
   catch(std::exception& e)
   {
      std::cerr << e.what() << std::endl;
      return 1;
   }

   return 0;
}


/*
 * Note: build command:
 * c++ -pedantic -ansi -Wall -Werror -O3 -o tcpproxy_server tcpproxy_server.cpp -L/usr/lib -lstdc++ -lpthread -lboost_thread -lboost_system
 */
