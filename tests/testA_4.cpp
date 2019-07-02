/**
\file testA_4.cpp
\brief test of the warning when assigning a global time out when one of the states has an AAT (=is a pass state)
*/

//#define SPAG_TRACK_RUNTIME
#define SPAG_EMBED_ASIO_WRAPPER
#define SPAG_ENUM_STRINGS
#define SPAG_ENABLE_LOGGING
//#define SPAG_PRINT_STATES
#define SPAG_USE_SIGNALS
#include "spaghetti.hpp"

enum States { st0, st1, st2, NB_STATES };
enum Events { ev0, NB_EVENTS };

SPAG_DECLARE_FSM_TYPE_ASIO( fsm_t, States, Events, int );


//int g_count = 0;
//int g_ie_active = 3;

void cb( int s )
{
	std::cout << "callback: current state=" << s << '\n';
}

int main( int argc, char* argv[] )
{
	fsm_t fsm;

	fsm.assignCallbackAutoval( cb );
	fsm.assignAAT( st1, st0 );

	fsm.assignGlobalTimeOut( 100, "ms", st2 );

/*	try
	{
		fsm.start();
	}
	catch( const std::exception& e )
	{
		std::cout << "Error: " << e.what() << '\n';
	}
*/
//	fsm.printLoggedData( std::cout );
}
