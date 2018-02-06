/**
\file spaghetti.hpp
\brief single header file of Spaghetti FSM library, see home page for full details:
https://github.com/skramm/spaghetti
*/

#ifndef HG_SPAGHETTI_FSM_HPP
#define HG_SPAGHETTI_FSM_HPP

#define SPAG_VERSION 0.1

#include <vector>
#include <algorithm>
#include <functional>
#include <cassert>
#include <fstream>

#ifdef SPAG_GENERATE_DOT
	#include <boost/graph/adjacency_list.hpp>
	#include <boost/graph/graphviz.hpp>
#endif

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

#define SPAG_STRINGIZE2( a ) #a
#define SPAG_STRINGIZE( a ) SPAG_STRINGIZE2( a )

// TEMP
typedef int Duration;

/// Main library namespace
namespace spag {


/// This is just to provide a dummy type for the callback argument, as \c void is not a valid type
struct DummyCbArg_t {};

//-----------------------------------------------------------------------------------
namespace priv {

/// Container holding information on timeout events. Each state will have one, event if it does not use it
template<typename ST>
struct TimerEvent
{
	ST        nextState = static_cast<ST>(0); ///< state to switch to
	Duration  duration  = 1;                     ///< duration
	bool      enabled   = 0;                     ///< this state uses or not a timeout (default is no)

	TimerEvent()
		: nextState(static_cast<ST>(0))
		, duration(0)
		, enabled(false)
	{
	}
	TimerEvent( ST st, Duration dur ): nextState(st), duration(dur)
	{
		enabled = true;
	}
};
} // local namespace end

//-----------------------------------------------------------------------------------
/// This namespace is just a security, so user code won't hit into this
namespace priv {

/// Holds the FSM dynamic data: current state, and logged data (if enabled at build, see symbol \c SPAG_ENABLE_LOGGING at \ref BuildOption )
#ifdef SPAG_ENABLE_LOGGING
template<typename ST,typename EV>
#else
template<typename ST>
#endif
struct RunTimeData
{
	ST _current = static_cast<ST>(0);

#ifdef SPAG_ENABLE_LOGGING
	RunTimeData()
	{
		_startTime = std::chrono::high_resolution_clock::now();
	}
	std::chrono::time_point<std::chrono::high_resolution_clock> _startTime;

/// a state-change event, used for logging, see _history
	struct StateChangeEvent
	{
		ST state;
		EV event;
		std::chrono::duration<double> elapsed;

		friend std::ostream& operator << ( std::ostream& s, const StateChangeEvent& sce )
		{
			char sep(';');
			s << sce.elapsed.count() << sep << sce.event << sep;
//#ifdef SPAG_ENUM_STRINGS
//			s << ?
//#endif
			s << sce.state << '\n';
			return s;
		}
	};

	std::vector<size_t>  _stateCounter;    ///< per state counter
	std::vector<size_t>  _eventCounter;    ///< per event counter
/// Dynamic history of a given run: holds a state and the event that led to it. For the latter, the value EV_NB_EVENTS is used to store a "timeout" event.
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
	void printData( std::ostream& str ) const
	{
		str << " - State counters:\n";
		for( size_t i=0; i<_stateCounter.size(); i++ )
			str << i << ": " << _stateCounter[i] << '\n';
		str << '\n';
		str << " - Event counters:\n";
		for( size_t i=0; i<_eventCounter.size(); i++ )
			str << i << ": " << _eventCounter[i] << '\n';
		str << '\n';
		str << " - Run history:\n#time;event;";
/// \todo find a way to print the enum string in history (require a pointer on parent struct. But lots of templating involved...)
//#ifdef SPAG_ENUM_STRINGS
//		str << "event string;";
//#endif
		str << "state\n";
		for( size_t i=0; i<_history.size(); i++ )
			str << _history[i];
	}
	void switchState( ST st, EV ev )
	{
		_current = st;
		assert( ev < EV::NB_EVENTS+1 );
		assert( st < ST::NB_STATES );
		_eventCounter.at( ev )++;
		_stateCounter[st]++;
		_history.push_back( StateChangeEvent{ st, ev, std::chrono::high_resolution_clock::now() - _startTime } );
	}
#else
	void switchState( ST st )
	{
		_current = st;
	}
#endif
};
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
 - ST: an enum defining the different states.
 - EV: an enum defining the different external events.
 - TIM: a type handling the timer, must provide the following methods:
   - timerStart( const SpagFSM* );
   - timerCancel();
 - CBA: the callback function type (single) argument

Requirements: the two enums \b MUST have the following requirements:
 - the last element \b must be NB_STATES and NB_EVENTS, respectively
 - the first state must have value 0
*/
template<typename ST, typename EV,typename TIM,typename CBA=DummyCbArg_t>
class SpagFSM
{
	typedef std::function<void(CBA)> Callback_t;

