/**
\file
\brief client-side for test_traffic_lights_1.cpp .
sends udp frames to the server, using port 12345

This file is part of Spaghetti, a C++ library for implementing Finite State Machines

usage: first start `$ traffic_lights_3`
then, in another terminal:
`$ traffic_lights_client [host]`
(`localhost` usually)

Homepage: https://github.com/skramm/spaghetti
*/

#include <string>
#include <iostream>
#include <boost/asio.hpp>

using boost::asio::ip::udp;

//-----------------------------------------------------------------------------------
int main( int argc, char* argv[] )
{
	try
	{
		if( argc != 2 )
		{
			std::cerr << "Usage: client <host>" << std::endl;
			return 1;
		}

		boost::asio::io_context io_service;
		udp::resolver resolver(io_service);

		auto results = resolver.resolve( udp::v4(), argv[1], "12345" );
		udp::endpoint endpoint = *results.begin();

//		udp::resolver::query query(udp::v4(), argv[1], "12345" );
//		udp::endpoint endpoint = *resolver.resolve(query);

		std::cout << "endpoint: " << endpoint << "\n";
		udp::socket socket( io_service );
		socket.open( udp::v4() );

		std::cout << "Enter key: (a:warning on, b:warning off, c:reset): ";
		do
		{
			std::string str;
			std::cin >> str;
			socket.send_to(                 // blocking data send
				boost::asio::buffer( str ),
				endpoint
			);
		}
		while(1);
	}
	catch( const std::exception& e )
	{
		std::cerr << "catch error: " << e.what() << std::endl;
	}
}

