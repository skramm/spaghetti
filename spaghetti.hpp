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

/// \todo Need performance evaluation of this build option. If not defined, it defaults to std::vector
#define SPAG_USE_ARRAY

#define SPAG_VERSION 0.1

#include <vector>
#include <algorithm>
#include <functional>
#include <cassert>
#include <fstream>
#include <iostream> // needed for expansion of SPAG_LOG

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

#define SPAG_P_STRINGIZE2( a ) #a
#define SPAG_STRINGIZE( a ) SPAG_P_STRINGIZE2( a )

#define SPAG_P_CAST2IDX( a ) static_cast<size_t>(a)

// TEMP
typedef size_t Duration;

/// Main library namespace
namespace spag {

// DEPRECATED
// This is just to provide a dummy type for the callback argument, as \c void is not a valid type
//struct DummyCbArg_t {};

//------------------------------------------------------------------------------------
/// Used in printLoggedData() as second argument
enum PrintFlags
{
	stateCount  = 0x01
	,eventCount = 0x02
	,history    = 0x04
	,all        = 0x07
};

//-----------------------------------------------------------------------------------
/// private namespace, so user code won't hit into this
namespace priv {

//-----------------------------------------------------------------------------------
/// Container holding information on timeout events. Each state will have one, event if it does not use it
template<typename ST>
struct TimerEvent
{
	ST        nextState = static_cast<ST>(0); ///< state to switch to
	Duration  duration  = 0;                  ///< duration
	bool      enabled   = false;              ///< this state uses or not a timeout (default is no)

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
template<typename ST,typename CBA>
struct StateInfo
{
	TimerEvent<ST>           _timerEvent;   ///< Holds for each state the information on timeout
	std::function<void(CBA)> _callback;     ///< callback function
	CBA                      _callbackArg;  ///< value of argument of callback function
	bool                     _isPassState = false;  ///< true if this is a "pass state", that is a state with only one transition and no timeout
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
/**
type T is \c std::vector<std::string>> or \c std::array<std::string>>
*/
template<typename T>
size_t
getMaxLength( const T& v_str )
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
//------------------------------------------------------------------------------------
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
		ST     _state;
		size_t _event; ///< stored as size_t because it will hold values other than the ones in the enum
		std::chrono::duration<double> _elapsed;

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
	void printData( std::ostream& out, PrintFlags pflags ) const
	{
#ifdef SPAG_ENUM_STRINGS
		size_t maxlength_e = priv::getMaxLength( _str_events );
		size_t maxlength_s = priv::getMaxLength( _str_states );
#endif
		char sep(';');

		if( pflags & PrintFlags::stateCount )
		{
			out << "# State counters:\n";
			for( size_t i=0; i<_stateCounter.size(); i++ )
			{
				out << i << sep,
#ifdef SPAG_ENUM_STRINGS
				priv::PrintEnumString( out, _str_states[i], maxlength_s );
				out << sep;
#endif
				out << _stateCounter[i] << '\n';
			}
		}

		if( pflags & PrintFlags::eventCount )
		{
			out << "\n# Event counters:\n";
			for( size_t i=0; i<_eventCounter.size(); i++ )
			{
				out << i << sep;
#ifdef SPAG_ENUM_STRINGS
				priv::PrintEnumString( out, _str_events[i], maxlength_e );
				out << sep;
#endif
				out << _eventCounter[i] << '\n';
			}
		}

		if( pflags & PrintFlags::history )
		{
			out << "\n# Run history:\n#time" << sep << "event" << sep
#ifdef SPAG_ENUM_STRINGS
				<< "event_string" << sep << "state" << sep << "state_string\n";
#else
				<< "state\n";
#endif
			for( size_t i=0; i<_history.size(); i++ )
			{
				size_t ev = _history[i]._event;
				size_t st = SPAG_P_CAST2IDX(_history[i]._state);
				out << _history[i]._elapsed.count() << sep << ev << sep;
#ifdef SPAG_ENUM_STRINGS
				priv::PrintEnumString( out, _str_events[ev], maxlength_e );
				out << sep;
#endif
				out << st << sep;
#ifdef SPAG_ENUM_STRINGS
				priv::PrintEnumString( out, _str_states[ev], maxlength_s );
#endif
				out << '\n';
			}
		}
	}
/// Logs a transition from current state to state \c st, that was produced by event \c ev
/**
event stored as size_t because we may pass values other thant the ones in the enum (timeout and Always Active transitions)
*/
	void logTransition( ST st, size_t ev )
	{
		assert( ev < EV::NB_EVENTS+2 );
		assert( st < ST::NB_STATES );
		_eventCounter[ ev ]++;
		_stateCounter[ SPAG_P_CAST2IDX(st) ]++;
		_history.push_back( StateChangeEvent{ st, ev, std::chrono::high_resolution_clock::now() - _startTime } );
	}

