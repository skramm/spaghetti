# spaghetti
A C++ library useful for simple and easy Finite State Machine (FSM) building

# General information
- Author: S. Kramm, LITIS, Rouen, France - 2018
- Status: alpha (WIP !)
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


# FAQ

- Q: Why that name ?
- A: Naming is hard. But, lets see: Finite State Machine = FSM = Flying Spaghetti Monster
(see [WP](https://en.wikipedia.org/wiki/Flying_Spaghetti_Monster)).
So you got it.
(and certainly not related to [this](https://en.wikipedia.org/wiki/Spaghetti_code), hopefully!)

- Q: Why does the optional callback value need a specific build option (SPAG_PROVIDE_CALLBACK_TYPE) ? Why not provide it as a template parameter ?
- A: because having callback with some value to pass to them requires to store them, obviously. So if that option is enabled, then the storage is generated.

# Reference

https://en.wikipedia.org/wiki/Finite-state_machine

More info to come...
