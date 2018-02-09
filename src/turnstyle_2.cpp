/**
\file turnstyle_2.cpp
\brief Turnstyle example: similar to turnstyle_1.cp but with an added timeout.
See https://en.wikipedia.org/wiki/Finite-state_machine#Example:_coin-operated_turnstile
*/

#define SPAG_ENABLE_LOGGING
//#define SPAG_PRINT_STATES

#include "spaghetti.hpp"
#include "asio_wrapper.hpp"

#include <thread>
#include <mutex>

enum States { st_Locked, st_Unlocked, NB_STATES };
enum Events { ev_Push, ev_Coin, NB_EVENTS };

/// callback function
void cb_func( bool b )
{
	if( b )
		std::cout << "State: Locked\n";
	else
		std::cout << "State: Unlocked\n";
}

SPAG_DECLARE_FSM_TYPE( fsm_t, States, Events, AsioWrapper, bool );

//-----------------------------------------------------------------------------------
void configureFSM( fsm_t& fsm )
{
	fsm.assignTransition( st_Locked,   ev_Coin, st_Unlocked );
	fsm.assignTransition( st_Unlocked, ev_Push, st_Locked );

	fsm.assignTransition( st_Locked,   ev_Push, st_Locked );
	fsm.assignTransition( st_Unlocked, ev_Coin, st_Unlocked );

	fsm.assignTimeOut( st_Unlocked, 4, st_Locked );

	fsm.assignCallback( st_Locked,   cb_func, true );
	fsm.assignCallback( st_Unlocked, cb_func, false );
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
		std::cout << "Thread start, enter key anytime (c: coin, p: push, q: quit)\n";
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

	AsioWrapper<States,Events,bool> timer;
	fsm.assignTimer( &timer );

	std::thread thread_ui( UI_thread<fsm_t>, &fsm );

	fsm.start();  // blocking !
	thread_ui.join();
	fsm.printLoggedData( std::cout );
}
//-----------------------------------------------------------------------------------
