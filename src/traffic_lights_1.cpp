/**
\file test_traffic_lights_1.cpp
\brief a simple traffic light example, build using boost::asio as timer backend
*/


//#include "udp_server.hpp"

//#define SPAG_GENERATE_DOT
//#define SPAG_PRINT_STATES
#include "spaghetti.hpp"

#include "asio_wrapper.hpp"


#include <memory>

//-----------------------------------------------------------------------------------
enum STATE { ST_INIT=0, ST_RED, ST_ORANGE, ST_GREEN, ST_WARNING_ON, ST_WARNING_OFF, NB_STATES };
enum EVENT { EV_RESET=0, EV_WARNING_ON, NB_EVENTS };


SPAG_DECLARE_FSM_TYPE( fsm_t, STATE, EVENT, AsioWrapper, std::string );

//-----------------------------------------------------------------------------------
void callback( std::string v )
{
	std::cout << "cb, value=" << v << '\n';
}
//-----------------------------------------------------------------------------------
int main( int, char* argv[] )
{
	std::cout << argv[0] << ": " << fsm_t::buildOptions() << '\n';

	fsm_t fsm;

	std::cout << "fsm: nb states=" << fsm.nbStates() << " nb_events=" << fsm.nbEvents() << "\n";
	fsm.assignTimeOut( ST_INIT,   3, ST_RED    ); // if state ST_INIT and time out of 5s occurs, then switch to state ST_RED
	fsm.assignTimeOut( ST_RED,    4, ST_GREEN  );
	fsm.assignTimeOut( ST_GREEN,  4, ST_ORANGE );
	fsm.assignTimeOut( ST_ORANGE, 1, ST_RED   );

	fsm.assignTransitionAlways( EV_RESET,      ST_INIT ); // if reception of message EV_RESET, then switch to state ST_RED, whatever the current state is
	fsm.assignTransitionAlways( EV_WARNING_ON, ST_WARNING_ON );

	fsm.assignTimeOut( ST_WARNING_ON,  1, ST_WARNING_OFF );
	fsm.assignTimeOut( ST_WARNING_OFF, 1, ST_WARNING_ON );

	fsm.assignCallback( ST_RED,    callback, std::string("RED") );
	fsm.assignCallback( ST_ORANGE, callback, std::string("ORANGE") );
	fsm.assignCallback( ST_GREEN,  callback, std::string("GREEN") );

	fsm.printConfig( std::cout );
	fsm.writeDotFile( "test1.dot" );

	AsioWrapper<STATE,EVENT,std::string> asio;
	fsm.assignTimer( &asio );

	fsm.start();
}

