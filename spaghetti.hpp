/**
\file spaghetti.hpp
\brief single header file of Spaghetti FSM library, see home page for full details:
https://github.com/skramm/spaghetti
*/

#ifndef HG_SPAGHETTI_FSM_HPP
#define HG_SPAGHETTI_FSM_HPP

#include <vector>
#include <algorithm>
#include <functional>
#include <cassert>
#include <fstream>

#ifdef SPAG_ENABLE_LOGGING
	#include <chrono>
#endif

#ifdef SPAG_PRINT_STATES
	#include <iostream>
#endif

#ifdef SPAG_FRIENDLY_CHECKING
	#define SPAG_CHECK_EQUAL( a, b ) \
	{ \
		if( (a) != (b) ) \
		{ \
			std::cerr << "Spaghetti: runtime error in func: " << __FUNCTION__ << "(), values are not equal:\n" \
			<< " - "   << #a << " value=" << a \
			<< "\n - " << #b << " value=" << b << "\nExiting...\n"; \
			exit(1); \
		} \
	}
#else
	#define SPAG_CHECK_EQUAL( a, b ) assert( a == b )
#endif

#ifdef SPAG_FRIENDLY_CHECKING
	#define SPAG_CHECK_LESS( a, b ) \
		if( !( (a) < (b) ) )\
		{ \
			std::cerr << "Spaghetti: runtime error in func: " << __FUNCTION__ << "(), value is incorrect:\n" \
			<< " - "   << #a << " value=" << a \
			<< "\n - " << #b << " max value=" << b << "\nExiting...\n"; \
			exit(1); \
		}
#else
	#define SPAG_CHECK_LESS( a, b ) assert( a < b )
#endif


/// Main library namespace
namespace spag {

//namespace priv {
/// This is just to provide a dummy type for the callback argument, as \c void is not a valid type
struct DummyCbArg_t {};
//typedef void DummyCbArg_t;

//}

//-----------------------------------------------------------------------------------
/// Container holding information on timeout events. Each state will have one, event if it does not use it
template<typename STATE>
struct TimerEvent
{
	STATE nextState = static_cast<STATE>(0); ///< state to switch to
	int   nbSec   = 1;                       ///< duration
	bool  enabled = 0;                       ///< this state uses or not a timeout (default is no)

	TimerEvent()
		: nextState(static_cast<STATE>(0))
		, nbSec(0)
		, enabled(false)
	{
	}
	TimerEvent( STATE st, int nbs ): nextState(st), nbSec(nbs)
	{
		enabled = true;
	}
};
//-----------------------------------------------------------------------------------
/// Holds the FSM dynamic data: current state, and logged data (if enabled at build, see symbol \c SPAG_ENABLE_LOGGING at \ref BuildOption )
#ifdef SPAG_ENABLE_LOGGING
template<typename STATE,typename EVENT>
#else
template<typename STATE>
#endif
struct FsmData
{
	STATE _current = static_cast<STATE>(0);

#ifdef SPAG_ENABLE_LOGGING
	FsmData()
	{
		_startTime = std::chrono::high_resolution_clock::now();
	}
	std::chrono::time_point<std::chrono::high_resolution_clock> _startTime;

/// a state-change event, used for logging, see _history
	struct StateChangeEvent
	{
		STATE state;
		EVENT event;
		std::chrono::duration<double> elapsed;

		friend std::ostream& operator << ( std::ostream& s, const StateChangeEvent& sce )
		{
			s << sce.elapsed.count() << ": event: " << sce.event << ": switch to state " << sce.state << '\n';
			return s;
		}
	};

	std::vector<size_t>  _stateCounter;    ///< per state counter
	std::vector<size_t>  _eventCounter;    ///< per event counter
/// Dynamic history of a given run: holds a state and the event that led to it. For the latter, the value EVENT_NB_EVENTS is used to store a "timeout" event.
	std::vector<StateChangeEvent> _history;

