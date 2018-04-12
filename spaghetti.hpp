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

/// At present, data is stored into arrays if this is defined. \todo Need performance evaluation of this build option. If not defined, it defaults to std::vector
#define SPAG_USE_ARRAY


#define SPAG_VERSION 0.7.0

#include <vector>
#include <map>
#include <algorithm>
#include <functional>
#include <cassert>
#include <iomanip>
#include <fstream>
#include <iostream> // needed for expansion of SPAG_LOG


#if defined (SPAG_EMBED_ASIO_TIMER)
	#define SPAG_USE_ASIO_TIMER
#endif

#if defined (SPAG_USE_ASIO_TIMER)
	#include <boost/bind.hpp>
	#include <boost/asio.hpp>
#endif

#if defined (SPAG_USE_ASIO_TIMER) || defined (SPAG_ENABLE_LOGGING)
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

#ifdef SPAG_NO_VERBOSE
	#define SPAG_P_LOG_ERROR \
		if(0) \
			std::cerr
#else
	#define SPAG_P_LOG_ERROR \
		if(1) \
			std::cerr << spag::priv::getSpagName()
#endif

#define SPAG_P_THROW_ERROR_RT( msg ) \
	{ \
		SPAG_P_LOG_ERROR << "error in " << __FUNCTION__ << "(): " << msg << '\n'; \
		throw std::runtime_error( spag::priv::getSpagName() + "runtime error in " + __FUNCTION__ + "(): " + msg ); \
	}

#define SPAG_P_THROW_ERROR_CFG( msg ) \
	{ \
		SPAG_P_LOG_ERROR << "error in " << __FUNCTION__ << "(): " << msg << '\n'; \
		throw std::logic_error( spag::priv::getSpagName() + "configuration error in " + __FUNCTION__ + "(): " + msg ); \
	}

#ifdef SPAG_FRIENDLY_CHECKING
	#define SPAG_CHECK_EQUAL( a, b ) \
	{ \
		if( (a) != (b) ) \
		{ \
			std::cerr << spag::priv::getSpagName() << "runtime error in func: " << __FUNCTION__ << "(), values are not equal:\n" \
				<< " - "   << #a << " value=" << a \
				<< "\n - " << #b << " value=" << b << '\n'; \
			SPAG_P_THROW_ERROR_CFG( "values are not equal" ); \
		} \
	}
#else
	#define SPAG_CHECK_EQUAL( a, b ) assert( (a) == (b) )
#endif

#ifdef NDEBUG
	#define SPAG_P_ASSERT( a, msg ) {}
#else
	#define SPAG_P_ASSERT( a, msg ) \
		if(!(a) ) \
		{ \
			std::cerr << priv::getSpagName() << "assert failure in function " << __FUNCTION__ \
				<< "(): condition \"" << #a << "\" is false, " << msg << '\n'; \
				std::exit(1); \
		}
#endif

#ifdef SPAG_FRIENDLY_CHECKING
	#define SPAG_CHECK_LESS( a, b ) \
		if( !( (a) < (b) ) )\
		{ \
			std::cerr << spag::priv::getSpagName() << "runtime error in func: " << __FUNCTION__ << "(), value is incorrect:\n" \
				<< " - "   << #a << " value=" << a \
				<< "\n - " << #b << " max value=" << b << '\n'; \
			SPAG_P_THROW_ERROR_CFG( "incorrect values" ); \
		}
#else
	#define SPAG_CHECK_LESS( a, b ) assert( (a) < (b) )
#endif

#define SPAG_P_STRINGIZE2( a ) #a
#define SPAG_STRINGIZE( a ) SPAG_P_STRINGIZE2( a )

/// Private macro, used to convert a 'state' type into an integer
#define SPAG_P_CAST2IDX( a ) static_cast<size_t>(a)


// TEMP
typedef size_t Duration;

/// Main library namespace
namespace spag {

//------------------------------------------------------------------------------------
/// Used in printLoggedData() as second argument
enum PrintFlags
{
	stateCount  = 0x01
	,eventCount = 0x02
	,ignoredEvents = 0x04
	,all        = 0xff
};

/// Timer units
enum class DurUnit { ms, sec, min };

//-----------------------------------------------------------------------------------
/// private namespace, so user code won't hit into this
namespace priv {

//-----------------------------------------------------------------------------------
/// helper function
inline
std::pair<bool,DurUnit>
timeUnitFromString( std::string str ) noexcept
{
	if( str == "ms" )
		return std::make_pair( true, DurUnit::ms );
	if( str == "sec" )
		return std::make_pair( true, DurUnit::sec );
	if( str == "min" )
		return std::make_pair( true, DurUnit::min );
	return std::make_pair( false, DurUnit::min );
}

//-----------------------------------------------------------------------------------
/// helper function
inline
std::string
stringFromTimeUnit( DurUnit du )
{
	std::string out;
	switch( du )
	{
		case DurUnit::ms:  out = "ms";  break;
		case DurUnit::sec: out = "sec"; break;
		case DurUnit::min: out = "min"; break;
	}
	return out;
}
//-----------------------------------------------------------------------------------
/// returns name of lib as static string, to save space
static std::string&
getSpagName()
{
	static std::string str{"Spaghetti: "};
	return str;
}
//-----------------------------------------------------------------------------------
/// Container holding information on timeout events. Each state will have one, event if it does not use it
template<typename ST>
struct TimerEvent
{
	ST       _nextState = static_cast<ST>(0); ///< state to switch to
	Duration _duration  = 0;                  ///< duration
	bool     _enabled   = false;              ///< this state uses or not a timeout (default is no)
	DurUnit  _durUnit   = DurUnit::sec;       ///< Duration unit

	TimerEvent()
		: _nextState(static_cast<ST>(0))
		, _duration(0)
		, _enabled(false)
	{
	}
	TimerEvent( ST st, Duration dur, DurUnit unit ): _nextState(st), _duration(dur), _durUnit(unit)
	{
		_enabled = true;
	}
};
//-----------------------------------------------------------------------------------
#ifdef SPAG_USE_SIGNALS
/// Holds information on inner events
template<typename ST,typename EV>
struct InnerTransition
{
	bool    _hasOne = false;
	bool    _isActive = false;
	ST      _destState;
	EV      _innerEvent;
	bool    _isPassState = false; /// True if this is a "pass-state", that is a state with only one transition and no timeout.

