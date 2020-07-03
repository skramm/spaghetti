/**
\file udp_server.hpp
\brief holds definition of class UdpServer, based on boost::asio

This file is part of Spaghetti, a C++ library for implementing Finite State Machines

Homepage: https://github.com/skramm/spaghetti
*/

#ifndef HG_UDP_SERVER_HPP
#define HG_UDP_SERVER_HPP

#include <array>
#include <boost/bind.hpp>
#include <boost/asio.hpp>

typedef unsigned char BYTE;
//-----------------------------------------------------------------------------------
/// A udp server based on boost::asio, templated by size of buffer.
/// Does not hold the io_service, it must be provided separately
/**
Callback: upon each reception of data, a callback function is called

The user of this class needs to provide a function getResponse()
that will return the data that needs to be send back.

The boost::io_service is assumed to be started externally
*/
template<size_t BUF_SIZE>
class UdpServer
{
	public:
		using Buffer_t = std::array<BYTE, BUF_SIZE>;

		UdpServer( boost::asio::io_service& io_service, int port_no ) //, bool sendack=false )
			: _socket( io_service, boost::asio::ip::udp::endpoint( boost::asio::ip::udp::v4(), port_no ) ) //, _sendack(sendack)
		{}

		void start_receive()
		{
			_socket.async_receive_from(
				boost::asio::buffer( _recv_buffer),
				_remote_endpoint,
				boost::bind(
					&UdpServer::_rx_handler,
					this,
					boost::asio::placeholders::error,
					boost::asio::placeholders::bytes_transferred
				)
			);
		}
	private:
		boost::asio::ip::udp::socket   _socket;
		boost::asio::ip::udp::endpoint _remote_endpoint;

//	protected:
		Buffer_t _recv_buffer;

/// This virtual function NEEDS to be implemented in inherited class
		virtual std::vector<BYTE> getResponse( const Buffer_t& buffer, std::size_t nb_bytes ) const = 0;

/// Reception callback function. Inherited classes can access the received data and need to provide a GetResponse() function
		void _rx_handler( const boost::system::error_code&, std::size_t nb_bytes )
		{
			std::vector<BYTE> v = getResponse( _recv_buffer, nb_bytes );
//			std::cout << "sending ack:" << std::string(v.begin(),v.end()) << "\n";
			_socket.send_to(                // synchronous send acknowledge
				boost::asio::buffer( v ),
				_remote_endpoint
			);
			start_receive();
		}
};
//-----------------------------------------------------------------------------------

#endif // HG_UDP_SERVER_HPP