	void alloc( size_t nbStates, size_t nbEvents )
	{
		_stateCounter.resize( nbStates,   0 );
		_eventCounter.resize( nbEvents+1, 0 );   // last element is used for timeout events
	}
	void clear()
	{
		_history.clear();
		_stateCounter.clear();
		_eventCounter.clear();
	}
	/// Print dynamic data to \c str
	void printLoggedData( std::ostream& str ) const
	{
		str << "FSM data:\n"; // - Nb States=" << nbStates() << "\n - Nb events=" << nbEvents();
		str << " - State counters:\n";
		for( size_t i=0; i<_stateCounter.size(); i++ )
			str << i << ": " << _stateCounter[i] << '\n';
		str << '\n';
		str << " - Event counters:\n";
		for( size_t i=0; i<_eventCounter.size(); i++ )
			str << i << ": " << _eventCounter[i] << '\n';
		str << '\n';
		str << " - Run history:\n";
		for( size_t i=0; i<_history.size(); i++ )
			str << _history[i];
	}
#endif

#ifdef SPAG_ENABLE_LOGGING
	void switchState( STATE st, EVENT ev )
	{
		_current = st;
		assert( ev < EVENT::NB_EVENTS+1 );
		assert( st < STATE::NB_STATES );
		_eventCounter.at( ev )++;
		_stateCounter[st]++;
		_history.push_back( StateChangeEvent{ st, ev, std::chrono::high_resolution_clock::now() - _startTime } );
	}
#else
	void switchState( STATE st )
	{
		_current = st;
	}
#endif
};
//-----------------------------------------------------------------------------------
/// This namespace is just a security, so user code won't hit into this
namespace priv {

//-----------------------------------------------------------------------------------
/// helper template function
template<typename T>
void
resizemat( std::vector<std::vector<T>>& mat, std::size_t nb_lines, std::size_t nb_cols )
{
	mat.resize( nb_lines );
	for( auto& e: mat )
		e.resize( nb_cols );
}
} // local namespace end
//-----------------------------------------------------------------------------------
/// A class holding data for a FSM, without the event loop
/**
types:
 - STATE: an enum defining the different states.
 - EVENT: an enum defining the different external events.
 - TIM: a type handling the timer, must provide the following methods:
   - timerStart( const SpagFSM* );
   - timerCancel();
 - CBA: the callback function type (single) argument

Requirements: the two enums \b MUST have the following requirements:
 - the last element \b must be NB_STATES and NB_EVENTS, respectively
 - the first state must have value 0
*/
template<typename STATE, typename EVENT,typename TIM,typename CBA=DummyCbArg_t>
class SpagFSM
{
	typedef std::function<void(CBA)> Callback_t;

	public:
/// Constructor
		SpagFSM() // : _current( static_cast<STATE>(0) )
		{
			static_assert( EVENT::NB_EVENTS > 0, "Error, you need to provide at least one event" );
			static_assert( STATE::NB_STATES > 1, "Error, you need to provide at least two states" );
			priv::resizemat( _transition_mat, EVENT::NB_EVENTS, STATE::NB_STATES );
			priv::resizemat( _ignored_events, EVENT::NB_EVENTS, STATE::NB_STATES );

			_callback.resize( STATE::NB_STATES );    // no callbacks stored at init
			if( !std::is_same<DummyCbArg_t, CBA>::value )
				_callbackArg.resize( STATE::NB_STATES );

			_timeout.resize(  STATE::NB_STATES );    // timeouts info (see class TimerEvent)

			for( auto& e: _ignored_events )      // all external events will be ignored at init
				std::fill( e.begin(), e.end(), 0 );

#ifdef SPAG_ENABLE_LOGGING
			_data.alloc( STATE::NB_STATES, EVENT::NB_EVENTS );
#endif
		}

