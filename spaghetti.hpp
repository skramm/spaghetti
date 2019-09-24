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

/// At present, data is stored into arrays if this is defined. \todo Need performance evaluation of this build option.
/// If not defined, it defaults to std::vector
#define SPAG_USE_ARRAY

#define SPAG_VERSION "0.9.5"

#include <vector>
#include <map>
#include <algorithm>
#include <functional>
#include <cassert>
#include <iomanip>
#include <fstream>
#include <iostream> // needed for expansion of SPAG_LOG


#if defined (SPAG_EMBED_ASIO_WRAPPER)
	#define SPAG_USE_ASIO_WRAPPER
#endif

#if defined (SPAG_USE_ASIO_WRAPPER)
	#include <boost/bind.hpp>
	#include <boost/asio.hpp>
#endif

#if defined (SPAG_USE_ASIO_WRAPPER) || defined (SPAG_ENABLE_LOGGING)
	#include <chrono>
#endif

#ifdef SPAG_PRINT_STATES
	#define SPAG_LOG \
		if(1) \
			std::cout << spag::priv::getSpagName() << __FUNCTION__ << "(): "
	#define SPAG_LOG_FLUSH std::cout << std::endl
#else
	#define SPAG_LOG \
		if(0) \
			std::cout
	#define SPAG_LOG_FLUSH ;
#endif

#ifdef SPAG_NO_VERBOSE
	#define SPAG_P_LOG_ERROR \
		if(0) \
			std::cerr
#else
	#define SPAG_P_LOG_ERROR \
		if(1) \
			std::cerr << spag::priv::getSpagName() << __FUNCTION__ << "(): "
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
				<< "(), line:" << __LINE__ \
				<< ", condition \"" << #a << "\" is false, " << msg << '\n'; \
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

#define SPAG_NOT_AVAILABLE(a) \
	{ \
		SPAG_P_LOG_ERROR << "This function is not available when symbol " << SPAG_STRINGIZE(a) << " is not defined, see manual.\n"; \
		assert(0); \
	}

#ifdef SPAG_TRACK_RUNTIME
int g_indent;
#define SPAG_P_START \
	static int funct_count; \
	{ \
		for( char i=0; i<3*g_indent; i++ ) \
			std::cout << '-'; \
		std::cout << "START " << __FUNCTION__ << "(): " << funct_count << std::endl; \
		g_indent++; \
		funct_count++; \
	}

#define SPAG_P_END \
	{ \
		g_indent--; \
		assert( g_indent >=0 ); \
		for( char i=0; i<3*g_indent; i++ ) \
			std::cout << '-'; \
		std::cout << "END  " << __FUNCTION__ << "(): " << funct_count << std::endl; \
	}

#else
	#define SPAG_P_START ;
	#define SPAG_P_END ;
#endif // SPAG_TRACK_RUNTIME

/// Private macro, used to convert a 'state' type into an integer
#define SPAG_P_CAST2IDX( a ) static_cast<size_t>(a)


// TEMP
typedef size_t Duration;

/// Main library namespace
namespace spag {

//------------------------------------------------------------------------------------
/// Used in \ref SpagFSM<>::Counters::print() as second argument and in Counters::getValue()
enum Item : uint8_t
{
	ItemStates  = 0x01
	,ItemEvents = 0x02
	,ItemIgnoredEvents = 0x04
};

/// Timer units
enum class DurUnit : char { ms, sec, min };

namespace priv {
// forward declaration
template<typename T, typename U>
struct RunTimeData;
};

//-----------------------------------------------------------------------------------
#ifdef SPAG_ENABLE_LOGGING
/// States and events counters, independent struct.
/**
If strings enabled, then we pass these to the constructor, else we only pass the number of states and events
*/
struct Counters
{
	template<typename T1,typename T2>
	friend struct priv::RunTimeData;

#ifdef SPAG_ENUM_STRINGS
	Counters( const std::vector<std::string>& strStates, const std::vector<std::string>& strEvents )
		: _strStates( strStates )
		, _strEvents( strEvents )
	{
		auto nb_states = strStates.size();
		auto nb_events = strEvents.size();
#else
	Counters( size_t nb_states, size_t nb_events )
	{
#endif
		assert( nb_states ); /// \todo remove this once tested
		assert( nb_events );

		_stateCounter.resize( nb_states );
		_eventCounter.resize( nb_events );
		_ignoredEventCounter.resize( nb_events-2 );  // because we don't need the last two elements
	}

	void print(
		std::ostream& out=std::cout,
		uint8_t flags = ItemStates + ItemEvents + ItemIgnoredEvents,
		char sep = ';'
	) const;

	size_t getValue( Item what, size_t index )
	{
		switch( what )
		{
			case ItemStates:
				return _stateCounter.at(index);
			break;
			case ItemEvents:
				return _eventCounter.at(index);
			break;
			case ItemIgnoredEvents:
				return _ignoredEventCounter.at(index);
			break;
			default: assert(0);
		}
	}

	private:
		std::vector<size_t> _stateCounter;   ///< per state counter
		std::vector<size_t> _eventCounter;   ///< per event counter
		std::vector<size_t> _ignoredEventCounter;  ///< ignored events counter. No need to do "+2" as here, time outs and AAT will never be counted as ignored

#ifdef SPAG_ENUM_STRINGS
		const std::vector<std::string> _strStates;
		const std::vector<std::string> _strEvents;
#endif
};
#endif // SPAG_ENABLE_LOGGING

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
	if( str == "msec" )
		return std::make_pair( true, DurUnit::ms );
	if( str == "sec" )
		return std::make_pair( true, DurUnit::sec );
	if( str == "min" )
		return std::make_pair( true, DurUnit::min );
	if( str == "mn" )
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
//	static std::string str("Spaghetti " + std::string(SPAG_VERSION) + ": "); // REMOVED ON 2019-07-10: will cause a break in tests at each new version !
	static std::string str("Spaghetti: ");
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
/**
The type StateInfo (one for every state) hold a vector of these: each state can handle several InnerTransition
*/
template<typename ST,typename EV>
struct InnerTransition
{
	ST _destState;
	EV _innerEvent;

	InnerTransition( EV ev, ST st ) : _destState(st), _innerEvent(ev)
	{}
	bool operator == ( const InnerTransition& it ) const
	{
		if( _destState != it._destState )
			return false;
		if( _innerEvent != it._innerEvent )
			return false;
		return true;
	}
	friend std::ostream& operator << ( std::ostream& s, const InnerTransition& it )
	{
		s << "InnerTransition:"
			<< " destState=" << (int)it._destState
			<< " innerEvent=" << (int)it._innerEvent;
		return s;
	}
};
#endif // SPAG_USE_SIGNALS
//-----------------------------------------------------------------------------------
/// Private class, holds informations about a state. The FSM holds one of these for every state.
template<typename ST,typename EV,typename CBA>
struct StateInfo
{
	TimerEvent<ST>           _timerEvent;   ///< Holds the information on timeout
	std::function<void(CBA)> _callback;     ///< callback function
	CBA                      _callbackArg;  ///< value of argument of callback function

#ifdef SPAG_USE_SIGNALS
	bool                     _isPassState = false; ///<if true, the next state is stored in transition table, at line nbEvents()+1
	std::vector<InnerTransition<ST,EV>> _innerTransList;

	friend std::ostream& operator << ( std::ostream& s, const StateInfo& si )
	{
		s << "StateInfo:"
			<< "\n -has callback=" << (si._callback==0?"NO":"YES")
			<< "\n -callbackArg=" << si._callbackArg

			<< "\n -isPassState=" << si._isPassState
			<< "\n -NbInnerTransition=" << si._innerTransList.size()
			<< '\n';
		for( const auto& it: si._innerTransList )
			s << "  -" << it << '\n';
		return s;
	}

/// Returns true if the set of internal transitions holds one with event \c ev leading to state \c st
	bool holdsInnerTransition( EV ev, ST st ) const
	{
		if(
			std::find(
				std::begin( _innerTransList ),
				std::end(   _innerTransList ),
				InnerTransition<ST,EV>( ev, st )
			)
			== std::end( _innerTransList )
		)
			return false;
		return true;
	}

#if 0 // unused
/// Returns true if the set of internal transitions holds one using event \c ev
	bool holdsInnerEvent( EV ev ) const
	{
		for( const auto& it: _innerTransList )
			if( it._innerEvent == ev )
				return true;
		return false;
	}
#endif

