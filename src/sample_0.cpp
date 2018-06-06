/**
\file sample_0.cpp
\brief Demo of a running 2-states FSM with:
- 1 hardware (keyboard) event to switch from initial state to other state (handled from a thread)
- 1 timeout event to switch from second state to initial state

Uses the embedded boost::asio timer

This file is part of Spaghetti, a C++ library for implementing Finite State Machines

Homepage: https://github.com/skramm/spaghetti
*/

#define SPAG_EMBED_ASIO_WRAPPER
#define SPAG_FRIENDLY_CHECKING
#define SPAG_ENABLE_LOGGING
#include "spaghetti.hpp"

#include <thread>
#include <mutex>

enum States { st_init, st_one, NB_STATES };
enum Events { ev_1, NB_EVENTS };

SPAG_DECLARE_FSM_TYPE_ASIO( fsm_t, States, Events, std::string );

//-----------------------------------------------------------------------------------
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

	fsm_t fsm;
	try
	{
		fsm.assignTransition( st_init, ev_1, st_one );
		fsm.assignTimeOut(    st_one,  1,    st_init);

		fsm.assignCallback( st_init, cb_func, "st0" );
		fsm.assignCallback( st_one, cb_func, "st1" );
		fsm.printConfig( std::cout );
		fsm.writeDotFile( "sample_0" );
		std::thread thread_ui( UI_thread<fsm_t>, &fsm );
		fsm.start();  // blocking !
		thread_ui.join();
	}
	catch( std::exception& e )
	{
		std::cerr << "catch: error: " << e.what() << std::endl;
	}
	fsm.printLoggedData( std::cout, spag::PrintFlags::stateCount );
}
//-----------------------------------------------------------------------------------