		void assignIgnEvMatrix( const std::vector<std::vector<int>>& mat )
		{
			SPAG_CHECK_EQUAL( mat.size(),    EVENT::NB_EVENTS );
			SPAG_CHECK_EQUAL( mat[0].size(), STATE::NB_STATES );
			_ignored_events = mat;
		}

		void assignTransitionMat( const std::vector<std::vector<STATE>>& mat )
		{
			SPAG_CHECK_EQUAL( mat.size(),    EVENT::NB_EVENTS );
			SPAG_CHECK_EQUAL( mat[0].size(), STATE::NB_STATES );
			_transition_mat = mat;
		}

#if 0
/// Use this to define for all states if timeout is to be considered or not.
/// For individual states, see UseTimeOut()
		void UseGlobalTimeOut( bool b )
		{
			for( auto& e: _timeout_active )
				e = static_cast<char>(b);
		}

/// Use this to define for a given state if timeout is to be considered or not.
/// To enable timeout for all the states, see UseGlobalTimeOut()
		void UseTimeOut( STATE st, bool b )
		{
//			assert( check_state( st ) );
			_timeout_active.at( st ) = static_cast<char>(b);
		}
#endif

/// Assigns an external transition event \c ev to switch from event \c st1 to event \c st2
		void assignExtTransition( STATE st1, EVENT ev, STATE st2 )
		{
			SPAG_CHECK_LESS( st1, nbStates() );
			SPAG_CHECK_LESS( st2, nbStates() );
			SPAG_CHECK_LESS( ev,  nbEvents() );
			_transition_mat.at( static_cast<int>( ev ) ).at( static_cast<int>( st1 ) ) = st2;
			_ignored_events.at( static_cast<int>( ev ) ).at( static_cast<int>( st1 ) ) = 1;
		}

/// Assigns an timeout transition event to switch from event \c st1 to event \c st2
		void assignTimeOut( STATE st1, int nb_sec, STATE st2 )
		{
			SPAG_CHECK_LESS( st1, nbStates() );
			SPAG_CHECK_LESS( st2, nbStates() );
			_timeout[ static_cast<int>( st1 ) ] = TimerEvent<STATE>( st2, nb_sec );
		}

/// Whatever state we are in, if the event \c ev occurs, we switch to state \c st
		void assignTransitionAlways( EVENT ev, STATE st )
		{
			SPAG_CHECK_LESS( st, nbStates() );
			SPAG_CHECK_LESS( ev, nbEvents() );
			for( auto& e: _transition_mat[ ev ] )
				e = st;
			for( auto& e: _ignored_events[ ev ] )
				e = 1;
		}
		void allowEvent( STATE st, EVENT ev )
		{
			SPAG_CHECK_LESS( st, nbStates() );
			SPAG_CHECK_LESS( ev, nbEvents() );
			_ignored_events[ ev ][ st ] = 1;
		}
/// Assigns a callback function to a state, will be called each time we arrive on this state
		void assignCallback( STATE st, Callback_t func, CBA cb_arg=CBA() )
		{
			SPAG_CHECK_LESS( st, nbStates() );
			_callback[ st ] = func;
			if( !std::is_same<CBA,DummyCbArg_t>::value )
				_callbackArg[ st ] = cb_arg;
		}

/// Assigns a callback function to all the states, will be called each time the state is activated
		void assignCallbackOnAll( Callback_t func )
		{
			for( size_t i=0; i<STATE::NB_STATES; i++ )
				_callback[ i ] = func;
		}

		void assignCallbackValue( STATE st, CBA cb_arg )
		{
			SPAG_CHECK_LESS( st, nbStates() );
			_callbackArg[ st ] = cb_arg;
		}

