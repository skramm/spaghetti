/**
\file sample_2.cpp
\brief demo program of concurrent FSM, each with its own timeouts, using ASIO.
Needs symbol SPAG_EXTERNAL_EVENT_LOOP


This file is part of Spaghetti, a C++ library for implementing Finite State Machines

Homepage: https://github.com/skramm/spaghetti
*/
#define SPAG_EXTERNAL_EVENT_LOOP
#define SPAG_GENERATE_DOTFILE
//#define SPAG_PRINT_STATES
//#define SPAG_ENUM_STRINGS
#include "spaghetti.hpp"
#include "asio_wrapper.hpp"

enum States { st_1, st_2, NB_STATES };
enum Events { NB_EVENTS };

SPAG_DECLARE_FSM_TYPE( fsm_t, States, Events, AsioWrapper, std::string );

void cb( std::string s )
{
	std::cout << "cb: " << s << '\n';
}
//-----------------------------------------------------------------------------------
int main( int, char* argv[] )
{
	std::cout << argv[0] << ": " << fsm_t::buildOptions() << '\n';

	fsm_t fsm_A, fsm_B;

	fsm_A.assignTimeOut( st_1, 2, st_2 );
	fsm_A.assignTimeOut( st_2, 2, st_1 );
	fsm_A.assignGlobalCallback( cb );

	fsm_B.assignConfig( fsm_A );

	fsm_B.assignTimeOut( st_1, 3, st_2 );
	fsm_B.assignTimeOut( st_2, 3, st_1 );

	fsm_A.assignCallbackValue( st_1, "st1-A" );
	fsm_A.assignCallbackValue( st_2, "st2-A" );
	fsm_B.assignCallbackValue( st_1, "st1-B" );
	fsm_B.assignCallbackValue( st_2, "st2-B" );

	fsm_A.printConfig( std::cout );
	fsm_B.printConfig( std::cout );

	boost::asio::io_service io_service;
	AsioWrapper<States,Events,std::string> asio_A( io_service );
	AsioWrapper<States,Events,std::string> asio_B( io_service );
	fsm_A.assignTimer( &asio_A );
	fsm_B.assignTimer( &asio_B );

	try
	{
		fsm_A.start();  // non-blocking: external event loop !
		fsm_B.start();
		io_service.run();
	}
	catch( std::exception& e )
	{
		std::cerr << "catch: error: " << e.what() << std::endl;
	}
	catch( ... )
	{
		std::cerr << "catch: unknown error\n";
	}
}
//-----------------------------------------------------------------------------------


