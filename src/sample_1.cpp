/**
\file sample_1.cpp
\brief Demo program of a simple FSM with 2 events (key press), 5 states, and 1 pass state. No timer.
\image html sample1.svg

This file is part of Spaghetti, a C++ library for implementing Finite State Machines

Homepage: https://github.com/skramm/spaghetti
*/

#define SPAG_ENABLE_LOGGING
//#define SPAG_PRINT_STATES
#include "spaghetti.hpp"

enum En_States { st0, st1, st2, st3, st4, NB_STATES };
enum En_Events { ev_1, ev_2, NB_EVENTS };

SPAG_DECLARE_FSM_TYPE_NOTIMER( fsm_t, En_States, En_Events, std::string );

void cbfunc( std::string s )
{
	std::cout << "callback: " << s << '\n';
}

int main( int, char* argv[] )
{
	std::cout << argv[0] << ": " << fsm_t::buildOptions() << '\n';

	fsm_t fsm;
	fsm.assignGlobalCallback( cbfunc );

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

	fsm.assignTransition( st4, st0 );   // st4 is a "pass state": no transition

	fsm.printConfig( std::cout );
	std::cout << "Enter event key: 1 or 2 (q:quit)\n";
	bool quit(false);
	do
	{
		char key;
		std::cin >> key;
		switch( key )
		{
			case '1':
				fsm.processEvent( ev_1 );
			break;

			case '2':
				fsm.processEvent( ev_2 );
			break;

			case 'q':
				quit = true;
			break;

			default:
				std::cout << "invalid key\n";
		}
	}
	while( !quit );
	fsm.printLoggedData( std::cout, spag::PrintFlags::history );
}