		void start() const
		{
			runAction();
		}

/// Your timer end function/callback should call this when the timer expires
		void processTimerEvent() const
		{
#ifdef SPAG_PRINT_STATES
			std::cout << "-processing timeout event, delay was " << _timeout.at( _data._current ).nbSec << " s.\n";
#endif
			assert( _timeout.at( _data._current ).enabled ); // or else, the timer shoudn't have been started, and thus we shouldn't be here...

#ifdef SPAG_ENABLE_LOGGING
			_data.switchState( _timeout.at( _data._current ).nextState, EVENT::NB_EVENTS );
#else
			_data.switchState( _timeout.at( _data._current ).nextState );
#endif

			runAction();
		}

/// Your callback should call this function when an external event occurs
		void processExtEvent( EVENT ev ) const
		{
			SPAG_CHECK_LESS( ev, nbEvents() );
#ifdef SPAG_PRINT_STATES
			std::cout << "-processing event " << ev << "\n";
#endif
			if( _ignored_events.at( ev ).at( _data._current ) != 0 )
			{
				if( _timeout.at( _data._current ).enabled )               // 1 - cancel the waiting timer, if any
					timer->timerCancel();

#ifdef SPAG_ENABLE_LOGGING
				_data.switchState( _transition_mat[ev].at( _data._current ), ev ); // 2 - switch to next state
#else
				_data.switchState( _transition_mat[ev].at( _data._current ) );
#endif

				runAction();                                        // 3 - call the callback function
			}
#ifdef SPAG_PRINT_STATES
			else
				std::cout << " (event ignored)\n";
#endif
		}
		size_t nbStates() const
		{
			assert( _transition_mat.size() );
			return _transition_mat[0].size();
		}
		size_t nbEvents() const
		{
			return _transition_mat.size();
		}
		STATE currentState() const
		{
			return _data._current;
		}

		TimerEvent<STATE> timeOutData( STATE st ) const
		{
			return _timeout.at(st);
		}

		void assignTimer( TIM* t )
		{
			timer = t;
		}

		void printConfig( std::ostream& str ) const;
#ifdef SPAG_ENABLE_LOGGING
/// Print dynamic data to \c str
		void printLoggedData( std::ostream& str ) const
		{
			_data.printLoggedData( str );
		}
#endif

/// Returns the build options
		std::string buildOptions() const
		{
			std::string out( "Spaghetti: build options:");
			out += "\n - SPAG_ENABLE_LOGGING: ";
#ifdef SPAG_ENABLE_LOGGING
			out += "yes";
#else
			out += "no";
#endif

			out += "\n - SPAG_PRINT_STATES: ";
#ifdef SPAG_PRINT_STATES
			out += "yes";
#else
			out += "no";
#endif

			out += "\n - SPAG_FRIENDLY_CHECKING: ";
#ifdef SPAG_FRIENDLY_CHECKING
			out += "yes";
#else
			out += "no";
#endif
			out += "\n";
			return out;
		}

	private:
		void runAction() const
		{
#ifdef SPAG_PRINT_STATES
			std::cout << "-switching to state " << _data._current << ", starting action\n";
#endif
			if( _timeout.at( _data._current ).enabled )
			{
				assert( timer );
#ifdef SPAG_PRINT_STATES
		std::cout << "  -timeout enabled, duration=" << _timeout.at( _data._current ).nbSec << "\n";
#endif
				timer->timerStart( this );
			}
			if( _callback.at( _data._current ) ) // if there is a callback stored, then call it
			{
#ifdef SPAG_PRINT_STATES
			std::cout << "  -callback function start:\n";
#endif
				if( std::is_same<CBA,DummyCbArg_t>::value )
					_callback.at( _data._current )( DummyCbArg_t() );
				else
					_callback.at( _data._current )( _callbackArg.at(_data._current) );
			}
#ifdef SPAG_PRINT_STATES
			else
				std::cout << "  -no callback provided\n";
#endif
		}

