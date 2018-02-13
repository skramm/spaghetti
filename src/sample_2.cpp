/**
\file sample_2.cpp
\brief demo program of stack overflow bug...

This file is part of Spaghetti, a C++ library for implementing Finite State Machines

Homepage: https://github.com/skramm/spaghetti
*/

//#define SPAG_PRINT_STATES
//#define SPAG_ENUM_STRINGS
#include "spaghetti.hpp"


enum En_States { st_1, st_2, NB_STATES };
enum En_Events { ev_1, NB_EVENTS };

SPAG_DECLARE_FSM_TYPE_NOTIMER( fsm_t, En_States, En_Events, int );


void cb( int )
{
	static int c;
	c++;
	if( !(c%200) )
		std::cout << "c=" << c << '\n';
}
//-----------------------------------------------------------------------------------
int main( int, char* argv[] )
{
	std::cout << argv[0] << ": " << fsm_t::buildOptions() << '\n';

	try
	{
		fsm_t fsm;
		fsm.assignString2State( st_1, "STATE 1" );
		fsm.assignString2State( st_2, "STATE-2" );

		fsm.assignTransition( st_1, st_2 );
		fsm.assignTransition( st_2, st_1 );
		fsm.assignGlobalCallback( cb );
		fsm.printConfig( std::cout );

		fsm.start();  // blocking !
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


