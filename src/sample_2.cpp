/**
\file sample_2.cpp
\brief demo program of stack overflow bug...

This file is part of Spaghetti, a C++ library for implementing Finite State Machines

Homepage: https://github.com/skramm/spaghetti
*/

#define SPAG_GENERATE_DOTFILE
//#define SPAG_PRINT_STATES
//#define SPAG_ENUM_STRINGS
#include "spaghetti.hpp"
#include "asio_wrapper.hpp"

enum A_States { st_1, st_2, NB_STATES };
enum A_Events { ev_1, NB_EVENTS };

//enum B_States { st_1, st_2, NB_STATES };
//enum B_Events { ev_1, NB_EVENTS };


SPAG_DECLARE_FSM_TYPE( fsm_t, A_States, A_Events, AsioWrapper, std::string );

void cb( std::string s )
{
	std::cout << "cb: " << s << '\n';
}
//-----------------------------------------------------------------------------------
int main( int, char* argv[] )
{
	std::cout << argv[0] << ": " << fsm_t::buildOptions() << '\n';

	fsm_t fsm_A, fsm_B;
//	fsm.assignString2State( st_1, "STATE 1" );
//	fsm.assignString2State( st_2, "STATE-2" );

	fsm_A.assignTimeOut( st_1, 1, st_2 );
	fsm_B.assignTimeOut( st_1, 1, st_2 );

	fsm_A.assignGlobalCallback( cb );
	fsm_B.assignGlobalCallback( cb );

	fsm_A.assignCallbackValue( st_1, "st1-A" );
	fsm_A.assignCallbackValue( st_2, "st2-A" );
	fsm_B.assignCallbackValue( st_1, "st1-B" );
	fsm_B.assignCallbackValue( st_2, "st2-B" );

	fsm_A.printConfig( std::cout );
	fsm_B.printConfig( std::cout );

	AsioWrapper<A_States,A_Events,std::string> asio_A;
	AsioWrapper<A_States,A_Events,std::string> asio_B;
	fsm_A.assignTimer( &asio_A );
	fsm_B.assignTimer( &asio_B );
	fsm_A.writeDotFile( "sample_2A.dot" );
	fsm_B.writeDotFile( "sample_2B.dot" );
	try
	{
		fsm_A.start();  // blocking !
	}
	catch( std::exception& e )
	{
		std::cerr << "catch: error: " << e.what() << std::endl;
	}
	catch( ... )
	{
		std::cerr << "catch: unknown error\n";
	}
}
//-----------------------------------------------------------------------------------


