/**
\file test_traffic_lights_client.cpp
\brief client-side for test_traffic_lights_1.cpp .
sends udp frames to the server, using port 12345

This file is part of Spaghetti, a C++ library for implementing Finite State Machines

Homepage: https://github.com/skramm/spaghetti
*/

#include <string>
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

		boost::asio::io_service io_service;
		udp::resolver resolver(io_service);
		udp::resolver::query query(udp::v4(), argv[1], "12345" );
		udp::endpoint endpoint = *resolver.resolve(query);

		std::cout << "endpoint: " << endpoint << "\n";
		udp::socket socket( io_service );
		socket.open( udp::v4() );

//		int iter(0);
		do
		{
			io_service.reset();
			std::string str;
			std::cout << "Enter key: (a:warning on, b:warning off, c:reset): ";
			std::cin >> str;
//			std::string str_frame( "frame " + std::to_string(iter++) + ": message=" + str );

			socket.send_to(
				boost::asio::buffer( str ),
				endpoint
			);
//			std::cout << "-data is sent\n";

//			io_service.run();
		}
		while(1);
	}
	catch (std::exception& e)
	{
		std::cerr << "catch error: " << e.what() << std::endl;
	}
}

