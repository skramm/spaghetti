/**
\file traffic_lights_2.cpp
\brief a simple traffic light example, build using boost::asio
Similar to version 1, with an added keyboard user interface as a separate thread

See companion file: traffic_lights_common.hpp

This file is part of Spaghetti, a C++ library for implementing Finite State Machines

Homepage: https://github.com/skramm/spaghetti
*/

#include "traffic_lights_common.hpp"

#define SPAG_EMBED_ASIO_TIMER
//#define SPAG_PRINT_STATES
#define SPAG_ENABLE_LOGGING
#define SPAG_ENUM_STRINGS
#define SPAG_GENERATE_DOTFILE
#include "spaghetti.hpp"

// states and events are declared in file traffic_lights_common.hpp
SPAG_DECLARE_FSM_TYPE_ASIO( fsm_t, States, Events, std::string );

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
		configureFSM<fsm_t>( fsm );

		fsm.printConfig( std::cout );
		fsm.writeDotFile( "traffic_lights_2.dot" );

		std::thread thread_ui( UI_thread<fsm_t>, &fsm );

		fsm.start();  // blocking !
		thread_ui.join();
		fsm.printLoggedData( std::cout );
	}
	catch( std::exception& e )
	{
		std::cerr << "catch error: " << e.what() << std::endl;
	}
}
//-----------------------------------------------------------------------------------
