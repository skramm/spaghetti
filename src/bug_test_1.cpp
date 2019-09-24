/**
\file bug_test_1.cpp
\brief temp


This file is part of Spaghetti, a C++ library for implementing Finite State Machines

Homepage: https://github.com/skramm/spaghetti
*/

#define SPAG_ENUM_STRINGS
#define SPAG_EMBED_ASIO_WRAPPER
#define SPAG_USE_SIGNALS
#define SPAG_PRINT_STATES
#include "spaghetti.hpp"

//-----------------------------------------------------------------------------------
enum States { st0, st1, st2, st_Cancel, NB_STATES };
enum Events { ev1, ev_cancel, NB_EVENTS };

SPAG_DECLARE_FSM_TYPE_ASIO( fsm_t, States, Events, std::string );

fsm_t fsm;

void cb_func( std::string s )
{
	static int c;
	std::cout << "callback: state " << s << ", c=" << ++c << " current state=" << (int)fsm.currentState() << '\n';
	if( c == 5 )
	{
		std::cout << "activating CANCEL\n";
		fsm.activateInnerEvent( ev_cancel );
	}
	if( c > 7 )
		fsm.stop();
}

std::map<States,std::string> states_str = {
	{ st0,    "Init" },
	{ st1,    "st1" },
	{ st_Cancel, "stCancel" },
	{ st2,    "st2" }
};

//-----------------------------------------------------------------------------------
int main( int, char* argv[] )
{
	std::cout << fsm_t::buildOptions();
	fsm.assignCallback( cb_func );
	fsm.assignStrings2States( states_str );

	fsm.assignCallbackValue( st0, "Init" );
	fsm.assignCallbackValue( st1, "st1" );
	fsm.assignCallbackValue( st2, "st2" );
	fsm.assignCallbackValue( st_Cancel, "CANCEL" );
	fsm.assignAAT( st_Cancel, st0 );
	fsm.assignTimeOut( st0, 1, st1 );
	fsm.assignTimeOut( st1, 1, st2 );
	fsm.assignTimeOut( st2, 1, st0 );
	fsm.assignInnerTransition( st2, ev_cancel, st_Cancel );
	fsm.writeDotFile( "bug" );
	fsm.start();
}
//-----------------------------------------------------------------------------------

