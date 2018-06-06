/**
\file sample_1.cpp
\brief Demo program of a simple FSM with 3 events (key press) and 5 states. No Timer
\image html sample_1.svg

This file is part of Spaghetti, a C++ library for implementing Finite State Machines

Homepage: https://github.com/skramm/spaghetti
*/

#define SPAG_ENABLE_LOGGING
#define SPAG_ENUM_STRINGS
#include "spaghetti.hpp"

enum States { st0, st1, st2, st3, st4, NB_STATES };
enum Events { ev_1, ev_2, ev_3, NB_EVENTS };

SPAG_DECLARE_FSM_TYPE_NOTIMER( fsm_t, States, Events, std::string );

void cb_func( std::string s )
{
	std::cout << "callback: " << s << '\n';
}

int main( int, char* argv[] )
{
	std::cout << argv[0] << ": " << fsm_t::buildOptions() << '\n';

	fsm_t fsm;

	fsm.assignCallback( cb_func );

	fsm.assignCallbackValue( st0, "Init" );
	fsm.assignCallbackValue( st1, "st1" );
	fsm.assignCallbackValue( st2, "st2" );
	fsm.assignCallbackValue( st3, "st3" );
	fsm.assignCallbackValue( st4, "Error" );

	fsm.assignTransition( st0, ev_1, st1 );
	fsm.assignTransition( st1, ev_1, st2 );
	fsm.assignTransition( st2, ev_1, st3 );
	fsm.assignTransition( st3, ev_1, st1 );

	fsm.assignTransition( st1, ev_2, st4 );
	fsm.assignTransition( st2, ev_2, st4 );
	fsm.assignTransition( st3, ev_2, st4 );

	fsm.assignTransition( st4, ev_3, st0 );

	fsm.printConfig( std::cout );
	fsm.writeDotFile( "sample_1" );
	fsm.start();
	std::cout << "Enter event key: 1 or 2 (q:quit)\n";
	bool quit(false);
	do
	{
		char key;
		std::cin >> key;
		switch( key )
		{
			case '1': fsm.processEvent( ev_1 ); break;
			case '2': fsm.processEvent( ev_2 ); break;
			case '3': fsm.processEvent( ev_3 ); break;
			case 'q': quit = true; break;
			default: std::cout << "invalid key\n";
		}
	}
	while( !quit );
	fsm.printLoggedData( std::cout );
}
