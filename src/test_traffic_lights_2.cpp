/**
\file test_traffic_lights_2.cpp
\brief a simple traffic light example, build using boost::asio

status: WIP

Similar to version 1, with an added server part, that can receive data from
test_traffic_lights_client.cpp
*/


#include "udp_server.hpp"

// this symbol is a build-time option
//#define SPAG_PRINT_STATES
#define SPAG_ENABLE_LOGGING
#include "spaghetti.hpp"

#include <memory>

//-----------------------------------------------------------------------------------
enum STATE { ST_INIT=0, ST_RED, ST_ORANGE, ST_GREEN, ST_WARNING_ON, ST_WARNING_OFF, NB_STATES };
enum EVENT { EV_RESET=0, EV_WARNING_ON, NB_EVENTS };

//-----------------------------------------------------------------------------------
/// Wraps the boost::asio stuff
/**
Rationale: holds a timer, created by constructor. It can then be used without having to create one explicitely.
That last point isn't that obvious, has it also must have a lifespan not limited to some callback function.
*/
template<typename ST, typename EV>
struct AsioWrapper
{
	boost::asio::io_service io_service;
	std::unique_ptr<boost::asio::deadline_timer> ptimer;

	AsioWrapper()
	{
		ptimer = std::unique_ptr<boost::asio::deadline_timer>( new boost::asio::deadline_timer(io_service) );
	}
	void run()
	{
		io_service.run();
	}

/// timer callback function, called when timer expires
	void timerCallback( const boost::system::error_code& err_code, const spag::SpagFSM<ST,EV,AsioWrapper>* fsm  )
	{
		switch( err_code.value() ) // check if called because of timeout, or because of canceling operation
		{
			case boost::system::errc::operation_canceled:    // do nothing
			break;
			case 0:
				fsm->processTimerEvent();                    // normal operation: timer has expired
			break;
			default:                                         // all other values
				std::cout << "unexpected error code, message=" << err_code.message() << "\n";
				throw;
		}
	}

	void timerCancel()
	{
		std::cout << "Canceling timer !\n";
		ptimer->cancel_one();
	}

	void timerStart( const spag::SpagFSM<ST,EV,AsioWrapper>* fsm )
	{
		int nb_sec = fsm->timeOutData( fsm->currentState() ).nbSec;
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

//-----------------------------------------------------------------------------------
/// concrete class, implements udp_server and SpagFSM

struct my_server : public udp_server<2048>
{
	my_server( AsioWrapper<STATE,EVENT>& asio_wrapper, int port_no )
		: udp_server( asio_wrapper.io_service, port_no )
	{}

/*	my_server( boost::asio::io_service& io_service, int port_no )
			:udp_server( io_service, port_no )
	{}*/

	std::vector<BYTE> GetResponse( const Buffer_t& buffer, std::size_t nb_bytes ) const
	{
		std::cout << "received " << nb_bytes << " bytes\n";
		switch( buffer.at(0) )
		{
			case 'A':
				fsm.processExtEvent( EV_WARNING_ON );
			break;
			case 'B':
				fsm.processExtEvent( EV_RESET );
			break;
			default:
				std::cout << "Error: invalid message received !\n";
				throw;
		}
		return std::vector<BYTE>(); // return empty vector at present...
	}

	spag::SpagFSM<STATE,EVENT,AsioWrapper<STATE,EVENT>> fsm;
};

//-----------------------------------------------------------------------------------
void TL_red()
{
	std::cout << "RED\n";
}
void TL_orange()
{
	std::cout << "ORANGE ON\n";
}
void TL_orange_off()
{
	std::cout << "ORANGE OFF\n";
}
void TL_green()
{
	std::cout << "GREEN\n";
}

//-----------------------------------------------------------------------------------
void
configureFSM( spag::SpagFSM<STATE,EVENT,AsioWrapper<STATE,EVENT>>& fsm )
{
//	auto& fsm = server.fsm;
	fsm.assignTimeOut( ST_INIT,   5, ST_RED    ); // if state ST_INIT and time out of 5s occurs, then switch to state ST_RED
	fsm.assignTimeOut( ST_RED,    5, ST_GREEN  );
	fsm.assignTimeOut( ST_GREEN,  5, ST_ORANGE );
	fsm.assignTimeOut( ST_ORANGE, 2, ST_RED    );

	fsm.assignTransitionAlways( EV_RESET,      ST_INIT ); // if reception of message EV_RESET, then switch to state ST_RED, whatever the current state is
	fsm.assignTransitionAlways( EV_WARNING_ON, ST_WARNING_ON );

	fsm.assignTimeOut( ST_WARNING_ON,  1, ST_WARNING_OFF );
	fsm.assignTimeOut( ST_WARNING_OFF, 1, ST_WARNING_ON );

	fsm.assignCallback( ST_RED,    TL_red );
	fsm.assignCallback( ST_ORANGE, TL_orange );
	fsm.assignCallback( ST_GREEN,  TL_green );

	fsm.assignCallback( ST_WARNING_ON, TL_orange );
	fsm.assignCallback( ST_WARNING_OFF, TL_orange_off );

//	fsm.handleEvent( EV_RESET,)
}
//-----------------------------------------------------------------------------------
int main( int argc, char* argv[] )
{
	try
	{
		AsioWrapper<STATE,EVENT> asio;
		std::cout << "io_service created\n";

		my_server server( asio, 12345 );

		std::cout << argv[0] << ": " << server.fsm.buildOptions() << '\n';
		std::cout << "server created\n";

		server.fsm.assignTimer( &asio );
		configureFSM( server.fsm );

		server.fsm.printConfig( std::cout );

		server.start_receive();
		std::cout << "server waiting\n";

		server.fsm.start();

		std::cout << " -start event loop\n";
		asio.run();
	}
	catch( std::exception& e )
	{
		std::cerr << "catch error: " << e.what() << std::endl;
	}

}

