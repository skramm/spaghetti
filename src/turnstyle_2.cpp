/**
\file turnstyle_2.cpp
\brief Turnstyle example: similar to turnstyle_1.cp but with an added timeout.
See https://en.wikipedia.org/wiki/Finite-state_machine#Example:_coin-operated_turnstile
*/

#define SPAG_ENABLE_LOGGING
#define SPAG_PRINT_STATES


#include "spaghetti.hpp"


#include <iostream>
#include <boost/asio.hpp>
#include <boost/bind.hpp>

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

template<typename ST, typename EV>
struct AsioWrapper
{
	boost::asio::io_service io_service;
	std::unique_ptr<boost::asio::deadline_timer> ptimer;

	AsioWrapper()
	{
		ptimer = std::unique_ptr<boost::asio::deadline_timer>( new boost::asio::deadline_timer(io_service) );
	}
	void timerInit()
	{
		io_service.run();
	}
	void timerCallback( const boost::system::error_code& , const spag::SpagFSM<ST,EV,AsioWrapper,bool>* fsm  )
	{
		fsm->processTimeOut();
	}
	void timerCancel()
	{

	}
	void timerStart( const spag::SpagFSM<ST,EV,AsioWrapper,bool>* fsm )
	{
		int nb_sec = fsm->timeOutDuration( fsm->currentState() );
		ptimer->expires_from_now( boost::posix_time::seconds(nb_sec) );

		ptimer->async_wait(
			boost::bind(
				&AsioWrapper<ST,EV>::timerCallback,
				this,
				boost::asio::placeholders::error,
				fsm
			)
		);
	}
};

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

//-----------------------------------------------------------------------------------
int main( int argc, char* argv[] )
{
	fsm_t fsm;
	std::cout << argv[0] << ": " << fsm.buildOptions() << '\n';
	std::cout << " - hit A or B for events. C: quit\n";


	configureFSM( fsm )	;

	AsioWrapper<States,Events> timer;
	fsm.assignTimer( &timer );

	fsm.start();

//	fsm.printConfig( std::cout );

	bool quit(false);
	do
	{
		char key;
		std::cout << "Enter command: ";
		std::cin >> key;
		switch( key )
		{
			case 'A':
			case 'a':
				std::cout << "Event: push\n";
				fsm.processEvent( ev_Push );
			break;

			case 'B':
			case 'b':
				std::cout << "Event: coin\n";
				fsm.processEvent( ev_Coin );
			break;

			case 'C':
			case 'c':
				quit = true;
			break;

			default: std:: cout << "Invalid key\n";
		}
	}
	while( !quit );
	fsm.printLoggedData( std::cout );
}
//-----------------------------------------------------------------------------------
