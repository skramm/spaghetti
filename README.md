# spaghetti
A C++ library useful for simple and easy Finite State Machine (FSM) building

## Key points
- Status: alpha (currently in dev. mode), API is not stabilized yet, so don't use until advised...
- Licence: GPL v3
- Audience: C++ developper having to implement a FSM
- Webpage: https://github.com/skramm/spaghetti
- Language: C++11
- Author: S. Kramm, LITIS, University of Rouen, France - 2018

## General information

### What is this ?
 This library doesn't provide any highend algorithm, you can consider it as a container holding all that is required to implement
 a FSM.
 However, it **does not** provide the main event loop. This is up to the user code.
 The rationale behind this is that this loop may vary greatly depending on the requirements
 (need for timeout or not, need to acquire hardware external events,...).

 This library provides an easy way to specify states and events, and how and when it will switch from one state to another.
 It also has some additional facilities, such as logging and easy timeout handling.


### Main features

 - single file header-only library, just fetch the file spaghetti.hpp and store it on your machine somewhere accessible by your compiler
 - C++11, no dependencies other than standard library
 - ease of use, and performance
 - runtime logging capability
 - provided with some sample programs, see folder src and or src/html/index.html

 ### Why/when should I use / not use this ?
  - pros: it is pretty simple to get running (sample programs provided)
  - cons: it is inherently not type safe, as states and events are provided as enum values.
  If you can live with that, then you may go ahead.


For manual, see [here](docs/spaghetti_manual.md).


## FAQ

- Q: Why that name ?
- A: Naming is hard. But, lets see: Finite State Machine = FSM = Flying Spaghetti Monster
(see [WP](https://en.wikipedia.org/wiki/Flying_Spaghetti_Monster)).
So you got it.
(and certainly not related to [this](https://en.wikipedia.org/wiki/Spaghetti_code), hopefully!)

- Q: What if I have more that a single argument to pass to my callback function ?
- A: then, you'll need to "pack it" in some class, or use a ```std::pair```, or ```std::tuple```.

## References

https://en.wikipedia.org/wiki/Finite-state_machine


