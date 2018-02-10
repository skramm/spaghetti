/**
\file test_traffic_lights_3.cpp
\brief a simple traffic light example, build using boost::asio

Similar to version 2, with an added udp server part, can receive data from
traffic_lights_client.cpp

This file is part of Spaghetti, a C++ library for implementing Finite State Machines

Homepage: https://github.com/skramm/spaghetti
*/

#include "udp_server.hpp"

//#define SPAG_PRINT_STATES
#define SPAG_ENABLE_LOGGING
#define SPAG_ENUM_STRINGS

#include "spaghetti.hpp"

#include "asio_wrapper.hpp"
#include "traffic_lights_common.hpp"

// states and events are declared in file traffic_lights_common.hpp
SPAG_DECLARE_FSM_TYPE( fsm_t, EN_States, EN_Events, AsioWrapper, std::string );

/// global pointer on mutex, will get initialized in getSingletonMutex()
std::mutex* g_mutex;

//-----------------------------------------------------------------------------------
/// concrete class, implements UdpServer and SpagFSM, and triggers event on the FSM
//template<typename ST, typename EV, typename CBA>
struct MyServer : public UdpServer<2048>
{
	MyServer( boost::asio::io_service& io_service, int port_no )
		: UdpServer( io_service, port_no )
	{}

	std::vector<BYTE> GetResponse( const Buffer_t& buffer, std::size_t nb_bytes ) const
	{
		std::cout << "received " << nb_bytes << " bytes\n";
		switch( buffer.at(0) )
		{
			case 'a':
				fsm.processEvent( ev_WarningOn );
			break;
			case 'b':
				fsm.processEvent( ev_WarningOff );
			break;
			case 'c':
				fsm.processEvent( ev_Reset );
			break;
			default:
				std::cout << "Error: invalid message received !\n";
				throw;
		}
		return std::vector<BYTE>(); // return empty vector at present...
	}

	fsm_t fsm;
};
//-----------------------------------------------------------------------------------
int main( int, char* argv[] )
{
	std::cout << argv[0] << ": " << fsm_t::buildOptions() << '\n';

	g_mutex = getSingletonMutex();
	try
	{
		AsioWrapper<EN_States,EN_Events,std::string> asio;
		std::cout << "io_service created\n";

		MyServer server( asio.io_service, 12345 ); // create udp server with asio

		std::cout << "-server created\n";

		server.fsm.assignTimer( &asio );
		configureFSM<fsm_t>( server.fsm );

		server.fsm.printConfig( std::cout );

		std::cout << "- server start\n";
		server.start_receive();

		std::cout << "- start UI thread\n";
		std::thread thread_ui( UI_thread<fsm_t>, &server.fsm );

		std::cout << "- start fsm\n";
		server.fsm.start();  // blocking !
		thread_ui.join();
		server.fsm.printLoggedData( std::cout );
	}
	catch( std::exception& e )
	{
		std::cerr << "catch error: " << e.what() << std::endl;
	}
}
//-----------------------------------------------------------------------------------
