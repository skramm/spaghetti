/**
\file turnstyle_1.cpp
\brief Turnstyle example: FSM without timer and with a custom callback function type.
See https://en.wikipedia.org/wiki/Finite-state_machine#Example:_coin-operated_turnstile

This file is part of Spaghetti, a C++ library for implementing Finite State Machines

Homepage: https://github.com/skramm/spaghetti
*/


#define SPAG_ENUM_STRINGS
#define SPAG_ENABLE_LOGGING

#include "spaghetti.hpp"

enum class States { st_Locked, st_Unlocked, NB_STATES };
enum class Events { ev_Push, ev_Coin, NB_EVENTS };

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
	fsm.assignTransition( States::st_Locked,   Events::ev_Coin, States::st_Unlocked );
	fsm.assignTransition( States::st_Unlocked, Events::ev_Push, States::st_Locked );

	fsm.assignTransition( States::st_Locked,   Events::ev_Push, States::st_Locked );
	fsm.assignTransition( States::st_Unlocked, Events::ev_Coin, States::st_Unlocked );

	fsm.assignCallback( States::st_Locked,   cb_func, true );
	fsm.assignCallback( States::st_Unlocked, cb_func, false );

	fsm.assignString2State( States::st_Locked, "Locked" );
	fsm.assignString2Event( Events::ev_Coin,   "Enter coin" );
}

//-----------------------------------------------------------------------------------
int main( int, char* argv[] )
{
	std::cout << argv[0] << ": " << fsm_t::buildOptions() << '\n';

	fsm_t fsm;
	std::cout << "Enter key anytime (c: coin, p: push, q: quit)\n";

	configureFSM( fsm )	;
//	fsm.printConfig( std::cout );
	fsm.start();
	bool quit(false);
	do
	{
		char key;
		std::cout << "Enter command: ";
		std::cin >> key;
		switch( key )
		{
			case 'p':
				std::cout << "Event: push\n";
				fsm.processEvent( Events::ev_Push );
			break;

			case 'c':
				std::cout << "Event: coin\n";
				fsm.processEvent( Events::ev_Coin );
			break;

			case 'q':
				quit = true;
			break;

			default: std:: cout << "Invalid key\n";
		}
	}
	while( !quit );

	fsm.getCounters().print();
}
//-----------------------------------------------------------------------------------