	friend std::ostream& operator << ( std::ostream& s, const InnerTransition& it )
	{
		s << "InnerTransition: hasOne=" << it._hasOne
			<< " isActive=" << it._isActive
			<< " isPassState=" << it._isPassState
			<< " destState=" << (int)it._destState
			<< " innerEvent=" << (int)it._innerEvent << '\n';
		return s;
	}

};
#endif
//-----------------------------------------------------------------------------------
/// Private class, holds information about a state
template<typename ST,typename EV,typename CBA>
struct StateInfo
{
	TimerEvent<ST>           _timerEvent;   ///< Holds for each state the information on timeout
	std::function<void(CBA)> _callback;     ///< callback function
	CBA                      _callbackArg;  ///< value of argument of callback function

#ifdef SPAG_USE_SIGNALS
	InnerTransition<ST,EV>    _innerTrans;
#endif
};
//-----------------------------------------------------------------------------------
/// Private, helper function
inline
void
PrintEnumString( std::ostream& out, std::string str, size_t maxlength )
{
	assert( !str.empty() );
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
	assert( !v_str.empty() );
	assert( !v_str[0].empty() );

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
/// Holds the FSM dynamic data: current state, and logged data (if enabled at build, see symbol \c SPAG_ENABLE_LOGGING)
#ifdef SPAG_ENABLE_LOGGING
template<typename ST,typename EV>
struct RunTimeData
{
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/// a state-change event, used for logging
	struct StateChangeEvent
	{
		size_t _state;
		size_t _event;   ///< stored as size_t because it will hold values other than the ones in the enum
		std::chrono::duration<double> _elapsed;
	};
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

	public:
#ifdef SPAG_ENUM_STRINGS
	void init()
	{
		_maxlength_e = priv::getMaxLength( _strEvents );
		_maxlength_s = priv::getMaxLength( _strStates );
	}

	RunTimeData( const std::vector<std::string>& str_events, const std::vector<std::string>& str_states )
		: _strEvents(str_events), _strStates( str_states )
#else
	RunTimeData()
#endif
	{
		_startTime = std::chrono::high_resolution_clock::now();
		_ignoredEvents.fill( 0 );
	}

	void alloc( size_t nbStates, size_t nbEvents )
	{
		_stateCounter.resize( nbStates,   0 );
		_eventCounter.resize( nbEvents+2, 0 );   // two last elements are used for timeout events and for "no event" transitions ("pass-states")
	}
	void incrementInitState()
	{
		assert( _stateCounter.size() );
		_stateCounter[0] = 1;
	}
	void clear()
	{
		_stateCounter.clear();
		_eventCounter.clear();
	}
	/// Print dynamic data (runtime data) to \c out
	void printData( std::ostream& out, PrintFlags pflags ) const
	{
		if( pflags & PrintFlags::stateCount )
		{
			out << "# State counters:\n";
			for( size_t i=0; i<_stateCounter.size(); i++ )
			{
				out << i << sep,
#ifdef SPAG_ENUM_STRINGS
				priv::PrintEnumString( out, _strStates[i], _maxlength_s );
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
				priv::PrintEnumString( out, _strEvents[i], _maxlength_e );
				out << sep;
#endif
				out << _eventCounter[i] << '\n';
			}
		}

		if( pflags & PrintFlags::ignoredEvents )
		{
			out << "\n# Ignored Events counters:\n";
			for( size_t i=0; i<_ignoredEvents.size(); i++ )
			{
				out << i << sep;
#ifdef SPAG_ENUM_STRINGS
				priv::PrintEnumString( out, _strEvents[i], _maxlength_e );
				out << sep;
#endif
				out << _ignoredEvents[i] << '\n';
			}
		}
	}

/// Logs a transition from current state to state \c st, that was produced by event \c ev
/**
event stored as size_t because we may pass values other than the ones in the enum (timeout and Always Active transitions)
*/
	void logTransition( ST st, size_t ev_idx )
	{
		assert( ev_idx < SPAG_P_CAST2IDX( EV::NB_EVENTS ) + 2 );
		assert( st < ST::NB_STATES );
		auto st_idx = SPAG_P_CAST2IDX(st);
		_eventCounter[ ev_idx ]++;
		_stateCounter[ st_idx ]++;

		if( !_logfile.is_open() )
		{
			_logfile.open( _logfileName );
			if( !_logfile.is_open() )
				SPAG_P_THROW_ERROR_RT( "unable to open file " + _logfileName );

			_logfile << "\n# FSM run history:\n#time" << sep << "event" << sep
#ifdef SPAG_ENUM_STRINGS
				<< "event_string" << sep << "state" << sep << "state_string\n";
#else
				<< "state\n";
#endif
		}
		print2LogFile( _logfile, StateChangeEvent{ st_idx, ev_idx, std::chrono::high_resolution_clock::now() - _startTime } );
	}

	void logIgnoredEvent( size_t ev_idx )
	{
		SPAG_CHECK_LESS( ev_idx, SPAG_P_CAST2IDX(EV::NB_EVENTS) );
		_ignoredEvents[ ev_idx ]++;
	}

//////////////////////////////////
// public data section
//////////////////////////////////

	std::string   _logfileName = "spaghetti.csv";
	char sep = ';';

//////////////////////////////////
// private member function section
//////////////////////////////////

	private:
	void print2LogFile( std::ofstream& f,  const StateChangeEvent sce ) const
	{
		char sep(';');
		f << sce._elapsed.count() << sep << sce._event << sep;

#ifdef SPAG_ENUM_STRINGS
		priv::PrintEnumString( f, _strEvents[sce._event], _maxlength_e );
		f << sep;
#endif
		f << sce._state << sep;
#ifdef SPAG_ENUM_STRINGS
		priv::PrintEnumString( f, _strStates[sce._state], _maxlength_s );
#endif
		f << '\n';
	}

//////////////////////////////////
// private data section
//////////////////////////////////

	private:
		std::vector<size_t>  _stateCounter;    ///< per state counter
		std::vector<size_t>  _eventCounter;    ///< per event counter
		std::chrono::time_point<std::chrono::high_resolution_clock> _startTime;
		std::ofstream _logfile;
		std::array<size_t,static_cast<size_t>(EV::NB_EVENTS)> _ignoredEvents;

#ifdef SPAG_ENUM_STRINGS
		const std::vector<std::string>& _strEvents; ///< reference on vector of strings of events
		const std::vector<std::string>& _strStates; ///< reference on vector of strings of states
		size_t _maxlength_e;
		size_t _maxlength_s;
#endif
};
#endif
//-----------------------------------------------------------------------------------
/// helper template function (unused if SPAG_USE_ARRAY defined)
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
	CE_TimeOutAndPassState   ///< state has both timeout and pass-state flags active
	,CE_IllegalPassState     ///< pass-state is followed by another pass-state
	,CE_SamePassState        ///< pass-state leads to same state
};

//-----------------------------------------------------------------------------------
/// Dummy struct, useful in case there is no need for a timer
template<typename ST, typename EV,typename CBA=int>
struct NoTimer;

} // namespace priv

#if defined (SPAG_USE_ASIO_TIMER)
/// Forward declaration
	template<typename ST, typename EV, typename CBA>
	struct AsioWrapper;
#endif

//-----------------------------------------------------------------------------------
/// Options for printing the dotfile, see SpagFSM::writeDotFile()
struct DotFileOptions
{
	bool showActiveState = true;
};

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
		SpagFSM() : _rtdata( _strEvents, _strStates )
#else
		SpagFSM()
#endif
		{
			static_assert( SPAG_P_CAST2IDX(ST::NB_STATES) > 1, "Error, you need to provide at least two states" );
#ifndef SPAG_USE_ARRAY
			priv::resizemat( _transitionMat, nbEvents(), nbStates() );
			priv::resizemat( _allowedMat, nbEvents(), nbStates() );
			_stateInfo.resize( nbStates() );    // states information
#endif
			for( auto& e: _allowedMat )      // all events will be ignored at init
				std::fill( e.begin(), e.end(), 0 );

#ifdef SPAG_ENABLE_LOGGING
			_rtdata.alloc( nbStates(), nbEvents() );
#endif

#ifdef SPAG_ENUM_STRINGS
			_strEvents.resize( nbEvents()+2 );
			_strStates.resize( nbStates() );
			std::generate(                          // assign default strings, so it doesn't stay empty
				_strStates.begin(),
				_strStates.end(),
				[](){ static int idx; std::string s = "St-"; s += std::to_string(idx++); return s; } // lambda
			);
			std::generate(                          // assign default strings, so it doesn't stay empty
				_strEvents.begin(),
				_strEvents.end(),
				[](){ static int idx; std::string s = "Ev-"; s += std::to_string(idx++); return s; } // lambda
			);
			_strEvents[ nbEvents()   ] = "*Timeout*";
			_strEvents[ nbEvents()+1 ] = "*  AAT  *"; // Always Active Transition
#endif

#ifdef SPAG_EMBED_ASIO_TIMER
			_timer = &_asioWrapper;
#endif
		}

/** \name Configuration of FSM */
///@{

/// Assigns allowed event matrix
		void assignEventMatrix( const std::vector<std::vector<char>>& mat )
		{
			SPAG_CHECK_EQUAL( mat.size(),    nbEvents() );
			SPAG_CHECK_EQUAL( mat[0].size(), nbStates() );
			_allowedMat = mat;
		}

