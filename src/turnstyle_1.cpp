/**
\file turnstyle_1.cpp
\brief Turnstyle example: FSM without timer and with a custom callback function type.
See https://en.wikipedia.org/wiki/Finite-state_machine#Example:_coin-operated_turnstile

*/
#include <iostream>

#define SPAG_ENABLE_LOGGING
//#define SPAG_PRINT_STATES


#include "spaghetti.hpp"

enum States { st_Locked, st_Unlocked, NB_STATES };
enum Events { ev_Push, ev_Coin, NB_EVENTS };

/// callback function
void cb_func( bool b )
{
	if( b )
		std::cout << "State: Locked\n";
	else
		std::cout << "State: Unlocked\n";
}

SPAG_DECLARE_FSM_TYPE_NOTIMER( fsm_t, States, Events, bool );


//-----------------------------------------------------------------------------------
void configureFSM( fsm_t& fsm )
{
	fsm.assignTransition( st_Locked,   ev_Coin, st_Unlocked );
	fsm.assignTransition( st_Unlocked, ev_Push, st_Locked );

	fsm.assignTransition( st_Locked,   ev_Push, st_Locked );
	fsm.assignTransition( st_Unlocked, ev_Coin, st_Unlocked );

	fsm.assignCallback( st_Locked,   cb_func, true );
	fsm.assignCallback( st_Unlocked, cb_func, false );
}

//-----------------------------------------------------------------------------------
int main( int, char* argv[] )
{
	std::cout << argv[0] << ": " << fsm_t::buildOptions() << '\n';

	fsm_t fsm;
	std::cout << " - hit A or B for events. C: quit\n";

	configureFSM( fsm )	;
//	fsm.printConfig( std::cout );
//	fsm.start();    // not needed here
	bool quit(false);
	do
	{
		char key;
		std::cout << "Enter command: ";
		std::cin >> key;
		switch( key )
		{
			case 'A':
			case 'a':
				std::cout << "Event: push\n";
				fsm.processEvent( ev_Push );
			break;

			case 'B':
			case 'b':
				std::cout << "Event: coin\n";
				fsm.processEvent( ev_Coin );
			break;

			case 'C':
			case 'c':
				quit = true;
			break;

			default: std:: cout << "Invalid key\n";
		}
	}
	while( !quit );
	fsm.printLoggedData( std::cout );
}
//-----------------------------------------------------------------------------------