	private:
#ifdef SPAG_ENABLE_LOGGING
		mutable FsmData<STATE,EVENT>       _data;
#else
		mutable FsmData<STATE>             _data;
#endif
		std::vector<std::vector<STATE>>    _transition_mat;  ///< describe what states the fsm switches to, when a message is received. lines: events, columns: states, value: states to switch to. DOES NOT hold timer events
		std::vector<std::vector<char>>     _ignored_events;  ///< matrix holding for each event a boolean telling is the event is ignored or not, for a given state (0:ignore, 1; handle)
		std::vector<TimerEvent<STATE>>     _timeout;         ///< Holds for each state the information on timeout
		std::vector<Callback_t>            _callback;        ///< holds for each state the callback function to be called

/// If the user code provides a value for the callbacks, then we must store these, per state. If not, then this will remain an empty vector
		std::vector<CBA>                   _callbackArg;
		TIM* timer;
};
//-----------------------------------------------------------------------------------
namespace priv
{
	/// helper function template for printConfig()
	template<typename T>
	void
	printMatrix( std::ostream& str, const std::vector<std::vector<T>>& mat, bool ch )
	{
		assert( mat.size() );
		str << "       STATES:\n   ";
		for( size_t i=0; i<mat[0].size(); i++ )
			str << i << "  ";
		str << "\n--|";
		for( size_t i=0; i<mat[0].size(); i++ )
			str << "---";
		str << '\n';
		for( size_t i=0; i<mat.size(); i++ )
		{
			str << i << " | ";
			for( auto e: mat[i] )
				str << (ch?e:(e?'X':'.')) << "  ";
			str << '\n';
		}
	}
}
//-----------------------------------------------------------------------------------
/// Printing function
template<typename ST, typename EV,typename T,typename CBA>
void
SpagFSM<ST,EV,T,CBA>::printConfig( std::ostream& str ) const
{
	str << "FSM config:\n - Nb States=" << nbStates() << "\n - Nb events=" << nbEvents();

	str << "\n - Transition matrix:\n";
	priv::printMatrix( str, _transition_mat, true );

	str << "\n - Ignored events:\n";
	priv::printMatrix( str, _ignored_events, false );

	str << "\n - States with timeout\n";
	str << "       STATES:\n   ";
	for( size_t i=0; i<_timeout.size(); i++ )
		str << i << "  ";
	str << "\n--|";
	for( size_t i=0; i<_timeout.size(); i++ )
		str <<  (_timeout[i].enabled?'X':'.') << "  ";
	str << '\n';
}
//-----------------------------------------------------------------------------------
/// dummy struct, useful in case there is no need for a timer
template<typename ST, typename EV,typename CBA=DummyCbArg_t>
struct NoTimer
{
	void timerStart( const SpagFSM<ST,EV,NoTimer,CBA>* ) {}
	void timerCancel() {}
};

//-----------------------------------------------------------------------------------

} // namespace spag end

/// A macro simplifying the FSM instanciation
#define SPAG_DECLARE_FSM( fsm, st, ev ) spag::SpagFSM<st,ev,spag::NoTimer<st,ev>> fsm

/// A macro simplifying the FSM instanciation (version 2, if a timer is needed)
#define SPAG_DECLARE_FSM_T( fsm, st, ev, tim ) spag::SpagFSM<st,ev,tim<st,ev>> fsm


#endif // HG_SPAGHETTI_FSM_HPP

