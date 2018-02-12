/**
\file spaghetti.hpp
\brief single header file of Spaghetti FSM C++ library, see home page for full details:
https://github.com/skramm/spaghetti

Copyright 2018 Sebastien Kramm

Licence: GPLv3

This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef HG_SPAGHETTI_FSM_HPP
#define HG_SPAGHETTI_FSM_HPP

#define SPAG_VERSION 0.1

#include <vector>
#include <algorithm>
#include <functional>
#include <cassert>
#include <fstream>
#include <iostream> // needed for expansion of SPAG_LOG

#ifdef SPAG_GENERATE_DOT
	#include <boost/graph/adjacency_list.hpp>
	#include <boost/graph/graphviz.hpp>
#endif

#ifdef SPAG_ENABLE_LOGGING
	#include <chrono>
#endif

#ifdef SPAG_PRINT_STATES
	#define SPAG_LOG \
		if(1) \
			std::cout << "Spaghetti: " << __FUNCTION__ << "(): "
#else
	#define SPAG_LOG \
		if(0) \
			std::cout
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
typedef size_t Duration;

/// Main library namespace
namespace spag {


/// This is just to provide a dummy type for the callback argument, as \c void is not a valid type
struct DummyCbArg_t {};

//-----------------------------------------------------------------------------------
/// private namespace, so user code won't hit into this
namespace priv {

//-----------------------------------------------------------------------------------
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
/// Helper function, returns max length of string in vector
size_t
getMaxLength( const std::vector<std::string>& v_str )
{
	size_t maxlength(0);
	if( v_str.size() > 1 )
	{
		auto itmax = std::max_element(
			v_str.begin(),
			v_str.end(),
			[]( const std::string& s1, const std::string& s2 ){ return s1.size()<s2.size(); } // lambda
		);
		maxlength = itmax->size();
	}
	return maxlength;
}
//-----------------------------------------------------------------------------------
/// Holds the FSM dynamic data: current state, and logged data (if enabled at build, see symbol \c SPAG_ENABLE_LOGGING at \ref ssec_BuildSymbols )
#ifdef SPAG_ENABLE_LOGGING
template<typename ST,typename EV>
struct RunTimeData
{
	public:
#ifdef SPAG_ENUM_STRINGS
		RunTimeData( const std::vector<std::string>& str_events, const std::vector<std::string>& str_states )
			: _str_events(str_events), _str_states( str_states )
#else
		RunTimeData()
#endif
		{
			_startTime = std::chrono::high_resolution_clock::now();
		}

/// a state-change event, used for logging, see _history
	struct StateChangeEvent
	{
		ST state;
		size_t event;
		std::chrono::duration<double> elapsed;

// deprecated, replaced by printData()
#if 0
		friend std::ostream& operator << ( std::ostream& s, const StateChangeEvent& sce )
		{
			char sep(';');
			s << sce.elapsed.count() << sep << sce.event << sep;
			s << sce.state << '\n';
			return s;
		}
#endif
	};

	void alloc( size_t nbStates, size_t nbEvents )
	{
		_stateCounter.resize( nbStates,   0 );
		_eventCounter.resize( nbEvents+2, 0 );   // two last elements are used for timeout events and for "no event" transitions ("pass states")
	}
	void incrementInitState()
	{
		assert( _stateCounter.size() );
		_stateCounter[0] = 1;
	}
	void clear()
	{
		_history.clear();
		_stateCounter.clear();
		_eventCounter.clear();
	}
	/// Print dynamic data (runtime data) to \c out
	void printData( std::ostream& out ) const
	{
		char sep(';');
#ifdef SPAG_ENUM_STRINGS
		size_t maxlength = priv::getMaxLength( _str_events );
#endif
		out << " - State counters:\n";
		for( size_t i=0; i<_stateCounter.size(); i++ )
			out << i << ": " << _stateCounter[i] << '\n';
		out << '\n';
		out << " - Event counters:\n";
		for( size_t i=0; i<_eventCounter.size(); i++ )
		{
			out << i << sep;
#ifdef SPAG_ENUM_STRINGS
			priv::PrintEnumString( out, _str_events[i], maxlength );
			out << sep;
#endif
			out << _eventCounter[i] << '\n';
		}
		out << '\n';
		out << " - Run history:\n#time" << sep << "event" << sep
#ifdef SPAG_ENUM_STRINGS
			<< "event_string" << sep
#endif
			<< "state\n";

		for( size_t i=0; i<_history.size(); i++ )
		{
			size_t ev =_history[i].event;
			out << _history[i].elapsed.count() << sep << ev << sep;
#ifdef SPAG_ENUM_STRINGS
			priv::PrintEnumString( out, _str_events[ev], maxlength );
#endif
			out << _history[i].state << '\n';
		}
	}
	void logTransition( ST st, size_t ev )
	{
		assert( ev < EV::NB_EVENTS+2 );
		assert( st < ST::NB_STATES );
		_eventCounter[ ev ]++;
		_stateCounter[ st ]++;
		_history.push_back( StateChangeEvent{ st, ev, std::chrono::high_resolution_clock::now() - _startTime } );
	}

	private:
		std::vector<size_t>  _stateCounter;    ///< per state counter
		std::vector<size_t>  _eventCounter;    ///< per event counter
/// Dynamic history of a given run: holds a state and the event that led to it. For the latter, the value EV_NB_EVENTS is used to store a "timeout" event.
		std::vector<StateChangeEvent> _history;
		std::chrono::time_point<std::chrono::high_resolution_clock> _startTime;

#ifdef SPAG_ENUM_STRINGS
		const std::vector<std::string>& _str_events; ///< reference on strings of events
		const std::vector<std::string>& _str_states; ///< reference on strings of states
#endif
};
#endif
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

} // priv namespace end
//-----------------------------------------------------------------------------------
/// A class holding data for a FSM, without the event loop
/**
types:
 - ST: an enum defining the different states.
 - EV: an enum defining the different external events.
 - TIM: a type handling the timer, must provide the following methods:
   - timerInit();
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

#if (defined SPAG_ENABLE_LOGGING) && (defined SPAG_ENUM_STRINGS)
		SpagFSM() : _rtdata( _str_events, _str_states )
#else
		SpagFSM()
#endif
		{
			static_assert( EV::NB_EVENTS > 0, "Error, you need to provide at least one event" );
			static_assert( ST::NB_STATES > 1, "Error, you need to provide at least two states" );
			priv::resizemat( _transition_mat, EV::NB_EVENTS, ST::NB_STATES );
			priv::resizemat( _ignored_events, EV::NB_EVENTS, ST::NB_STATES );

			_callback.resize( ST::NB_STATES );    // no callbacks stored at init
			if( !std::is_same<DummyCbArg_t, CBA>::value )
				_callbackArg.resize( ST::NB_STATES );

			_timeout.resize( ST::NB_STATES );    // timeouts info (see class TimerEvent)
			_isPassState.resize( ST::NB_STATES );

			for( auto& e: _ignored_events )      // all external events will be ignored at init
				std::fill( e.begin(), e.end(), 0 );

#ifdef SPAG_ENABLE_LOGGING
			_rtdata.alloc( ST::NB_STATES, EV::NB_EVENTS );
#endif
#ifdef SPAG_ENUM_STRINGS
			_str_events.resize( EV::NB_EVENTS+2 );
			_str_states.resize( ST::NB_STATES );
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
			_transition_mat[ static_cast<int>( ev ) ][ static_cast<int>( st1 ) ] = st2;
			_ignored_events[ static_cast<int>( ev ) ][ static_cast<int>( st1 ) ] = 1;
		}

/// Assigns a transition to a "pass state": once on state \c st1, the FSM will swith right away to \c st2
		void assignTransition( ST st1, ST st2 )
		{
			SPAG_CHECK_LESS( st1, nbStates() );
			SPAG_CHECK_LESS( st2, nbStates() );
			for( auto& line: _transition_mat )
				line[static_cast<int>( st1 )] = st2;
			for( auto& line: _ignored_events )
				line[static_cast<int>( st1 )] = 1;
			_isPassState[st1] = 1;
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
			p_timer = t;
		}

#ifdef SPAG_ENUM_STRINGS
/// Assign a string to an enum event value (available only if option SPAG_ENUM_STRINGS is enabled)
		void assignString2Event( EV ev, std::string str )
		{
			SPAG_CHECK_LESS( ev, nbEvents() );
			_str_events[ev] = str;
		}
/// Assign a string to an enum state value (available only if option SPAG_ENUM_STRINGS is enabled)
		void assignString2State( ST st, std::string str )
		{
			SPAG_CHECK_LESS( st, nbStates() );
			_str_states[st] = str;
		}
/// Assign strings to enum event values (available only if option SPAG_ENUM_STRINGS is enabled)
		void assignStrings2Events( const std::vector<std::pair<EV,std::string>>& v_str )
		{
			SPAG_CHECK_EQUAL( v_str.size(), EV::NB_EVENTS );
			for( const auto& p: v_str )
				assignString2Event( p.first, p.second );
			_str_events[EV::NB_EVENTS]   = "*Timeout*";
			_str_events[EV::NB_EVENTS+1] = "*  AA   *"; // Always Active Transition
		}
/// Assign strings to enum state values (available only if option SPAG_ENUM_STRINGS is enabled)
		void assignStrings2States( const std::vector<std::pair<ST,std::string>>& v_str )
		{
			SPAG_CHECK_EQUAL( v_str.size(), ST::NB_STATES );
			for( const auto& p: v_str )
				assignString2State( p.first, p.second );
		}
#else
		void assignString2Event( EV, std::string ) {}
		void assignString2State( ST, std::string ) {}
		void assignStrings2Events( const std::vector<std::pair<ST,std::string>>& ) {}
		void assignStrings2States( const std::vector<std::pair<ST,std::string>>& ) {}
#endif

///@}

/** \name Run time functions */
///@{
/// start FSM : run callback associated to initial state (if any), an run timer (if any)
		void start() const
		{
			static bool bFirst = true;
			if(bFirst)
				bFirst = false;
			else
			{
				std::cerr << "Spaghetti: error, attempt to start the FSM twice!\n";
				std::exit(1);
			}
			runAction();
#ifdef SPAG_ENABLE_LOGGING
			_rtdata.incrementInitState();
#endif
			if( p_timer )                 // if timing is involved,
				p_timer->timerInit();     // then it must be started
		}

