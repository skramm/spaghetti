/**
\file test_traffic_lights_2.cpp
\brief a simple traffic light example, build using boost::asio

status: WIP

Similar to version 1, with an added udp server part, that can receive data from
test_traffic_lights_client.cpp
*/


#include "udp_server.hpp"

// this symbol is a build-time option
//#define SPAG_PRINT_STATES
#define SPAG_ENABLE_LOGGING
#include "spaghetti.hpp"

#include <memory>
#include <thread>
#include <mutex>

/// global pointer on mutex, will get initialised in getSingletonMutex()
std::mutex* g_mutex;

//-----------------------------------------------------------------------------------
/// initialization of mutex pointer (classical static init pattern)
static std::mutex* getSingletonMutex()
{
    static std::mutex instance;
    return &instance;
}

//-----------------------------------------------------------------------------------
enum STATE { ST_INIT=0, ST_RED, ST_ORANGE, ST_GREEN, ST_WARNING_ON, ST_WARNING_OFF, NB_STATES };
enum EVENT { EV_RESET=0, EV_WARNING_ON, NB_EVENTS };

//-----------------------------------------------------------------------------------
/// Wraps the boost::asio stuff
/**
Rationale: holds a timer, created by constructor. It can then be used without having to create one explicitely.
That last point isn't that obvious, has it also must have a lifespan not limited to some callback function.
*/
template<typename ST, typename EV, typename CBA>
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
	void timerCallback( const boost::system::error_code& err_code, const spag::SpagFSM<ST,EV,AsioWrapper,CBA>* fsm  )
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

	void timerStart( const spag::SpagFSM<ST,EV,AsioWrapper,CBA>* fsm )
	{
		int nb_sec = fsm->timeOutData( fsm->currentState() ).nbSec;
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

//-----------------------------------------------------------------------------------
/// concrete class, implements udp_server and SpagFSM
template<typename ST, typename EV, typename CBA>
struct my_server : public udp_server<2048>
{
	my_server( AsioWrapper<STATE,EVENT,CBA>& asio_wrapper, int port_no )
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

	spag::SpagFSM<ST,EV,AsioWrapper<ST,EV,CBA>,CBA> fsm;
};

//-----------------------------------------------------------------------------------
/// callback function

void cb_func( std::string s)
{
	std::cout << s << '\n';
}

typedef spag::SpagFSM<
	STATE,
	EVENT,
	AsioWrapper<STATE,EVENT,std::string>,
	std::string
	> fsm_t;

//-----------------------------------------------------------------------------------
void
configureFSM( fsm_t& fsm )
{
	fsm.assignTimeOut( ST_INIT,   3, ST_RED    ); // if state ST_INIT and time out of 5s occurs, then switch to state ST_RED
	fsm.assignTimeOut( ST_RED,    4, ST_GREEN  );
	fsm.assignTimeOut( ST_GREEN,  4, ST_ORANGE );
	fsm.assignTimeOut( ST_ORANGE, 2, ST_RED    );

	fsm.assignTransitionAlways( EV_RESET,      ST_INIT ); // if reception of message EV_RESET, then switch to state ST_RED, whatever the current state is
	fsm.assignTransitionAlways( EV_WARNING_ON, ST_WARNING_ON );

	fsm.assignTimeOut( ST_WARNING_ON,  1, ST_WARNING_OFF );
	fsm.assignTimeOut( ST_WARNING_OFF, 1, ST_WARNING_ON );

	fsm.assignGlobalCallback( cb_func );
	fsm.assignCallbackValue( ST_RED,         "RED" );
	fsm.assignCallbackValue( ST_GREEN,       "GREEN" );
	fsm.assignCallbackValue( ST_ORANGE,      "ORANGE" );
	fsm.assignCallbackValue( ST_ORANGE,      "ORANGE" );
	fsm.assignCallbackValue( ST_WARNING_ON,  "ORANGE-ON" );
	fsm.assignCallbackValue( ST_WARNING_OFF, "ORANGE-OFF" );
	fsm.assignCallbackValue( ST_INIT,        "Init" );
}
//-----------------------------------------------------------------------------------
void
UI_thread( const fsm_t* fsm )
{
	{
		std::lock_guard<std::mutex> lock(*g_mutex);
		std::cout << "Thread start, enter key anytime\n";
	}
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
					fsm->processExtEvent( EV_WARNING_ON );
				break;
				case 'b':
					std::cout << ": reset\n";
					fsm->processExtEvent( EV_RESET );
				break;
				default:
					std::cout << ": invalid key" << std::endl;
			}
		}
    }
    while(1);
}
//-----------------------------------------------------------------------------------
int main( int argc, char* argv[] )
{
	g_mutex = getSingletonMutex();
	try
	{
		AsioWrapper<STATE,EVENT,std::string> asio;
		std::cout << "io_service created\n";

		my_server<STATE,EVENT,std::string> server( asio, 12345 );

		std::cout << argv[0] << ": " << server.fsm.buildOptions() << '\n';
		std::cout << "server created\n";

		server.fsm.assignTimer( &asio );
		configureFSM( server.fsm );

		server.fsm.printConfig( std::cout );

		server.start_receive();
		std::cout << "server waiting\n";


		server.fsm.start();

		std::cout << " -start event loop\n";
		std::thread thread_ui( UI_thread, &server.fsm );
		asio.run();
	}
	catch( std::exception& e )
	{
		std::cerr << "catch error: " << e.what() << std::endl;
	}

}

