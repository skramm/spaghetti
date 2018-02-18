/**
\file asio_wrapper.hpp
\brief This is a wrapper on boost::asio components

\todo To keep the io_service running when not timer is active, a
\c boost::asio::io_service::work is launched.
Unfortunately, this was available in boost 1.54, but got deprecated and is not present anymore
(at least in 1.66).
So this needs a fix.

This file is part of Spaghetti, a C++ library for implementing Finite State Machines

Homepage: https://github.com/skramm/spaghetti

Licence: GPL 3
*/

#include "spaghetti.hpp"
#include <boost/bind.hpp>
#include <boost/asio.hpp>

//-----------------------------------------------------------------------------------
/// Wraps the boost::asio stuff to have an asynchronous timer easily available
/**
Rationale: holds a timer, created by constructor. It can then be used without having to create one explicitely.
That last point isn't that obvious, has it also must have a lifespan not limited to some callback function.
*/

template<typename ST, typename EV, typename CBA=int>
struct AsioWrapper
{
	private:

#if BOOST_VERSION < 106600
	boost::asio::io_service _io_service;
/// see http://www.boost.org/doc/libs/1_54_0/doc/html/boost_asio/reference/io_service.html
/// "Stopping the io_service from running out of work" at bottom of page
	boost::asio::io_service::work _work;
#else
	boost::asio::io_context _io_service;
#endif

	std::unique_ptr<boost::asio::deadline_timer> ptimer; ///< pointer on timer, will be allocated int constructor

	public:
/// Constructor
#if BOOST_VERSION < 106600
	AsioWrapper() : _work( _io_service )
	{
//		std::cout << "Boost < 1.66, started work\n";
		ptimer = std::unique_ptr<boost::asio::deadline_timer>( new boost::asio::deadline_timer(_io_service) );
	}
#else
	AsioWrapper()
	{
//		std::cout << "Boost >= 1.66, started executor_work_guard\n";

// see http://www.boost.org/doc/libs/1_66_0/doc/html/boost_asio/reference/io_service.html
		boost::asio::executor_work_guard<boost::asio::io_context::executor_type> = boost::asio::make_work_guard( _io_service );
		ptimer = std::unique_ptr<boost::asio::deadline_timer>( new boost::asio::deadline_timer(_io_service) );
	}
#endif

	AsioWrapper( const AsioWrapper& ) = delete; // non copyable

	boost::asio::io_service& get_io_service()
	{
		return _io_service;
	}

/// Mandatory function for SpagFSM. Called only once, when FSM is started
	void timerInit()
	{
		_io_service.run();          // blocking call !!!
	}
	void timerKill()
	{
		_io_service.stop();
	}
/// Timer callback function, called when timer expires.
	void timerCallback( const boost::system::error_code& err_code, const spag::SpagFSM<ST,EV,AsioWrapper,CBA>* fsm  )
	{
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
/// Mandatory function for SpagFSM. Cancel the pending async timer
	void timerCancel()
	{
//		SPAG_LOG << "Canceling timer, expiry in " << ptimer->expires_from_now().total_milliseconds() << " ms.\n";
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

