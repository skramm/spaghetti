/**
\file sample_0.cpp
\brief Demo of a running 2-states FSM with:
- 1 hardware (keyboard) event to switch from initial state to other state (handled from a thread)
- 1 timeout event to switch from second state to initial state

This file is part of Spaghetti, a C++ library for implementing Finite State Machines

Homepage: https://github.com/skramm/spaghetti
*/

#define SPAG_ENABLE_LOGGING
//#define SPAG_PRINT_STATES
#include "spaghetti.hpp"

#include "asio_wrapper.hpp"

#include <thread>
#include <mutex>


enum En_States { st_init, st_one, NB_STATES };
enum En_Events { ev_1, NB_EVENTS };

SPAG_DECLARE_FSM_TYPE( fsm_t, En_States, En_Events, AsioWrapper, std::string );

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
	bool quit(false);
    do
    {
		char key;
		std::cin >> key;
		{
			switch( key )
			{
				case 'a':
					fsm->processEvent( ev_1 );
				break;
				case 'q':
					fsm->stop();
					quit = true;
				break;
			}
		}
    }
    while( !quit );
}
//-----------------------------------------------------------------------------------
void cb_func( std::string s )
{
	std::cout << "callback: " << s << '\n';
}
//-----------------------------------------------------------------------------------
int main( int, char* argv[] )
{
	std::cout << argv[0] << ": " << fsm_t::buildOptions() << '\n';

	std::cout << "enter 'a' for event, 'q' to quit\n";

	g_mutex = getSingletonMutex();

	fsm_t fsm;
	AsioWrapper<En_States,En_Events,std::string> asio;
	fsm.assignTimer( &asio );
	try
	{
		fsm.assignTransition( st_init, ev_1, st_one );
		fsm.assignTimeOut(    st_one,  1,    st_init);

		fsm.assignCallback( st_init, cb_func, "st0" );
		fsm.assignCallback( st_one, cb_func, "st1" );
		fsm.printConfig( std::cout );
		std::thread thread_ui( UI_thread<fsm_t>, &fsm );
		fsm.start();  // blocking !
		thread_ui.join();
	}
	catch( std::exception& e )
	{
		std::cerr << "catch: error: " << e.what() << std::endl;
	}
	catch( ... )
	{
		std::cerr << "catch: unknown error\n";
	}
	fsm.printLoggedData( std::cout, spag::PrintFlags::stateCount );
}
//-----------------------------------------------------------------------------------


