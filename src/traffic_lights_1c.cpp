/**
\file traffic_lights_1c.cpp
\brief same as traffic_lights_1.cpp but with the fsm and the callback function inside a class.

Also demonstrates:
- how a inner event can be activated from a callback function: here, once the
RED state has been activate 5 times, we transition to the ALL state.
- how a pass state is implemented (see state


This file is part of Spaghetti, a C++ library for implementing Finite State Machines

Homepage: https://github.com/skramm/spaghetti
*/

#define SPAG_ENUM_STRINGS
#define SPAG_EMBED_ASIO_TIMER
#define SPAG_GENERATE_DOTFILE
#define SPAG_USE_SIGNALS
#include "spaghetti.hpp"

//-----------------------------------------------------------------------------------
enum States { st_Init, st_Red, st_Orange, st_Green, st_All, st_PASS, NB_STATES };
enum Events { ev_special, NB_EVENTS };

SPAG_DECLARE_FSM_TYPE_ASIO( fsm_t, States, Events, std::string );

std::map<States,std::string> states_str = {
	{ st_Init,    "Init" },
	{ st_Red,     "RED" },
	{ st_Orange,  "ORANGE" },
	{ st_Green,   "GREEN" },
	{ st_All,     "All" },
	{ st_PASS,    "PASS-STATE" },
};

struct TestClass
{
	fsm_t fsm;
	int redCounter = 0;
	void start()
	{
		fsm.start();
	}
	void callback( std::string v )
	{
		std::cout << "cb, value=" << v << '\n';
		if( v == "RED" )
			redCounter++;

		if( redCounter == 5 )
		{
			std::cout << "Activate inner event, next time on RED => proceed to ALL\n";
			redCounter =0;
			fsm.activateInnerEvent( ev_special );
		}
//		std::cout << "callback end\n";
	}

	void config()
	{
		fsm.assignStrings2States( states_str );
		fsm.setTimerDefaultUnit( "ms" );
		fsm.assignTimeOut( st_Init,   200, st_Red    );
		fsm.assignTimeOut( st_Red,    600, st_Green  );
		fsm.assignTimeOut( st_Green,  600, st_Orange );
		fsm.assignTimeOut( st_Orange, 300, st_Red   );

		fsm.assignInnerTransition( st_Red, ev_special, st_All );

		fsm.assignTransition( st_Orange, st_PASS );
//		fsm.assignTimeOut( st_PASS, 200, st_Red    );
		fsm.assignTransition( st_PASS,  st_Red    );

		fsm.assignCallback( std::bind( &TestClass::callback, this, std::placeholders::_1 ) );
		fsm.assignCallbackValue( st_Init,    "Init state" );
		fsm.assignCallbackValue( st_Red,     "RED" );
		fsm.assignCallbackValue( st_Orange,  "ORANGE" );
		fsm.assignCallbackValue( st_Green,   "GREEN" );
		fsm.assignCallbackValue( st_All,     "ALL: RED-GREEN-ORANGE" );
		fsm.assignCallbackValue( st_PASS, "PASS STATE" );

		fsm.printConfig( std::cout );
		fsm.writeDotFile( "traffic_lights_1c.dot" );
	}
};

//-----------------------------------------------------------------------------------
int main( int, char* argv[] )
{
	std::cout << fsm_t::buildOptions();
	TestClass test;
	test.config();
	test.start();
}
//-----------------------------------------------------------------------------------

