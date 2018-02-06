/**
\file sample_1.cpp
\brief


*/

//#define SPAG_PRINT_STATES
#include "spaghetti.hpp"
#include <iostream>

enum En_States { st0, st1, st2, st3, st4, NB_STATES };
enum En_Events { ev_1, ev_2, NB_EVENTS };

typedef spag::SpagFSM<En_States,En_Events,spag::NoTimer<En_States,En_Events,std::string>,std::string> fsm_t;

void cbfunc( std::string s )
{
	std::cout << "callback: " << s << '\n';
}

int main()
{
	fsm_t fsm;
	fsm.assignGlobalCallback( cbfunc );

	fsm.assignCallbackValue( st0, "Init" );
	fsm.assignCallbackValue( st1, "st1" );
	fsm.assignCallbackValue( st2, "st2" );
	fsm.assignCallbackValue( st3, "st3" );
	fsm.assignCallbackValue( st4, "Error" );

	fsm.assignTransition( st0, ev_1, st1 );

	fsm.assignTransition( st1, ev_1, st2 );
	fsm.assignTransition( st2, ev_1, st3 );
	fsm.assignTransition( st3, ev_1, st1 );

	fsm.assignTransition( st1, ev_2, st4 );
	fsm.assignTransition( st2, ev_2, st4 );
	fsm.assignTransition( st3, ev_2, st4 );

	fsm.assignTransition( st4, st0 );   // st4 is a "pass state"

	do
	{
		char key;
		std::cin >> key;
		switch( key )
		{
			case 'a':
				fsm.processEvent( ev_1 );
			break;

			case 'b':
				fsm.processEvent( ev_2 );
			break;

			default:
				std::cout << "invalid key\n";
		}
	}
	while(1);

}
