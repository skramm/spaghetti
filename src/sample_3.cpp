/**
\file sample_3.cpp
\brief Demo program of a simple FSM: 5 states with 1 ms between each
\image html sample_3.svg

This file is part of Spaghetti, a C++ library for implementing Finite State Machines

Homepage: https://github.com/skramm/spaghetti
*/

#define SPAG_EMBED_ASIO_TIMER
#define SPAG_GENERATE_DOTFILE
#define SPAG_ENABLE_LOGGING
//#define SPAG_PRINT_STATES
#include "spaghetti.hpp"

enum States { st0, st1, st2, st3, st4, NB_STATES };
enum Events { NB_EVENTS };

SPAG_DECLARE_FSM_TYPE_ASIO( fsm_t, States, Events, int );

void cb_func( int )
{
	static int c;
	c++;
	if( !(c%100) )
		std::cout << "c=" << c << '\n';
}

int main( int, char* argv[] )
{
	std::cout << argv[0] << ": " << fsm_t::buildOptions() << '\n';

	fsm_t fsm;
	fsm.setTimerDefaultUnit( "ms" );

	fsm.assignCallback( st0, cb_func );
	fsm.assignTimeOut( st0, 1 , st1 );
	fsm.assignTimeOut( st1, 1 , st2 );
	fsm.assignTimeOut( st2, 1 , st3 );
	fsm.assignTimeOut( st3, 1 , st4 );
	fsm.assignTimeOut( st4, 1 , st0 );

	fsm.printConfig( std::cout );
	fsm.writeDotFile( "sample_3.dot" );
	fsm.start();
}