		void assignTransitionMat( const std::vector<std::vector<ST>>& mat )
		{
			SPAG_CHECK_EQUAL( mat.size(),    nbEvents() );
			SPAG_CHECK_EQUAL( mat[0].size(), nbStates() );
			_transitionMat = mat;
		}

/// Assigns an external transition event \c ev to switch from state \c st1 to state \c st2
		void assignTransition( ST st1, EV ev, ST st2 )
		{
			SPAG_CHECK_LESS( SPAG_P_CAST2IDX(st1), nbStates() );
			SPAG_CHECK_LESS( SPAG_P_CAST2IDX(st2), nbStates() );
			SPAG_CHECK_LESS( SPAG_P_CAST2IDX(ev),  nbEvents() );
			auto st1_idx = SPAG_P_CAST2IDX(st1);

#ifdef SPAG_USE_SIGNALS
			if( _stateInfo[st1_idx]._innerTrans._isPassState )
			{
				std::string err_msg{ "error, attempting to assign a transition to state" };
				err_msg += std::to_string( st1_idx );
#ifdef SPAG_ENUM_STRINGS
				err_msg += " (" + _strStates[ st1_idx ] + ")";
#endif
				err_msg += ", was previously declared as pass-state";
				SPAG_P_THROW_ERROR_CFG( err_msg );
			}
#endif
			_transitionMat[ SPAG_P_CAST2IDX(ev) ][ st1_idx ] = st2;
			_allowedMat[    SPAG_P_CAST2IDX(ev) ][ st1_idx ] = 1;
		}

#ifdef SPAG_USE_SIGNALS
/// Assigns a transition to a "pass-state": once on state \c st1, the FSM will switch right away to \c st2
/**
\warning If a time out has previously been assigned to state \c st1, it will be removed

\warning Only available when \ref SPAG_USE_SIGNALS is defined, see manual.
*/
		void assignTransition( ST st1, ST st2 )
		{
			size_t st1_idx = SPAG_P_CAST2IDX(st1);
			size_t st2_idx = SPAG_P_CAST2IDX(st2);
			SPAG_CHECK_LESS( st1_idx, nbStates() );
			SPAG_CHECK_LESS( st2_idx, nbStates() );
			for( auto& line: _transitionMat )
				line[ st1_idx ] = st2;
			for( auto& line: _allowedMat )
				line[ st1_idx ] = 1;
			_stateInfo[st1_idx]._innerTrans._isPassState = true;
			_stateInfo[st1_idx]._innerTrans._hasOne   = false;
			_stateInfo[st1_idx]._innerTrans._isActive = true;
			_stateInfo[st1_idx]._innerTrans._destState = st2;

			auto& te = _stateInfo[st1_idx]._timerEvent;
			if( te._enabled )
			{
				SPAG_P_LOG_ERROR << "warning, removal of timeout of "
					<< te._duration << ' ' << priv::stringFromTimeUnit( te._durUnit )
					<< " on state " << SPAG_P_CAST2IDX(st1)
#ifdef SPAG_ENUM_STRINGS
					<< " (" << _strStates[st1] << ')'
#endif
					<< '\n';
				te._enabled = false;
			}
		}

/// Assigns a inner transition between \c st1 and \c st2, triggered by event \c ev
/// \warning Only available when \ref SPAG_USE_SIGNALS is defined, see manual.
		void assignInnerTransition( ST st1, EV ev, ST st2 )
		{
			auto st1_idx = SPAG_P_CAST2IDX(st1);
			_stateInfo[ st1_idx ]._innerTrans._hasOne = true;
			_stateInfo[ st1_idx ]._innerTrans._innerEvent = ev;
			_stateInfo[ st1_idx ]._innerTrans._destState  = st2;
			_eventInfo[ ev ] = st1;
		}
#endif

/// Assigns an timeout event on \b all states except \c st_final, using default timer units
		void assignGlobalTimeOut( Duration dur, ST st_final )
		{
			assignGlobalTimeOut( dur, _defaultTimerUnit, st_final );
		}

/// Assigns an timeout event on \b all states except \c st_final, using unit \c durUnit
		void assignGlobalTimeOut( Duration dur, std::string durUnit, ST st_final )
		{
			auto tu = priv::timeUnitFromString( durUnit );
			if( !tu.first )
				SPAG_P_THROW_ERROR_CFG( "invalid string value: " + durUnit );
			assignGlobalTimeOut( dur, tu.second, st_final );
		}

/// Assigns an timeout event on \b all states except \c st_final, using unit \c durUnit
/**
After this, on all the states except \c st_final, if \c duration expires, the FSM will switch to \c st_final
(where there may or may not be a timeout assigned)
*/
		void assignGlobalTimeOut( Duration dur, DurUnit durUnit, ST st_final )
		{
			static_assert( std::is_same<TIM,priv::NoTimer<ST,EV,CBA>>::value == false, "ERROR, FSM build without timer" );
			for( size_t i=0; i<nbStates(); i++ )
				if( i != SPAG_P_CAST2IDX(st_final) )
				{
					auto te = _stateInfo[ SPAG_P_CAST2IDX( i ) ]._timerEvent;
					if( te._enabled )
					{
						std::string err_msg = "error, assigning global timeout of " + std::to_string( dur ) + " " + priv::stringFromTimeUnit( durUnit );
						err_msg += ": unable because of previously assigned timeout on state '" + std::to_string( i ) + "' ";
#ifdef SPAG_ENUM_STRINGS
						err_msg += "(" + _strStates[i] + ") ";
#endif
						err_msg += ", value " + std::to_string( te._duration ) + " " + priv::stringFromTimeUnit( te._durUnit );
						SPAG_P_THROW_ERROR_CFG( err_msg );
					}
					assignTimeOut( static_cast<ST>(i), dur, durUnit, st_final );
				}
		}

/// Assigns an timeout event on state \c st_curr, will switch to event \c st_next
		void assignTimeOut( ST st_curr, Duration dur, ST st_next )
		{
			assignTimeOut( st_curr, dur, _defaultTimerUnit, st_next );
		}

/// Assigns an timeout event on state \c st_curr, will switch to event \c st_next. With units
		void assignTimeOut( ST st_curr, Duration dur, DurUnit unit, ST st_next )
		{
			static_assert( std::is_same<TIM,priv::NoTimer<ST,EV,CBA>>::value == false, "ERROR, FSM build without timer" );
			SPAG_CHECK_LESS( SPAG_P_CAST2IDX(st_curr), nbStates() );
			SPAG_CHECK_LESS( SPAG_P_CAST2IDX(st_next), nbStates() );
			_stateInfo[ SPAG_P_CAST2IDX( st_curr ) ]._timerEvent = priv::TimerEvent<ST>( st_next, dur, unit );
		}

/// Assigns an timeout event on state \c st_curr, will switch to event \c st_next. With units as strings
		void assignTimeOut( ST st_curr, Duration dur, std::string unit, ST st_next )
		{
			auto tu = priv::timeUnitFromString( unit );
			if( !tu.first )
				SPAG_P_THROW_ERROR_CFG( "invalid string value: " + unit );
			assignTimeOut( st_curr, dur, tu.second, st_next );
		}
/// Removes all the timeouts
		void clearTimeOuts()
		{
			static_assert( std::is_same<TIM,priv::NoTimer<ST,EV,CBA>>::value == false, "ERROR, FSM build without timer" );
			for( size_t i=0; i<nbStates(); i++ )
				_stateInfo[ SPAG_P_CAST2IDX( i ) ]._timerEvent._enabled = false;
		}
/// Removes the timeout on state \c st
		void clearTimeOut( ST st )
		{
			static_assert( std::is_same<TIM,priv::NoTimer<ST,EV,CBA>>::value == false, "ERROR, FSM build without timer" );
			auto st_idx = SPAG_P_CAST2IDX( st );
			SPAG_CHECK_LESS( st_idx, nbStates() );
			if( !_stateInfo[ st_idx ]._timerEvent._enabled )
			{
				SPAG_P_LOG_ERROR << "warning: asking for removal of timeout on state " << st_idx
#ifdef SPAG_ENUM_STRINGS
					<< " (" << _strStates[st_idx]  << ')'
#endif
					<< " but state has no timeout assigned...\n";
			}
			_stateInfo[ st_idx ]._timerEvent._enabled = false;
		}

/// Whatever state we are in, if the event \c ev occurs, we switch to state \c st
		void assignTransition( EV ev, ST st )
		{
			SPAG_CHECK_LESS( SPAG_P_CAST2IDX(st), nbStates() );
			SPAG_CHECK_LESS( SPAG_P_CAST2IDX(ev), nbEvents() );
			for( auto& e: _transitionMat[ ev ] )
				e = st;
			for( auto& e: _allowedMat[ ev ] )
				e = 1;
		}
/// Allow all events of the transition matrix
		void allowAllEvents()
		{
			for( auto& line: _allowedMat )
				for( auto& e: line )
					e = 1;
		}
/// Allow event \c ev when on state \c st
		void allowEvent( ST st, EV ev, bool what=true )
		{
			SPAG_CHECK_LESS( SPAG_P_CAST2IDX(st), nbStates() );
			SPAG_CHECK_LESS( SPAG_P_CAST2IDX(ev), nbEvents() );
			_allowedMat[ SPAG_P_CAST2IDX(ev) ][ SPAG_P_CAST2IDX(st) ] = (what?1:0);
		}
/// Assigns a callback function to a state, will be called each time we arrive on this state
		void assignCallback( ST st, Callback_t func, CBA cb_arg=CBA() )
		{
			SPAG_CHECK_LESS( SPAG_P_CAST2IDX(st), nbStates() );
			_stateInfo[ SPAG_P_CAST2IDX(st) ]._callback    = func;
			_stateInfo[ SPAG_P_CAST2IDX(st) ]._callbackArg = cb_arg;
		}

/// Assigns a callback function to all the states, will be called each time the state is activated
		void assignCallback( Callback_t func )
		{
			for( size_t i=0; i<nbStates(); i++ )
				_stateInfo[ SPAG_P_CAST2IDX(i) ]._callback = func;
		}

/// Assigns a callback function called when an ignored event occurs
		void assignIgnoredEventsCallback( std::function<void(ST,EV)> func )
		{
			_ignEventCallback = func;
		}

