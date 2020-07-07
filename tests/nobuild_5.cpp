/**
This cannot build as a states machines needs to have at least 2 states
*/

//#define SPAG_EMBED_ASIO_WRAPPER
#include "../spaghetti.hpp"

enum States { st0, NB_STATES };
enum Events { ev1, ev2, NB_EVENTS };

SPAG_DECLARE_FSM_TYPE_NOTIMER( fsm_t, States, Events, std::string );

int main()
{
	fsm_t fsm;
}
