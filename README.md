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

## Instanciate the FSM


## configure the FSM

## run the FSM

As explained above, two parts are to be considered:

 1. Initializing the FSM into the first state.
```C++
	fsm.start();
```

2. calling the trigger member functions when the event is detected

```C++
	fsm.start();
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