	private:
		std::vector<size_t>  _stateCounter;    ///< per state counter
		std::vector<size_t>  _eventCounter;    ///< per event counter
/// Dynamic history of a given run: holds a state and the event that led to it. For the latter, the value EV_NB_EVENTS is used to store a "timeout" event.
		std::vector<StateChangeEvent> _history;
		std::chrono::time_point<std::chrono::high_resolution_clock> _startTime;

#ifdef SPAG_ENUM_STRINGS
		const std::vector<std::string>& _str_events; ///< reference on vector of strings of events
		const std::vector<std::string>& _str_states; ///< reference on vector of strings of states
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
//-----------------------------------------------------------------------------------
/// Used for configuration errors (more to be added). Used through priv::getConfigErrorMessage()
enum EN_ConfigError
{
	CE_TimeOutAndPassState,   ///< state has both timeout and pass-state flags active
	CE_IllegalPassState       ///< pass-state is followed by another pass-state
};

//-----------------------------------------------------------------------------------
/// Configuration error printing function
template<typename ST>
std::string
getConfigErrorMessage( priv::EN_ConfigError ce, ST st )
{
	std::string msg( "Spaghetti: configuration error: " );
	switch( ce )
	{
		case priv::CE_TimeOutAndPassState:
			msg += "state ";
			msg += std::to_string( static_cast<size_t>(st) );
//#ifdef SPAG_ENUM_STRINGS
//			msg += " '";
//			msg += _str_states[st];
//			msg += "'";
//#endif
			msg += " cannot have both a timeout and a pass-state flag";
		break;
		case CE_IllegalPassState:
			msg += "state ";
			msg += std::to_string( static_cast<size_t>(st) );
//#ifdef SPAG_ENUM_STRINGS
//			msg += " '";
//			msg += _str_states[st];
//			msg += "'";
//#endif
			msg += " cannot be followed by another pass-state";
		break;
		default: assert(0);
	}
	return msg;
}

template<typename ST, typename EV,typename CBA=int>
struct NoTimer;


} // namespace priv

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
template<typename ST, typename EV,typename TIM,typename CBA=int>
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
			static_assert( SPAG_P_CAST2IDX(EV::NB_EVENTS) > 0, "Error, you need to provide at least one event" );
			static_assert( SPAG_P_CAST2IDX(ST::NB_STATES) > 1, "Error, you need to provide at least two states" );
			priv::resizemat( _transition_mat, SPAG_P_CAST2IDX(EV::NB_EVENTS), SPAG_P_CAST2IDX(ST::NB_STATES) );
			priv::resizemat( _ignored_events, SPAG_P_CAST2IDX(EV::NB_EVENTS), SPAG_P_CAST2IDX(ST::NB_STATES) );

#ifndef SPAG_USE_ARRAY
			_stateInfo.resize( ST::NB_STATES );    // states information
#endif
			for( auto& e: _ignored_events )      // all events will be ignored at init
				std::fill( e.begin(), e.end(), 0 );

#ifdef SPAG_ENABLE_LOGGING
			_rtdata.alloc( SPAG_P_CAST2IDX(ST::NB_STATES), SPAG_P_CAST2IDX(EV::NB_EVENTS) );
#endif

#ifdef SPAG_ENUM_STRINGS
			_str_events.resize( SPAG_P_CAST2IDX(EV::NB_EVENTS)+2 );
			_str_states.resize( SPAG_P_CAST2IDX(ST::NB_STATES) );
			_str_events[ SPAG_P_CAST2IDX(EV::NB_EVENTS)   ] = "*Timeout*";
			_str_events[ SPAG_P_CAST2IDX(EV::NB_EVENTS+1) ] = "*  AA   *"; // Always Active Transition
#endif
		}

/** \name Configuration of FSM */
///@{
/// Assigned ignored event matrix
		void assignEventMatrix( const std::vector<std::vector<int>>& mat )
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

/// Assigns an external transition event \c ev to switch from state \c st1 to state \c st2
		void assignTransition( ST st1, EV ev, ST st2 )
		{
			SPAG_CHECK_LESS( SPAG_P_CAST2IDX(st1), nbStates() );
			SPAG_CHECK_LESS( SPAG_P_CAST2IDX(st2), nbStates() );
			SPAG_CHECK_LESS( SPAG_P_CAST2IDX(ev),  nbEvents() );
			_transition_mat[ SPAG_P_CAST2IDX(ev) ][ SPAG_P_CAST2IDX( st1 ) ] = st2;
			_ignored_events[ SPAG_P_CAST2IDX(ev) ][ SPAG_P_CAST2IDX( st1 ) ] = 1;
		}

/// Assigns a transition to a "pass state": once on state \c st1, the FSM will switch right away to \c st2
		void assignTransition( ST st1, ST st2 )
		{
			SPAG_CHECK_LESS( SPAG_P_CAST2IDX(st1), nbStates() );
			SPAG_CHECK_LESS( SPAG_P_CAST2IDX(st2), nbStates() );
			for( auto& line: _transition_mat )
				line[ SPAG_P_CAST2IDX( st1 ) ] = st2;
			for( auto& line: _ignored_events )
				line[ SPAG_P_CAST2IDX( st1 ) ] = 1;
			_stateInfo[st1]._isPassState = 1;
			if( _stateInfo[st1]._timerEvent.enabled )
				throw std::logic_error( priv::getConfigErrorMessage( priv::CE_TimeOutAndPassState, st1 ) );
		}

/// Assigns an timeout event on \b all states except \c st_final:
/**
After this, on all the states except \c st_final, if \c duration expires, the FSM will switch to \c st_final
(where there may or may not be a timeout assigned)
*/
		void assignGlobalTimeOut( ST st_final, Duration dur )
		{
			static_assert( std::is_same<TIM,priv::NoTimer<ST,EV,CBA>>::value == false, "ERROR, FSM build without timer" );
			SPAG_CHECK_LESS( SPAG_P_CAST2IDX(st_final), nbStates() );
			for( size_t i=0; i<nbStates(); i++ )
				if( i != SPAG_P_CAST2IDX(st_final) )
					_stateInfo[ SPAG_P_CAST2IDX( st_final ) ]._timerEvent = priv::TimerEvent<ST>( st_final, dur );
		}

/// Assigns an timeout event on state \c st_curr, will switch to event \c st_next
		void assignTimeOut( ST st_curr, Duration dur, ST st_next )
		{
			static_assert( std::is_same<TIM,priv::NoTimer<ST,EV,CBA>>::value == false, "ERROR, FSM build without timer" );
			SPAG_CHECK_LESS( SPAG_P_CAST2IDX(st_curr), nbStates() );
			SPAG_CHECK_LESS( SPAG_P_CAST2IDX(st_next), nbStates() );
			_stateInfo[ SPAG_P_CAST2IDX( st_curr ) ]._timerEvent = priv::TimerEvent<ST>( st_next, dur );
			if( _stateInfo[st_curr]._isPassState )
				throw std::logic_error( priv::getConfigErrorMessage( priv::CE_TimeOutAndPassState, st_curr ) );
		}

/// Whatever state we are in, if the event \c ev occurs, we switch to state \c st
		void assignTransitionAlways( EV ev, ST st )
		{
			SPAG_CHECK_LESS( SPAG_P_CAST2IDX(st), nbStates() );
			SPAG_CHECK_LESS( SPAG_P_CAST2IDX(ev), nbEvents() );
			for( auto& e: _transition_mat[ ev ] )
				e = st;
			for( auto& e: _ignored_events[ ev ] )
				e = 1;
		}
/// Allow all events of the transition matrix
		void allowAllEvents()
		{
			for( auto& line: _ignored_events )
				for( auto& e: line )
					e = 1;
		}
/// Allow event \c ev when on state \c st
		void allowEvent( ST st, EV ev, bool what=true )
		{
			SPAG_CHECK_LESS( SPAG_P_CAST2IDX(st), nbStates() );
			SPAG_CHECK_LESS( SPAG_P_CAST2IDX(ev), nbEvents() );
			_ignored_events[ SPAG_P_CAST2IDX(ev) ][ SPAG_P_CAST2IDX(st) ] = (what?1:0);
		}
/// Assigns a callback function to a state, will be called each time we arrive on this state
		void assignCallback( ST st, Callback_t func, CBA cb_arg=CBA() )
		{
			SPAG_CHECK_LESS( SPAG_P_CAST2IDX(st), nbStates() );
			_stateInfo[ SPAG_P_CAST2IDX(st) ]._callback    = func;
			_stateInfo[ SPAG_P_CAST2IDX(st) ]._callbackArg = cb_arg;
		}

/// Assigns a callback function to all the states, will be called each time the state is activated
		void assignGlobalCallback( Callback_t func )
		{
			for( size_t i=0; i<ST::NB_STATES; i++ )
				_stateInfo[ SPAG_P_CAST2IDX(i) ]._callback = func;
		}