		void assignCallbackValue( ST st, CBA cb_arg )
		{
			SPAG_CHECK_LESS( SPAG_P_CAST2IDX(st), nbStates() );
			_stateInfo[ SPAG_P_CAST2IDX(st) ]._callbackArg = cb_arg;
		}

#ifndef SPAG_EMBED_ASIO_TIMER
		void assignTimer( TIM* t )
		{
			static_assert( std::is_same<TIM,priv::NoTimer<ST,EV,CBA>>::value == false, "ERROR, FSM build without timer" );
			_timer = t;
		}
#endif

/// Assign configuration from other FSM
		void assignConfig( const SpagFSM& fsm )
		{
			SPAG_CHECK_EQUAL( nbEvents(), fsm.nbEvents() );
			SPAG_CHECK_EQUAL( nbStates(), fsm.nbStates() );
			_transitionMat = fsm._transitionMat;
			_allowedMat    = fsm._allowedMat;
			_stateInfo     = fsm._stateInfo;
#ifdef SPAG_ENUM_STRINGS
			_strEvents     = fsm._strEvents;
			_strStates     = fsm._strStates;
#endif
		}

#ifdef SPAG_ENUM_STRINGS
/// Assign a string to an enum event value (available only if option SPAG_ENUM_STRINGS is enabled)
		void assignString2Event( EV ev, std::string str )
		{
			SPAG_CHECK_LESS( SPAG_P_CAST2IDX(ev), nbEvents() );
			_strEvents[ SPAG_P_CAST2IDX(ev) ] = str;
		}
/// Assign a string to an enum state value (available only if option SPAG_ENUM_STRINGS is enabled)
		void assignString2State( ST st, std::string str )
		{
			SPAG_CHECK_LESS( SPAG_P_CAST2IDX(st), nbStates() );
			_strStates[ SPAG_P_CAST2IDX(st) ] = str;
		}
/// Assign strings to enum event values (available only if option SPAG_ENUM_STRINGS is enabled)
		void assignStrings2Events( const std::vector<std::pair<EV,std::string>>& v_str )
		{
			SPAG_CHECK_LESS( v_str.size(), nbEvents()+1 );
			for( const auto& p: v_str )
				assignString2Event( p.first, p.second );
		}
/// Assign strings to enum state values (available only if option SPAG_ENUM_STRINGS is enabled)
		void assignStrings2States( const std::vector<std::pair<ST,std::string>>& v_str )
		{
			SPAG_CHECK_LESS( v_str.size(), nbStates()+1 );
			for( const auto& p: v_str )
				assignString2State( p.first, p.second );
		}
/// Assign strings to enum event values (available only if option SPAG_ENUM_STRINGS is enabled) - overload 1
		void assignStrings2Events( std::map<EV,std::string>& m_str )
		{
			for( const auto& p: m_str )
				assignString2Event( p.first, p.second );
		}
/// Assign strings to enum state values (available only if option SPAG_ENUM_STRINGS is enabled) - overload 1
		void assignStrings2States( std::map<ST,std::string>& m_str )
		{
			for( const auto& p: m_str )
				assignString2State( p.first, p.second );
		}
		void assignCBValuesStrings()
		{
			static_assert( std::is_same<CBA,std::string>::value, "Error, unable to assign strings to callback values, callback type is not std::string\n" );
			for( size_t i=0; i<nbStates(); i++ )
				assignCallbackValue( static_cast<ST>(i), _strStates[i] );
		}
#else
		void assignString2Event( EV, std::string ) {}
		void assignString2State( ST, std::string ) {}
		void assignStrings2Events( const std::vector<std::pair<EV,std::string>>& ) {}
		void assignStrings2States( const std::vector<std::pair<ST,std::string>>& ) {}
		void assignStrings2Events( const std::map<EV,std::string>& ) {}
		void assignStrings2States( const std::map<ST,std::string>& ) {}
		void assignCBValuesStrings() const {}
#endif

///@}

/** \name Run time functions */
///@{
/// start FSM : run callback associated to initial state (if any), an run timer (if any)
		void start() const
		{
			SPAG_P_ASSERT( !_isRunning, "attempt to start an already running FSM" );

			doChecking();

#if (defined SPAG_ENABLE_LOGGING) && (defined SPAG_ENUM_STRINGS)
			_rtdata.init();
#endif
			_isRunning = true;

#ifdef SPAG_ENABLE_LOGGING
			_rtdata.incrementInitState();
#endif
			runAction();

#ifndef SPAG_EXTERNAL_EVENT_LOOP
			if( !std::is_same<TIM,priv::NoTimer<ST,EV,CBA>>::value )
			{
				SPAG_P_ASSERT( _timer, "Timer has not been allocated" );
				_timer->timerInit( this );   // blocking function !
			}
#endif
		}

/// stop FSM : needed only if timer is used, this will cancel (and kill) the pending timer
		void stop() const
		{
			SPAG_P_ASSERT( _isRunning, "attempt to stop an already stopped FSM" );

			if( _timer )
			{
				SPAG_LOG << "call timerCancel()\n";
				_timer->timerCancel();
				SPAG_LOG << "call timerKill()\n";
				_timer->timerKill();
			}
			_isRunning = false;
		}

/// User-code timer end function/callback should call this when the timer expires
		void processTimeOut() const
		{
			SPAG_LOG << "processing timeout event, delay was " << _stateInfo[ _current ]._timerEvent._duration << "\n";
			assert( _stateInfo[ SPAG_P_CAST2IDX(_current) ]._timerEvent._enabled ); // or else, the timer shouldn't have been started, and thus we shouldn't be here...
			_current = _stateInfo[ SPAG_P_CAST2IDX( _current ) ]._timerEvent._nextState;
#ifdef SPAG_ENABLE_LOGGING
			_rtdata.logTransition( _current, nbEvents() );
#endif
			runAction();
		}

/// User-code should call this function when an external event occurs
		void processEvent( EV ev ) const
		{
			SPAG_CHECK_LESS( SPAG_P_CAST2IDX(ev), nbEvents() );
			SPAG_P_ASSERT( _isRunning, "attempting to process an event but FSM is not started" );

#ifdef SPAG_USE_SIGNALS
			if( _eventInfo.find( ev ) != std::end( _eventInfo ) ) // if found, this means that this event is "special", thus should not be processed that way
			{
				SPAG_P_THROW_ERROR_RT(
					"illegal processing of special event of id=" + std::to_string( SPAG_P_CAST2IDX(ev) )
				);
			}
#endif

			auto ev_idx = SPAG_P_CAST2IDX( ev );
#ifdef SPAG_ENUM_STRINGS
			SPAG_LOG << "processing event " << ev_idx << ": \"" << _strEvents[ev_idx] << "\"\n";
#else
			SPAG_LOG << "processing event " << ev_idx << '\n';
#endif
			if( _allowedMat[ ev_idx ][ SPAG_P_CAST2IDX(_current) ] != 0 )
			{
				if( _stateInfo[ SPAG_P_CAST2IDX( _current ) ]._timerEvent._enabled )  // 1 - cancel the waiting timer, if any
				{
					SPAG_P_ASSERT( _timer, "Timer has not been allocated" );
					_timer->timerCancel();
				}
				_current = _transitionMat[ ev_idx ][ SPAG_P_CAST2IDX(_current) ];     // 2 - switch to next state
#ifdef SPAG_ENABLE_LOGGING
				_rtdata.logTransition( _current, ev_idx );
#endif
				runAction();                                                          // 3 - call the callback function
			}
			else
			{
				SPAG_LOG << "event is ignored\n";
				if( _ignEventCallback )
					_ignEventCallback( _current, ev );

#ifdef SPAG_ENABLE_LOGGING
				_rtdata.logIgnoredEvent( ev_idx );
#endif
			}
		}

#ifdef SPAG_USE_SIGNALS
/// activate inner event: set the inner event to true, so that a signal will be raised when we are on that state (and once callback has been completed).
/// \warning Only available when \ref SPAG_USE_SIGNALS is defined, see manual.
		void activateInnerEvent( EV ev )
		{
			SPAG_LOG << "\n";

/// \todo provide more details in error message
			if( _eventInfo.find( ev ) == _eventInfo.end() )
				throw std::runtime_error( "request for processing inner event but has not been configured" );

			ST st1 = _eventInfo[ ev ];

			assert( _stateInfo[ SPAG_P_CAST2IDX(st1) ]._innerTrans._hasOne );
			_stateInfo[ SPAG_P_CAST2IDX(st1) ]._innerTrans._isActive = true;
		}

/// This is to be called by event loop wrapper, by the signal handler.
/// <strong>DO NOT CALL in user code</strong>
/// \warning Only available when \ref SPAG_USE_SIGNALS is defined, see manual.
		void processInnerEvent( const priv::InnerTransition<ST,EV>& it ) const
		{
			SPAG_P_ASSERT( _isRunning, "attempting to process a inner event but FSM is not started" );
			_current = it._destState;
#ifdef SPAG_ENABLE_LOGGING
				_rtdata.logTransition( _current, it._innerEvent );
#endif
				runAction();                                                          // 3 - call the callback function
		}
#endif // SPAG_USE_SIGNALS
///@}

/** \name Misc. helper functions */
///@{
/// Does configuration checks
		void doChecking() const;
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
		priv::StateInfo<ST,EV,CBA> getStateInfo( size_t idx ) const
		{
			assert( idx < nbStates() );
			return _stateInfo[idx];
		}
/// Return duration of time out for state \c st, or 0 if none
		std::pair<Duration,DurUnit> timeOutDuration( ST st ) const
		{
			assert( SPAG_P_CAST2IDX(st) < nbStates() );
			return std::make_pair(
				_stateInfo[ SPAG_P_CAST2IDX(st) ]._timerEvent._duration,
				_stateInfo[ SPAG_P_CAST2IDX(st) ]._timerEvent._durUnit
			);
		}

