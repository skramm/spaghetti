/**
\file testA_3.cpp
\brief test3
*/

//#define SPAG_TRACK_RUNTIME
#define SPAG_EMBED_ASIO_WRAPPER
#define SPAG_ENUM_STRINGS
#define SPAG_ENABLE_LOGGING
//#define SPAG_PRINT_STATES
#define SPAG_USE_SIGNALS
#include "spaghetti.hpp"

enum States { st0, st1, NB_STATES };
enum Events { ev0, NB_EVENTS };

SPAG_DECLARE_FSM_TYPE_ASIO( fsm_t, States, Events, int );

fsm_t fsm;
int g_count = 0;
int g_ie_active = 3;

void cb( int s )
{
	std::cout << "callback: current state=" << s << " count=" << g_count << std::endl;
	if( g_count == g_ie_active )
	{
		std::cout << "activating inner event ev0\n";
		fsm.activateInnerEvent( ev0 );
	}
	if( g_count == 6 )
		fsm.stop();
	g_count++;
}

int main( int argc, char* argv[] )
{
	if( argc > 1 )
	{
		g_ie_active = std::stoi( argv[1] );
	}
	fsm.assignCallbackAutoval( cb );
	fsm.assignAAT( st1, st0 );

	fsm.assignTimeOut( st0, 100, "ms", st1 );
	fsm.assignInnerTransition( st0, ev0, st1 );

	fsm.writeDotFile( "test_3" );
	try
	{
		fsm.start();
	}
	catch( const std::exception& e )
	{
		std::cout << "Error: " << e.what() << '\n';
	}
	fsm.printCounters( std::cout );
}
