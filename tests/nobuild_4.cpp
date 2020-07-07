/**
This cannot build as Always Active Transitions require enabling signals with 
build option SPAG_USE_SIGNAL
*/

#define SPAG_EMBED_ASIO_WRAPPER
#include "../spaghetti.hpp"

enum States { st0, st1, NB_STATES };
enum Events { ev1, ev2, NB_EVENTS };

SPAG_DECLARE_FSM_TYPE_ASIO( fsm_t, States, Events, std::string );

int main()
{
	fsm_t fsm;
	fsm.assignAAT( st0, st1 );
}
