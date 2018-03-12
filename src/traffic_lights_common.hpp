/**
\file traffic_lights_common.hpp
\brief holds some common code shared in traffic_lights_2.cpp and traffic_lights_3.cpp

This file is part of Spaghetti, a C++ library for implementing Finite State Machines

Homepage: https://github.com/skramm/spaghetti
*/

#include <thread>
#include <mutex>
#include <iostream>
#include <vector>
#include <boost/version.hpp>

extern std::mutex* g_mutex;

//-----------------------------------------------------------------------------------
/// initialization of mutex pointer (classical static initialization pattern)
static std::mutex* getSingletonMutex()
{
    static std::mutex instance;
    return &instance;
}
//-----------------------------------------------------------------------------------
/// traffic light states
enum States { st_Init=0, st_Red, st_Orange, st_Green, st_BlinkOn, st_BlinkOff, NB_STATES };

/// traffic light events
enum Events {
	ev_Reset=0,    ///< reset button
	ev_WarningOn,  ///< blinking mode on
	ev_WarningOff, ///< blinking mode off
	NB_EVENTS
};

//-----------------------------------------------------------------------------------
/// callback function
void cb_func( std::string s )
{
	std::cout << "Callback function: " << s << '\n';
}
//-----------------------------------------------------------------------------------
template<typename FSM>
void
configureFSM( FSM& fsm )
{
	fsm.setTimerDefaultUnit( "ms" );
	fsm.assignTimeOut( st_Init,      50,   st_Red    );
	fsm.assignTimeOut( st_Green,     800,  st_Orange );

	fsm.setTimerDefaultUnit( "sec" );
	fsm.assignTimeOut( st_Red,       1, st_Green  );
	fsm.assignTimeOut( st_Orange,    1, st_Red    );

	fsm.assignTimeOut( st_BlinkOn,  500, "ms", st_BlinkOff );
	fsm.assignTimeOut( st_BlinkOff, 500, "ms", st_BlinkOn );

	fsm.assignTransition( ev_Reset,     st_Init ); // if reception of message ev_Reset, then switch to state st_Init, whatever the current state is
//	fsm.assignTransitionAlways( ev_Reset,     st_Init ); // if reception of message ev_Reset, then switch to state st_Init, whatever the current state is

	fsm.assignTransition( ev_WarningOn, st_BlinkOn );
//	fsm.assignTransitionAlways( ev_WarningOn, st_BlinkOn );

	fsm.assignTransition(       st_BlinkOff,  ev_WarningOff, st_Red );
	fsm.assignTransition(       st_BlinkOn,   ev_WarningOff, st_Red );

	fsm.assignCallback( cb_func );
	fsm.assignCallbackValue( st_Red,      "RED" );
	fsm.assignCallbackValue( st_Green,    "GREEN" );
	fsm.assignCallbackValue( st_Orange,   "ORANGE" );
	fsm.assignCallbackValue( st_BlinkOn,  "BLINK-ON" );
	fsm.assignCallbackValue( st_BlinkOff, "BLINK-OFF" );
	fsm.assignCallbackValue( st_Init,     "Init" );

	std::vector<std::pair<Events,std::string>> v_str = {
//		{ ev_Reset,      "Reset" },
		{ ev_WarningOn,  "Warning On" },
		{ ev_WarningOff, "Warning Off" }
	};
	fsm.assignStrings2Events( v_str );
}
//-----------------------------------------------------------------------------------
template<typename FSM>
void
UI_thread( const FSM* fsm )
{
	{
		std::lock_guard<std::mutex> lock(*g_mutex);
		std::cout << "Thread start, enter key anytime (a: warning mode on, b: normal mode on, c: reset, q: quit)\n";
	}
	bool quit(false);
    do
    {
		char key;
		std::cin >> key;
		{
			std::lock_guard<std::mutex> lock(*g_mutex);
			std::cout << "**********************KEY FETCH: " << key;

			switch( key )
			{
				case 'a':
					std::cout << ": switch to warning mode\n";
					fsm->processEvent( ev_WarningOn );
				break;
				case 'b':
					std::cout << ": switch to normal mode\n";
					fsm->processEvent( ev_WarningOff );
				break;
				case 'c':
					std::cout << ": reset\n";
					fsm->processEvent( ev_Reset );
				break;
				case 'q':
					std::cout << ": QUIT\n";
					fsm->stop();
					quit = true;
				break;

				default:
					std::cout << ": invalid key" << std::endl;
			}
		}
    }
    while( !quit );
}
//-----------------------------------------------------------------------------------
/// from https://stackoverflow.com/questions/3708706/
std::string
GetBoostVersion()
{
	std::string dot( "." );
	return std::string( "Boost "
          + std::to_string( BOOST_VERSION / 100000     ) + dot  // major version
          + std::to_string( BOOST_VERSION / 100 % 1000 ) + dot  // minor version
          + std::to_string( BOOST_VERSION % 100 )               // patch level
          + '\n');
}
//-----------------------------------------------------------------------------------

/**
\mainpage

This is the reference pages related to the samples provided with the
Spaghetti library.

To get back to the reference manual of the library,
<a href="../../html/index.html">click here</a>.
*/
