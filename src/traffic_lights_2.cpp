/**
\file test_traffic_lights_2.cpp
\brief a simple traffic light example, build using boost::asio

Similar to version 1, with an added keyboard user interface
*/

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
int main( int, char* argv[] )
{
	std::cout << argv[0] << ": " << fsm_t::buildOptions() << '\n';
	std::cout << GetBoostVersion();

	g_mutex = getSingletonMutex();
	try
	{
		fsm_t fsm;
		AsioWrapper<EN_States,EN_Events,std::string> asio;

		fsm.assignTimer( &asio );
		configureFSM<fsm_t>( fsm );

		fsm.printConfig( std::cout );

		std::cout << " -start UI thread\n";
		std::thread thread_ui( UI_thread<fsm_t>, &fsm );

		fsm.start();  // blocking !
		thread_ui.join();
		fsm.printLoggedData( std::cout );
	}
	catch( std::exception& e )
	{
		std::cerr << "catch error: " << e.what() << std::endl;
	}
	catch( ... )
	{
		std::cerr << "catch unknown error\n";
	}

}
//-----------------------------------------------------------------------------------