/// stop FSM : needed only if timer is used, this will cancel (and kill) the pending timer
		void stop() const
		{
			if( p_timer )
			{
				SPAG_LOG << "call timerCancel()\n";
				p_timer->timerCancel();
				SPAG_LOG << "call timerKill()\n";
				p_timer->timerKill();  /// \todo WHE SHOULDN'T HAVE TO USE THIS !!!
			}
		}

/// Your timer end function/callback should call this when the timer expires
		void processTimeOut() const
		{
			SPAG_LOG << "processing timeout event, delay was " << _timeout.at( _current ).duration << "\n";
			assert( _timeout.at( _current ).enabled ); // or else, the timer shoudn't have been started, and thus we shouldn't be here...

			_current = _timeout[ _current ].nextState;

#ifdef SPAG_ENABLE_LOGGING
			_rtdata.logTransition( _current, EV::NB_EVENTS );
#endif
			runAction();
		}

/// Your callback should call this function when an external event occurs
		void processEvent( EV ev ) const
		{
			SPAG_CHECK_LESS( ev, nbEvents() );

#ifdef SPAG_ENUM_STRINGS
			SPAG_LOG << "processing event " << ev << ": \"" << _str_events[ev] << "\"\n";
#else
			SPAG_LOG << "processing event " << ev << '\n';
#endif
			if( _ignored_events.at( ev ).at( _current ) != 0 )
			{
				SPAG_LOG << "current state=" << _current << '\n';
				if( _timeout.at( _current ).enabled )               // 1 - cancel the waiting timer, if any
					p_timer->timerCancel();

				_current = _transition_mat[ev].at( _current );  // 2 - switch to next state

#ifdef SPAG_ENABLE_LOGGING
				_rtdata.logTransition( _current, ev );
#endif
				runAction();                                        // 3 - call the callback function
			}
			else
				SPAG_LOG << "event is ignored\n";
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
			return _current;
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
		static std::string buildOptions()
		{
			std::string out( "Spaghetti version " );
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
#else
		void writeDotFile( std::string ) const {}
#endif
///@}

///////////////////////////////////
// private member function section
///////////////////////////////////

	private:
		void runAction() const
		{
			SPAG_LOG << "switching to state " << _current << ", starting action\n";

			if( _timeout.at( _current ).enabled )
			{
				assert( p_timer );
				SPAG_LOG << "timeout enabled, duration=" << _timeout.at( _current ).duration << "\n";
				p_timer->timerStart( this );
			}
			if( _callback.at( _current ) ) // if there is a callback stored, then call it
			{
				SPAG_LOG << "callback function start:\n";
				if( std::is_same<CBA,DummyCbArg_t>::value )
					_callback.at( _current )( CBA() );
				else
					_callback.at( _current )( _callbackArg.at( _current ) );
			}
			else
				SPAG_LOG << "no callback provided\n";

			if( _isPassState[ currentState() ] )
			{
				SPAG_LOG << "is pass-state, switching to state " << _timeout.at( _current ).nextState << '\n';
				_current =  _timeout.at( _current ).nextState;
#ifdef SPAG_ENABLE_LOGGING
				_rtdata.logTransition( _timeout.at( _current ).nextState, EV::NB_EVENTS+1 );
#endif
				runAction();              // re-entry
			}
		}
		void printMatrix( std::ostream& str ) const;

/////////////////////////////
// private data section
/////////////////////////////

	private:
#ifdef SPAG_ENABLE_LOGGING
		mutable priv::RunTimeData<ST,EV>   _rtdata;
#endif
		mutable ST                         _current = static_cast<ST>(0);   ///< current state
		std::vector<std::vector<ST>>       _transition_mat;  ///< describe what states the fsm switches to, when a message is received. lines: events, columns: states, value: states to switch to. DOES NOT hold timer events
		std::vector<std::vector<char>>     _ignored_events;  ///< matrix holding for each event a boolean telling is the event is ignored or not, for a given state (0:ignore event, 1:handle event)
		std::vector<priv::TimerEvent<ST>>  _timeout;         ///< Holds for each state the information on timeout
		std::vector<Callback_t>            _callback;        ///< holds for each state the callback function to be called
		std::vector<char>                  _isPassState;     ///< holds 1 if this is a "pass state", that is a state with only one transition and no timeout
#ifdef SPAG_ENUM_STRINGS
		std::vector<std::string>           _str_events;      ///< holds events strings
		std::vector<std::string>           _str_states;      ///< holds states strings
#endif
/// If the user code provides a value for the callbacks, then we must store these, per state. If not, then this will remain an empty vector
		std::vector<CBA>                   _callbackArg;
		TIM*                               p_timer = nullptr;   ///< pointer on timer
};
//-----------------------------------------------------------------------------------
namespace priv
{
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
	maxlength = priv::getMaxLength( _str_events );
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
	out << "\n - Nb States=" << nbStates() << "\n - Nb events=" << nbEvents();

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
	void timerInit() {}
};

//-----------------------------------------------------------------------------------

} // namespace spag end

#define SPAG_DECLARE_FSM_TYPE_NOTIMER( type, st, ev, cbarg ) \
	typedef spag::SpagFSM<st,ev,spag::NoTimer<st,ev,cbarg>,cbarg> type;

#define SPAG_DECLARE_FSM_TYPE( type, st, ev, timer, cbarg ) \
	typedef spag::SpagFSM<st,ev,timer<st,ev,cbarg>,cbarg> type;


#endif // HG_SPAGHETTI_FSM_HPP

/**
\page p_manual Spaghetti manual


Sample programs: see the list of
<a href="../src/html/files.html" target="_blank">sample programs</a>.






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

\todo add an option so that in case we transition from one state to the same state, should the callback be called each time, or not ?

\todo add serialisation capability

\todo for enum to string automatic conversion, maybe use this ? :
https://github.com/aantron/better-enums

\todo Problem (demonstrated in traffic_lights_3.cpp): if a network asynchronous receive is pending and timeout occurs before,
then if a stop() is requested, the io_service will still be pending, waiting for incoming data.
At present, have added a timerKill() function that will free the io_service, thus cancelling reception and closing socket.
BUT: it would be better not to havec to do that...

\todo handle FSM with no events ??? (only timeouts) Possible ?
*/