		void printConfig( std::ostream& str, const char* msg=nullptr ) const;

#ifdef SPAG_ENABLE_LOGGING
		void setLogFilename( std::string fn ) const
		{
			assert( !fn.empty() );
			_rtdata._logfileName = fn;
		}
/// Print dynamic data to \c str
		void printLoggedData( std::ostream& str, PrintFlags pf=PrintFlags::all ) const
		{
			_rtdata.printData( str, pf );
		}
#else
		void printLoggedData( std::ostream&, PrintFlags pf=PrintFlags::all ) const {}
		void setLogFilename( std::string fn ) const {}
#endif

		void setTimerDefaultUnit( DurUnit unit ) const
		{
			static_assert( std::is_same<TIM,priv::NoTimer<ST,EV,CBA>>::value == false, "ERROR, FSM build without timer" );
            _defaultTimerUnit = unit;
		}
/// Provided so that we can use this in a function templated with the FSM type, without having the type \c DurUnit available
/// (see for example in src/traffic_lights_common.hpp)
		void setTimerDefaultUnit( std::string str ) const
		{
			static_assert( std::is_same<TIM,priv::NoTimer<ST,EV,CBA>>::value == false, "ERROR, FSM build without timer" );
			auto tu = priv::timeUnitFromString( str );
			if( !tu.first )
				SPAG_P_THROW_ERROR_CFG( "invalid string value: " + str );
			_defaultTimerUnit = tu.second;
		}

/// Returns the build options
/**
Usage (example): <code>std::cout << fsm_t::buildOptions();<code>
*/
		static std::string buildOptions()
		{
			std::string yes(" = yes\n"), no(" = no\n");
			std::string out( "Spaghetti version " );
			out += SPAG_STRINGIZE( SPAG_VERSION );
			out += "\nBuild options:\n";

			out += SPAG_P_STRINGIZE2( SPAG_USE_ASIO_TIMER );
#ifdef SPAG_USE_ASIO_TIMER
			out += yes;
#else
			out += no;
#endif
			out += SPAG_P_STRINGIZE2( SPAG_EMBED_ASIO_TIMER );
#ifdef SPAG_EMBED_ASIO_TIMER
			out += yes;
#else
			out += no;
#endif
			out += SPAG_P_STRINGIZE2( SPAG_USE_SIGNALS );
#ifdef SPAG_USE_SIGNALS
			out += yes;
#else
			out += no;
#endif
			out += SPAG_P_STRINGIZE2( SPAG_EXTERNAL_EVENT_LOOP );
#ifdef SPAG_EXTERNAL_EVENT_LOOP
			out += yes;
#else
			out += no;
#endif
			out += SPAG_P_STRINGIZE2( SPAG_ENABLE_LOGGING );
#ifdef SPAG_ENABLE_LOGGING
			out += yes;
#else
			out += no;
#endif
			out += SPAG_P_STRINGIZE2( SPAG_PRINT_STATES );
#ifdef SPAG_PRINT_STATES
			out += yes;
#else
			out += no;
#endif
			out += SPAG_P_STRINGIZE2( SPAG_FRIENDLY_CHECKING );
#ifdef SPAG_FRIENDLY_CHECKING
			out += yes;
#else
			out += no;
#endif
			out += SPAG_P_STRINGIZE2( SPAG_ENUM_STRINGS );
#ifdef SPAG_ENUM_STRINGS
			out += yes;
#else
			out += no;
#endif
			return out;
		}

#ifdef SPAG_GENERATE_DOTFILE
/// Generates in current folder a dot file corresponding to the FSM
		void writeDotFile( std::string fn, DotFileOptions opt=DotFileOptions() ) const;
#else
		void writeDotFile( std::string, DotFileOptions ) const {}
#endif
///@}

///////////////////////////////////
// private member function section
///////////////////////////////////

	private:
/// Run associated action with a state switch (state has already switched)
/**
-# first, starts timer, if needed (first, because running callback can take some time).
-# second, calls callback function, if any.
-# third, raises a signal if a deferred action has been requested on this state (only if signals have been enabled, see manual).
*/
		void runAction() const
		{
			SPAG_LOG << "switching to state " << SPAG_P_CAST2IDX(_current) << ", starting action\n";
			auto curr_idx = SPAG_P_CAST2IDX(_current);
			auto stateInfo = _stateInfo[ curr_idx ];

			if( stateInfo._timerEvent._enabled )
			{
				SPAG_P_ASSERT( _timer, "Timer has not been allocated" );
				SPAG_LOG << "timeout enabled, duration=" <<  stateInfo._timerEvent._duration << "\n";
				_timer->timerStart( this );
			}
			if( stateInfo._callback ) // if there is a callback stored, then call it
			{
				SPAG_LOG << "callback function start:\n";
				stateInfo._callback( _stateInfo[ SPAG_P_CAST2IDX(_current) ]._callbackArg );
			}
			else
				SPAG_LOG << "no callback provided\n";

#ifdef SPAG_USE_SIGNALS
			const auto& si_it = stateInfo._innerTrans;
			if( si_it._isPassState || ( si_it._hasOne && si_it._isActive ) )
			{
//				std::cout << "stateInfo._hasInternalTransition => raise signal!\n";
				_timer->raiseSignal();
				_timer->timerCancel();
			}
#endif
		}

