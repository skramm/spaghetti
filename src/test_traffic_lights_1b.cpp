/**
\file test_traffic_lights_1b.cpp
\brief a simple traffic light example, build using boost::asio

With a unique callback function

status: seems ok

Just a POC, no network involved here, only timers
*/


#include "udp_server.hpp"

//#define SPAG_PRINT_STATES

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
template<typename ST, typename EV,typename CBA>
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

	void timerCallback( const boost::system::error_code& , const spag::SpagFSM<ST,EV,AsioWrapper<ST,EV,CBA>,CBA>* fsm  )
	{
		fsm->processTimeOut();
	}

	void timerStart( const spag::SpagFSM<ST,EV,AsioWrapper<ST,EV,CBA>,CBA>* fsm )
	{
		int nb_sec = fsm->timeOutDuration( fsm->currentState() );
		ptimer->expires_from_now( boost::posix_time::seconds(nb_sec) );

		ptimer->async_wait(
			boost::bind(
				&AsioWrapper<ST,EV,CBA>::timerCallback,
				this,
				boost::asio::placeholders::error,
				fsm
			)
		);
	}
};

typedef spag::SpagFSM<STATE,EVENT,AsioWrapper<STATE,EVENT,std::string>,std::string> fsm_t;

//-----------------------------------------------------------------------------------
/// concrete class, implements udp_server
struct my_server : public udp_server<2048>
{
	my_server( AsioWrapper<STATE,EVENT,std::string>& asio_wrapper, int port_no )
		: udp_server( asio_wrapper.io_service, port_no )
	{}

/*	my_server( boost::asio::io_service& io_service, int port_no )
			:udp_server( io_service, port_no )
	{}*/

	std::vector<BYTE> GetResponse( const Buffer_t& buffer, std::size_t nb_bytes ) const
	{
		return std::vector<BYTE>(); // return empty vector at present...
	}
};

//-----------------------------------------------------------------------------------
void callback( std::string v )
{
	std::cout << "cb, value=" << v << '\n';
}


//-----------------------------------------------------------------------------------
int main( int argc, char* argv[] )
{
	fsm_t fsm;

	std::cout << argv[0] << ": " << fsm.buildOptions() << '\n';

	std::cout << "fsm: nb states=" << fsm.nbStates() << " nb_events=" << fsm.nbEvents() << "\n";
	fsm.assignTimeOut( ST_INIT,   3, ST_RED    ); // if state ST_INIT and time out of 5s occurs, then switch to state ST_RED
	fsm.assignTimeOut( ST_RED,    4, ST_GREEN  );
	fsm.assignTimeOut( ST_GREEN,  4, ST_ORANGE );
	fsm.assignTimeOut( ST_ORANGE, 2, ST_RED   );

	fsm.assignTransitionAlways( EV_RESET,      ST_INIT ); // if reception of message EV_RESET, then switch to state ST_RED, whatever the current state is
	fsm.assignTransitionAlways( EV_WARNING_ON, ST_WARNING_ON );

	fsm.assignTimeOut( ST_WARNING_ON,  1, ST_WARNING_OFF );
	fsm.assignTimeOut( ST_WARNING_OFF, 1, ST_WARNING_ON );

	fsm.assignCallback( ST_RED,    callback, std::string("RED") );
	fsm.assignCallback( ST_ORANGE, callback, std::string("ORANGE") );
	fsm.assignCallback( ST_GREEN,  callback, std::string("GREEN") );

	fsm.printConfig( std::cout );
	try
	{
		AsioWrapper<STATE,EVENT,std::string> asio;
		std::cout << "io_service created\n";
		fsm.assignTimer( &asio );

		my_server server( asio, 12345 );
		std::cout << "server created\n";

//		server.start_receive();
//		std::cout << "server waiting\n";

		fsm.start();

		std::cout << " -start event loop\n";
		asio.run();
	}
	catch( std::exception& e )
	{
		std::cerr << "catch error: " << e.what() << std::endl;
	}

}

