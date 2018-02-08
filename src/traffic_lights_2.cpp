/**
\file test_traffic_lights_2.cpp
\brief a simple traffic light example, build using boost::asio

status: WIP

Similar to version 1, with an added keyboard user interface
*/

//#include "udp_server.hpp"

#define SPAG_PRINT_STATES
#define SPAG_ENABLE_LOGGING
#define SPAG_ENUM_STRINGS

#include "spaghetti.hpp"

#include "asio_wrapper.hpp"

#include <memory>
#include <thread>
#include <mutex>

/// global pointer on mutex, will get initialised in getSingletonMutex()
std::mutex* g_mutex;

//-----------------------------------------------------------------------------------
/// initialization of mutex pointer (classical static initialization pattern)
static std::mutex* getSingletonMutex()
{
    static std::mutex instance;
    return &instance;
}
//-----------------------------------------------------------------------------------
enum STATE { ST_INIT=0, ST_RED, ST_ORANGE, ST_GREEN, ST_BLINK_ON, ST_BLINK_OFF, NB_STATES };
enum EVENT {
	EV_RESET=0,     ///< reset button
	EV_WARNING_ON,  ///< blinking mode on
	EV_WARNING_OFF, ///< blinking mode off
	NB_EVENTS
};

#include "keyb_ui_thread.hpp"

//-----------------------------------------------------------------------------------
/// callback function
void cb_func( std::string s)
{
	std::cout << s << '\n';
}

SPAG_DECLARE_FSM_TYPE( fsm_t, STATE, EVENT, AsioWrapper, std::string );

//-----------------------------------------------------------------------------------
void
configureFSM( fsm_t& fsm )
{
	fsm.assignTimeOut( ST_INIT,      1, ST_RED    ); // if state ST_INIT and time out of 5s occurs, then switch to state ST_RED
	fsm.assignTimeOut( ST_RED,       2, ST_GREEN  );
	fsm.assignTimeOut( ST_GREEN,     2, ST_ORANGE );
	fsm.assignTimeOut( ST_ORANGE,    1, ST_RED    );

	fsm.assignTimeOut( ST_BLINK_ON,  1, ST_BLINK_OFF );
	fsm.assignTimeOut( ST_BLINK_OFF, 1, ST_BLINK_ON );

	fsm.assignTransitionAlways( EV_RESET,       ST_INIT ); // if reception of message EV_RESET, then switch to state ST_INIT, whatever the current state is
	fsm.assignTransitionAlways( EV_WARNING_ON,  ST_BLINK_ON );
	fsm.assignTransition(       ST_BLINK_OFF, EV_WARNING_OFF, ST_RED );
	fsm.assignTransition(       ST_BLINK_ON,  EV_WARNING_OFF, ST_RED );

	fsm.assignGlobalCallback( cb_func );
	fsm.assignCallbackValue( ST_RED,       "RED" );
	fsm.assignCallbackValue( ST_GREEN,     "GREEN" );
	fsm.assignCallbackValue( ST_ORANGE,    "ORANGE" );
	fsm.assignCallbackValue( ST_BLINK_ON,  "BLINK-ON" );
	fsm.assignCallbackValue( ST_BLINK_OFF, "BLINK-OFF" );
	fsm.assignCallbackValue( ST_INIT,      "Init" );

	std::vector<std::pair<EVENT,std::string>> v_str = {
		{ EV_RESET,       "Reset" },
		{ EV_WARNING_ON,  "Warning On" },
		{ EV_WARNING_OFF, "Warning Off" }
	};
	fsm.assignStrings2Events( v_str );
}
//-----------------------------------------------------------------------------------
int main( int, char* argv[] )
{
	std::cout << argv[0] << ": " << fsm_t::buildOptions() << '\n';

	g_mutex = getSingletonMutex();
	try
	{
		fsm_t fsm;
		AsioWrapper<STATE,EVENT,std::string> asio;

		fsm.assignTimer( &asio );
		configureFSM( fsm );

		fsm.printConfig( std::cout );

		std::cout << " -start UI thread\n";
		std::thread thread_ui( UI_thread<fsm_t>, &fsm );

		fsm.start();  // blocking !
		thread_ui.join();
	}
	catch( std::exception& e )
	{
		std::cerr << "catch error: " << e.what() << std::endl;
	}
	catch( ... )
	{
		std::cerr << "catch unknown error\n";
	}

}
//-----------------------------------------------------------------------------------