		void printMatrix( std::ostream& str ) const;
		bool isReachable( size_t ) const;
		std::string getConfigErrorMessage( priv::EN_ConfigError ce, size_t st ) const;

/////////////////////////////
// private data section
/////////////////////////////

	private:
#ifdef SPAG_ENABLE_LOGGING
		mutable priv::RunTimeData<ST,EV> _rtdata;
#endif
		mutable ST                       _current = static_cast<ST>(0);   ///< current state
		mutable bool                     _isRunning = false;
		mutable DurUnit                  _defaultTimerUnit = DurUnit::sec;   ///< default timer units

		mutable TIM*                     _timer = nullptr;   ///< pointer on timer

#ifdef SPAG_USE_ARRAY
		std::array<
			std::array<ST, static_cast<size_t>(ST::NB_STATES)>,
			static_cast<size_t>(EV::NB_EVENTS)
		> _transitionMat;  ///< describe what states the fsm switches to, when a message is received. lines: events, columns: states, value: states to switch to. DOES NOT hold timer events
		std::array<
			std::array<char, static_cast<size_t>(ST::NB_STATES)>,
			static_cast<size_t>(EV::NB_EVENTS)
		> _allowedMat;  ///< matrix holding for each event a boolean telling is the event is ignored or not, for a given state (0:ignore event, 1:handle event)
#else
		std::vector<std::vector<ST>>       _transitionMat;  ///< describe what states the fsm switches to, when a message is received. lines: events, columns: states, value: states to switch to. DOES NOT hold timer events
		std::vector<std::vector<char>>     _allowedMat;  ///< matrix holding for each event a boolean telling is the event is ignored or not, for a given state (0:ignore event, 1:handle event)
#endif // SPAG_USE_ARRAY

#ifdef SPAG_USE_ARRAY
		std::array<priv::StateInfo<ST,EV,CBA>,static_cast<size_t>(ST::NB_STATES)>  _stateInfo;         ///< Holds for each state the details
#else
		std::vector<priv::StateInfo<ST,EV,CBA>>  _stateInfo;         ///< Holds for each state the details
#endif

#ifdef SPAG_USE_SIGNALS
		std::map<EV,ST>   _eventInfo; ///< holds for inner event the state it is assigned to
#endif

#ifdef SPAG_ENUM_STRINGS
		std::vector<std::string>           _strEvents;      ///< holds events strings
		std::vector<std::string>           _strStates;      ///< holds states strings
#endif

#ifdef SPAG_EMBED_ASIO_TIMER
		AsioWrapper<ST,EV,CBA>             _asioWrapper; ///< optional wrapper around boost::asio::io_service
#endif

		std::function<void(ST,EV)> _ignEventCallback;     ///< ignored events callback function

};
//-----------------------------------------------------------------------------------
namespace priv
{
//-----------------------------------------------------------------------------------
inline
void
printChars( std::ostream& out, size_t n, char c )
{
	for( size_t i=0; i<n; i++ )
		out << c;
}
//-----------------------------------------------------------------------------------

} // namespace priv

//-----------------------------------------------------------------------------------
/// Configuration error printing function
template<typename ST, typename EV,typename T,typename CBA>
std::string
SpagFSM<ST,EV,T,CBA>::getConfigErrorMessage( priv::EN_ConfigError ce, size_t st ) const
{
	std::string msg( priv::getSpagName() + "configuration error: state " );
	msg += std::to_string( st );
#ifdef SPAG_ENUM_STRINGS
	msg += " '";
	msg += _strStates[st];
	msg += "'";
#endif
	msg += ' ';

	switch( ce )
	{
		case priv::CE_TimeOutAndPassState:
			msg += "cannot have both a timeout and a pass-state flag";
		break;
		case priv::CE_IllegalPassState:
			msg += "cannot be followed by another pass-state";
		break;
		case priv::CE_SamePassState:
			msg += "pass-state cannot lead to itself";
		break;
		default: assert(0);
	}
	return msg;
}

