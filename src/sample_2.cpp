/**
\file sample_2.cpp
\brief demo program of concurrent FSM, each with its own timeouts, using boost::asio.
Uses the same enum for events and states.
Needs symbol SPAG_EXTERNAL_EVENT_LOOP


This file is part of Spaghetti, a C++ library for implementing Finite State Machines

Homepage: https://github.com/skramm/spaghetti
*/

#define SPAG_USE_ASIO_WRAPPER
#define SPAG_EXTERNAL_EVENT_LOOP

#define SPAG_ENABLE_LOGGING
#define SPAG_PRINT_STATES

#define SPAG_USE_SIGNALS
#define SPAG_ENUM_STRINGS
//#define SPAG_FRIENDLY_CHECKING
#include "spaghetti.hpp"

enum States { stA_1, stA_2, stB_1, stB_2, NB_STATES };
enum Events { NB_EVENTS };

SPAG_DECLARE_FSM_TYPE_ASIO( fsm_t, States, Events, std::string );

void cbA( std::string s )
{
	std::cout << "cbA: " << s << '\n';
}
void cbB( std::string s )
{
	std::cout << "cbB: " << s << '\n';
}
//-----------------------------------------------------------------------------------
int main( int, char* argv[] )
{
	std::cout << argv[0] << ": " << fsm_t::buildOptions() << '\n';

	fsm_t fsm_A, fsm_B;

	fsm_A.assignString2State( stA_1, "stA1" );
	fsm_A.assignString2State( stA_2, "stA2" );
	fsm_B.assignString2State( stB_1, "stB1" );
	fsm_B.assignString2State( stB_2, "stB2" );

	fsm_A.assignTimeOut( stA_1, 2, stA_2 );
	fsm_A.assignTimeOut( stA_2, 2, stA_1 );

	fsm_B.assignAAT( stA_1, stB_1 );
//	fsm_B.assignTimeOut( stA_1, 800, "ms", stB_2 );
	fsm_B.assignTimeOut( stB_1, 800, "ms", stB_2 );
	fsm_B.assignTimeOut( stB_2, 500, "ms", stB_1 );

	fsm_A.assignCallbackAutoval( cbA );
	fsm_B.assignCallbackAutoval( cbB );

	fsm_A.printConfig( std::cout, "FSM-A" );
	fsm_B.printConfig( std::cout, "FSM-B" );

	boost::asio::io_context io_service;
	spag::AsioEL asio_A( io_service );
	spag::AsioEL asio_B( io_service );
	fsm_A.assignEventHandler( &asio_A );
	fsm_B.assignEventHandler( &asio_B );

	fsm_A.setLogFileName( "spaghetti_A.csv" );
	fsm_B.setLogFileName( "spaghetti_B.csv" );
	try
	{
		fsm_A.start();
		std::cout << "fsm A started\n";
		fsm_B.start();  // non-blocking: external event loop !
		std::cout << "fsm B started\n";
		io_service.run();
		std::cout << "The end!\n";
	}
	catch( std::exception& e )
	{
		std::cerr << "catch: error: " << e.what() << std::endl;
	}
}
//-----------------------------------------------------------------------------------