	public:
/// Constructor
		SpagFSM()
		{
			static_assert( EV::NB_EVENTS > 0, "Error, you need to provide at least one event" );
			static_assert( ST::NB_STATES > 1, "Error, you need to provide at least two states" );
			priv::resizemat( _transition_mat, EV::NB_EVENTS, ST::NB_STATES );
			priv::resizemat( _ignored_events, EV::NB_EVENTS, ST::NB_STATES );

			_callback.resize( ST::NB_STATES );    // no callbacks stored at init
			if( !std::is_same<DummyCbArg_t, CBA>::value )
				_callbackArg.resize( ST::NB_STATES );

			_timeout.resize( ST::NB_STATES );    // timeouts info (see class TimerEvent)

			for( auto& e: _ignored_events )      // all external events will be ignored at init
				std::fill( e.begin(), e.end(), 0 );

#ifdef SPAG_ENABLE_LOGGING
			_rtdata.alloc( ST::NB_STATES, EV::NB_EVENTS );
#endif
#ifdef SPAG_ENUM_STRINGS
			_str_events.resize( EV::NB_EVENTS );
#endif
		}

/** \name Configuration of FSM */
///@{
/// Assigned ignored event matrix
		void assignIgnEvMatrix( const std::vector<std::vector<int>>& mat )
		{
			SPAG_CHECK_EQUAL( mat.size(),    EV::NB_EVENTS );
			SPAG_CHECK_EQUAL( mat[0].size(), ST::NB_STATES );
			_ignored_events = mat;
		}

		void assignTransitionMat( const std::vector<std::vector<ST>>& mat )
		{
			SPAG_CHECK_EQUAL( mat.size(),    EV::NB_EVENTS );
			SPAG_CHECK_EQUAL( mat[0].size(), ST::NB_STATES );
			_transition_mat = mat;
		}

/// Assigns an external transition event \c ev to switch from event \c st1 to event \c st2
		void assignTransition( ST st1, EV ev, ST st2 )
		{
			SPAG_CHECK_LESS( st1, nbStates() );
			SPAG_CHECK_LESS( st2, nbStates() );
			SPAG_CHECK_LESS( ev,  nbEvents() );
			_transition_mat.at( static_cast<int>( ev ) ).at( static_cast<int>( st1 ) ) = st2;
			_ignored_events.at( static_cast<int>( ev ) ).at( static_cast<int>( st1 ) ) = 1;
		}

/// Assigns an timeout event on \b all states except \c st_final:
/**
After this, on all the states except \c st_final, if \c duration expires, the FSM will switch to \c st_final
(where there may or may not be a timeout assigned)
*/
		void assignGlobalTimeOut( ST st_final, Duration dur )
		{
			SPAG_CHECK_LESS( st_final, nbStates() );
			for( size_t i=0; i<nbStates(); i++ )
				if( i != static_cast<size_t>(st_final) )
					_timeout[ static_cast<size_t>( st_final ) ] = priv::TimerEvent<ST>( st_final, dur );
		}

/// Assigns an timeout event on state \c st_curr, will switch to event \c st_next
		void assignTimeOut( ST st_curr, Duration dur, ST st_next )
		{
			SPAG_CHECK_LESS( st_curr, nbStates() );
			SPAG_CHECK_LESS( st_next, nbStates() );
			_timeout[ static_cast<int>( st_curr ) ] = priv::TimerEvent<ST>( st_next, dur );
		}

/// Whatever state we are in, if the event \c ev occurs, we switch to state \c st
		void assignTransitionAlways( EV ev, ST st )
		{
			SPAG_CHECK_LESS( st, nbStates() );
			SPAG_CHECK_LESS( ev, nbEvents() );
			for( auto& e: _transition_mat[ ev ] )
				e = st;
			for( auto& e: _ignored_events[ ev ] )
				e = 1;
		}
		void allowEvent( ST st, EV ev )
		{
			SPAG_CHECK_LESS( st, nbStates() );
			SPAG_CHECK_LESS( ev, nbEvents() );
			_ignored_events[ ev ][ st ] = 1;
		}
/// Assigns a callback function to a state, will be called each time we arrive on this state
		void assignCallback( ST st, Callback_t func, CBA cb_arg=CBA() )
		{
			SPAG_CHECK_LESS( st, nbStates() );
			_callback[ st ] = func;
			if( !std::is_same<CBA,DummyCbArg_t>::value )
				_callbackArg[ st ] = cb_arg;
		}

/// Assigns a callback function to all the states, will be called each time the state is activated
		void assignGlobalCallback( Callback_t func )
		{
			for( size_t i=0; i<ST::NB_STATES; i++ )
				_callback[ i ] = func;
		}

