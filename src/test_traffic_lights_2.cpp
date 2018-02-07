/**
\file test_traffic_lights_2.cpp
\brief a simple traffic light example, build using boost::asio

status: WIP

Similar to version 1, with an added udp server part, that can receive data from
test_traffic_lights_client.cpp
*/

#include "udp_server.hpp"

//#define SPAG_PRINT_STATES
#define SPAG_ENABLE_LOGGING
#define SPAG_ENUM_STRINGS
#include "spaghetti.hpp"

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
	void timerInit()
	{
		io_service.run();
	}

/// Timer callback function, called when timer expires. Mandatory function for SpagFSM
	void timerCallback( const boost::system::error_code& err_code, const spag::SpagFSM<ST,EV,AsioWrapper,CBA>* fsm  )
	{
		switch( err_code.value() ) // check if called because of timeout, or because of canceling operation
		{
			case boost::system::errc::operation_canceled:    // do nothing
			break;
			case 0:
				fsm->processTimeOut();                    // normal operation: timer has expired
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

/// Start timer. Instanciation of mandatory function for SpagFSM
	void timerStart( const spag::SpagFSM<ST,EV,AsioWrapper,CBA>* fsm )
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

//-----------------------------------------------------------------------------------
/// concrete class, implements udp_server and SpagFSM, and triggers event on the FSM
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
			case 'a':
				fsm.processEvent( EV_WARNING_ON );
			break;
			case 'b':
				fsm.processEvent( EV_WARNING_OFF );
			break;
			case 'c':
				fsm.processEvent( EV_RESET );
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
	fsm.assignTimeOut( ST_INIT,      3, ST_RED    ); // if state ST_INIT and time out of 5s occurs, then switch to state ST_RED
	fsm.assignTimeOut( ST_RED,       4, ST_GREEN  );
	fsm.assignTimeOut( ST_GREEN,     4, ST_ORANGE );
	fsm.assignTimeOut( ST_ORANGE,    2, ST_RED    );
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
/// Console User Interface, enables action on FSM from user input
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

				case 'x':
					std::cout << ": x: QUIT\n";
					fsm->printLoggedData( std::cout );
					exit(0);
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

		std::cout << " -start UI thread\n";
		std::thread thread_ui( UI_thread, &server.fsm );
	}
	catch( std::exception& e )
	{
		std::cerr << "catch error: " << e.what() << std::endl;
	}

}