	typename std::vector<InnerTransition<ST,EV>>::iterator
	findInnerEvent( EV ev )
	{
		for(
			auto it=std::begin(_innerTransList);
			it != std::end(_innerTransList);
			++it
		)
			if( it->_innerEvent == ev )
				return it;
		return std::end( _innerTransList );
	}
#endif // SPAG_USE_SIGNALS
};
//-----------------------------------------------------------------------------------
/// Private, helper function
/**
Used to format nicely with (fixed-spacing font) the configuration data.

This function will print out the string \c str and fill in with spaces until we reach the \c maxlength value
*/
inline
void
PrintEnumString( std::ostream& out, std::string str, size_t maxlength=0 )
{
	assert( !str.empty() );
	assert( !maxlength || str.size() <= maxlength );
	out << str;
	if( maxlength )
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

} // namespace priv

//-----------------------------------------------------------------------------------
#ifdef SPAG_ENABLE_LOGGING
/// Holds the values of the counters, can be fetched with \c fsm.getCounters()
/**
Also holds the string (if option enabled), for nice printing
*/
inline
void
Counters::print( std::ostream& out, uint8_t flags, char sep ) const
{
#ifdef SPAG_ENUM_STRINGS
	auto maxlength_e = priv::getMaxLength( _strEvents );
	auto maxlength_s = priv::getMaxLength( _strStates );
#endif
	if( flags & ItemStates )
	{
		out << "# State counters:\n";
		for( size_t i=0; i<_stateCounter.size(); i++ )
		{
			out << i << sep,

#ifdef SPAG_ENUM_STRINGS
			priv::PrintEnumString( out, _strStates[i], maxlength_s );
			out << sep;
#endif
			out << _stateCounter[i] << '\n';
		}
	}

	if( flags & ItemEvents )
	{
		out << "\n# Event counters:\n";
		for( size_t i=0; i<_eventCounter.size(); i++ )
		{
			out << i << sep;
#ifdef SPAG_ENUM_STRINGS
			priv::PrintEnumString( out, _strEvents[i], maxlength_e );
			out << sep;
#endif
			out << _eventCounter[i] << '\n';
		}
	}

	if( ( flags & ItemIgnoredEvents ) && _ignoredEventCounter.size()>0 )
	{
		out << "\n# Ignored Events counters:\n";
		for( size_t i=0; i<_ignoredEventCounter.size(); i++ )
		{
			out << i << sep;
#ifdef SPAG_ENUM_STRINGS
			priv::PrintEnumString( out, _strEvents[i], maxlength_e );
			out << sep;
#endif
			out << _ignoredEventCounter[i] << '\n';
		}
	}
}
#endif // SPAG_ENABLE_LOGGING

namespace priv {
//------------------------------------------------------------------------------------
/// Holds the FSM dynamic data: current state, and logged data (if enabled at build, see symbol \c SPAG_ENABLE_LOGGING)
#ifdef SPAG_ENABLE_LOGGING
template<typename ST,typename EV>
struct RunTimeData
{
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/// A state-change event, used for logging
	struct StateChangeEvent
	{
		size_t _state;
		size_t _event;   ///< stored as size_t because it will hold values other than the ones in the enum
		std::chrono::duration<double> _elapsed;
	};
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

	public:
#ifdef SPAG_ENUM_STRINGS
	RunTimeData( const std::vector<std::string>& str_events, const std::vector<std::string>& str_states )
		: _strEvents_R(str_events), _strStates_R( str_states )
#else
	RunTimeData()
#endif
	{
		_startTime = std::chrono::high_resolution_clock::now();
		clear();
		_stateCounter[0] = 1; // because we start on state 0, so it starts at 1
	}

