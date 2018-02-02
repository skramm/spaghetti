/**
\file test_buttons.cpp
\brief Simple example of an SpagFSM without timer

https://en.wikipedia.org/wiki/Finite-state_machine#Example:_coin-operated_turnstile

*/
#define SPAG_ENABLE_LOGGING
#define SPAG_PRINT_STATES
#include "spaghetti.hpp"
#include <iostream>

enum States { st_Locked, st_Unlocked, NB_STATES };
enum Events { ev_Push, ev_Coin, NB_EVENTS };

void cb_Locked()
{
	std::cout << "Locked!\n";
}
void cb_Unlocked()
{
	std::cout << "Unlocked!\n";
}
//-----------------------------------------------------------------------------------
void configureFSM( spag::SpagFSM<States,Events,spag::NoTimer<States,Events>>& fsm )
{
	fsm.AssignExtTransition( st_Locked, ev_Coin, st_Unlocked );
	fsm.AssignExtTransition( st_Unlocked, ev_Push, st_Locked );

	fsm.assignCallback( st_Locked,   cb_Locked );
	fsm.assignCallback( st_Unlocked, cb_Unlocked );
}

//-----------------------------------------------------------------------------------
int main( int argc, char* argv[] )
{

	spag::SpagFSM<States,Events,spag::NoTimer<States,Events>> fsm;

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
				std::cout << " push!\n";
				fsm.processExtEvent( ev_Push );
			break;

			case 'B':
				std::cout << " coin!\n";
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