		void assignCallbackValue( ST st, CBA cb_arg )
		{
			SPAG_CHECK_LESS( SPAG_P_CAST2IDX(st), nbStates() );
			_stateInfo[ SPAG_P_CAST2IDX(st) ]._callbackArg = cb_arg;
		}

		void assignTimer( TIM* t )
		{
			static_assert( std::is_same<TIM,priv::NoTimer<ST,EV,CBA>>::value == false, "ERROR, FSM build without timer" );
			p_timer = t;
		}

#ifdef SPAG_ENUM_STRINGS
/// Assign a string to an enum event value (available only if option SPAG_ENUM_STRINGS is enabled)
		void assignString2Event( EV ev, std::string str )
		{
			SPAG_CHECK_LESS( SPAG_P_CAST2IDX(ev), nbEvents() );
			_str_events[ SPAG_P_CAST2IDX(ev) ] = str;
		}
/// Assign a string to an enum state value (available only if option SPAG_ENUM_STRINGS is enabled)
		void assignString2State( ST st, std::string str )
		{
			SPAG_CHECK_LESS( SPAG_P_CAST2IDX(st), nbStates() );
			_str_states[ SPAG_P_CAST2IDX(st) ] = str;
		}
/// Assign strings to enum event values (available only if option SPAG_ENUM_STRINGS is enabled)
		void assignStrings2Events( const std::vector<std::pair<EV,std::string>>& v_str )
		{
			SPAG_CHECK_EQUAL( v_str.size(), EV::NB_EVENTS );
			for( const auto& p: v_str )
				assignString2Event( p.first, p.second );
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
			doChecking();

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
				p_timer->timerKill();
			}
		}

/// User-code timer end function/callback should call this when the timer expires
		void processTimeOut() const
		{
			SPAG_LOG << "processing timeout event, delay was " << _stateInfo[ _current ]._timerEvent.duration << "\n";
			assert( _stateInfo[ SPAG_P_CAST2IDX(_current) ]._timerEvent.enabled ); // or else, the timer shoudn't have been started, and thus we shouldn't be here...
			_current = _stateInfo[ SPAG_P_CAST2IDX( _current ) ]._timerEvent.nextState;
#ifdef SPAG_ENABLE_LOGGING
			_rtdata.logTransition( _current, EV::NB_EVENTS );
#endif
			runAction();
		}

/// User-code should call this function when an external event occurs
		void processEvent( EV ev ) const
		{
			SPAG_CHECK_LESS( SPAG_P_CAST2IDX(ev), nbEvents() );

#ifdef SPAG_ENUM_STRINGS
			SPAG_LOG << "processing event " << ev << ": \"" << _str_events[ev] << "\"\n";
#else
			SPAG_LOG << "processing event " << ev << '\n';
#endif
			if( _ignored_events[ SPAG_P_CAST2IDX( ev ) ][ SPAG_P_CAST2IDX(_current) ] != 0 )
			{
				if( _stateInfo[ SPAG_P_CAST2IDX( _current ) ]._timerEvent.enabled )               // 1 - cancel the waiting timer, if any
					p_timer->timerCancel();
				_current = _transition_mat[ SPAG_P_CAST2IDX(ev) ][ SPAG_P_CAST2IDX(_current) ];      // 2 - switch to next state
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
/// Return nb of states
		constexpr size_t nbStates() const
		{
			return SPAG_P_CAST2IDX(ST::NB_STATES);
		}
/// Return nb of events
		constexpr size_t nbEvents() const
		{
			return SPAG_P_CAST2IDX(EV::NB_EVENTS);
		}
/// Return current state
		ST currentState() const
		{
			return _current;
		}
/// Return duration of time out for state \c st, or 0 if none
		Duration timeOutDuration( ST st ) const
		{
			SPAG_CHECK_LESS( SPAG_P_CAST2IDX(st), nbStates() );
			return _stateInfo[ SPAG_P_CAST2IDX(st) ]._timerEvent.duration;
		}

		void printConfig( std::ostream& str, const char* msg=nullptr ) const;
#ifdef SPAG_ENABLE_LOGGING
/// Print dynamic data to \c str
		void printLoggedData( std::ostream& str, PrintFlags pf=PrintFlags::all ) const
		{
			_rtdata.printData( str, pf );
		}
#else
		void printLoggedData( std::ostream&, PrintFlags pf=PrintFlags::all ) const {}
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

#ifdef SPAG_GENERATE_DOTFILE
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
/// Run associated action with a state switch
		void runAction() const
		{
			SPAG_LOG << "switching to state " << SPAG_P_CAST2IDX(_current) << ", starting action\n";

			auto stateInfo = _stateInfo[ SPAG_P_CAST2IDX(_current) ];
			runAction_DoJob( stateInfo );

			if( stateInfo._isPassState )
			{
				assert( !stateInfo._timerEvent.enabled );
				SPAG_LOG << "is pass-state, switching to state " << SPAG_P_CAST2IDX(_transition_mat[0][ SPAG_P_CAST2IDX(_current) ]) << '\n';
				_current =  _transition_mat[0][ SPAG_P_CAST2IDX(_current) ];
#ifdef SPAG_ENABLE_LOGGING
				_rtdata.logTransition( _current, EV::NB_EVENTS+1 );
#endif
				runAction_DoJob( stateInfo );
			}
		}

/// sub-function of runAction(), needed for pass-states
		void runAction_DoJob( const priv::StateInfo<ST,CBA>& stateInfo ) const
		{
			if( stateInfo._timerEvent.enabled )
			{
				assert( p_timer );
				assert( !stateInfo._isPassState );
				SPAG_LOG << "timeout enabled, duration=" <<  stateInfo._timerEvent.duration << "\n";
				p_timer->timerStart( this );
			}
			if( stateInfo._callback ) // if there is a callback stored, then call it
			{
				SPAG_LOG << "callback function start:\n";
				stateInfo._callback( _stateInfo[ SPAG_P_CAST2IDX(_current) ]._callbackArg );
			}
			else
				SPAG_LOG << "no callback provided\n";
		}

		void printMatrix( std::ostream& str ) const;

/// Checks configuration for any illegal situation. Throws error if one is encountered.
		void doChecking() const
		{
			for( size_t i=0; i<nbStates(); i++ )
			{
				auto state = _stateInfo[i];
				if( state._isPassState )
				{
					size_t nextState = SPAG_P_CAST2IDX( _transition_mat[0][i] );
					if( _stateInfo[ nextState ]._isPassState )
						throw( std::logic_error( priv::getConfigErrorMessage( priv::CE_IllegalPassState, i ) ) );
					if( state._timerEvent.enabled )
						throw( std::logic_error( priv::getConfigErrorMessage( priv::CE_TimeOutAndPassState, i ) ) );
				}
			}
		}

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

#ifdef SPAG_USE_ARRAY
		std::array<priv::StateInfo<ST,CBA>,static_cast<size_t>(ST::NB_STATES)>  _stateInfo;         ///< Holds for each state the details
#else
		std::vector<priv::StateInfo<ST,CBA>>  _stateInfo;         ///< Holds for each state the details
#endif

#ifdef SPAG_ENUM_STRINGS
		std::vector<std::string>           _str_events;      ///< holds events strings
		std::vector<std::string>           _str_states;      ///< holds states strings
#endif
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

} // namespace priv

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
	for( size_t i=0; i<_stateInfo.size(); i++ )
		out << i << "  ";
	out << "\n   ";

#ifdef SPAG_ENUM_STRINGS
	size_t maxlength = priv::getMaxLength( _str_states );
#endif

	for( size_t i=0; i<_stateInfo.size(); i++ )
	{
		out << i;
#ifdef SPAG_ENUM_STRINGS
		out << ':';
		priv::PrintEnumString( out, _str_states[i], maxlength );
#endif
		out << '|' << (_stateInfo[i]._timerEvent.enabled?'o':'.') << '\n';
	}
}
//-----------------------------------------------------------------------------------
#ifdef SPAG_GENERATE_DOTFILE
/// Saves in current folder a .dot file of the FSM, to be rendered with Graphviz
template<typename ST, typename EV,typename T,typename CBA>
void
SpagFSM<ST,EV,T,CBA>::writeDotFile( std::string fname ) const
{
#if 0
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
		throw std::runtime_error( std::string( "Spaghetti: error, unable to open file: " + fname ) );
	boost::write_graphviz( f, g );
#else
	std::ofstream f ( fname );
	if( !f.is_open() )
		throw std::runtime_error( std::string( "Spaghetti: error, unable to open file: " + fname ) );
	f << "digraph G {\n";
	f << "0 [shape=\"doublecircle\"];\n";
	for( size_t i=0; i<EV::NB_EVENTS; i++ )
		for( size_t j=0; j<ST::NB_STATES; j++ )
			if( _ignored_events[i][j] )
				if( !_stateInfo[j]._isPassState )
					f << j << " -> " << _transition_mat[i][j] << " [label=\"" << i << "\"];\n";

	for( size_t j=0; j<ST::NB_STATES; j++ )
		if( _stateInfo[j]._isPassState )
			f << j << " -> " << _transition_mat[0][j] << " [label=\"AA\"];\n";

	f << "}\n";


#endif
}

#endif // SPAG_GENERATE_DOTFILE

//-----------------------------------------------------------------------------------
namespace priv {
/// dummy struct, useful in case there is no need for a timer
template<typename ST, typename EV,typename CBA>
struct NoTimer
{
	void timerStart( const SpagFSM<ST,EV,NoTimer,CBA>* ) {}
	void timerCancel() {}
	void timerInit() {}
};
} // namespace priv

//-----------------------------------------------------------------------------------

} // namespace spag