	void clear()
	{
		_stateCounter.fill( 0 );
		_eventCounter.fill( 0 );
		_ignoredEventCounter.fill( 0 );
	}
/// Returns a copy of all the counters.
	Counters buildCounters() const
	{
#ifdef SPAG_ENUM_STRINGS
		Counters cnt( _strStates_R, _strEvents_R );
#else
		Counters cnt( _stateCounter.size(), _eventCounter.size() );
#endif

		std::copy( std::begin(_stateCounter),  std::end(_stateCounter),  std::begin(cnt._stateCounter) );
		std::copy( std::begin(_eventCounter),  std::end(_eventCounter),  std::begin(cnt._eventCounter) );
		std::copy( std::begin(_ignoredEventCounter), std::end(_ignoredEventCounter), std::begin(cnt._ignoredEventCounter) );

		return cnt;
	}

/// Logs a transition from current state to state \c st, that was produced by event \c ev
/**
This will both:
- increment the event and state counters
- log the transition in the logfile

Events are passed as \c size_t because we may pass values other than the ones in the enum (timeout and Always Active transitions)
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

			_logfile << "# FSM runtime history\n#index" << _sepChar << "time" << _sepChar << "event-Id" << _sepChar
	#ifdef SPAG_ENUM_STRINGS
				<< "event_string" << _sepChar << "state-Id" << _sepChar << "state_string\n";
	#else
				<< "state-Id\n";
	#endif
		}

		print2LogFile( _logfile, StateChangeEvent{ st_idx, ev_idx, std::chrono::high_resolution_clock::now() - _startTime } );
		_logfile.flush();
	}

	void logIgnoredEvent( size_t ev_idx )
	{
		SPAG_CHECK_LESS( ev_idx, SPAG_P_CAST2IDX(EV::NB_EVENTS) );
		_ignoredEventCounter[ ev_idx ]++;
	}

//////////////////////////////////
// private member function section
//////////////////////////////////

	private:
	void print2LogFile( std::ofstream& f, const StateChangeEvent sce ) const
	{
		f << std::setw(6) << std::setfill('0') << _logIndex++
			<< _sepChar << sce._elapsed.count() << _sepChar << sce._event << _sepChar;
//		std::cout << "c=" << c << '\n';
	#ifdef SPAG_ENUM_STRINGS
		f << _strEvents_R[sce._event] << _sepChar;
	#endif
		f << sce._state << _sepChar;
	#ifdef SPAG_ENUM_STRINGS
		f << _strStates_R[sce._state];
	#endif
		f << '\n';
	}

//////////////////////////////////
// private data section
//////////////////////////////////

	private:
		mutable size_t _logIndex = 0;
		std::array<size_t,static_cast<size_t>(ST::NB_STATES)>   _stateCounter;   ///< per state counter
		std::array<size_t,static_cast<size_t>(EV::NB_EVENTS)+2> _eventCounter;   ///< per event counter
		std::array<size_t,static_cast<size_t>(EV::NB_EVENTS)>   _ignoredEventCounter;  ///< ignored events counter. No need to do "+2" as here, time outs and AAT will never be counted as ignored

		std::chrono::time_point<std::chrono::high_resolution_clock> _startTime;
		std::ofstream _logfile;

	#ifdef SPAG_ENUM_STRINGS
		const std::vector<std::string>& _strEvents_R; ///< reference on vector of strings of events
		const std::vector<std::string>& _strStates_R; ///< reference on vector of strings of states
	#endif

		char _sepChar = ';';          ///< log file separator
	public:
		std::string _logfileName = "spaghetti.csv";
};
#endif // SPAG_ENABLE_LOGGING

//-----------------------------------------------------------------------------------
#ifndef SPAG_USE_ARRAY
/// helper template function (unused if SPAG_USE_ARRAY defined)
template<typename T>
void
resizemat( std::vector<std::vector<T>>& mat, std::size_t nb_lines, std::size_t nb_cols )
{
	mat.resize( nb_lines );
	for( auto& line: mat )
	{
		line.resize( nb_cols );
		for( auto& elem: line )
			elem = static_cast<T>(0);
	}
}
#endif
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

#if defined (SPAG_USE_ASIO_WRAPPER)
// Forward declaration
	template<typename ST, typename EV, typename CBA>
	struct AsioWrapper;
#endif

//-----------------------------------------------------------------------------------
/// Options for printing the dotfile, see SpagFSM::writeDotFile()
struct DotFileOptions
{
	std::string nodeShape = "circle"; ///< Default shape for nodes. See https://www.graphviz.org/doc/info/shapes.html
	bool showActiveState = false;
	bool showTimeOuts    = true;
	bool showInnerEvents = true;
	bool showAAT         = true;
	bool showStateIndex  = true;
	bool showStateString = true;
	bool showEventIndex  = true;
	bool showEventString = true;
	bool showUnreachableStates = true;
	bool fixedNodeWidth  = false;
	std::string nodeWidth = "1.5";  ///< used only if \c fixedNodeWidth is true
	bool useColorsEventType = true;

};

//-----------------------------------------------------------------------------------
/// Main class, holding data for a FSM, without the event loop
/**
types:
 - ST: an enum defining the different states.
 - EV: an enum defining the different external events.
 - TIM: a type handling the events, must provide the following methods:
   - init();
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

#ifdef SPAG_USE_ARRAY
			for( auto& e: _allowedMat )      // all events will be ignored at init
				std::fill( e.begin(), e.end(), 0 );
			for( auto& e: _transitionMat )      // transition table filled with state 0
				std::fill( e.begin(), e.end(), static_cast<ST>(0) );
#else
			priv::resizemat( _transitionMat, nbEvents(), nbStates() );
			priv::resizemat( _allowedMat, nbEvents(), nbStates() );
			_stateInfo.resize( nbStates() );    // states information
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

#ifdef SPAG_EMBED_ASIO_WRAPPER
			_eventHandler = &_asioWrapper;
#endif
		}

/** \name Configuration of FSM */
///@{

/// Assigns allowed event matrix
/**
Allowed types for \c T:
 - \c std::vector<std::vector<A>>
 - \c std::array<std::array<A,N1>,N2> (with N1 the number of states, N2 the number of events)

Type \c A can be \c bool, \c char, \c uchar, ...
*/
		template<typename T>
		void assignEventMat( const T& mat )
		{
			SPAG_CHECK_EQUAL( mat.size(),    nbEvents() );
			SPAG_CHECK_EQUAL( mat[0].size(), nbStates() );

			auto li_out = std::begin( _allowedMat );
			for( auto li_in : mat )
			{
				std::copy( std::begin(li_in), std::end(li_in), std::begin(*li_out) );
				li_out++;
			}
		}

/// Assigns transition matrix
/**
Copying of elements is done because the input matrix can be a \c std::vector, or an \c std::array

Allowed types for \c T:
 - \c std::vector<std::vector<ST>>
 - \c std::array<std::array<ST,N1>,N2> (with N1 the number of states, N2 the number of events)
*/
		template<typename T>
		void assignTransitionMat( const T& mat )
		{
			SPAG_CHECK_EQUAL( mat.size(),    nbEvents() );
			SPAG_CHECK_EQUAL( mat[0].size(), nbStates() );
			auto li_out = std::begin( _transitionMat );
			for( auto li_in : mat )
			{
				std::copy( std::begin(li_in), std::end(li_in), std::begin(*li_out) );
				li_out++;
			}
		}

/// Assigns an external transition event \c ev to switch from state \c st1 to state \c st2
/**
\note Transition to same state are allowed.
*/
		void assignTransition( ST st1, EV ev, ST st2 )
		{
			SPAG_CHECK_LESS( SPAG_P_CAST2IDX(st1), nbStates() );
			SPAG_CHECK_LESS( SPAG_P_CAST2IDX(st2), nbStates() );
			SPAG_CHECK_LESS( SPAG_P_CAST2IDX(ev),  nbEvents() );
			auto st1_idx = SPAG_P_CAST2IDX(st1);

#ifdef SPAG_USE_SIGNALS
			if( _stateInfo[st1_idx]._isPassState )
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
/// Assigns a transition to a "pass-state" (AAT): once on state \c st1, the FSM will switch right away to \c st2
/**
\warning If a time out has previously been assigned to state \c st1, it will be removed

\warning Only available when \ref SPAG_USE_SIGNALS is defined, see manual.
*/
		void assignAAT( ST st1, ST st2 )
		{
			auto st1_idx = SPAG_P_CAST2IDX(st1);
			auto st2_idx = SPAG_P_CAST2IDX(st2);
			SPAG_CHECK_LESS( st1_idx, nbStates() );
			SPAG_CHECK_LESS( st2_idx, nbStates() );
			if( st1 == st2 )
				SPAG_P_THROW_ERROR_CFG(
					"unable to assign an AAT to same states: S"
					 + std::to_string( st1_idx ) + "and S" + std::to_string( st2_idx )
				);

			_transitionMat[ nbEvents()+1 ][st1_idx] = st2;
			for( auto& line: _allowedMat ) // disable other transitions for that state
				line[ st1_idx ] = 0;

			auto& stinf = _stateInfo[st1_idx];
			stinf._isPassState = true;

			if( stinf._innerTransList.size() )
				SPAG_P_LOG_ERROR << "warning, assign AAT transition from state "
#ifdef SPAG_ENUM_STRINGS
					<< st1_idx << " (" << _strStates[st1_idx] << ") to state "
					<< st2_idx << " (" << _strStates[st2_idx] << ")"
#else
					<< st1_idx << " to state " << st2_idx
#endif
					<< " removes the "
					<< stinf._innerTransList.size() << " inner transition(s) previously assigned to this state.\n";
			stinf._innerTransList.clear();

			auto& tev = stinf._timerEvent;
			if( tev._enabled )
			{
				SPAG_P_LOG_ERROR << "warning, removal of timeout of "
					<< tev._duration << ' ' << priv::stringFromTimeUnit( tev._durUnit )
					<< " on state S" << std::setfill('0') << std::setw(2) << SPAG_P_CAST2IDX(st1)
#ifdef SPAG_ENUM_STRINGS
					<< " (" << _strStates[st1_idx] << ')'
#endif
					<< ".\n";
				tev._enabled = false;
			}
		}

/// Assigns a inner transition between \c st1 and \c st2, triggered by internal event \c ev
/// \warning Only available when \ref SPAG_USE_SIGNALS is defined, see manual.
		void assignInnerTransition( ST st1, EV iev, ST st2 )
		{
			auto st1_idx = SPAG_P_CAST2IDX(st1);
			auto ev_idx  = SPAG_P_CAST2IDX(iev);
			SPAG_CHECK_LESS( st1_idx,              nbStates() );
			SPAG_CHECK_LESS( SPAG_P_CAST2IDX(st2), nbStates() );
			SPAG_CHECK_LESS( ev_idx,               nbEvents() );

			auto& stinf = _stateInfo[ st1_idx ];
			if( stinf._isPassState )
				SPAG_P_THROW_ERROR_CFG( "error, removing pass-state" ); /// \todo maybe a warning instead ?
			stinf._isPassState = false;
			stinf._innerTransList.push_back( priv::InnerTransition<ST,EV>(iev, st2) );
			_innerEventFlag[iev] = false;
			_transitionMat[ ev_idx ][st1_idx] = st2;
			_allowedMat[    ev_idx ][st1_idx] = -1;
		}

/// Whatever state we are on, when internal event \c iev occurs, we will switch to state \c st (except if we are already on that state).
/**
To remove afterwards the inner events on some states, use \c disableInnerTransition()
*/
		void assignInnerTransition( EV iev, ST st )
		{
			auto ev_idx = SPAG_P_CAST2IDX(iev);
			auto st_idx = SPAG_P_CAST2IDX(st);
			SPAG_CHECK_LESS( st_idx, nbStates() );
			SPAG_CHECK_LESS( ev_idx, nbEvents() );

			_innerEventFlag[iev] = false;

			assert( _stateInfo.size() == nbStates() );
			for( size_t i=0; i<_stateInfo.size(); ++i )
				if( i != st_idx )
				{
					if( !_stateInfo[i].holdsInnerTransition( iev, st ) )
					{
						_stateInfo[i]._innerTransList.push_back( priv::InnerTransition<ST,EV>( iev, st ) );
						_transitionMat[ ev_idx ][i] = st;
						_allowedMat   [ ev_idx ][i] = -1;
					}
				}
		}
#else
		void assignInnerTransition( ST st1, EV ev, ST st2 )
			SPAG_NOT_AVAILABLE(SPAG_USE_SIGNALS);
		void assignInnerTransition( EV ev, ST st )
			SPAG_NOT_AVAILABLE(SPAG_USE_SIGNALS);
		void assignAAT( ST st1, ST st2 )
			SPAG_NOT_AVAILABLE(SPAG_USE_SIGNALS);
#endif // SPAG_USE_SIGNALS

/// Assigns a timeout event leading to state \c st_final, on \b all states except \c st_final,
/// using default timer unit and default timer duration value
		void assignGlobalTimeOut( ST st_final )
		{
			assignGlobalTimeOut( _defaultTimerValue, _defaultTimerUnit, st_final );
		}

/// Assigns a timeout event on \b all states except \c st_final, using duration \c dur and default timer unit.
		void assignGlobalTimeOut( Duration dur, ST st_final )
		{
			assignGlobalTimeOut( dur, _defaultTimerUnit, st_final );
		}

/// Assigns a timeout event on \b all states except \c st_final, using duration \c dur and unit \c durUnit
		void assignGlobalTimeOut( Duration dur, std::string durUnit, ST st_final )
		{
			auto tu = priv::timeUnitFromString( durUnit );
			if( !tu.first )
				SPAG_P_THROW_ERROR_CFG( "invalid string value: " + durUnit );
			assignGlobalTimeOut( dur, tu.second, st_final );
		}

/// Assigns a timeout event on \b all states except \c st_final, using unit \c durUnit
/**
After this, on all the states except \c st_final, if \c duration expires, the FSM will switch to \c st_final
(where there may or may not be a timeout assigned)
*/
		void assignGlobalTimeOut( Duration dur, DurUnit durUnit, ST st_final )
		{
			static_assert( std::is_same<TIM,priv::NoTimer<ST,EV,CBA>>::value == false, "ERROR, FSM build without timer" );

			for( size_t i=0; i<nbStates(); i++ )                                 // iterate on all the states
				if( i != SPAG_P_CAST2IDX(st_final) )                             // and for all of them, except the designated one,
				{
					auto tev = _stateInfo[ SPAG_P_CAST2IDX( i ) ]._timerEvent;   // get its "timer event" data.

#ifdef SPAG_USE_SIGNALS
					if( _stateInfo[ SPAG_P_CAST2IDX( i ) ]._isPassState )        // if it has already been assigned an AAT, then
					{                                                            //  issue a warning and process next one.
						SPAG_P_LOG_ERROR << " warning: state " << i
	#ifdef SPAG_ENUM_STRINGS
							<< " (" << _strStates[i] << ')'
	#endif
							<< " is a pass state (holds an AAT), time out not assigned.\n";
					}
					else
#endif
					{
						if( tev._enabled )
						{
							SPAG_P_LOG_ERROR << " warning, removal of previously assigned timeout leading from state " << i
	#ifdef SPAG_ENUM_STRINGS
								<< " (" << _strStates[i] << ')'
	#endif
								<< " to state " << tev._nextState
	#ifdef SPAG_ENUM_STRINGS
								<< " (" << _strStates.at( SPAG_P_CAST2IDX(tev._nextState)) << ')'
	#endif
								<< " after " << tev._duration << ' ' << priv::stringFromTimeUnit( tev._durUnit ) << ".\n";
						}
						assignTimeOut( static_cast<ST>(i), dur, durUnit, st_final );
					}
				}
		}

/// Assigns a timeout event on state \c st_curr, will switch to event \c st_next
/**
Duration will be:
 - if a timeout has been \b previously assigned to \c st_curr, then its value will be retained.
 - if not, the default value and units will be used.
See setTimerDefaultValue() and setTimerDefaultUnit()
*/
		void assignTimeOut( ST st_curr, ST st_next )
		{
			auto st_idx = SPAG_P_CAST2IDX( st_curr );
			SPAG_CHECK_LESS( st_idx, nbStates() );
			SPAG_CHECK_LESS( SPAG_P_CAST2IDX(st_next), nbStates() );
			if( _stateInfo[ st_idx ]._timerEvent._enabled )         // if already one assigned,
				_stateInfo[ st_idx ]._timerEvent._nextState = st_next;  // then just change the destination state
			else
				_stateInfo[ st_idx ]._timerEvent = priv::TimerEvent<ST>( st_next, _defaultTimerValue, _defaultTimerUnit );
		}

/// Assigns a timeout event on state \c st_curr, will switch to event \c st_next
/// Duration will be \c dur, with the default unit
		void assignTimeOut( ST st_curr, Duration dur, ST st_next )
		{
			assignTimeOut( st_curr, dur, _defaultTimerUnit, st_next );
		}

/// Assigns a timeout event on state \c st_curr, will switch to event \c st_next. With units
		void assignTimeOut( ST st_curr, Duration dur, DurUnit unit, ST st_next )
		{
			static_assert( std::is_same<TIM,priv::NoTimer<ST,EV,CBA>>::value == false, "ERROR, FSM build without timer" );
			SPAG_CHECK_LESS( SPAG_P_CAST2IDX(st_curr), nbStates() );
			SPAG_CHECK_LESS( SPAG_P_CAST2IDX(st_next), nbStates() );
			_stateInfo[ SPAG_P_CAST2IDX( st_curr ) ]._timerEvent = priv::TimerEvent<ST>( st_next, dur, unit );
		}

/// Assigns a timeout event on state \c st_curr, will switch to event \c st_next. With units as strings
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
				SPAG_P_LOG_ERROR << "warning: asking for removal of timeout on state S" << st_idx
#ifdef SPAG_ENUM_STRINGS
					<< " (" << _strStates[st_idx]  << ')'
#endif
					<< " but state has no timeout assigned.\n";
			}
			_stateInfo[ st_idx ]._timerEvent._enabled = false;
		}

/// Whatever state we are on, if the (external) event \c ev occurs, we switch to state \c st.
/// (Except for state \c st, of course)
		void assignTransition( EV ev, ST st )
		{
			SPAG_CHECK_LESS( SPAG_P_CAST2IDX(st), nbStates() );
			SPAG_CHECK_LESS( SPAG_P_CAST2IDX(ev), nbEvents() );
			for( auto& s: _transitionMat[ ev ] ) // for all columns (=states) in the line "ev"
					s = st;
			for( size_t i=0; i<_allowedMat[ ev ].size(); i++ )
				if( i != SPAG_P_CAST2IDX(st) )
					_allowedMat[ ev ][ i ] = (i == SPAG_P_CAST2IDX(st) ? 0 : 1);
		}

/// Allow all events of the transition matrix
/// \todo change this: for internal events, the value must not be 1
		void allowAllEvents()
		{
			for( auto& line: _allowedMat )
				for( auto& col: line )
					col = 1;
		}

/// Allow event \c ev when on state \c st
/**
\warning This does not assign a transition, it only changes things in the allowed transitions matrix.
Thus it is also not usable for inner transitions.
*/
		void allowEvent( ST st, EV ev, bool what=true )
		{
			auto st_idx = SPAG_P_CAST2IDX(st);
			SPAG_CHECK_LESS( st_idx, nbStates() );
			SPAG_CHECK_LESS( SPAG_P_CAST2IDX(ev), nbEvents() );

#ifdef SPAG_USE_SIGNALS
			if( _stateInfo[st_idx].holdsInnerTransition( ev, st ) )
				throw std::runtime_error( "usage of allowEvent() not possible for inner events" );
#endif // SPAG_USE_SIGNALS

			_allowedMat[ SPAG_P_CAST2IDX(ev) ][ st_idx ] = (what?1:0);
		}

#ifdef SPAG_USE_SIGNALS
/// Remove inner transition \c ev that is assigned on state \c st_from
/**
This is a companion function of \c assignInnerTransition( EV, ST )

Hence, as this latter ones assigns an inner event to all the states, we need a function
to remove this event on some states
*/
		void disableInnerTransition( EV ev, ST st_from )
		{
			auto st_idx = SPAG_P_CAST2IDX(st_from);
			auto& stinf = _stateInfo[st_idx];

			auto it = stinf.findInnerEvent( ev );
			if( it == std::end( stinf._innerTransList ) )
				SPAG_P_THROW_ERROR_CFG( "state "
					+ std::to_string( st_idx )
#ifdef SPAG_ENUM_STRINGS
					+ " (" + _strStates[st_idx] + ") "
#endif
					+ " has no inner transition"
				);

			stinf._innerTransList.erase( it );
			_allowedMat[ev][st_from] = 0;
		}
#endif // SPAG_USE_SIGNALS

/// Assigns a callback function to a state, will be called each time we arrive on this state
		void assignCallback( ST st, Callback_t func, CBA cb_arg=CBA() )
		{
			auto st_idx = SPAG_P_CAST2IDX(st);
			SPAG_CHECK_LESS( st_idx, nbStates() );
			_stateInfo[ st_idx ]._callback    = func;
			_stateInfo[ st_idx ]._callbackArg = cb_arg;
		}

/// Assigns a callback function to all the states, will be called each time the state is activated
		void assignCallback( Callback_t func )
		{
//			static_assert( std::is_same<Callback_t,CBA>::value, "Callback function is not of same type as the one declared in FSM" );
			for( size_t i=0; i<nbStates(); i++ )
				_stateInfo[ SPAG_P_CAST2IDX(i) ]._callback = func;
		}

/// Assigns a callback function to all the states, with argument value being the state value/index, converted to an \c int .
/**
Requires that argument type is an "integer" type (int, long int, char, ...).
See https://en.cppreference.com/w/cpp/types/numeric_limits/is_integer
*/
		void assignCallbackAutoval( Callback_t func )
		{
			static_assert( std::numeric_limits<CBA>::is_integer, "To use this, Callback function argument MUST be 'int'" );
			for( size_t i=0; i<nbStates(); i++ )
			{
				_stateInfo[ i ]._callback = func;
				_stateInfo[ i ]._callbackArg = static_cast<CBA>(i);
				if( i > std::numeric_limits<CBA>::max() )
					SPAG_P_THROW_ERROR_CFG( "type of callback argument too small to hold all the states" );
			}
		}

/// Assigns a callback function called when an ignored event occurs
		void assignIgnoredEventsCallback( std::function<void(ST,EV)> func )
		{
			_ignEventCallback = func;
		}

/// Assigns the callback function value \c cb_arg, for state \c st
		void assignCallbackValue( ST st, CBA cb_arg )
		{
			SPAG_CHECK_LESS( SPAG_P_CAST2IDX(st), nbStates() );
			_stateInfo[ SPAG_P_CAST2IDX(st) ]._callbackArg = cb_arg;
		}

#ifndef SPAG_EMBED_ASIO_WRAPPER
		void assignEventHandler( TIM* t )
		{
			static_assert( std::is_same<TIM,priv::NoTimer<ST,EV,CBA>>::value == false, "ERROR, FSM build without timer" );
			_eventHandler = t;
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
	private:
/// parses the strings and returns false if one of the strings is present more than once
		bool checkUnicity( const std::vector<std::string>& v_str ) const
		{
#if 1
			for( size_t i=0; i<v_str.size()-1; i++ )
				for( size_t j=i+1; j<v_str.size(); j++ )
					if( v_str[i] == v_str[j] )
						return false;
			return true;
#else
		template<typename Key>
		bool checkUnicity( const std::map<Key,std::string>& strMap ) const
		{
			std::vector<Key> v_res;                        // this is the version to use if
			for( const auto& pair1: strMap )               // we replace the container
				for( const auto& pair2: strMap )           // for a std::map
					if( pair1.second == pair2.second )     // (untested !)
						v_res.push_back( pair1.first );
			if( v_res.size() > 1 )
				return false;
			return true;
#endif
		}
	public:
/// Assign a string to an enum event value (available only if option SPAG_ENUM_STRINGS is enabled)
/// \todo Replace the assert with something more user-friendly (same with the other functions)
		void assignString2Event( EV ev, std::string str )
		{
			SPAG_CHECK_LESS( SPAG_P_CAST2IDX(ev), nbEvents() );
			_strEvents[ SPAG_P_CAST2IDX(ev) ] = str;
			assert( checkUnicity( _strEvents ) );
		}
/// Assign a string to an enum state value (available only if option SPAG_ENUM_STRINGS is enabled)
		void assignString2State( ST st, std::string str )
		{
			SPAG_CHECK_LESS( SPAG_P_CAST2IDX(st), nbStates() );
			_strStates[ SPAG_P_CAST2IDX(st) ] = str;
			assert( checkUnicity( _strStates ) );
		}
/// Assign strings to enum event values (available only if option SPAG_ENUM_STRINGS is enabled)
		void assignStrings2Events( const std::vector<std::pair<EV,std::string>>& v_str )
		{
			SPAG_CHECK_LESS( v_str.size(), nbEvents()+1 );
			for( const auto& p: v_str )
				assignString2Event( p.first, p.second );
			assert( checkUnicity( _strEvents ) );
		}
/// Assign strings to enum state values (available only if option SPAG_ENUM_STRINGS is enabled)
		void assignStrings2States( const std::vector<std::pair<ST,std::string>>& v_str )
		{
			SPAG_CHECK_LESS( v_str.size(), nbStates()+1 );
			for( const auto& p: v_str )
				assignString2State( p.first, p.second );
			assert( checkUnicity( _strStates ) );
		}
/// Assign strings to enum event values (available only if option SPAG_ENUM_STRINGS is enabled) - overload 1
		void assignStrings2Events( std::map<EV,std::string>& m_str )
		{
			for( const auto& p: m_str )
				assignString2Event( p.first, p.second );
			assert( checkUnicity( _strEvents ) );
		}
/// Assign strings to enum state values (available only if option SPAG_ENUM_STRINGS is enabled) - overload 1
		void assignStrings2States( std::map<ST,std::string>& m_str )
		{
			for( const auto& p: m_str )
				assignString2State( p.first, p.second );
			assert( checkUnicity( _strStates ) );
		}
/// Assigns to callback functions an argument value that is the state name (requires that callback argument is a string)
		void assignCBValuesStrings()
		{
			static_assert( std::is_same<CBA,std::string>::value, "Error, unable to assign strings to callback values, callback type is not std::string\n" );
			for( size_t i=0; i<nbStates(); i++ )
				assignCallbackValue( static_cast<ST>(i), _strStates[i] );
		}
/// Returns the string label associated with event \c ev
		std::string getString( EV ev ) const
		{
			return _strEvents[ev];
		}
/// Returns the string label associated with state \c st
		std::string getString( ST st ) const
		{
			return _strStates[st];
		}
#else
		void assignString2Event( EV, std::string ) {}
		void assignString2State( ST, std::string ) {}
		void assignStrings2Events( const std::vector<std::pair<EV,std::string>>& ) {}
		void assignStrings2States( const std::vector<std::pair<ST,std::string>>& ) {}
		void assignStrings2Events( const std::map<EV,std::string>& ) {}
		void assignStrings2States( const std::map<ST,std::string>& ) {}
		void assignCBValuesStrings() const {}
		std::string getString( EV ev ) const { return std::to_string( SPAG_P_CAST2IDX(ev) ); }
		std::string getString( ST st ) const { return std::to_string( SPAG_P_CAST2IDX(st) ); }
#endif

///@}

/** \name Run time functions */
///@{
/// start FSM : run callback associated to initial state (if any), an run timer (if any)
		void start()
		{
			SPAG_P_ASSERT( !_isRunning, "attempt to start an already running FSM" );

			doChecking();

			_isRunning = true;
			runAction();

#ifndef SPAG_EXTERNAL_EVENT_LOOP
			if( !std::is_same<TIM,priv::NoTimer<ST,EV,CBA>>::value )
			{
				SPAG_P_ASSERT( _eventHandler, "Event handler has not been allocated" );
				_eventHandler->init( this );   // blocking function !
			}
#endif
		}

/// stop FSM : needed only if timer is used, this will cancel (and kill) the pending timer
		void stop() const
		{
			SPAG_P_ASSERT( _isRunning, "attempt to stop an already stopped FSM" );

			if( _eventHandler )
			{
				SPAG_LOG << "call timerCancel()\n";
				_eventHandler->timerCancel();
				SPAG_LOG << "call event loop kill()\n";
				_eventHandler->kill();
			}
			_isRunning = false;
		}

/// User-code timer end function/callback should call this when the timer expires
		void processTimeOut() const
		{
			SPAG_P_START;
			SPAG_LOG << "processing timeout event, delay was " << _stateInfo[ _current ]._timerEvent._duration << "\n";
			assert( _stateInfo[ SPAG_P_CAST2IDX(_current) ]._timerEvent._enabled ); // or else, the timer shouldn't have been started, and thus we shouldn't be here...
			_previous = _current;
			_current = _stateInfo[ SPAG_P_CAST2IDX( _current ) ]._timerEvent._nextState;
#ifdef SPAG_ENABLE_LOGGING
			_rtdata.logTransition( _current, nbEvents() );
#endif
			runAction();
			SPAG_P_END;
		}

/// User-code should call this function when an external event occurs
		void processEvent( EV ev ) const
		{
			SPAG_P_START;

			SPAG_CHECK_LESS( SPAG_P_CAST2IDX(ev), nbEvents() );
			SPAG_P_ASSERT( _isRunning, "attempting to process an event but FSM is not started" );

			auto ev_idx = SPAG_P_CAST2IDX( ev );
#ifdef SPAG_ENUM_STRINGS
			SPAG_LOG << "processing event " << ev_idx << ": \"" << _strEvents[ev_idx] << "\"\n";
#else
			SPAG_LOG << "processing event " << ev_idx << '\n';
#endif
			if( _allowedMat[ ev_idx ][ SPAG_P_CAST2IDX(_current) ] == 1 )
			{
				if( _stateInfo[ SPAG_P_CAST2IDX( _current ) ]._timerEvent._enabled )  // 1 - cancel the waiting timer, if any
				{
					SPAG_P_ASSERT( _eventHandler, "Event handler has not been allocated" );
					_eventHandler->timerCancel();
				}
				_previous = _current;
				_current = _transitionMat[ ev_idx ][ SPAG_P_CAST2IDX(_current) ];     // 2 - switch to next state
#ifdef SPAG_ENABLE_LOGGING
				_rtdata.logTransition( _current, ev_idx );
#endif
				runAction();                                                          // 3 - call the callback function
			}
			else
			{
				SPAG_LOG << "event is ignored on current state\n";
				if( _ignEventCallback )
					_ignEventCallback( _current, ev );

#ifdef SPAG_ENABLE_LOGGING
				_rtdata.logIgnoredEvent( ev_idx );
#endif
			}
			SPAG_P_END;
		}

#ifdef SPAG_USE_SIGNALS
/// Activate inner event: set the inner event to true, so that a signal will be raised
/// when we are on a state that has the event enabled as inner transition
/// (and once callback has been completed).
/**
\warning Only available when \ref SPAG_USE_SIGNALS is defined, see manual.
\todo implement "early quit" (as soon as found)
\todo check if one of the states on which we activate the event is the current one. If so,
then we need to raise the signal right away! (instead of waiting)
*/
		void activateInnerEvent( EV ev )
		{
			SPAG_P_START;

			if( 0 == _innerEventFlag.count( ev ) )
				throw std::runtime_error(
					"request to activate inner event "
					+ std::to_string( SPAG_P_CAST2IDX(ev) )
#ifdef SPAG_ENUM_STRINGS
					+ " (" + _strEvents[ SPAG_P_CAST2IDX(ev) ] + ")"
#endif
					+ ", but not found in list of Internal Events"
				);

			_innerEventFlag[ ev ] = true;
			SPAG_LOG << "activating event " << SPAG_P_CAST2IDX(ev)
#ifdef SPAG_ENUM_STRINGS
				<< " (" << _strEvents[ SPAG_P_CAST2IDX(ev) ] << ')'
#endif
				<< " current state is " << (int)currentState()
#ifdef SPAG_ENUM_STRINGS
				<< " (" << _strStates[ currentState() ] << ')'
#endif
				<< '\n';

			SPAG_P_END;
		}

/// Deactivate an inner event. Will issue a warning if the event is presently not active
		void clearInternalEvent( EV ev )
		{
			if( 0 == _innerEventFlag.count( ev ) )
				throw std::runtime_error(
					"request to clear inner event "
					+ std::to_string( SPAG_P_CAST2IDX(ev) )
#ifdef SPAG_ENUM_STRINGS
					+ " (" + _strEvents[ SPAG_P_CAST2IDX(ev) ] + ")"
#endif
					+ ", but not found in list of Internal Events"
				);

			if( _innerEventFlag[ ev ] == false )
				SPAG_P_LOG_ERROR << "warning, request to clear inner event idx=" << SPAG_P_CAST2IDX(ev)
#ifdef SPAG_ENUM_STRINGS
					<< " (" + _strEvents[ SPAG_P_CAST2IDX(ev) ] + ")"
#endif
					<< ", but event was not active.\n";

			_innerEventFlag[ ev ] = false;
			SPAG_LOG << "deactivating event " << SPAG_P_CAST2IDX(ev)
#ifdef SPAG_ENUM_STRINGS
				<< " (" << _strEvents[ SPAG_P_CAST2IDX(ev) ] << ')'
#endif
				<< " current state is " << (int)currentState()
#ifdef SPAG_ENUM_STRINGS
				<< " (" << _strStates[ currentState() ] << ')'
#endif
				<< ".\n";
		}

/// This is to be called by event loop wrapper, by the signal handler.
/// <strong>DO NOT CALL in user code</strong>.
/**
This function will be called by the signal handler of the event handler class ONLY (see AsioWrapper::signalHandler() )
\warning Only available when \ref SPAG_USE_SIGNALS is defined, see manual.
*/
		void processInnerEvent( const priv::StateInfo<ST,EV,CBA>& stinf ) const
		{
			SPAG_P_START;

			SPAG_P_ASSERT( _isRunning, "attempting to process an inner event but FSM is not started" );
#ifdef SPAG_ENABLE_LOGGING
			size_t ev_idx = nbEvents() + 1;
#endif
			if( stinf._isPassState )
			{
				auto next = _transitionMat[ nbEvents()+1 ][_current];
				SPAG_LOG << "is pass state, switch from state " << (int)currentState() << " to state " << (int)next << '\n';
				_previous = _current;
				_current  = next;
			}
			else
			{
				for( const auto& innerTrans: stinf._innerTransList )
				{
					if( _innerEventFlag.count( innerTrans._innerEvent ) == 0 )       // step 1: check that the event is correctly registered in map
						SPAG_P_THROW_ERROR_RT( "Unable to find inner event" );     /// \todo expand user message


					if( _innerEventFlag[ innerTrans._innerEvent ] )   // step 2 : check if given event assigned has been activated
					{
						_previous = _current;
						_current = innerTrans._destState;
#ifdef SPAG_ENABLE_LOGGING
						ev_idx   = innerTrans._innerEvent;
#endif
						_innerEventFlag[ innerTrans._innerEvent ] = false;       // deactivate event
					}
				}
			}
//			SPAG_LOG << "stinf:\n" << stinf << '\n';

#ifdef SPAG_ENABLE_LOGGING
			_rtdata.logTransition( _current, ev_idx );
#endif
			runAction();                                                          // 3 - call the callback function
			SPAG_P_END;
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
/// Return previous state (or initial state upon start() )
		ST previousState() const
		{
			return _previous;
		}

#ifdef SPAG_ENUM_STRINGS
		size_t getStateIndex( std::string str ) const
		{
			auto it = std::find(
				std::begin(_strStates),
				std::end(_strStates),
				str
			);
			if( it == std::end(_strStates) )
				SPAG_P_THROW_ERROR_RT( "invalid state string" );
			return it - std::begin(_strStates);
		}

		size_t getEventIndex( std::string str ) const
		{
			auto it = std::find(
				std::begin(_strEvents),
				std::end(_strEvents),
				str
			);
			if( it == std::end(_strEvents) )
				SPAG_P_THROW_ERROR_RT("invalid event string" );
			return it - std::begin(_strEvents);
		}
#endif
		priv::StateInfo<ST,EV,CBA>& getStateInfo( size_t idx )
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
/// Assigns a new name for the output log file (default is spaghetti.csv)
		void setLogFileName( std::string fn ) const
		{
			assert( !fn.empty() );
			_rtdata._logfileName = fn;
		}

		Counters getCounters() const
		{
			return _rtdata.buildCounters();
		}
		void clearCounters()
		{
			_rtdata.clear();
		}
#else
		void setLogFilename( std::string fn ) const {}
//		Counters getCounters() const {}
//		void clearCounters() {}
#endif // SPAG_ENABLE_LOGGING

/// Sets the timer default value. See assignTimeOut()
		void setTimerDefaultValue( Duration val ) const
		{
			static_assert( std::is_same<TIM,priv::NoTimer<ST,EV,CBA>>::value == false, "ERROR, FSM build without timer" );
            _defaultTimerValue = val;
		}

/// Sets the timer default unit. See assignTimeOut()
		void setTimerDefaultUnit( DurUnit unit ) const
		{
			static_assert( std::is_same<TIM,priv::NoTimer<ST,EV,CBA>>::value == false, "ERROR, FSM build without timer" );
            _defaultTimerUnit = unit;
		}

/// Sets the timer default unit, given as a string. See assignTimeOut()
/**
Provided so that we can use this in a function templated with the FSM type, without having the type \c DurUnit available
(see for example in src/traffic_lights_common.hpp)
*/
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
Usage (example): <code>std::cout << fsm_t::buildOptions();</code>
*/
		static std::string buildOptions()
		{
			std::string yes(" = yes\n"), no(" = no\n");
			std::string out( "Spaghetti version " );
			out += SPAG_VERSION;
			out += "\nBuild options:\n";

			out += SPAG_P_STRINGIZE2( SPAG_USE_ASIO_WRAPPER );
#ifdef SPAG_USE_ASIO_WRAPPER
			out += yes;
#else
			out += no;
#endif
			out += SPAG_P_STRINGIZE2( SPAG_EMBED_ASIO_WRAPPER );
#ifdef SPAG_EMBED_ASIO_WRAPPER
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

		void writeDotFile( std::string fn, DotFileOptions opt=DotFileOptions() ) const;
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
			SPAG_P_START;
			SPAG_LOG << "switched to state " << SPAG_P_CAST2IDX(_current)
#ifdef SPAG_ENUM_STRINGS
				<< " (" << _strStates[ SPAG_P_CAST2IDX(_current) ] << ")"
#endif
				<< ", starting handler\n";
			auto curr_idx = SPAG_P_CAST2IDX(_current);
			auto& stateInfo = _stateInfo[ curr_idx ];

			if( stateInfo._timerEvent._enabled )
			{
				SPAG_P_ASSERT( _eventHandler, "Event handler has not been allocated" );
				SPAG_LOG << "timeout enabled, duration=" <<  stateInfo._timerEvent._duration << "\n";
				_eventHandler->timerStart( this );
			}
			if( stateInfo._callback ) // if there is a callback stored, then call it
			{
				SPAG_LOG << "callback function start:\n";
				stateInfo._callback( _stateInfo[ SPAG_P_CAST2IDX(_current) ]._callbackArg );
			}
			else
				SPAG_LOG << "no callback provided\n";

#ifdef SPAG_USE_SIGNALS
			if( _isRunning )  // we need this, because the callback could have stopped the FSM, thus we must not generate a signal !
			{
				bool do_raise_sig = false;
				if( stateInfo._isPassState )
				{
					SPAG_LOG << "Is pass-state, raise signal.\n";
					do_raise_sig = true;
				}
				else
				{
					for( auto& itr: stateInfo._innerTransList )
						if( _innerEventFlag.count( itr._innerEvent ) ) // if event is registered
							if( _innerEventFlag.at( itr._innerEvent ) ) // and active
							{
								SPAG_LOG << "Inner Event idx=" << SPAG_P_CAST2IDX(itr._innerEvent)
								#ifdef SPAG_ENUM_STRINGS
									<< " (" << _strEvents[ SPAG_P_CAST2IDX(itr._innerEvent) ] << ")"
								#endif
									<< " is active, raise signal.\n";
								do_raise_sig = true;
								break;                    // no need to check the others
							}
				}
				if( do_raise_sig )
				{
					SPAG_LOG << "raising signal\n";
					SPAG_LOG_FLUSH;
					_eventHandler->raiseSignal();
					_eventHandler->timerCancel();
				}
			}
//			SPAG_LOG << "current state info:\n";
//			std::cout << _stateInfo[ curr_idx ] << '\n';
#endif
			SPAG_P_END;
		}

		void printLineHeader(  std::ostream&, size_t idx, bool firstline_flag, size_t maxlength ) const;
		void printMatrix(      std::ostream& ) const;
		void printStateConfig( std::ostream& ) const;
		bool isReachable( size_t ) const;
		std::string getConfigErrorMessage( priv::EN_ConfigError ce, size_t st ) const;

/////////////////////////////
// private data section
/////////////////////////////

	private:
#ifdef SPAG_ENABLE_LOGGING
		mutable priv::RunTimeData<ST,EV> _rtdata;
#endif
		mutable ST        _current           = static_cast<ST>(0);   ///< current state
		mutable ST        _previous          = static_cast<ST>(0);   ///< previous state
		mutable bool      _isRunning         = false;
		mutable DurUnit   _defaultTimerUnit  = DurUnit::sec;         ///< default timer units
		mutable Duration  _defaultTimerValue = 1;                    ///< default timer value
		mutable TIM*      _eventHandler      = nullptr;              ///< pointer on timer/ event-loop handling type

#ifdef SPAG_USE_ARRAY
	#ifdef SPAG_USE_SIGNALS
		std::array<
			std::array<ST, static_cast<size_t>(ST::NB_STATES)>,
			static_cast<size_t>(EV::NB_EVENTS)+2                   // + 2 is used to hold the AAT
		> _transitionMat;  ///< describe what states the fsm switches to, when a message is received. lines: events, columns: states, value: states to switch to. DOES NOT hold timer events
	#else
		std::array<
			std::array<ST, static_cast<size_t>(ST::NB_STATES)>,
			static_cast<size_t>(EV::NB_EVENTS)+1                  // +1 is used to hold the timeout events
		> _transitionMat;  ///< describe what states the fsm switches to, when a message is received. lines: events, columns: states, value: states to switch to. DOES NOT hold timer events
	#endif

/// Matrix holding for each event a byte telling is the event is ignored or not, for a given state (0:ignore event, 1:handle external event, -1: internal event)
		std::array<
			std::array<char, static_cast<size_t>(ST::NB_STATES)>,
			static_cast<size_t>(EV::NB_EVENTS)
		> _allowedMat;
#else
		std::vector<std::vector<ST>>   _transitionMat;
		std::vector<std::vector<char>> _allowedMat;
#endif // SPAG_USE_ARRAY

#ifdef SPAG_USE_ARRAY
		std::array<priv::StateInfo<ST,EV,CBA>,static_cast<size_t>(ST::NB_STATES)> _stateInfo;         ///< Holds for each state the details
#else
		std::vector<priv::StateInfo<ST,EV,CBA>> _stateInfo;         ///< Holds for each state the details
#endif
		mutable std::map<EV,bool> _innerEventFlag; ///< holds the activation flag for each inner event

#ifdef SPAG_ENUM_STRINGS
		std::vector<std::string> _strEvents;      ///< holds events strings
		std::vector<std::string> _strStates;      ///< holds states strings
#endif

#ifdef SPAG_EMBED_ASIO_WRAPPER
		AsioWrapper<ST,EV,CBA> _asioWrapper; ///< optional wrapper around boost::asio::io_service
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
	out << std::setfill('0');

	priv::printChars( out, maxlength, spc_char );
	out << "        STATES:\nEVENTS ";
	priv::printChars( out, maxlength, spc_char );
	for( size_t i=0; i<nbStates(); i++ )
		out << spc_char<< 'S' << std::setw(2) << i;
	out << '\n';
	priv::printChars( out, maxlength+6, '-' );
	out << '|';
	for( size_t i=0; i<nbStates(); i++ )
		out << "----";
	out << '\n';

	auto nbLines = nbEvents()+1; // +1 for timeout events
#ifdef SPAG_USE_SIGNALS
	nbLines++;                   // +1 for pass-states
#endif

#ifndef SPAG_ENUM_STRINGS
	std::string capt( "EVENTS" );
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
			out << spc_char << 'E' << std::setw(2) << i << " | ";

			for( size_t j=0; j<nbStates(); j++ )
			{
				if( _allowedMat[i][j] )
					out << 'S' << std::setw(2) << _transitionMat[i][j];
				else
					out << " . ";
				out << spc_char;
			}

		}
		if( i == nbEvents() ) // TimeOut events
		{
			out << "  TO | ";
			for( size_t j=0; j<nbStates(); j++ )
			{
				if( _stateInfo[j]._timerEvent._enabled )
					out << 'S' << std::setw(2) << _stateInfo[j]._timerEvent._nextState;
				else
					out << " . ";
				out << spc_char;
			}
		}
#ifdef SPAG_USE_SIGNALS
		if( i == nbEvents()+1 ) // Pass-state
		{
			out << " AAT | ";
			for( size_t j=0; j<nbStates(); j++ )
			{
				if( _stateInfo[j]._isPassState )
					out << 'S' << std::setw(2) << _transitionMat[ nbEvents()+1 ][j];
				else
					out << " . ";
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
					if( _allowedMat[k][i] != 0 )
						return true;

			if( _stateInfo[i]._timerEvent._enabled )
				if( SPAG_P_CAST2IDX( _stateInfo[i]._timerEvent._nextState ) == st )
					return true;

#ifdef SPAG_USE_SIGNALS
			if( _stateInfo[i]._isPassState )
				if( _transitionMat[ nbEvents()+1 ][i] == st )
					return true;

			for( const auto& itr: _stateInfo[i]._innerTransList )
				if( SPAG_P_CAST2IDX( itr._destState ) == st )
					return true;
#endif
		}

	return false;
}
//-----------------------------------------------------------------------------------
/// Checks configuration for any illegal situation. Throws error if one is encountered.
template<typename ST, typename EV,typename T,typename CBA>
void
SpagFSM<ST,EV,T,CBA>::doChecking() const
{
 #if 0
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

//			if( state._timerEvent._enabled )
//				SPAG_P_THROW_ERROR_CFG( getConfigErrorMessage( priv::CE_TimeOutAndPassState, i ) );
		}
	}
#endif

// check for unreachable states
	std::vector<size_t> unreachableStates;
	for( size_t i=1; i<nbStates(); i++ )         // we start from index 1, because 0 is the initial state, and thus is always reachable!
		if( !isReachable( i ) )
			unreachableStates.push_back( i );
	for( const auto& st: unreachableStates )
	{
		std::cout << priv::getSpagName() << "Warning, state S" << std::setw(2) << st
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
#ifdef SPAG_USE_SIGNALS
		if( _stateInfo[i]._isPassState )
			foundValid = true;
#endif
		if( !foundValid )       // else
		{
			for( size_t j=0; j<nbEvents(); j++ )
				if( SPAG_P_CAST2IDX( _transitionMat[j][i] ) != i )   // if the transition leads to another state
					if( _allowedMat[j][i] != 0 )                  // AND it is allowed
						foundValid = true;
		}

		if( !foundValid )                     // if we didn't find a valid transition
			if( std::find(
				unreachableStates.begin(),
				unreachableStates.end(),
				i
			) == unreachableStates.end() )     // AND it is not in the unreachable states list
		{
			std::cout << priv::getSpagName() << "Warning, state S" << std::setw(2) << i
#ifdef SPAG_ENUM_STRINGS
				<< " (" << _strStates[i] << ')'
#endif
				<< " is a dead-end\n";
		}
	}
}
//-----------------------------------------------------------------------------------
/// Helper function for printConfig()
template<typename ST, typename EV,typename T,typename CBA>
void
SpagFSM<ST,EV,T,CBA>::printLineHeader( std::ostream& out, size_t idx, bool firstline_flag, size_t maxlength ) const
{
	if( firstline_flag )
		out << 'S' << std::setw(2) << idx;
	else
		out << "   ";
#ifdef SPAG_ENUM_STRINGS
	out << ':';
	if( firstline_flag )
	{
//		firstline_flag = false;
		priv::PrintEnumString( out, _strStates[idx], maxlength );
	}
	else
		priv::printChars( out, maxlength, ' ' );
#endif
		out << "| ";
}
//-----------------------------------------------------------------------------------
template<typename ST, typename EV,typename T,typename CBA>
void
SpagFSM<ST,EV,T,CBA>::printStateConfig( std::ostream& out ) const
{
	size_t maxlength = 0;
#ifdef SPAG_ENUM_STRINGS
	maxlength = priv::getMaxLength( _strStates );
#endif

	for( size_t i=0; i<nbStates(); i++ )
	{
		printLineHeader( out, i, true, maxlength );
		bool print_content = false;

		const auto& stinf = _stateInfo[i];
		const auto& tev = stinf._timerEvent;
		if( tev._enabled )
		{
			print_content = true;
			out << "TO: " <<  tev._duration << ' ' << priv::stringFromTimeUnit( tev._durUnit )
				<< " => S" << std::setw(2) << SPAG_P_CAST2IDX( tev._nextState );
#ifdef SPAG_ENUM_STRINGS
			out << " (";
			priv::PrintEnumString( out, _strStates[tev._nextState] );
			out << ')';
#endif // SPAG_ENUM_STRINGS
			out << '\n';
		}

#ifdef SPAG_USE_SIGNALS
		for( size_t j=0; j<stinf._innerTransList.size(); ++j )
		{
			if( print_content )
				printLineHeader( out, i, false, maxlength );
			else
				print_content = true;

			const auto &itr = stinf._innerTransList[j];
			auto dst_st = SPAG_P_CAST2IDX(itr._destState);
			auto i_ev   = SPAG_P_CAST2IDX(itr._innerEvent);
			out << "IT ("
				<< ( _innerEventFlag.at(itr._innerEvent)?'A':'I')
				<< "): E" << std::setw(2) << i_ev;
#ifdef SPAG_ENUM_STRINGS
			out << " (";
			priv::PrintEnumString( out, _strEvents[i_ev] );
			out << ')';
#endif // SPAG_ENUM_STRINGS

			out << " => S" << std::setw(2) << dst_st;

#ifdef SPAG_ENUM_STRINGS
			out << " (";
			priv::PrintEnumString( out, _strStates[dst_st] );
			out << ')';
#endif // SPAG_ENUM_STRINGS

			out << '\n';
		}

		if( stinf._isPassState )
		{
			if( print_content )
				printLineHeader( out, i, false, maxlength );
			else
				print_content = true;

			out << "AAT: => S" << std::setw(2) << SPAG_P_CAST2IDX( _transitionMat[ nbEvents()+1 ][i] );
#ifdef SPAG_ENUM_STRINGS
			out << " (";
			priv::PrintEnumString( out, _strStates[_transitionMat[ nbEvents()+1 ][i]] );
			out << ')';
#endif // SPAG_ENUM_STRINGS
			out << '\n';
		}
#endif // SPAG_USE_SIGNALS

		if( !print_content )
			out << "-\n";
	}
}
//-----------------------------------------------------------------------------------
/// Printing function, prints transition table and states info
template<typename ST, typename EV,typename T,typename CBA>
void
SpagFSM<ST,EV,T,CBA>::printConfig( std::ostream& out, const char* msg ) const
{
	out << "\n* FSM Configuration: ";
	if( msg )
		out << msg;
	out << "\n - Transition table:\n";
	printMatrix( out );
	out << "\n - State info:\n";
	printStateConfig( out );
	out << "---------------------\n";
}

//-----------------------------------------------------------------------------------
/// Saves in current folder a .dot file of the FSM, to be rendered with Graphviz
/**
DO NOT give the extension in argument, is is added here.
*/
template<typename ST, typename EV,typename T,typename CBA>
void
SpagFSM<ST,EV,T,CBA>::writeDotFile( std::string fname, DotFileOptions opt ) const
{
	std::string full_fn = fname + ".dot";
	std::ofstream f( full_fn );
	if( !f.is_open() )
		SPAG_P_THROW_ERROR_RT( "error, unable to open file: " + full_fn );
	f << "/*\nGenerated by Spaghetti library\nhttps://github.com/skramm/spaghetti\n"
		<< "Version " << SPAG_STRINGIZE(SPAG_VERSION) << "\n*/\n";
	f << "digraph G {\n"
		<< "rankdir=LR;\n"
		<< "edge[style=\"bold\"];\n"
		<< "node[shape=\"" << opt.nodeShape << "\"";
	if( opt.fixedNodeWidth )
		f << ",fixedsize=true,width=" << opt.nodeWidth;
	f << "];\n";
	f << std::setfill( '0' );

	f << "\n/* States (=nodes) */\n";
	for( size_t j=0; j<nbStates(); j++ )
	{
		if( opt.showUnreachableStates || isReachable( j ) )
		{
			f << j << " [label=\"";
			if( opt.showStateIndex )
				f << 'S' << std::setw(2) << j;
	#ifdef SPAG_ENUM_STRINGS
			if( opt.showStateString )
			{
				if( opt.showStateIndex )
					f << "\\n";
				f << _strStates[j];
			}
	#endif
			f << '"';

			if( j == 0 )                                // initial state
				f << ",shape=doublecircle";
			if( opt.showActiveState )
				if( SPAG_P_CAST2IDX( currentState() ) == j )
					f << ",style=filled,fillcolor=black,fontcolor=white";
	#if 0
			f << ",URL=\"" << fname << ".html#node_" << j << "\"";
	#endif
			f << "];\n";
		}
	}
	f << "\n/* External events */\n";
	for( size_t i=0; i<nbEvents(); i++ )
		for( size_t j=0; j<nbStates(); j++ )
			if( _allowedMat[i][j] == 1 )
			{
#ifdef SPAG_USE_SIGNALS
				if( !_stateInfo[j]._isPassState )
#endif
					if( isReachable( j ) || opt.showUnreachableStates )
					{
						f << j << " -> " << _transitionMat[i][j] << " [label=\"";
						if( opt.showEventIndex )
							f << 'E' << std::setw(2) << i;
#ifdef SPAG_ENUM_STRINGS
						if( opt.showEventString )
						{
							if( opt.showEventIndex )
								f << ':';
							f << _strEvents[i];
						}
#endif
						f << "\"];\n";
					}
			}

	f << "\n/* Inner events and timeout transitions */\n";
	for( size_t j=0; j<nbStates(); j++ )
	{
		const auto& tev = _stateInfo[j]._timerEvent;
		if( tev._enabled && opt.showTimeOuts )
			if( isReachable( j ) || opt.showUnreachableStates )
			{
				f << j << " -> " << tev._nextState
					<< " [label=\"TO:"
					<< tev._duration
					<< priv::stringFromTimeUnit( tev._durUnit )
					<< "\"";
				if( opt.useColorsEventType )
					f << ",color=blue";
				f << "];\n";
			}
#ifdef SPAG_USE_SIGNALS
		if( _stateInfo[j]._isPassState && opt.showAAT )
			if( isReachable( j ) || opt.showUnreachableStates )
			{
				f << j << " -> " << _transitionMat[ nbEvents()+1 ][j] << " [label=\"AAT\"";
				if( opt.useColorsEventType )
					f << ",color=green";
				f << "];\n";
			}
		if( opt.showInnerEvents )
		{
			for( const auto& itr: _stateInfo[j]._innerTransList )
			{
				if( isReachable( j ) || opt.showUnreachableStates )
				{
					f << j << " -> " << SPAG_P_CAST2IDX( itr._destState ) << " [label=\"";
					if( opt.showEventIndex )
						f << "IE" << std::setw(2) << SPAG_P_CAST2IDX( itr._innerEvent );
#ifdef SPAG_ENUM_STRINGS
					if( opt.showEventString )
					{
						if( opt.showEventIndex )
							f << ':';
						f << _strEvents.at(itr._innerEvent);
					}
#endif // SPAG_ENUM_STRINGS
					f << '"';
					if( opt.useColorsEventType )
						f << ",color=red";
					f << "];\n";
				}
			}
		}
#endif // SPAG_USE_SIGNALS
	}
	f << "}\n";

// experimental
/** idea: generate an html page embedding both the generated figure from the .dot file
(using graphviz dot tool) and a list holding the string label of each node.
The point was to add a link to the svg nodes, as this is supposed to work.

Problem: it doesn't. The link works only if you load the svg file in the browser, but not
if the svg file is loaded as an image inside the web page.
*/
#if 0
#ifdef SPAG_ENUM_STRINGS
	std::string full_fn2 = fname + ".html";
	std::ofstream f2( full_fn2 );
	if( !f2.is_open() )
		SPAG_P_THROW_ERROR_RT( "error, unable to open file: " + full_fn2 );
	f2 << "<html>\n<head>\n<title>";
	f2 << fname;
	f2 << "</title>\n</head>\n<body>\n";

	f2 << "<img src=\"" << fname << ".svg\" alt=\"" << fname << "\">\n";
	f2 << "<hr><ul>\n";
	for( size_t j=0; j<nbStates(); j++ )
	{
		f2 << "<li id=\"node_" << j << "\">node " << j << ": " << _strStates[j];
		f2 << "</li>\n";
	}
	f2 << "</ul>\n";
	f2 << "</body>\n";
	f2 << "</html>\n";
#endif
#endif
}
//-----------------------------------------------------------------------------------
namespace priv {

/// Dummy type, used if no timer requested by user
template<typename ST, typename EV,typename CBA>
struct NoTimer
{
	void timerStart( const SpagFSM<ST,EV,NoTimer,CBA>* ) {}
	void init(  const SpagFSM<ST,EV,NoTimer,CBA>* ) {}
	void timerCancel() {}
	void raiseSignal() {}
};

} // namespace priv

//-----------------------------------------------------------------------------------

#if defined (SPAG_USE_ASIO_WRAPPER)

//-----------------------------------------------------------------------------------
/// Wraps the boost::asio stuff to have an asynchronous timer easily available
/**
Rationale: holds a timer, created by constructor. It can then be used without having to create one explicitely.
That last point isn't that obvious, has it also must have a lifespan not limited to some callback function.

For timer duration, see
http://en.cppreference.com/w/cpp/chrono/duration

\bug we have a problem with signal when both symbols SPAG_USE_SIGNALS and SPAG_EXTERNAL_EVENT_LOOP are defined:
the signal handler is NOT called!!!
See src/sample_3c.cpp that demonstrates the problem.
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
	void init( spag::SpagFSM<ST,EV,AsioWrapper,CBA>* fsm )
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

/// terminates all pending events, timers events or signals
	void kill()
	{
		SPAG_LOG << '\n';
#ifdef SPAG_USE_SIGNALS
		_signals.cancel();
		_signals.clear();
#endif
		_io_service.stop();
	}

/// Timer callback function, called when timer expires.
	void timerCallback( const boost::system::error_code& err_code, const spag::SpagFSM<ST,EV,AsioWrapper,CBA>* fsm  )
	{
		SPAG_P_START;

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
		SPAG_P_END;
	}
/// Mandatory function for SpagFSM. Cancel the pending async timer
	void timerCancel()
	{
//		SPAG_LOG << '\n';
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
	void signalHandler( const boost::system::error_code& err_code, int signal_number, spag::SpagFSM<ST,EV,AsioWrapper,CBA>* fsm )
	{
		SPAG_P_START;

		SPAG_LOG << "handling signal " << signal_number << " errorcode=" << err_code.message() << ", current state=" << fsm->currentState() << std::endl;
		auto st_idx = SPAG_P_CAST2IDX( fsm->currentState() );
		auto& stateInfo = fsm->getStateInfo( st_idx );
//		std::cout << "signal handler, processing " << stateInfo._innerTrans;

		SPAG_LOG << "BEFORE processInnerEvent(): " << stateInfo << '\n';
		fsm->processInnerEvent( stateInfo );
		SPAG_LOG << "AFTER processInnerEvent(): " << stateInfo << '\n';

		if( err_code == 0 )
			_signals.async_wait(                                   // re-initialize signal handler, only if the handler is not called whith a "cancel" message
				boost::bind(
					&AsioWrapper<ST,EV,CBA>::signalHandler,
					this,
					boost::asio::placeholders::error,
					boost::asio::placeholders::signal_number,
					fsm
				)
			);
		SPAG_P_END;
	}
	void raiseSignal()
	{
		SPAG_P_START;
		std::raise( SIGUSR1 );
		SPAG_P_END;
	}
#endif
};

#endif // SPAG_USE_ASIO_WRAPPER

//-----------------------------------------------------------------------------------

} // namespace spag

/// Shorthand to declare a member function as callback function.
/// \warning needs to be done inside another class member function
#define SPAG_ASSIGN_MEMBER_CALLBACK( fsm, State, ClassName, CallbackFunc ) \
	fsm.assignCallback( State, std::bind( &ClassName::CallbackFunc, this, std::placeholders::_1 ) )

/// Shorthand to declare a member function as callback function for all states.
/// \warning needs to be done inside another class member function
#define SPAG_ASSIGN_MEMBER_CALLBACK_ALL( fsm, ClassName, CallbackFunc ) \
	fsm.assignCallback( std::bind( &ClassName::CallbackFunc, this, std::placeholders::_1 ) )

/// Shorthand for declaring the type of FSM, without a timer
#ifdef SPAG_USE_ASIO_WRAPPER
	#define SPAG_DECLARE_FSM_TYPE_NOTIMER( type, st, ev, cbarg ) \
		static_assert( 0, "Error, can't use this macro with symbol SPAG_USE_ASIO_WRAPPER defined" )
#else
	#define SPAG_DECLARE_FSM_TYPE_NOTIMER( type, st, ev, cbarg ) \
		typedef spag::SpagFSM<st,ev,spag::priv::NoTimer<st,ev,cbarg>,cbarg> type
#endif

/// Shorthand for declaring the type of FSM with an arbitrary timer class
#define SPAG_DECLARE_FSM_TYPE( type, st, ev, timer, cbarg ) \
	typedef spag::SpagFSM<st,ev,timer<st,ev,cbarg>,cbarg> type

#ifdef SPAG_USE_ASIO_WRAPPER
	#ifdef SPAG_EMBED_ASIO_WRAPPER
/// Shorthand for declaring the type of FSM with the provided Boost::asio timer class. Does not create the \c AsioEL type (user code doesn't need it)
		#define SPAG_DECLARE_FSM_TYPE_ASIO( type, st, ev, cbarg ) \
			typedef spag::SpagFSM<st,ev,spag::AsioWrapper<st,ev,cbarg>,cbarg> type
	#else
/// Shorthand for declaring the type of FSM with the provided Boost::asio timer class. Also creates the \c AsioEL type
		#define SPAG_DECLARE_FSM_TYPE_ASIO( type, st, ev, cbarg ) \
			typedef spag::SpagFSM<st,ev,spag::AsioWrapper<st,ev,cbarg>,cbarg> type; \
			namespace spag { \
				typedef spag::AsioWrapper<st,ev,cbarg> AsioEL; \
			}
	#endif
#else
	#define SPAG_DECLARE_FSM_TYPE_ASIO( type, st, ev, cbarg ) \
		static_assert( 0, "Error, can't use this macro without symbol SPAG_EMBED_ASIO_WRAPPER or SPAG_USE_ASIO_WRAPPER defined" )
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


*/
