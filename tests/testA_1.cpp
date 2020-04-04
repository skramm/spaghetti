/**
\file testA_1.cpp
\brief some test case to check configuration
*/


#define SPAG_EMBED_ASIO_WRAPPER
#define SPAG_ENUM_STRINGS
#define SPAG_ENABLE_LOGGING
#define SPAG_PRINT_STATES

#include "spaghetti.hpp"

enum States { st0, st1, st2, st3, NB_STATES };
enum Events { ev0, NB_EVENTS };

SPAG_DECLARE_FSM_TYPE_ASIO( fsm_t, States, Events, bool );

//-----------------------------------------------------------------------------------
void configureFSM( fsm_t& fsm )
{
	fsm.assignTransition( st0, ev0, st1 );
	fsm.assignTimeOut( st1, st2 );

	std::map<States,std::string> mstr_st = {
		{ st0, "init state" },
//		{ st0, "state_2" },
		{ st2, "state_2" }
	};
	fsm.assignStrings2States( mstr_st );
	fsm.assignString2Event( ev0, "event" );
}

int main( int, char* argv[] )
{
	std::cout << "Boost version: " << BOOST_VERSION << "\n";

//	std::cout << argv[0] << ": " << fsm_t::buildOptions() << '\n';

	fsm_t fsm;

	configureFSM( fsm )	;
	fsm.printConfig( std::cout );
	fsm.doChecking();

/*	try
	{
		fsm.start();
	}
	catch( const std::exception& e )
	{
		std::cout << "Error: " << e.what() << '\n';
	}*/
}
