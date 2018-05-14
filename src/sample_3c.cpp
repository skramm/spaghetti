/**
\file sample_3c.cpp
\brief Similar to sample_3b.cpp, but with external event handler


This file is part of Spaghetti, a C++ library for implementing Finite State Machines

Homepage: https://github.com/skramm/spaghetti
*/
#define SPAG_USE_ASIO_WRAPPER
#define SPAG_EXTERNAL_EVENT_LOOP
#define SPAG_GENERATE_DOTFILE
#define SPAG_ENABLE_LOGGING
#define SPAG_USE_SIGNALS
#define SPAG_FRIENDLY_CHECKING
#include "spaghetti.hpp"

enum States { st0, st1, st2, st3, st4, st_Final, NB_STATES };
enum Events { ev_1000, NB_EVENTS };

SPAG_DECLARE_FSM_TYPE_ASIO( fsm_t, States, Events, int );

fsm_t fsm;

void cb_func( int v )
{
	if( v == -1 )
	{
		std::cout << "Final state!\n";
		fsm.stop();
	}
	static int c;
	if( c < 5 )
		fsm.writeDotFile( "sample_3_" + std::to_string(c) );
	c++;
	if( !(c%100) )
		std::cout << "c=" << c << '\n';
	if( c>1000 )
		fsm.activateInnerEvent( ev_1000 );
}

int main( int, char* argv[] )
{
	std::cout << argv[0] << ": " << fsm_t::buildOptions() << '\n';

	boost::asio::io_service io_service;
	spag::AsioTimer asio(io_service);
	fsm.assignEventHandler( &asio );
	fsm.setTimerDefaultUnit( "ms" );

	fsm.assignCallback( cb_func );
	fsm.assignTimeOut( st0, 1 , st1 );
	fsm.assignTimeOut( st1, 2 , st2 );
	fsm.assignTimeOut( st2, 3 , st3 );
	fsm.assignTimeOut( st3, 4 , st4 );
	fsm.assignTimeOut( st4, 5 , st0 );

	fsm.assignInnerTransition( ev_1000, st_Final );
	fsm.assignCallbackValue( st_Final, -1 );
	fsm.printConfig( std::cout );
	fsm.writeDotFile( "sample_3" );

	fsm.start();
	io_service.run();
	std::cout << "FSM Stopped by inner event\n";
}
