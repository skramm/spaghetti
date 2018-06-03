/**
\file test_3.cpp
\brief test3
*/

//#define SPAG_TRACK_RUNTIME
#define SPAG_EMBED_ASIO_WRAPPER
#define SPAG_ENUM_STRINGS
#define SPAG_ENABLE_LOGGING
//#define SPAG_PRINT_STATES
#define SPAG_USE_SIGNALS
#define SPAG_GENERATE_DOTFILE
#include "spaghetti.hpp"

enum States { st0, st1, NB_STATES };
enum Events { ev0, NB_EVENTS };

SPAG_DECLARE_FSM_TYPE_ASIO( fsm_t, States, Events, int );

fsm_t fsm;
int g_count = 0;

void cb( int s )
{
	std::cout << "callback: current state=" << s << " count=" << g_count << std::endl;
	if( g_count == 3 )
	{
		std::cout << "activating inner event ev0\n";
		fsm.activateInnerEvent( ev0 );
	}
	if( g_count == 11 )
		fsm.stop();
	g_count++;
}

int main( int, char* argv[] )
{
	fsm.assignCallbackAutoval( cb );
	fsm.assignTransition( st1, st0 );

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
//	fsm.printLoggedData( std::cout );
}