		void assignCallbackValue( ST st, CBA cb_arg )
		{
			SPAG_CHECK_LESS( st, nbStates() );
			_callbackArg[ st ] = cb_arg;
		}

		void assignTimer( TIM* t )
		{
			timer = t;
		}

#ifdef SPAG_ENUM_STRINGS
/// Assign a string to an enum value (available only if option SPAG_ENUM_STRINGS is enabled)
		void assignString2Event( EV ev, std::string str )
		{
			SPAG_CHECK_LESS( ev, nbEvents() );
			_str_events[ev] = str;
		}
/// Assign strings to enum values (available only if option SPAG_ENUM_STRINGS is enabled)
		void assignStrings2Events( const std::vector<std::pair<EV,std::string>>& v_str )
		{
			SPAG_CHECK_EQUAL( v_str.size(), EV::NB_EVENTS );
			for( const auto& p: v_str )
				assignString2Event( p.first, p.second );
		}
#else
		void assignString2Event( EV, std::string ) {}
		void assignStrings2Events( const std::vector<std::pair<EV,std::string>>& ) {}
#endif
///@}

/** \name Run time functions */
///@{
		void start() const
		{
			runAction();
		}

/// Your timer end function/callback should call this when the timer expires
		void processTimeOut() const
		{
#ifdef SPAG_PRINT_STATES
			std::cout << "-processing timeout event, delay was " << _timeout.at( _rtdata._current ).duration << "\n";
#endif
			assert( _timeout.at( _rtdata._current ).enabled ); // or else, the timer shoudn't have been started, and thus we shouldn't be here...

#ifdef SPAG_ENABLE_LOGGING
			_rtdata.switchState( _timeout.at( _rtdata._current ).nextState, EV::NB_EVENTS );
#else
			_rtdata.switchState( _timeout.at( _rtdata._current ).nextState );
#endif

			runAction();
		}

/// Your callback should call this function when an external event occurs
		void processEvent( EV ev ) const
		{
			SPAG_CHECK_LESS( ev, nbEvents() );
#ifdef SPAG_PRINT_STATES
			std::cout << "-processing event " << ev << "\n";
#endif
			if( _ignored_events.at( ev ).at( _rtdata._current ) != 0 )
			{
				if( _timeout.at( _rtdata._current ).enabled )               // 1 - cancel the waiting timer, if any
					timer->timerCancel();

#ifdef SPAG_ENABLE_LOGGING
				_rtdata.switchState( _transition_mat[ev].at( _rtdata._current ), ev ); // 2 - switch to next state
#else
				_rtdata.switchState( _transition_mat[ev].at( _rtdata._current ) );
#endif

				runAction();                                        // 3 - call the callback function
			}
#ifdef SPAG_PRINT_STATES
			else
				std::cout << " (event ignored)\n";
#endif
		}
///@}

/** \name Misc. helper functions */
///@{

		size_t nbStates() const
		{
			assert( _transition_mat.size() );
			return _transition_mat[0].size();
		}
		size_t nbEvents() const
		{
			return _transition_mat.size();
		}
		ST currentState() const
		{
			return _rtdata._current;
		}

		Duration timeOutDuration( ST st ) const
		{
			return _timeout.at(st).duration;
		}

		void printConfig( std::ostream& str, const char* msg=nullptr ) const;
#ifdef SPAG_ENABLE_LOGGING
/// Print dynamic data to \c str
		void printLoggedData( std::ostream& str ) const
		{
			_rtdata.printData( str );
		}
#else
		void printLoggedData( std::ostream& ) const
		{
//			#warning "printLoggedData(): disabled function (SPAG_ENABLE_LOGGING undefined)"
		}
#endif

/// Returns the build options
		std::string buildOptions() const
		{
			std::string out( "Spaghetti: version " );
			out += SPAG_STRINGIZE( SPAG_VERSION );
			out += "\nBuild options:";

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

			out += "\n - SPAG_ENUM_STRINGS: ";
#ifdef SPAG_ENUM_STRINGS
			out += "yes";
#else
			out += "no";
#endif
			out += "\n";
			return out;
		}

#ifdef SPAG_GENERATE_DOT
/// Generates in current folder a dot file corresponding to the FSM (EXPERIMENTAL)
		void writeDotFile( std::string fn ) const;
#endif
///@}
///////////////////////////////////
// private member function section
///////////////////////////////////

