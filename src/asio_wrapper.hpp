/**
\file asio_wrapper.hpp
*/

#include <boost/bind.hpp>
#include <boost/asio.hpp>

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