/**
\page p_manual Spaghetti manual


No event loop here, you must provide it.

Dependencies: ONLY standard headers (no boost here, although it could be required in client code)

\section sec_steps Main steps

Usage:
 -# define the states
\code
 enum States { st_Tired, st_WakingUp, st_Perfect, NB_STATES };
\endcode

 -# define the events
\code
 enum Events { ev_Breakfast, ev_Work, ev_, NB_EVENTS };
\endcode

 -# Provide a Timer class \c T, if needed (see below)

 -# instanciate the class:
 \code
 SpagFSM<States,Events,T> fsm;
\endcode

 -# configure the FSM,

 -# run the FSM:
 \code
	fsm.start();
 \endcode

 -# enter your waiting loop, that will call appropriate member function.


 \section sec_usage Usage


 \subsection ssec_configure Configuring the FSM


 \subsection ssec_running Running the FSM


 \subsection ss_tools Additional tools


Sample programs: see the list of
<a href="../src/html/files.html" target="_blank">sample programs</a>.





\subsection ssec_BuildSymbols Build Options

These symbols can change the behavior of the library and/or add additional capabilities, you can define them either by adding them in your makefile
(with GCC, its \c -DSPAG_SOME_SYMBOL ), or by hardcoding in your program, like this:

\code
#define SPAG_SOME_SYMBOL
#include "spaghetti.hpp"
\endcode

They all start with these 5 characters: \c SPAG_

The available options/symbols are:
- \c SPAG_PRINT_STATES : will print on stdout the steps, useful only for debugging your SpagFSM
- \c SPAG_ENABLE_LOGGING : will enable logging of dynamic data (see spag::SpagFSM::printLoggedData() )
- \c SPAG_FRIENDLY_CHECKING: A lot of checking is done to ensure no nasty bug will crash your program.
However, in case of incorrect usage of the library by your client code (say, invalid index value),
the default behavior is to spit a standard error message that can be difficult to understand.
So if you define this symbol at build time, instead of getting this:
\code
myfile: /usr/local/include/spaghetti.hpp:236: void spag::SpagFSM<STATE, EVENT, TIM>::assignTransitionMat(const std::vector<std::vector<T> >&) [with STATE = SERSTAT; EVENT = EN_EVENTS; TIM = AsioWrapper]: Assertion `mat.size() == EVENT::NB_EVENTS' failed.
Aborted
\endcode
you will get this:
\code
Spaghetti: runtime error in func: assignTransitionMat(), values are not equal:
 - mat.size() value=7
 - EVENT::NB_EVENTS value=8
Exiting...
\endcode
If this symbol is not defined, regular checking is done with the classical \c assert().
As usual, this checking can be removed by defining the symbol \c NDEBUG.
<br>
- \c SPAG_PROVIDE_CALLBACK_TYPE :

<br>
DEPRECATED!!!
<br>

the default callback function signature is <code> void f()</code>.
With this symbol defined, user code can use callbacks having an argument.
The type of the argument has to be defined before including the library.
For example, to have an \c int argument, use this:
\code
#define SPAG_PROVIDE_CALLBACK_TYPE
typedef int CallbackArg_t;
#include "spaghetti.hpp"
\endcode
The value to be used when the callback function is called is stored (copied by value) inside the FSM,
and is given during configuration time:
\code
fsm.assignCallback( st_State1, cb_myCallback, 42 );
\endcode
See also example file \c test_buttons.cpp
<br>
If several arguments are needed, then the user code needs to pack them into a \c struct,
or use \c std::pair or \c std::tuple.
And of course it needs to be copyable.

\section sec_misc Misc.

\subsection sec_related Related software

 - Boost MSM: http://www.boost.org/doc/libs/release/libs/msm/
 - Boost Statechart: http://www.boost.org/doc/libs/release/libs/statechart/
 - tinyFSM: https://github.com/digint/tinyfsm



\section sec_devinfo Developper information

\subsection_codingConventions Coding style

Most of it is pretty obvious by parsing the code, but here are some additional points:

- TABS for indentation, SPACE for spacing
- Identifiers
 - \c camelCaseIsUsed for functions, variables
 - class/struct member data is prepended with '_' ( \c _thisIsADataMember )
 - Types are \c CamelCase (UpperCase first letter). Example: \c ThisIsAType


\todo find a way to ease up the usage for no timer (dummy timer struct)

\todo add some way to define "passage states", that is states that have some callback but on which we just pass to another state without any condition (i.e. right away)

\todo add an option so that in case we transition from one state to the same state, should the callback be called each time, or not ?

\todo write tutorial

\todo add serialisation capability

\todo add graphviz dot output

*/