	private:
		void runAction() const
		{
#ifdef SPAG_PRINT_STATES
			std::cout << "-switching to state " << _rtdata._current << ", starting action\n";
#endif
			if( _timeout.at( _rtdata._current ).enabled )
			{
				assert( timer );
#ifdef SPAG_PRINT_STATES
		std::cout << "  -timeout enabled, duration=" << _timeout.at( _rtdata._current ).duration << "\n";
#endif
				timer->timerStart( this );
			}
			if( _callback.at( _rtdata._current ) ) // if there is a callback stored, then call it
			{
#ifdef SPAG_PRINT_STATES
			std::cout << "  -callback function start:\n";
#endif
				if( std::is_same<CBA,DummyCbArg_t>::value )
					_callback.at( _rtdata._current )( CBA() );
				else
					_callback.at( _rtdata._current )( _callbackArg.at(_rtdata._current) );
			}
#ifdef SPAG_PRINT_STATES
			else
				std::cout << "  -no callback provided\n";
#endif
		}
		void printMatrix( std::ostream& str ) const;

/////////////////////////////
// private data section
/////////////////////////////

	private:
#ifdef SPAG_ENABLE_LOGGING
		mutable priv::RunTimeData<ST,EV>       _rtdata;
#else
		mutable priv::RunTimeData<ST>             _rtdata;
#endif
		std::vector<std::vector<ST>>    _transition_mat;  ///< describe what states the fsm switches to, when a message is received. lines: events, columns: states, value: states to switch to. DOES NOT hold timer events
		std::vector<std::vector<char>>     _ignored_events;  ///< matrix holding for each event a boolean telling is the event is ignored or not, for a given state (0:ignore event, 1:handle event)
		std::vector<priv::TimerEvent<ST>>     _timeout;         ///< Holds for each state the information on timeout
		std::vector<Callback_t>            _callback;        ///< holds for each state the callback function to be called
#ifdef SPAG_ENUM_STRINGS
		std::vector<std::string>           _str_events;     ///< holds events strings
#endif
/// If the user code provides a value for the callbacks, then we must store these, per state. If not, then this will remain an empty vector
		std::vector<CBA>                   _callbackArg;
		TIM* timer;
};
//-----------------------------------------------------------------------------------
namespace priv
{
//-----------------------------------------------------------------------------------
void
PrintEnumString( std::ostream& out, std::string str, size_t maxlength )
{
	assert( str.size() <= maxlength );
	out << str;
	for( size_t i=0; i<maxlength-str.size(); i++ )
		out << ' ';
}
//-----------------------------------------------------------------------------------
void printChars( std::ostream& out, size_t n, char c )
{
	for( size_t i=0; i<n; i++ )
		out << c;
}
//-----------------------------------------------------------------------------------
} // namespace priv end

//-----------------------------------------------------------------------------------
/// helper function template for printConfig()
template<typename ST, typename EV,typename T,typename CBA>
void
SpagFSM<ST,EV,T,CBA>::printMatrix( std::ostream& out ) const
{
	assert( _transition_mat.size() );
	size_t maxlength(0);
#ifdef SPAG_ENUM_STRINGS
	if( _str_events.size() > 1 )
	{
		auto itmax = std::max_element(
			_str_events.begin(),
			_str_events.end(),
			[]( const std::string& s1, const std::string& s2 ){ return s1.size()<s2.size(); } // lambda
		);
		maxlength = itmax->size();
	}
#endif

	std::string capt( "EVENTS" );
	priv::printChars( out, maxlength, ' ' );
	out << "       STATES:\n      ";
	priv::printChars( out, maxlength, ' ' );
	for( size_t i=0; i<_transition_mat[0].size(); i++ )
		out << i << "  ";
	out << "\n----";
	priv::printChars( out, maxlength, '-' );
	out << '|';
	for( size_t i=0; i<_transition_mat[0].size(); i++ )
		out << "---";
	out << '\n';

#ifdef SPAG_ENUM_STRINGS
	for( size_t i=0; i<_transition_mat.size(); i++ )
	{
		if( maxlength )
			priv::PrintEnumString( out, _str_events[i], maxlength );
#else
	for( size_t i=0; i<std::max(capt.size(),_transition_mat.size()); i++ )
	{
		if( i<capt.size() )
			out << capt[i];
		else
#endif
			out << ' ';

		if( i<_transition_mat.size() )
		{
			out << ' ' << i << " | ";
			for( size_t j=0; j<_transition_mat[i].size(); j++ )
			{
				if( _ignored_events[i][j] )
					out << _transition_mat[i][j];
				else
					out << 'X';
				out << "  ";
			}
		}
		out << '\n';
	}
}
//-----------------------------------------------------------------------------------
/// Printing function
template<typename ST, typename EV,typename T,typename CBA>
void
SpagFSM<ST,EV,T,CBA>::printConfig( std::ostream& out, const char* msg  ) const
{
	out << "FSM config:";
	if( msg )
		out << "msg=" << msg;
	out << "\n - Nb States=" << nbStates() << "\n - Nb external events=" << nbEvents();

	out << "\n - Transition matrix: (X:ignored event)\n";
	printMatrix( out );

	out << "\n - States with timeout (.:no timeout, o: timeout enabled)\n";
	out << "       STATES:\n   ";
	for( size_t i=0; i<_timeout.size(); i++ )
		out << i << "  ";
	out << "\n   ";
	for( size_t i=0; i<_timeout.size(); i++ )
		out <<  (_timeout[i].enabled?'o':'.') << "  ";
	out << '\n';
}
//-----------------------------------------------------------------------------------
#ifdef SPAG_GENERATE_DOT
/// WIP, still experimental
template<typename ST, typename EV,typename T,typename CBA>
void
SpagFSM<ST,EV,T,CBA>::writeDotFile( std::string fname ) const
{
// 1 - build graph
	typedef boost::adjacency_list<
		boost::vecS,
		boost::vecS,
		boost::directedS
		> graph_t;

	typedef boost::graph_traits<graph_t>::vertex_descriptor vertex_t;
//	typedef boost::graph_traits<graph_t>::edge_descriptor   edge_t;

	graph_t g;

	for( size_t i=0; i<ST::NB_STATES; i++ )
		boost::add_vertex(g);

	for( size_t i=0; i<nbEvents(); i++ )
		for( size_t j=0; j<nbStates(); j++ )
			if( _ignored_events[i][j] )
			{
				vertex_t v1 = j;
				vertex_t v2 = _transition_mat[i][j];
				boost::add_edge( v1, v2, g );
			}

// 2 - print
	std::ofstream f ( fname );
	if( !f.is_open() )
		throw std::string( "unable to open file: " + fname );
	boost::write_graphviz( f, g );
}
#endif // SPAG_GENERATE_DOT

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

// A macro simplifying the FSM instanciation
//#define SPAG_DECLARE_FSM( fsm, st, ev ) spag::SpagFSM<st,ev,spag::NoTimer<st,ev>> fsm

// A macro simplifying the FSM instanciation (version 2, if a timer is needed)
//#define SPAG_DECLARE_FSM_T( fsm, st, ev, tim ) spag::SpagFSM<st,ev,tim<st,ev>> fsm

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
- \c SPAG_SPAG_ENUM_STRINGS : this enables the usage of enum-string mapping, for events only.
You can provide a string either individually with
\code
	fsm.assignString2Event( std::make_pair(EV_MyEvent, "something happened" );
\endcode
or globally, by providing a vector of pairs(enum values, string). For example:
\code
	std::vector<std::pair<EVENT,std::string>> v_str = {
		{ EV_RESET,       "Reset" },
		{ EV_WARNING_ON,  "Warning On" },
		{ EV_WARNING_OFF, "Warning Off" }
	};
	fsm.assignStrings2Events( v_str );
\endcode
These strings will then be printed out when calling the printConfig() member function.

\section sec_callback CALLBACK (TEMP WILL BE MOVED)

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

\subsection sec_related Possibly related software

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
 - To avoid name collisions, all the symbols defined here start with "SPAG_"

\todo find a way to ease up the usage for no timer (dummy timer struct)

\todo add some way to define "passage states", that is states that have some callback but on which we just pass to another state without any condition (i.e. right away)

\todo add an option so that in case we transition from one state to the same state, should the callback be called each time, or not ?

\todo write tutorial

\todo add serialisation capability

\todo for enum to string automatic conversion, maybe use this ? :
https://github.com/aantron/better-enums

*/
