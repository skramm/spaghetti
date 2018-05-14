/**
\file turnstyle_2.cpp
\brief Turnstyle example: similar to turnstyle_1.cp but with an added timeout.
See https://en.wikipedia.org/wiki/Finite-state_machine#Example:_coin-operated_turnstile

This file is part of Spaghetti, a C++ library for implementing Finite State Machines

Homepage: https://github.com/skramm/spaghetti
*/
#define SPAG_EMBED_ASIO_WRAPPER
#define SPAG_GENERATE_DOTFILE
#define SPAG_ENABLE_LOGGING
#define SPAG_ENUM_STRINGS
#include "spaghetti.hpp"

#include <thread>
#include <mutex>

enum States { st_Locked, st_Unlocked, st_error, NB_STATES };
enum Events { ev_Push, ev_Coin, ev_Reset, NB_EVENTS };

/// callback function
void cb_func( bool b )
{
	if( b )
		std::cout << "State: Locked\n";
	else
		std::cout << "State: Unlocked\n";
}

void cb_error( bool )
{
	std::cout << "Nobody showed up in 6 seconds, system blocked !\n";
}

void cb_ignEvents( States st, Events ev )
{
	std::cout << "Received ignored event " << ev << " while on states " << st << '\n';
}

SPAG_DECLARE_FSM_TYPE_ASIO( fsm_t, States, Events, bool );

//-----------------------------------------------------------------------------------
void configureFSM( fsm_t& fsm )
{
	fsm.assignTransition( st_Locked,   ev_Coin, st_Unlocked );
	fsm.assignTransition( st_Unlocked, ev_Push, st_Locked );

	fsm.assignTransition( st_Locked,   ev_Push, st_Locked );
//	fsm.assignTransition( st_Unlocked, ev_Coin, st_Unlocked ); // only one coin allowed !

	fsm.assignGlobalTimeOut( 6, "sec", st_error );
	fsm.assignTimeOut( st_Unlocked, 3, "sec", st_Locked );

	fsm.assignCallback( st_Locked,   cb_func, true );
	fsm.assignCallback( st_Unlocked, cb_func, false );
	fsm.assignCallback( st_error, cb_error );

	fsm.assignTransition( st_error, ev_Reset, st_Locked );
	fsm.assignIgnoredEventsCallback( cb_ignEvents );
}
/// global pointer on mutex, will get initialized in getSingletonMutex()
std::mutex* g_mutex;

//-----------------------------------------------------------------------------------
/// initialization of mutex pointer (classical static initialization pattern)
static std::mutex* getSingletonMutex()
{
    static std::mutex instance;
    return &instance;
}
//-----------------------------------------------------------------------------------
template<typename FSM>
void
UI_thread( const FSM* fsm )
{
	{
		std::lock_guard<std::mutex> lock(*g_mutex);
		std::cout << "Thread start, enter key anytime (c: coin, p: push, r: reset, q: quit)\n";
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
				case 'p':
					std::cout << ": push event\n";
					fsm->processEvent( ev_Push );
				break;
				case 'c':
					std::cout << ": coin event\n";
					fsm->processEvent( ev_Coin );
				break;
				case 'r':
					std::cout << ": RESET !\n";
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
int main( int, char* argv[] )
{
	std::cout << argv[0] << ": " << fsm_t::buildOptions() << '\n';
	g_mutex = getSingletonMutex();

	fsm_t fsm;
//	std::cout << " - hit c or B for events. C: quit\n";

	configureFSM( fsm )	;
	fsm.printConfig( std::cout );
	fsm.writeDotFile( "turnstyle_2" );

	std::thread thread_ui( UI_thread<fsm_t>, &fsm );

	fsm.start();  // blocking !
	thread_ui.join();
	fsm.printLoggedData( std::cout );
}
//-----------------------------------------------------------------------------------
