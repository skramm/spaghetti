/**
\file bad_configs.cpp
\brief some test case to check configuration
*/


#define SPAG_EMBED_ASIO_TIMER
#define SPAG_ENUM_STRINGS
#define SPAG_ENABLE_LOGGING
#define SPAG_PRINT_STATES

#include "spaghetti.hpp"

enum States { st0, st1, st2, NB_STATES };
enum Events { ev0, ev1, ev2, NB_EVENTS };

SPAG_DECLARE_FSM_TYPE_ASIO( fsm_t, States, Events, bool );

//-----------------------------------------------------------------------------------
void configureFSM( fsm_t& fsm )
{
	fsm.assignTransition( st0, ev0, st1 );
	fsm.assignTimeOut( st1, 4, st2 );

	std::map<States,std::string> mstr_st = {
		{ st0, "init state" },
		{ st2, "state_2" }
	};
	fsm.assignStrings2States( mstr_st );

	std::map<Events,std::string> mstr_ev = {
		{ ev0, "eveeeeent" }
	};
	fsm.assignStrings2Events( mstr_ev );
}

int main( int, char* argv[] )
{
	std::cout << argv[0] << ": " << fsm_t::buildOptions() << '\n';

	fsm_t fsm;

	configureFSM( fsm )	;
	fsm.printConfig( std::cout );

	try
	{
		fsm.start();
	}
	catch( const std::exception& e )
	{
		std::cout << "Error: " << e.what() << '\n';
	}
}
