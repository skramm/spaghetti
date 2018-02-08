/**
\file traffic_lights_common.hpp
\brief holds some common code shared in traffic_lights_2.cpp and traffic_lights_3.cpp
*/
#include <thread>
#include <mutex>

extern std::mutex* g_mutex;

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

//-----------------------------------------------------------------------------------
/// callback function
void cb_func( std::string s)
{
	std::cout << s << '\n';
}
//-----------------------------------------------------------------------------------
template<typename FSM>
void
configureFSM( FSM& fsm )
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
					fsm->processEvent( EV_WARNING_ON );
				break;
				case 'b':
					std::cout << ": switch to normal mode\n";
					fsm->processEvent( EV_WARNING_OFF );
				break;
				case 'c':
					std::cout << ": reset\n";
					fsm->processEvent( EV_RESET );
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
	fsm->printLoggedData( std::cout );
}
//-----------------------------------------------------------------------------------


