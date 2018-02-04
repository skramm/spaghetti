/**
\file test_buttons_2.cpp
\brief Simple example of an SpagFSM without timer and with the default callback function type (different callback function for each state).

https://en.wikipedia.org/wiki/Finite-state_machine#Example:_coin-operated_turnstile

*/
#include <iostream>

//#define SPAG_ENABLE_LOGGING
//#define SPAG_PRINT_STATES

#include "spaghetti.hpp"

enum States { st_Locked, st_Unlocked, NB_STATES };
enum Events { ev_Push, ev_Coin, NB_EVENTS };

/// callback function
void cb_Lock( spag::DummyCbArg_t )
{
	std::cout << "Locked!\n";
}
void cb_Unlock( spag::DummyCbArg_t )
{
	std::cout << "Unlocked!\n";
}

typedef spag::SpagFSM<States,Events,spag::NoTimer<States,Events>> fsm_t;

//-----------------------------------------------------------------------------------
void configureFSM( fsm_t& fsm )
{
	fsm.assignExtTransition( st_Locked,   ev_Coin, st_Unlocked );
	fsm.assignExtTransition( st_Unlocked, ev_Push, st_Locked );
spag::DummyCbArg_t a;
	fsm.assignCallback( st_Locked,   cb_Lock );
	fsm.assignCallback( st_Unlocked, cb_Unlock, a );
}

//-----------------------------------------------------------------------------------
int main( int argc, char* argv[] )
{
//	SPAG_DECLARE_FSM( fsm, States, Events );
//	spag::SpagFSM<States,Events,spag::NoTimer<States,Events>> fsm;

	fsm_t fsm;
	std::cout << argv[0] << ": " << fsm.buildOptions() << '\n';

	configureFSM( fsm )	;
	fsm.printConfig( std::cout );
	fsm.start();
	do
	{
		char key;
		std::cout << "Enter command: ";
		std::cin >> key;
		switch( key )
		{
			case 'A':
				std::cout << "Event: push\n";
				fsm.processExtEvent( ev_Push );
			break;

			case 'B':
				std::cout << "Event: coin\n";
				fsm.processExtEvent( ev_Coin );
			break;

			default: std:: cout << "invalid key\n";
		}
#ifdef SPAG_ENABLE_LOGGING
		fsm.printLoggedData( std::cout );
#endif
	}
	while( 1 );
}
//-----------------------------------------------------------------------------------
