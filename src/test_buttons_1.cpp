/**
\file test_buttons_1.cpp
\brief Simple example of an SpagFSM without timer and with a custom callback function type

https://en.wikipedia.org/wiki/Finite-state_machine#Example:_coin-operated_turnstile

*/
#include <iostream>

//#define SPAG_ENABLE_LOGGING
//#define SPAG_PRINT_STATES


#include "spaghetti.hpp"

enum States { st_Locked, st_Unlocked, NB_STATES };
enum Events { ev_Push, ev_Coin, NB_EVENTS };

/// callback function
void cb_Lock( bool b )
{
	if( b )
		std::cout << "State: Locked\n";
	else
		std::cout << "State: Unlocked\n";
}
typedef spag::SpagFSM<States,Events,spag::NoTimer<States,Events,bool>,bool> fsm_t;

//-----------------------------------------------------------------------------------
void configureFSM( fsm_t& fsm )
{
	fsm.assignExtTransition( st_Locked,   ev_Coin, st_Unlocked );
	fsm.assignExtTransition( st_Unlocked, ev_Push, st_Locked );

	fsm.assignCallback( st_Locked,   cb_Lock, true );
	fsm.assignCallback( st_Unlocked, cb_Lock, false );
}

//-----------------------------------------------------------------------------------
int main( int argc, char* argv[] )
{
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