//-----------------------------------------------------------------------------------
/// helper function template for printConfig()
template<typename ST, typename EV,typename T,typename CBA>
void
SpagFSM<ST,EV,T,CBA>::printMatrix( std::ostream& out ) const
{
	size_t maxlength(0);
#ifdef SPAG_ENUM_STRINGS
	maxlength = priv::getMaxLength( _strEvents );
#endif

	char spc_char{ ' ' };

	priv::printChars( out, maxlength, spc_char );
	out << "       STATES:\n        ";
	priv::printChars( out, maxlength, spc_char );
	for( size_t i=0; i<nbStates(); i++ )
		out << i << "  ";
	out << "\n-----";
	priv::printChars( out, maxlength, '-' );
	out << '|';
	for( size_t i=0; i<nbStates(); i++ )
		out << "---";
	out << '\n';

	auto nbLines = nbEvents()+1; // +1 for timeout events
#ifdef SPAG_USE_SIGNALS
	nbLines++;                   // +1 for pass-states
#endif

#ifndef SPAG_ENUM_STRINGS
	std::string capt( "EVENTS" );
//	nbLines = std::max( capt.size(), nbEvents()+2 );
	nbLines = std::max( capt.size(), nbLines );
#endif

	for( size_t i=0; i<nbLines; i++ )
	{
#ifdef SPAG_ENUM_STRINGS
		if( maxlength )
			priv::PrintEnumString( out, _strEvents[i], maxlength );
#else
		if( i<capt.size() )
			out << capt[i];
		else
#endif
			out << spc_char;

		if( i<nbEvents() )
		{
			out << spc_char << std::setw(2) << i << " | ";

			for( size_t j=0; j<nbStates(); j++ )
			{
				if( _allowedMat[i][j] ) //&& !_stateInfo[j]._innerTrans._isPassState )
					out << std::setw(2) << _transitionMat[i][j];
				else
					out << " .";
				out << spc_char;
			}

		}
		if( i == nbEvents() ) // TimeOut events
		{
			out << "    | ";
			for( size_t j=0; j<nbStates(); j++ )
			{
				if( _stateInfo[j]._timerEvent._enabled )
					out << std::setw(2) << _stateInfo[j]._timerEvent._nextState;
				else
					out << " .";
				out << spc_char;
			}
		}
#ifdef SPAG_USE_SIGNALS
		if( i == nbEvents()+1 ) // Pass-state
		{
			out << "    | ";
			for( size_t j=0; j<nbStates(); j++ )
			{
				if( _stateInfo[j]._innerTrans._isPassState )
					out << std::setw(2) << _transitionMat[0][j];
				else
					out << " .";
				out << spc_char;
			}
		}
#endif
		out << '\n';
	}
}
//-----------------------------------------------------------------------------------
/// Helper function, returns true if state \c st is referenced in \c _transitionMat (and that the transition is allowed)
/// or it has a Timeout or pass-state transition
template<typename ST, typename EV,typename T,typename CBA>
bool
SpagFSM<ST,EV,T,CBA>::isReachable( size_t st ) const
{
	for( size_t i=0; i<nbStates(); i++ )
		if( i != st )
		{
			for( size_t k=0; k<nbEvents(); k++ )
				if( SPAG_P_CAST2IDX( _transitionMat[k][i] ) == st )
					if( _allowedMat[k][i] == 1 )
						return true;

			if( _stateInfo[i]._timerEvent._enabled )
				if( SPAG_P_CAST2IDX( _stateInfo[i]._timerEvent._nextState ) == st )
					return true;
		}

	return false;
}
//-----------------------------------------------------------------------------------
/// Checks configuration for any illegal situation. Throws error if one is encountered.
/// \todo correct: some states are flagged as unreachable, although they can be reached through inner events
template<typename ST, typename EV,typename T,typename CBA>
void
SpagFSM<ST,EV,T,CBA>::doChecking() const
{
/*
	for( size_t i=0; i<nbStates(); i++ )
	{
		auto state = _stateInfo[i];
		if( state._isPassState )
		{
			size_t nextState = SPAG_P_CAST2IDX( _transitionMat[0][i] );
			if( nextState == i )
				SPAG_P_THROW_ERROR_CFG( getConfigErrorMessage( priv::CE_SamePassState, i ) );

//			if( _stateInfo[ nextState ]._isPassState )
//				SPAG_P_THROW_ERROR_CFG( getConfigErrorMessage( priv::CE_IllegalPassState, i ) );

			if( state._timerEvent._enabled )
				SPAG_P_THROW_ERROR_CFG( getConfigErrorMessage( priv::CE_TimeOutAndPassState, i ) );
		}
	}
*/
// check for unreachable states
	std::vector<size_t> unreachableStates;
	for( size_t i=1; i<nbStates(); i++ )         // we start from index 1, because 0 is the initial state, and thus is always reachable!
		if( !isReachable( i ) )
			unreachableStates.push_back( i );
	for( const auto& st: unreachableStates )
	{
		std::cout << priv::getSpagName() << "Warning, state " << st
#ifdef SPAG_ENUM_STRINGS
			<< " (" << _strStates[st] << ')'
#endif
			<< " is unreachable\n";
	}

	for( size_t i=0; i<nbStates(); i++ ) // check for any dead-end situations
	{
		bool foundValid(false);
		if( _stateInfo[i]._timerEvent._enabled )
			foundValid = true;
		else
		{
			for( size_t j=0; j<nbEvents(); j++ )
				if( SPAG_P_CAST2IDX( _transitionMat[j][i] ) != i )   // if the transition leads to another state
					if( _allowedMat[j][i] == 1 )                  // AND it is allowed
						foundValid = true;
		}

		if( !foundValid )                     // if we didn't find a valid transition
			if( std::find(
				unreachableStates.begin(),
				unreachableStates.end(),
				i
			) == unreachableStates.end() )     // AND it is not in the unreachable states list
		{
			std::cout << priv::getSpagName() << "Warning, state " << i
#ifdef SPAG_ENUM_STRINGS
				<< " (" << _strStates[i] << ')'
#endif
				<< " is a dead-end\n";
		}
	}
}
//-----------------------------------------------------------------------------------
/// Printing function, prints transition table
template<typename ST, typename EV,typename T,typename CBA>
void
SpagFSM<ST,EV,T,CBA>::printConfig( std::ostream& out, const char* msg  ) const
{
	out << "\n* FSM Configuration: ";
	if( msg )
		out << msg;
	out << "\n - Transition table:\n";
	printMatrix( out );

#ifdef SPAG_ENUM_STRINGS
	size_t maxlength = priv::getMaxLength( _strStates );
#endif
	out << "\n - State info:\n";
	for( size_t i=0; i<nbStates(); i++ )
	{
		const auto& te = _stateInfo[i]._timerEvent;
		out << i;
#ifdef SPAG_ENUM_STRINGS
		out << ':';
		priv::PrintEnumString( out, _strStates[i], maxlength );
#endif
		out << "| ";
		if( te._enabled )
		{
			out << te._duration << ' ' << priv::stringFromTimeUnit( te._durUnit ) << " => St." << te._nextState;
#ifdef SPAG_ENUM_STRINGS
			out << " (";
			priv::PrintEnumString( out, _strStates[te._nextState], maxlength );
			out << ')';
#endif
		}
		else
		{
#ifdef SPAG_USE_SIGNALS
			if( _stateInfo[i]._innerTrans._isPassState )
			{
				out << "AAT => St." << _transitionMat[0][i];
#ifdef SPAG_ENUM_STRINGS
				out << " (";
				priv::PrintEnumString( out, _strStates[_transitionMat[0][i]], maxlength );
				out << ')';
#endif
			}
			else
#endif
				out << '-';
		}
		out << '\n';
	}
	out << "---------------------\n";
}
//-----------------------------------------------------------------------------------
#ifdef SPAG_GENERATE_DOTFILE
/// Saves in current folder a .dot file of the FSM, to be rendered with Graphviz
template<typename ST, typename EV,typename T,typename CBA>
void
SpagFSM<ST,EV,T,CBA>::writeDotFile( std::string fname, DotFileOptions opt ) const
{
	std::ofstream f( fname );
	if( !f.is_open() )
		SPAG_P_THROW_ERROR_RT( "error, unable to open file: " + fname );
	f << "/*\nGenerated by Spaghetti library\nhttps://github.com/skramm/spaghetti\n"
		<< "Version " << SPAG_STRINGIZE(SPAG_VERSION) << "\n*/\n";
	f << "digraph G {\n"
		<< "rankdir=LR;\n"
		<< "edge[style=\"bold\"];\n"
		<< "node[shape=\"circle\"];\n";

	f << "\n/* States */\n";
	for( size_t j=0; j<nbStates(); j++ )
	{
		f << j << " [label=\"S" << j << "\"";
		if( j == 0 )                                // initial state
			f << ",shape=doublecircle";
		if( opt.showActiveState )
			if( SPAG_P_CAST2IDX( currentState() ) == j )
				f << ",style=filled,fillcolor=black,fontcolor=white";
		f << "];\n";
	}
	f << "\n/* External events */\n";
	for( size_t i=0; i<nbEvents(); i++ )
		for( size_t j=0; j<nbStates(); j++ )
			if( _allowedMat[i][j] )
#ifdef SPAG_USE_SIGNALS
				if( !_stateInfo[j]._innerTrans._isPassState )
#endif
					f << j << " -> " << _transitionMat[i][j] << " [label=\"E" << i << "\"];\n";

	f << "\n/* Inner events and timeout transitions */\n";
	for( size_t j=0; j<nbStates(); j++ )
	{
		const auto& te = _stateInfo[j]._timerEvent;
		if( te._enabled )
			f << j << " -> " << te._nextState
				<< " [label=\"TO:"
				<< te._duration
				<< priv::stringFromTimeUnit( te._durUnit )
				<< "\"];\n";
#ifdef SPAG_USE_SIGNALS
		const auto& itr = _stateInfo[j]._innerTrans;
		if( itr._isPassState )
			f << j << " -> " << _transitionMat[0][j] << " [label=\"AAT\"];\n";
		if( itr._hasOne )
			f << j << " -> " << SPAG_P_CAST2IDX( itr._destState )
				<< " [label=\"IE" << SPAG_P_CAST2IDX( itr._innerEvent ) << "\"];\n";
#endif
	}
	f << "}\n";
}

#endif // SPAG_GENERATE_DOTFILE

//-----------------------------------------------------------------------------------
namespace priv {

/// Dummy type, used if no timer requested by user
template<typename ST, typename EV,typename CBA>
struct NoTimer
{
	void timerStart( const SpagFSM<ST,EV,NoTimer,CBA>* ) {}
	void timerInit(  const SpagFSM<ST,EV,NoTimer,CBA>* ) {}
	void timerCancel() {}
	void raiseSignal() {}
};

} // namespace priv

//-----------------------------------------------------------------------------------

#if defined (SPAG_USE_ASIO_TIMER)

//-----------------------------------------------------------------------------------
/// Wraps the boost::asio stuff to have an asynchronous timer easily available
/**
Rationale: holds a timer, created by constructor. It can then be used without having to create one explicitely.
That last point isn't that obvious, has it also must have a lifespan not limited to some callback function.

For timer duration, see
http://en.cppreference.com/w/cpp/chrono/duration
*/
template<typename ST, typename EV, typename CBA>
struct AsioWrapper
{
	private:

// if external io_service, then we only hold a reference on it
#ifdef SPAG_EXTERNAL_EVENT_LOOP
	boost::asio::io_service& _io_service;
#else
	boost::asio::io_service _io_service;
#endif

#if BOOST_VERSION < 106600
/// see http://www.boost.org/doc/libs/1_54_0/doc/html/boost_asio/reference/io_service.html
/// "Stopping the io_service from running out of work" at bottom of page
	boost::asio::io_service::work _work;
#endif

