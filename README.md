# spaghetti
A C++ library useful for simple and easy Finite State Machine (FSM) building

# General information
- Author: S. Kramm, LITIS, Rouen, France - 2018
- Status: alpha (WIP !), API is not stabilized yet
- Licence: GPL v3
- hosting: https://github.com/skramm/spaghetti

# Main features

 - single file header-only library, just fetch the file spaghetti.hpp and store it on your machine
 - C++11, no dependencies other than standard library
 - ease of use, no unreadable templaple BS in user code
 - "you pay for what you use" code
 - user code has to provide the event loop (waiting loop)
 - logging capability
 - provided with some sample programs, see src/html/index.html

 ## Why should I use this ?
  - pros: it is pretty simple to get running (sample programs provided)
  - cons: it is inherently not type safe, as states and events are provided as enum values.
  If you can live with that, then you may go ahead.

# Usage:

--- TO BE CONTINUED ---

## Instanciate the FSM

First, create enums for states and events:
```C++
enum States { st_init, st_state_1, st_state_2, st_state_3, NB_STATES };
enum Events { ev_button_1, ev_button_2, ev_button_3, NB_EVENTS };
```
Then, if you you don't need any timer:
```C++
spag::SpagFSM<States,Events> fsm;
```

## Configure the FSM

Now, you need to build your FSM, that is:
- define what event in what state will trigger switching to what state.
For example, say that if the active state is `st_state_1` and we have an event `ev_button_1`, then we want to switch to state `st_state_1`.
This will be done as below:
```C++
	fsm.assignTransition( st_state_1, ev_button_1, st_state_2 );
```

- define what states will have a time out, and to what state we want to switch when the time out is expired.
```C++
	fsm.assignTimeOut( st_state_1, 5, st_state_2 );
```

## Run the FSM

As explained above, two parts are to be considered:

 1. Initializing the FSM into the first state, before entering your event loop:
```C++
	fsm.start();
```
This will run the callback that is assigne to initial state (state "0") and start the timer, if a time out has been assigned to this state.

2. calling the trigger member functions when the event is detected

If its an "external" event:
```C++
	fsm.processEvent( Event );
```
If it's a "TimeOut" event:
```C++
	fsm.processTimeOut();
```

# FAQ

- Q: Why that name ?
- A: Naming is hard. But, lets see: Finite State Machine = FSM = Flying Spaghetti Monster
(see [WP](https://en.wikipedia.org/wiki/Flying_Spaghetti_Monster)).
So you got it.
(and certainly not related to [this](https://en.wikipedia.org/wiki/Spaghetti_code), hopefully!)

# References

https://en.wikipedia.org/wiki/Finite-state_machine

More info to come...
