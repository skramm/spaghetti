/**
This cannot build as we cannot have a "no-timer" fsm type if we have the
build option SPAG_EMBED_ASIO_WRAPPER active

This is because the SPAG_EMBED_ASIO_WRAPPER symbol enable some stuff in the class declaration, so it
becomes impossible to define a FSM of another type.
*/

#define SPAG_EMBED_ASIO_WRAPPER
#include "../spaghetti.hpp"

enum States { st0, st1, NB_STATES };
enum Events { ev1, ev2, NB_EVENTS };

SPAG_DECLARE_FSM_TYPE_NOTIMER( fsm_t, States, Events, std::string );

int main()
{
	fsm_t fsm;
}
