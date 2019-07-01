/**
\file testB_4.cpp
\brief testing unique inner event handled in different states
*/

//#define SPAG_TRACK_RUNTIME
#define SPAG_EMBED_ASIO_WRAPPER
#define SPAG_ENUM_STRINGS
#define SPAG_ENABLE_LOGGING
//#define SPAG_PRINT_STATES
#define SPAG_USE_SIGNALS
#include "spaghetti.hpp"

#include <thread>
#include <mutex>

enum States { st0, st1, st2, st3, NB_STATES };
enum Events { ev0, ev1, ev2, internal, NB_EVENTS };

SPAG_DECLARE_FSM_TYPE_ASIO( fsm_t, States, Events, int );

fsm_t fsm;
int g_count = 0;
int g_ie_active = 3;


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
void cb( int s )
{
	{
		std::lock_guard<std::mutex> lock(*g_mutex);
		std::cout << "callback: current state=" << s << " count=" << g_count << std::endl;
	}
	if( g_count == g_ie_active )
	{
		{
			std::lock_guard<std::mutex> lock(*g_mutex);
			std::cout << "activating inner event ev0\n";
		}
		fsm.activateInnerEvent( internal );
	}
	if( g_count == 10 )
		fsm.stop();

	g_count++;
}

//-----------------------------------------------------------------------------------
template<typename FSM>
void UI_thread( const FSM* fsm )
{
	bool quit(false);
	static int iter;
    do
    {
		{
			std::lock_guard<std::mutex> lock(*g_mutex);
			std::cout << std::string( "enter key (iter="+ std::to_string( ++iter ) + "): " );
		}
		char key;
		std::cin >> key;
		{
			switch( key )
			{
				case '0': fsm->processEvent( ev0 ); break;
				case '1': fsm->processEvent( ev1 ); break;
				case '2': fsm->processEvent( ev2 ); break; // this is only to make sure that this will generate an error
				case 'q': quit = true; fsm->stop();  break;
			}
		}
    }
    while( !quit );
}

//-----------------------------------------------------------------------------------
int main( int argc, char* argv[] )
{
	g_mutex = getSingletonMutex();

	fsm.assignCallbackAutoval( cb );

	fsm.assignTransition( st0, ev0, st1 );
	fsm.assignTransition( st1, ev0, st2 );
	fsm.assignTransition( st2, ev0, st0 );

	fsm.assignInnerTransition( st1, internal, st3 );
	fsm.assignInnerTransition( st2, internal, st3 );

	fsm.assignTransition( st3, ev1, st1 );
	fsm.assignTransition( st3, ev2, st2 );

	fsm.writeDotFile( "test_4" );
	try
	{
		std::thread thread_ui( UI_thread<fsm_t>, &fsm );
		fsm.start();  // blocking !
		thread_ui.join();

	}
	catch( const std::exception& e )
	{
		std::cout << "Error: " << e.what() << '\n';
	}
	fsm.printLoggedData( std::cout );
}
