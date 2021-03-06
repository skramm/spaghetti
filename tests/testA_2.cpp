/**
\file testA_2.cpp
\brief Demo of the graph rendering options
*/

#define SPAG_EMBED_ASIO_WRAPPER
#define SPAG_ENUM_STRINGS
#define SPAG_ENABLE_LOGGING
#define SPAG_PRINT_STATES
#define SPAG_USE_SIGNALS
#include "spaghetti.hpp"

enum States { st0, st1, st2, st3, st4, st5, st6, NB_STATES };
enum Events { ev0, ev1, NB_EVENTS };

SPAG_DECLARE_FSM_TYPE_ASIO( fsm_t, States, Events, int );

fsm_t fsm;
int g_count = 0;

void cb( int s )
{
	std::cout << "callback: current state=" << s << " count=" << g_count << '\n';
	if( g_count++ == 5 )
	{
		std::cout << "activating inner event ev0\n";
		fsm.activateInnerEvent( ev0 );
	}
}
//-----------------------------------------------------------------------------------
void configureFSM( fsm_t& fsm )
{
	fsm.assignCallbackAutoval( cb );

//	fsm.assignGlobalTimeOut( st0 );

	fsm.assignGlobalTimeOut( 600, "ms", st0 );

	fsm.assignTimeOut( st0, 1500, "ms", st2 );
	fsm.assignTimeOut( st1, st2 );
	fsm.assignTimeOut( st3, st4 );
	fsm.assignTimeOut( st2, st4 );
	fsm.assignTimeOut( st4, st0 );

	fsm.assignInnerTransition( st0, ev0, st1 );
	fsm.assignInnerTransition( st2, ev0, st3 );
//	fsm.assignTransition( st2, st0 );
	fsm.assignTransition( st6, ev1, st0 );
	fsm.assignAAT( st1, st2 );

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
		dfo.showActiveState = true;
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
	{
		spag::DotFileOptions dfo;
		dfo.showUnreachableStates = false;
		WRITE_INCREMENTAL_DOT_FILE( fsm, dfo );
	}
	{
		spag::DotFileOptions dfo;
		dfo.fixedNodeWidth = true;
		WRITE_INCREMENTAL_DOT_FILE( fsm, dfo );
	}
	{
		spag::DotFileOptions dfo;
		dfo.useColorsEventType = false;
		WRITE_INCREMENTAL_DOT_FILE( fsm, dfo );
	}

#if 0
	try
	{
		fsm.start();
	}
	catch( const std::exception& e )
	{
		std::cout << "Error: " << e.what() << '\n';
	}
#endif
}