/// Shorthand for declaring the type of FSM, without a timer
#define SPAG_DECLARE_FSM_TYPE_NOTIMER( type, st, ev, cbarg ) \
	typedef spag::SpagFSM<st,ev,spag::priv::NoTimer<st,ev,cbarg>,cbarg> type;

/// Shorthand for declaring the type of FSM
#define SPAG_DECLARE_FSM_TYPE( type, st, ev, timer, cbarg ) \
	typedef spag::SpagFSM<st,ev,timer<st,ev,cbarg>,cbarg> type;


#endif // HG_SPAGHETTI_FSM_HPP

/**
\page p_manual Spaghetti reference manual

- for doc on classes/functions, see doxygen-generated index just above.
- for user manual, see markdown pages:
 - home page: https://github.com/skramm/spaghetti
 - manual: https://github.com/skramm/spaghetti/blob/master/docs/spaghetti_manual.md

\section sec_misc Misc. other informations

\subsection ssec_samples Sample programs

Check list here:
<a href="../src/html/files.html" target="_blank">sample programs</a>.



\subsection ssec_related Possibly related software

 - Boost MSM: http://www.boost.org/doc/libs/release/libs/msm/
 - Boost Statechart: http://www.boost.org/doc/libs/release/libs/statechart/
 - tinyFSM: https://github.com/digint/tinyfsm



\section ssec_devinfo Developper information

<b>Coding style</b>

Most of it is pretty obvious by parsing the code, but here are some additional points:

- TABS for indentation, SPACE for spacing
- Identifiers
 - \c camelCaseIsUsed for functions, variables
 - class/struct member data is prepended with '_' ( \c _thisIsADataMember )
 - Types are \c CamelCase (UpperCase first letter). Example: \c ThisIsAType
 - To avoid name collisions, all the symbols defined here start with "SPAG_"


\subsection ssec_todos TODOS


\todo add serialisation capability

\todo for enum to string automatic conversion, maybe use this ? :
https://github.com/aantron/better-enums


\todo handle FSM with no events ??? (only timeouts) Possible ?

\todo enable passing the FSM itself to the callback, to enable dynamic behaviour
(either editing the config at run-time, or generating events when certain situation is met).

\todo add some tests, and write a sample to evaluation performance

\todo in writeDotFile(), try to add the strings, if any.

*/
