/**
\file test_2.cpp
\brief test2
*/


#define SPAG_EMBED_ASIO_WRAPPER
#define SPAG_ENUM_STRINGS
#define SPAG_ENABLE_LOGGING
//#define SPAG_PRINT_STATES
#define SPAG_USE_SIGNALS
#define SPAG_GENERATE_DOTFILE
#include "spaghetti.hpp"

enum States { st0, st1, st2, NB_STATES };
enum Events { ev0, NB_EVENTS };

SPAG_DECLARE_FSM_TYPE_ASIO( fsm_t, States, Events, bool );

fsm_t fsm;
int g_count = 0;

void cb( int s )
{
	std::cout << "current state=" << s << " count=" << g_count << '\n';
	if( g_count++ == 10 )
		fsm.activateInnerEvent( ev0 );
}
//-----------------------------------------------------------------------------------
void configureFSM( fsm_t& fsm )
{
	fsm.assignCallback( st0, cb, (int)st0 );
	fsm.assignCallback( st1, cb, (int)st1 );
	fsm.assignCallback( st2, cb, (int)st2 );

	fsm.assignTimeOut( st0, 400, "ms", st2 );
	fsm.assignTimeOut( st2, 800, "ms", st0 );
	fsm.assignInnerTransition( st0, ev0, st1 );
//	fsm.assignTransition( st2, st0 );
	fsm.assignTransition( st1, st2 );

	std::map<States,std::string> mstr_st = {
		{ st0, "init state" },
		{ st2, "state_2" }
	};
	fsm.assignStrings2States( mstr_st );

	fsm.assignString2Event( ev0, "my_event" );
}

#define WRITE_INCREMENTAL_DOT_FILE( fsm, dfo ) \
	{ \
		std::ostringstream oss; \
		oss << "test_2_" << std::setfill('0') << std::setw(2) << fn_index++; \
		fsm.writeDotFile( oss.str(), dfo ); \
	}

int main( int, char* argv[] )
{
	int fn_index{0};
	configureFSM( fsm )	;
	fsm.printConfig( std::cout );
	fsm.doChecking();

	spag::DotFileOptions dfo;                   // default
	WRITE_INCREMENTAL_DOT_FILE( fsm, dfo );

	{
		spag::DotFileOptions dfo;
		dfo.showActiveState = false;
		WRITE_INCREMENTAL_DOT_FILE( fsm, dfo );
	}
	{
		spag::DotFileOptions dfo;
		dfo.showInnerEvents = false;
		WRITE_INCREMENTAL_DOT_FILE( fsm, dfo );
	}
	{
		spag::DotFileOptions dfo;
		dfo.showAAT = false;
		WRITE_INCREMENTAL_DOT_FILE( fsm, dfo );
	}
	{
		spag::DotFileOptions dfo;
		dfo.showTimeOuts = false;
		WRITE_INCREMENTAL_DOT_FILE( fsm, dfo );
	}
	{
		spag::DotFileOptions dfo;
		dfo.showEventIndex = false;
		WRITE_INCREMENTAL_DOT_FILE( fsm, dfo );
	}
	{
		spag::DotFileOptions dfo;
		dfo.showEventString = false;
		WRITE_INCREMENTAL_DOT_FILE( fsm, dfo );
	}
	{
		spag::DotFileOptions dfo;
		dfo.showStateIndex = false;
		WRITE_INCREMENTAL_DOT_FILE( fsm, dfo );
	}
	{
		spag::DotFileOptions dfo;
		dfo.showStateString = false;
		WRITE_INCREMENTAL_DOT_FILE( fsm, dfo );
	}

	try
	{
		fsm.start();
	}
	catch( const std::exception& e )
	{
		std::cout << "Error: " << e.what() << '\n';
	}
}