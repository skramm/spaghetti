/**
\file traffic_lights_1b.cpp
\brief Same as traffic_lights_1b.cpp (traffic light), but with the boost.Asio Wrapper not embedded inside the FSM, provided separately

This file is part of Spaghetti, a C++ library for implementing Finite State Machines

Homepage: https://github.com/skramm/spaghetti
*/

#define SPAG_USE_ASIO_TIMER
#define SPAG_GENERATE_DOTFILE
//#define SPAG_PRINT_STATES

#include "spaghetti.hpp"

//-----------------------------------------------------------------------------------
enum States { st_Init, st_Red, st_Orange, st_Green, NB_STATES };
enum Events { NB_EVENTS };

SPAG_DECLARE_FSM_TYPE( fsm_t, States, Events, spag::AsioWrapper, std::string );

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
	spag::AsioWrapper<States,Events,std::string> asioWrapper;
	fsm.assignTimer( &asioWrapper );

	std::cout << "fsm: nb states=" << fsm.nbStates() << " nb_events=" << fsm.nbEvents() << "\n";
	fsm.assignTimeOut( st_Init,   3, st_Red    ); // if state st_Init and time out of 5s occurs, then switch to state st_Red
	fsm.assignTimeOut( st_Red,    4, st_Green  );
	fsm.assignTimeOut( st_Green,  4, st_Orange );
	fsm.assignTimeOut( st_Orange, 1, st_Red   );

	fsm.assignCallback( st_Red,    callback, std::string("RED") );
	fsm.assignCallback( st_Orange, callback, std::string("ORANGE") );
	fsm.assignCallback( st_Green,  callback, std::string("GREEN") );

	fsm.printConfig( std::cout );
	fsm.writeDotFile( "traffic_lights_1.dot" );

	fsm.start();
}
//-----------------------------------------------------------------------------------

