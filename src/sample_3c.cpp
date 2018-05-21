/**
\file sample_3c.cpp
\brief Similar to sample_3b.cpp, but with external event handler

\bug for some reason, we have a problem here:
when SPAG_EXTERNAL_EVENT_LOOP is defined, the signal is raised but the handler is NOT called.
Need to investigate this...

This file is part of Spaghetti, a C++ library for implementing Finite State Machines

Homepage: https://github.com/skramm/spaghetti
*/

/// try to def and undef this:
//#define SPAG_EXTERNAL_EVENT_LOOP

#define SPAG_USE_ASIO_WRAPPER

#define SPAG_ENABLE_LOGGING
#define SPAG_USE_SIGNALS
#define SPAG_FRIENDLY_CHECKING
#include "spaghetti.hpp"

enum States { st0, st1, st2, st3, st_Final, NB_STATES };
enum Events { ev_special, NB_EVENTS };

SPAG_DECLARE_FSM_TYPE_ASIO( fsm_t, States, Events, int );

fsm_t fsm;

void cb_func( int v )
{
	std::cout << "cb v=" << v << '\n';

	if( v == -1 )
	{
		std::cout << "Final state!\n";
		fsm.stop();
	}
	static int c;
	c++;
	if( c==10)
	{
		std::cout << "c=" << c << ": activate special event!\n";
		fsm.activateInnerEvent( ev_special );
	}
}

int main( int, char* argv[] )
{
	std::cout << argv[0] << ": " << fsm_t::buildOptions() << '\n';

#ifdef SPAG_EXTERNAL_EVENT_LOOP
	boost::asio::io_service io_service;
	spag::AsioEL asio(io_service);
#else
	spag::AsioEL asio;
#endif

	fsm.assignEventHandler( &asio );
	fsm.setTimerDefaultUnit( "ms" );
	int dur = 300;
	fsm.assignCallback( cb_func );
	fsm.assignTimeOut( st0, dur, st1 );
	fsm.assignTimeOut( st1, dur, st2 );
	fsm.assignTimeOut( st2, dur, st3 );
	fsm.assignTimeOut( st3, dur, st0 );

	fsm.assignInnerTransition( ev_special, st_Final );
	fsm.assignCallbackValue( st_Final, -1 );
	fsm.assignCallbackValue( st0, 0 );
	fsm.assignCallbackValue( st1, 1 );
	fsm.assignCallbackValue( st2, 2 );
	fsm.assignCallbackValue( st3, 3 );
//	fsm.printConfig( std::cout );

	fsm.start();
	std::cout << "FSM started\n";
#ifdef SPAG_EXTERNAL_EVENT_LOOP
	io_service.run();
#endif
	std::cout << "io_service stopped\n";
}