	typedef boost::asio::basic_waitable_timer<std::chrono::steady_clock> SteadyClock;

	std::unique_ptr<SteadyClock> _asioTimer; ///< pointer on timer, will be allocated in constructor

#ifdef SPAG_USE_SIGNALS
	boost::asio::signal_set      _signals;
#endif

	public:
/// Constructor

#ifdef SPAG_EXTERNAL_EVENT_LOOP
	AsioWrapper( boost::asio::io_service& io ) : _io_service(io), _work( _io_service )
#else
	AsioWrapper() : _work( _io_service )
#endif

#ifdef SPAG_USE_SIGNALS
	, _signals( _io_service, SIGUSR1 )
#endif
	{
// see http://www.boost.org/doc/libs/1_66_0/doc/html/boost_asio/reference/io_service.html
#if BOOST_VERSION >= 106600
//		std::cout << "Boost >= 1.66, started executor_work_guard\n";
		boost::asio::executor_work_guard<boost::asio::io_context::executor_type> = boost::asio::make_work_guard( _io_service );
#endif
		_asioTimer = std::unique_ptr<SteadyClock>( new SteadyClock(_io_service) );

//		_signals.add( SIGUSR1 );                               // assign signal handler for SIGUSR1
	}

	AsioWrapper( const AsioWrapper& ) = delete; // non copyable

	boost::asio::io_service& get_io_service()
	{
		return _io_service;
	}

/// Mandatory function for SpagFSM. Called only once, when FSM is started
	void timerInit( const spag::SpagFSM<ST,EV,AsioWrapper,CBA>* fsm )
	{
		SPAG_LOG << '\n';
#ifdef SPAG_USE_SIGNALS
		_signals.async_wait(            // initialize the signal handler, for deferred events
			boost::bind(
				&AsioWrapper<ST,EV,CBA>::signalHandler,
				this,
				boost::asio::placeholders::error,
				boost::asio::placeholders::signal_number,
				fsm
			)
		);
#endif
		_io_service.run();          // blocking call !!!
	}

	void timerKill()
	{
		SPAG_LOG << '\n';
#ifdef SPAG_USE_SIGNALS
		_signals.clear();
#endif
		_io_service.stop();
	}

/// Timer callback function, called when timer expires.
	void timerCallback( const boost::system::error_code& err_code, const spag::SpagFSM<ST,EV,AsioWrapper,CBA>* fsm  )
	{
		SPAG_LOG << '\n';
		switch( err_code.value() ) // check if called because of timeout, or because of canceling timeout operation
		{
			case boost::system::errc::operation_canceled:    // do nothing
				SPAG_LOG << "err_code=operation_canceled\n";
			break;
			case 0:
				fsm->processTimeOut();                    // normal operation: timer has expired
			break;
			default:                                         // all other values
				SPAG_P_THROW_ERROR_RT( "boost::asio timer unexpected error: " + err_code.message() );
		}
	}
/// Mandatory function for SpagFSM. Cancel the pending async timer
	void timerCancel()
	{
		SPAG_LOG << '\n';
		_asioTimer->cancel_one();
	}

/// Start timer. Instanciation of mandatory function for SpagFSM
	void timerStart( const spag::SpagFSM<ST,EV,AsioWrapper,CBA>* fsm )
	{
		auto duration = fsm->timeOutDuration( fsm->currentState() );
		SPAG_LOG << "Starting timer with duration=" << duration.first << '\n';
		switch( duration.second )
		{
			case DurUnit::ms:
				_asioTimer->expires_from_now( std::chrono::milliseconds(duration.first) );
			break;
			case DurUnit::sec:
				_asioTimer->expires_from_now( std::chrono::seconds(duration.first) );
			break;
			case DurUnit::min:
				_asioTimer->expires_from_now( std::chrono::minutes(duration.first) );
			break;
			default: assert(0); // this should not happen...
		}

		_asioTimer->async_wait(
			boost::bind(
				&AsioWrapper<ST,EV,CBA>::timerCallback,
				this,
				boost::asio::placeholders::error,
				fsm
			)
		);
	}

#ifdef SPAG_USE_SIGNALS
/// This is a handler, automatically called by boost::io_service when an OS signal USR1 is detected.
/// \warning Only available when \ref SPAG_USE_SIGNALS is defined, see manual.
	void signalHandler( const boost::system::error_code& error, int signal_number, const spag::SpagFSM<ST,EV,AsioWrapper,CBA>* fsm )
	{
//		std::cout << "handling signal " << signal_number << " errorcode=" << error.message() << " current=" << fsm->currentState() << std::endl;
		auto st_idx = SPAG_P_CAST2IDX( fsm->currentState() );
		const auto& stateInfo = fsm->getStateInfo( st_idx );
//		std::cout << "signal handler, processing " << stateInfo._innerTrans;
		assert( ( stateInfo._innerTrans._hasOne && stateInfo._innerTrans._isActive ) || stateInfo._innerTrans._isPassState );
		fsm->processInnerEvent( stateInfo._innerTrans );

		_signals.async_wait(                                   // re-initialize signal handler
			boost::bind(
				&AsioWrapper<ST,EV,CBA>::signalHandler,
				this,
				boost::asio::placeholders::error,
				boost::asio::placeholders::signal_number,
				fsm
			)
		);
	}
	void raiseSignal()
	{
		SPAG_LOG << '\n';
		std::raise( SIGUSR1 );
	}
#endif
};

#endif // SPAG_USE_ASIO_TIMER

//-----------------------------------------------------------------------------------

} // namespace spag

/// Shorthand for declaring the type of FSM, without a timer
#ifdef SPAG_USE_ASIO_TIMER
	#define SPAG_DECLARE_FSM_TYPE_NOTIMER( type, st, ev, cbarg ) \
		static_assert( 0, "Error, can't use this macro with symbol SPAG_USE_ASIO_TIMER defined" )
#else
	#define SPAG_DECLARE_FSM_TYPE_NOTIMER( type, st, ev, cbarg ) \
		typedef spag::SpagFSM<st,ev,spag::priv::NoTimer<st,ev,cbarg>,cbarg> type
#endif
/// Shorthand for declaring the type of FSM with an arbitrary timer class
#define SPAG_DECLARE_FSM_TYPE( type, st, ev, timer, cbarg ) \
	typedef spag::SpagFSM<st,ev,timer<st,ev,cbarg>,cbarg> type

#ifdef SPAG_USE_ASIO_TIMER
	#ifdef SPAG_EMBED_ASIO_TIMER
/// Shorthand for declaring the type of FSM with the provided Boost::asio timer class. Does not create the \c AsioTimer type (user code doesn't need it)
		#define SPAG_DECLARE_FSM_TYPE_ASIO( type, st, ev, cbarg ) \
			typedef spag::SpagFSM<st,ev,spag::AsioWrapper<st,ev,cbarg>,cbarg> type
	#else
/// Shorthand for declaring the type of FSM with the provided Boost::asio timer class. Also creates the \c AsioTimer type
		#define SPAG_DECLARE_FSM_TYPE_ASIO( type, st, ev, cbarg ) \
			typedef spag::SpagFSM<st,ev,spag::AsioWrapper<st,ev,cbarg>,cbarg> type; \
			namespace spag { \
				typedef AsioWrapper<st,ev,cbarg> AsioTimer; \
			}
	#endif
#else
	#define SPAG_DECLARE_FSM_TYPE_ASIO( type, st, ev, cbarg ) \
		static_assert( 0, "Error, can't use this macro without symbol SPAG_EMBED_ASIO_TIMER defined" )
#endif

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


\todo for enum to string automatic conversion, maybe use this ? :
https://github.com/aantron/better-enums


\todo add some tests, and write a sample to evaluation performance

\todo Currently works using std::array as storage (see SPAG_USE_ARRAY).
Shall we switch permanently ?
If so, we will NOT be able to add states dynamically (not possible at present, but could be in future releases,
if storage remains std::vector

*/
