/**
\file traffic_lights_1c.cpp
\brief same as traffic_lights_1.cpp but with the fsm and the callback function inside a class

This file is part of Spaghetti, a C++ library for implementing Finite State Machines

Homepage: https://github.com/skramm/spaghetti
*/

#define SPAG_EMBED_ASIO_TIMER
#define SPAG_GENERATE_DOTFILE
#define SPAG_USE_SIGNALS
#include "spaghetti.hpp"

//-----------------------------------------------------------------------------------
enum States { st_Init, st_Red, st_Orange, st_Green, st_All, NB_STATES };
enum Events { ev_special, NB_EVENTS };

SPAG_DECLARE_FSM_TYPE_ASIO( fsm_t, States, Events, std::string );

struct TestClass
{
	fsm_t fsm;
//	bool surprise = false;
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

		if( redCounter == 2 )
		{
//			surprise = true;
			std::cout << "process special !\n";
			redCounter =0;
			fsm.activateInnerEvent( ev_special );
		}
		std::cout << "callback end\n";
	}

	void config()
	{
		fsm.setTimerDefaultUnit( "ms" );
		fsm.assignTimeOut( st_Init,   200, st_Red    );
		fsm.assignTimeOut( st_Red,    600, st_Green  );
		fsm.assignTimeOut( st_Green,  600, st_Orange );
		fsm.assignTimeOut( st_Orange, 300, st_Red   );

		fsm.assignInnerTransition( st_Red, ev_special, st_All );

		fsm.assignCallback( std::bind( &TestClass::callback, this, std::placeholders::_1 ) );
		fsm.assignCallbackValue( st_Red,    "RED" );
		fsm.assignCallbackValue( st_Orange, "ORANGE" );
		fsm.assignCallbackValue( st_Green,  "GREEN" );
		fsm.assignCallbackValue( st_All,  "RED-GREEN-ORANGE" );

		fsm.printConfig( std::cout );
		fsm.writeDotFile( "traffic_lights_1c.dot" );
	}
};

//-----------------------------------------------------------------------------------
int main( int, char* argv[] )
{
	TestClass test;
	test.config();
	test.start();
}
//-----------------------------------------------------------------------------------

