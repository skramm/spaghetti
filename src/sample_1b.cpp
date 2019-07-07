/**
\file sample_1b.cpp
\brief Similar to sample_1.cpp but with a timer (and subsequently, the UI in a thread).
simple FSM with 2 events (key press), 5 states, 1 timeout and 1 pass state.

Also demonstrates how string states values can be used as callback arguments.

\image html sample_1b.svg

This file is part of Spaghetti, a C++ library for implementing Finite State Machines

Homepage: https://github.com/skramm/spaghetti
*/
#define SPAG_EMBED_ASIO_WRAPPER
#define SPAG_USE_SIGNALS
#define SPAG_ENABLE_LOGGING
#define SPAG_ENUM_STRINGS
#include "spaghetti.hpp"

#include <thread>

enum States { st0, st1, st2, st3, st_err, NB_STATES };
enum Events { ev_1, ev_2, ev_inner, NB_EVENTS };

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
				case '3': fsm->processEvent( ev_inner ); break; // this is only to make sure that this will generate an error
				case 'q': quit = true; fsm->stop();  break;
			}
		}
    }
    while( !quit );
}

void configureFSM( fsm_t& fsm )
{
	fsm.assignCallback( cb_func );

	std::map<States,std::string> mstr = {
		{ st0, "START"  },
		{ st_err, "Error state" }
	};
	fsm.assignStrings2States( mstr );
	fsm.assignCBValuesStrings();

	fsm.assignTimeOut( st0, 1200, "ms", st1 );
	fsm.assignTransition( st1, ev_1, st2 );
	fsm.assignTransition( st2, ev_1, st3 );
	fsm.assignTransition( st3, ev_1, st1 );

	fsm.assignTransition( st1, ev_2, st_err );
	fsm.assignTransition( st2, ev_2, st_err );
	fsm.assignTransition( st3, ev_2, st_err );

	fsm.assignAAT( st_err, st0 );   // st_err is a "pass state": Always Active Transition
	fsm.assignInnerTransition( st1, ev_inner, st2 ); // dummy transition, just for error checking
}

int main( int, char* argv[] )
{
	std::cout << argv[0] << ": " << fsm_t::buildOptions() << '\n';

	fsm_t fsm;
	configureFSM( fsm );

	fsm.printConfig( std::cout );
	fsm.writeDotFile( "sample_1b" );
	std::cout << "Enter event key: 1 or 2 (q:quit)\n";

	std::thread thread_ui( UI_thread<fsm_t>, &fsm );
	fsm.start();  // blocking !
	thread_ui.join();

	fsm.getCounters().print();
}
