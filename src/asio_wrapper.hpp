/**
\file asio_wrapper.hpp
\brief This is a wrapper on boost::asio components

This file is part of Spaghetti, a C++ library for implementing Finite State Machines

Homepage: https://github.com/skramm/spaghetti

Licence: GPL 3
*/

#include <boost/bind.hpp>
#include <boost/asio.hpp>
#define CHECK_IF_RUNNING(a) \
	{ \
		std::cout << __FUNCTION__ << "(): call " << a <<  ": io_service = " << (io_service.stopped()?"stopped":"running") << '\n'; \
	}

//-----------------------------------------------------------------------------------
/// Wraps the boost::asio stuff to have an asynchronous timer easily available
/**
Rationale: holds a timer, created by constructor. It can then be used without having to create one explicitely.
That last point isn't that obvious, has it also must have a lifespan not limited to some callback function.
*/

template<typename ST, typename EV, typename CBA=int>
struct AsioWrapper
{
	boost::asio::io_service io_service;
/// see http://www.boost.org/doc/libs/1_54_0/doc/html/boost_asio/reference/io_service.html
/// "Stopping the io_service from running out of work" at bottom of page
	boost::asio::io_service::work work;
	std::unique_ptr<boost::asio::deadline_timer> ptimer;

	AsioWrapper(): work( io_service )
	{
		ptimer = std::unique_ptr<boost::asio::deadline_timer>( new boost::asio::deadline_timer(io_service) );
	}
/// Mandatory function for SpagFSM. Called only once, when FSM is started
	void timerInit()
	{
		io_service.run();          // blocking call !!!
	}
	void timerKill()
	{
		io_service.stop();
	}
/// Timer callback function, called when timer expires.
	void timerCallback( const boost::system::error_code& err_code, const spag::SpagFSM<ST,EV,AsioWrapper,CBA>* fsm  )
	{
//		SPAG_LOG << "start, err_code=" << err_code.value() << '\n';
		switch( err_code.value() ) // check if called because of timeout, or because of canceling timeout operation
		{
			case boost::system::errc::operation_canceled:    // do nothing
				SPAG_LOG << "err_code=operation_canceled\n";
			break;
			case 0:
				fsm->processTimeOut();                    // normal operation: timer has expired
			break;
			default:                                         // all other values
				std::cout << "unexpected error code, message=" << err_code.message() << "\n";
				throw;
		}
	}

/// Mandatory function for SpagFSM. Cancel the pending async timer, but restarts the io_service if needed \todo HOW DO I DO THAT!!!
	void timerCancel()
	{
//		SPAG_LOG << "Canceling timer, expiry in " << ptimer->expires_from_now().total_milliseconds() << " ms.\n";
		ptimer->cancel_one();
	}

/// Start timer. Instanciation of mandatory function for SpagFSM
	void timerStart( const spag::SpagFSM<ST,EV,AsioWrapper,CBA>* fsm )
	{
		int nb_sec = fsm->timeOutDuration( fsm->currentState() );
//		SPAG_LOG << "current state=" <<  fsm->currentState() << "launch timer with " << nb_sec << " s.\n";
		ptimer->expires_from_now( boost::posix_time::seconds(nb_sec) );

		ptimer->async_wait(
			boost::bind(
				&AsioWrapper<ST,EV,CBA>::timerCallback,
				this,
				boost::asio::placeholders::error,
				fsm
			)
		);
//		SPAG_LOG << "timer started, expiry in " << ptimer->expires_from_now().total_milliseconds() << " ms.\n";
	}
};

