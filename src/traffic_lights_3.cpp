/**
\file test_traffic_lights_3.cpp
\brief a simple traffic light example, build using boost::asio

Similar to version 2, with an added udp server part, can receive data from
traffic_lights_client.cpp
*/

#include "udp_server.hpp"

#define SPAG_PRINT_STATES
#define SPAG_ENABLE_LOGGING
#define SPAG_ENUM_STRINGS

#include "spaghetti.hpp"

#include "asio_wrapper.hpp"
#include "traffic_lights_common.hpp"

SPAG_DECLARE_FSM_TYPE( fsm_t, STATE, EVENT, AsioWrapper, std::string );

/// global pointer on mutex, will get initialized in getSingletonMutex()
std::mutex* g_mutex;

//-----------------------------------------------------------------------------------
/// concrete class, implements udp_server and SpagFSM, and triggers event on the FSM
template<typename ST, typename EV, typename CBA>
struct my_server : public udp_server<2048>
{
	my_server( AsioWrapper<STATE,EVENT,CBA>& asio_wrapper, int port_no )
		: udp_server( asio_wrapper.io_service, port_no )
	{}

	std::vector<BYTE> GetResponse( const Buffer_t& buffer, std::size_t nb_bytes ) const
	{
		std::cout << "received " << nb_bytes << " bytes\n";
		switch( buffer.at(0) )
		{
			case 'a':
				fsm.processEvent( EV_WARNING_ON );
			break;
			case 'b':
				fsm.processEvent( EV_WARNING_OFF );
			break;
			case 'c':
				fsm.processEvent( EV_RESET );
			break;
			default:
				std::cout << "Error: invalid message received !\n";
				throw;
		}
		return std::vector<BYTE>(); // return empty vector at present...
	}

	spag::SpagFSM<ST,EV,AsioWrapper<ST,EV,CBA>,CBA> fsm;
};
//-----------------------------------------------------------------------------------
int main( int, char* argv[] )
{
	std::cout << argv[0] << ": " << fsm_t::buildOptions() << '\n';

	g_mutex = getSingletonMutex();
	try
	{
		AsioWrapper<STATE,EVENT,std::string> asio;
		std::cout << "io_service created\n";

		my_server<STATE,EVENT,std::string> server( asio, 12345 );

		std::cout << "server created\n";

		server.fsm.assignTimer( &asio );
		configureFSM<fsm_t>( server.fsm );

		server.fsm.printConfig( std::cout );

		std::cout << __FUNCTION__ << "(): server.start_receive()\n";
		server.start_receive();

		std::cout << __FUNCTION__ << "(): start UI thread\n";
		std::thread thread_ui( UI_thread<fsm_t>, &server.fsm );

		std::cout << __FUNCTION__ << "(): start fsm\n";
		server.fsm.start();  // blocking !
		std::cout << __FUNCTION__ << "(): fsm is stopped:\n";
		thread_ui.join();
	}
	catch( std::exception& e )
	{
		std::cerr << "catch error: " << e.what() << std::endl;
	}
}
//-----------------------------------------------------------------------------------
