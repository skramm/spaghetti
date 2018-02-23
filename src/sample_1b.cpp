/**
\file sample_1b.cpp
\brief Similar to sample_1.cpp but with a timer (and subsequently, the UI in a thread).
simple FSM with 2 events (key press), 5 states, 1 timeout and 1 pass state
\image html sample_1b.svg

This file is part of Spaghetti, a C++ library for implementing Finite State Machines

Homepage: https://github.com/skramm/spaghetti
*/
#define SPAG_EMBED_ASIO_TIMER
#define SPAG_GENERATE_DOTFILE
#define SPAG_ENABLE_LOGGING
#define SPAG_ENUM_STRINGS
//#define SPAG_PRINT_STATES
#include "spaghetti.hpp"

#include <thread>

enum States { st0, st1, st2, st3, st4, NB_STATES };
enum Events { ev_1, ev_2, NB_EVENTS };

SPAG_DECLARE_FSM_TYPE_ASIO( fsm_t, States, Events, std::string );

void cb_func( std::string s )
{
	std::cout << "callback: " << s << '\n';
}

template<typename FSM>
void UI_thread( const FSM* fsm )
{
	bool quit(false);
    do
    {
		char key;
		std::cin >> key;
		{
			switch( key )
			{
				case '1': fsm->processEvent( ev_1 ); break;
				case '2': fsm->processEvent( ev_2 ); break;
				case 'q': quit = true; fsm->stop();  break;
			}
		}
    }
    while( !quit );
}

void configureFSM( fsm_t& fsm )
{
	fsm.assignGlobalCallback( cb_func );

	fsm.assignString2State( st0, "START" );
	fsm.assignCallbackValue( st0, "Init" );
	fsm.assignCallbackValue( st1, "st1" );
	fsm.assignCallbackValue( st2, "st2" );
	fsm.assignCallbackValue( st3, "st3" );
	fsm.assignCallbackValue( st4, "Error" );

	fsm.assignTimeOut( st0, 1200, "ms", st1 );
	fsm.assignTransition( st1, ev_1, st2 );
	fsm.assignTransition( st2, ev_1, st3 );
	fsm.assignTransition( st3, ev_1, st1 );

	fsm.assignTransition( st1, ev_2, st4 );
	fsm.assignTransition( st2, ev_2, st4 );
	fsm.assignTransition( st3, ev_2, st4 );

	fsm.assignTransition( st4, st0 );   // st4 is a "pass state": no transition
}

int main( int, char* argv[] )
{
	std::cout << argv[0] << ": " << fsm_t::buildOptions() << '\n';

	fsm_t fsm;
	configureFSM( fsm );

	fsm.printConfig( std::cout );
	fsm.writeDotFile( "sample_1b.dot" );
	std::cout << "Enter event key: 1 or 2 (q:quit)\n";

	std::thread thread_ui( UI_thread<fsm_t>, &fsm );
	fsm.start();  // blocking !
	thread_ui.join();

	fsm.printLoggedData( std::cout, spag::PrintFlags::history );
}
