/**
\file sample_3.cpp
\brief Demo program of a simple FSM: 5 states with 1, 2, 3... ms between each, and with FSM as a global variable.
Also demonstrates how user code can stop the running FSM.

\image html sample_3.svg

This file is part of Spaghetti, a C++ library for implementing Finite State Machines

Homepage: https://github.com/skramm/spaghetti
*/

#define SPAG_EMBED_ASIO_TIMER
#define SPAG_GENERATE_DOTFILE
#define SPAG_ENABLE_LOGGING
#include "spaghetti.hpp"

enum States { st0, st1, st2, st3, st4, NB_STATES };
enum Events { NB_EVENTS };

SPAG_DECLARE_FSM_TYPE_ASIO( fsm_t, States, Events, int );

fsm_t fsm;

void cb_func( int )
{
	static int c;
	if( c < 5 )
		fsm.writeDotFile( "sample_3_" + std::to_string(c) );
	c++;
	if( !(c%100) )
		std::cout << "c=" << c << '\n';
	if( c>1000 )
		fsm.stop();
}

int main( int, char* argv[] )
{
	std::cout << argv[0] << ": " << fsm_t::buildOptions() << '\n';

	fsm.setTimerDefaultUnit( "ms" );

	fsm.assignCallback( cb_func );
	fsm.assignTimeOut( st0, 1 , st1 );
	fsm.assignTimeOut( st1, 2 , st2 );
	fsm.assignTimeOut( st2, 3 , st3 );
	fsm.assignTimeOut( st3, 4 , st4 );
	fsm.assignTimeOut( st4, 5 , st0 );

	fsm.printConfig( std::cout );
	fsm.writeDotFile( "sample_3" );

	fsm.start();
	std::cout << "FSM Stopped by callback action\n";
}
