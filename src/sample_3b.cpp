/**
\file sample_3b.cpp
\brief Demo program of a simple FSM: 5 states with 1, 2, 3... ms between each, and with FSM as a global variable.
Also demonstrates how user code can stop the running FSM, using signals.

\image html sample_3.svg

This file is part of Spaghetti, a C++ library for implementing Finite State Machines

Homepage: https://github.com/skramm/spaghetti
*/

#define SPAG_EMBED_ASIO_WRAPPER
#define SPAG_ENABLE_LOGGING
#define SPAG_USE_SIGNALS
#define SPAG_FRIENDLY_CHECKING
#include "spaghetti.hpp"

enum States { st0, st1, st2, st3, st4, st_Final, NB_STATES };
enum Events { ev_1000, NB_EVENTS };

SPAG_DECLARE_FSM_TYPE_ASIO( fsm_t, States, Events, int );

fsm_t fsm;
spag::DotFileOptions options;

void cb_func( int v )
{
	if( v == -1 )
	{
		std::cout << "Final state!\n";
		fsm.stop();
	}
	static int c;
	if( c < 5 )
		fsm.writeDotFile( "sample_3b_" + std::to_string(c), options );
	c++;
	if( !(c%100) )
		std::cout << "c=" << c << '\n';
	if( c>1000 )
		fsm.activateInnerEvent( ev_1000 );
}

int main( int, char* argv[] )
{
	std::cout << argv[0] << ": " << fsm_t::buildOptions() << '\n';
	options.showActiveState = true;
	fsm.setTimerDefaultUnit( "ms" );

	fsm.assignCallback( cb_func );
	fsm.assignTimeOut( st0, 1 , st1 );
	fsm.assignTimeOut( st1, 2 , st2 );
	fsm.assignTimeOut( st2, 3 , st3 );
	fsm.assignTimeOut( st3, 4 , st4 );
	fsm.assignTimeOut( st4, 5 , st0 );

	fsm.assignInnerTransition( ev_1000, st_Final );
	fsm.assignCallbackValue( st_Final, -1 );
	fsm.printConfig( std::cout );
	fsm.writeDotFile( "sample_3b", options );

	fsm.start();
	std::cout << "FSM Stopped by inner event\n";
}
