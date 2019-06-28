
## Spaghetti: Build information

- [Homepage](https://github.com/skramm/spaghetti)
- [Manual](spaghetti_manual.md)

This pages summarizes how to build a program that uses this library, and how to build the provided samples.

### Building the samples


All the provided samples can be build with
```make demo```
(this implies you have Boost::asio installed)


### Build options

Spaghetti itself is a header-only single file library, thus there is nothing to build:
just "include" that file in your program and your done.
However, several symbols can change the behavior of the library and/or add additional capabilities, you can define them either by adding them in your makefile
(with GCC, its ```-DSPAG_SOME_SYMBOL``` ), or by hardcoding in your program, BEFORE including the library file, like this:

```C++
#define SPAG_SOME_SYMBOL
#include "spaghetti.hpp"
```

They all start with these 5 characters: ```SPAG_```

You can printout at runtime the build options with this static function (once you have defined the type ```fsm_t```, see [manual](spaghetti_manual.md#showcase1)):
```C++
std::cout << fsm_t::buildOptions()
```

There are two types of symbols:
* Behavioral symbols: these add up some feature, but your program will compile anyway, whether the symbol is defined or not,
* Structural symbols: these change the inner structure of the classes:
you need to define them based on the desired FSM architecture, as described in manual.
Incorrect usage might make your program fail to compile.


### 1 - Structural symbols

Theses options/symbols must be chosen carefully, depending on what you need:

* ```SPAG_USE_ASIO_WRAPPER``` : this enables the usage of an included event handling class: ```AsioWrapper```, that fits nicely for timeout events.
It is provided as an option as it brings in a dependency to the Boost libraries, which might not be wanted by end user.
It is automatically activated if ```SPAG_EMBED_ASIO_WRAPPER``` (see below) is defined.<br>
If you do not define this symbol, then you may not use timers and inner events, unless you provide your own event handling class.

* ```SPAG_EMBED_ASIO_WRAPPER```: this option embeds the above boost::asio-based Timer class **inside** the Spaghetti FSM type.
This is the easiest way to do if you need a timer, **and** you have Boost available.<br>
However, it might not fit your needs in some special situations, even if you want to use the ```AsioWrapper``` class.<br>
If you do not define this symbol but only ```SPAG_USE_ASIO_WRAPPER```, then you will need to instanciate yourself a variable of type
```AsioWrapper```.

* ```SPAG_EXTERNAL_EVENT_LOOP``` : this is needed if you intend to run several FSM concurrently.
In that case, the event handling class must **not** hold the timer
(*If it does, then starting the FSM with ```fsm.start()``` will be a blocking function, thus it would not be possible to start a second FSM*).<br>
So you need to provide the event loop separately and define this symbol.
The change is that now the start function will not be blocking:
you can start all the needed FSM, then eventually start the event loop.
This is demonstrated in sample program [src/sample_2.cpp](../../../tree/master/src/sample_2.cpp).<br>
**Note** : If you intend to use the provided ```AsioWrapper``` class with this symbol defined, be aware that this will make significant
changes to that class: instead of embedding the boost::asio event loop structure (aka "io_service" or "io_context"), it is now up
to you to provide it (see example above for details).<br>
Also, the constructor changes: you will now need to provide the boost::io_service:
```C++
boost::asio::io_service io_service;
AsioWrapper asio( io_service );
```

* ```SPAG_USE_SIGNALS``` : this is needed if you intend to have "Pass-states" and "inner events".
It enables the data structures used to handle this.
See section 7 in manual.

### 2 - Behavioral symbols

Theses options/symbols will not impact correct build of your program:

* ```SPAG_PRINT_STATES``` : will print on stdout the steps, useful only for debugging your SpagFSM.
This can be automatically enabled when building the samples by passing the option ```DEBUG=Y```:<br>
```make demo -j4 DEBUG=Y```

* ```SPAG_ENABLE_LOGGING``` : will enable logging of dynamic data (see spag::SpagFSM::printLoggedData() )

* ```SPAG_FRIENDLY_CHECKING```: A lot of checking is done to ensure no nasty bug will crash your program.
However, in case of incorrect usage of the library by your client code (say, invalid index value),
the default behavior is to spit a standard error message that can be difficult to understand.
So if you define this symbol at build time, instead of getting this:
```
myfile: /usr/local/include/spaghetti.hpp:236: void spag::SpagFSM<STATE, EVENT, TIM>::assignTransitionMat(const std::vector<std::vector<T> >&) [with STATE = SERSTAT; EVENT = EN_EVENTS; TIM = AsioWrapper]: Assertion `mat.size() == EVENT::NB_EVENTS' failed.
Aborted
```
you will get this:
```
Spaghetti: runtime error in func: assignTransitionMat(), values are not equal:
 - mat.size() value=7
 - EVENT::NB_EVENTS value=8
Exiting...
```
If this symbol is \b not defined, regular checking is still done with the classical ```assert()```.
As usual, this checking can be removed by defining the symbol ```NDEBUG```.

* ```SPAG_ENUM_STRINGS``` : this enables the usage of enum-string mapping, for states and events.
You can provide a string either individually with
```C++
	fsm.assignString2Event( ev_MyEvent, "something happened" );
	fsm.assignString2State( st_Arizona, "Arizona state" );
```
or globally, by providing a vector of pairs (enum values, string).
For example:
```C++
	std::vector<std::pair<EVENT,std::string>> v_str = {
		{ ev_Reset,      "Reset" },
		{ ev_WarningOn,  "Warning On" },
		{ ev_WarningOff, "Warning Off" }
	};
	fsm.assignStrings2Events( v_str );
```
It also possible to provide a `std::map`:
```C++
	std::map<EVENT,std::string> map_str = {
		{ ev_Reset,      "Reset" },
		{ ev_WarningOn,  "Warning On" },
		{ ev_WarningOff, "Warning Off" }
	};
	fsm.assignStrings2Events( map_str );
```
This works similarly for states.
<br>
These strings will then be printed out when calling the ```printConfig()``` and ```printData()``` member function.
<br>
Default values are generated when this option is enabled, in the form "St-x" and "Ev-x".
<br>
You can also access these strings at runtime with ```fsm.getString( some_event_enum_value )```.


--- Copyright S. Kramm - 2018-2019 ---
