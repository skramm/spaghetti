/**
\file doc_fig_1cpp
\brief just there to produce a figure included in manual

Homepage: https://github.com/skramm/spaghetti
*/

#define SPAG_EMBED_ASIO_WRAPPER
#define SPAG_FRIENDLY_CHECKING
#define SPAG_ENABLE_LOGGING
#include "spaghetti.hpp"

//#include <thread>
//#include <mutex>

enum States { st0,st1,st2,NB_STATES };
enum Events { ev1, ev2, NB_EVENTS };


SPAG_DECLARE_FSM_TYPE_ASIO( fsm_t, States, Events, std::string );

//-----------------------------------------------------------------------------------
void cb_func( std::string s )
{
	std::cout << "callback: " << s << '\n';
}
//-----------------------------------------------------------------------------------
int main( int, char* argv[] )
{
	std::cout << argv[0] << ": " << fsm_t::buildOptions() << '\n';

	fsm_t fsm;

	std::vector<std::vector<States>> trMat = {
//           st0 - st1 - st2
/* ev1 */  { st1 , st2 , st0 },
/* ev2 */  { st0 , st0 , st0 }
	};
	fsm.assignTransitionMat( trMat );

{
	std::vector<std::vector<bool>> eventMat = {
//          st0 - st1 - st2
/* ev1 */  { 1 ,   1 ,   1 },
/* ev2 */  { 1 ,   1 ,   0 }
	};
	fsm.assignEventMat( eventMat );
}

{
	std::vector<std::vector<unsigned char>> eventMat = {
//          st0 - st1 - st2
/* ev1 */  { 1 ,   1 ,   1 },
/* ev2 */  { 1 ,   1 ,   0 }
	};
	fsm.assignEventMat( eventMat );
}

	fsm.printConfig( std::cout );
	fsm.writeDotFile( "dog_fig_1" );
}
//-----------------------------------------------------------------------------------


