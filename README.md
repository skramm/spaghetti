# Spaghetti

![logo](https://raw.githubusercontent.com/skramm/spaghetti/master/docs/SpagFSM_logo_200.png)

[![Build Status](https://travis-ci.org/skramm/spaghetti.svg?branch=master)](https://travis-ci.org/skramm/spaghetti)
[![Latest release](https://img.shields.io/github/v/release/skramm/spaghetti)](https://github.com/skramm/spaghetti/releases)


A C++ library useful for simple and easy Finite State Machine (FSM) building

## RECENT NEWS
 - 2020-07-08: changed licence to BSL1.0<br>
 ([changelog](docs/spaghetti_changelog.md))

## Key points
- Licence: [BSL1.0](https://www.boost.org/LICENSE_1_0.txt)
- Audience: C++ developper having to implement a FSM
- Webpage: https://github.com/skramm/spaghetti
- Manual : [online here](docs/spaghetti_manual.md)
- Language: C++11
- OS: any one, as long as you have a C++11 compiler
- Author: Sebastien Kramm, LITIS, University of Rouen, France - 2018-2020.

![FSM gif](https://github.com/skramm/spaghetti/blob/master/docs/out.gif)

## General information

### What is this ?
This library provides a container and API functions to easily implement an event-driven Finite State Machine
(see [WP link](https://en.wikipedia.org/wiki/Finite-state_machine)).
It can be used for FSM requiring the handling of hardware events, or time out situations.
It has been designed with good "Modern C++" practices, but provides an API as simple as possible to the developer.

This library provides an easy way to specify states and events, and how and when it will switch from one state to another.
It also has some additional features, such as logging the execution history.

The manual describes many uses cases, all of them are included in source tree and directly runnable for testing.

A lot of efforts has been put on safety: most of the potential errors are detected at build time, and everything is checked at runtime.

Feedback welcome, please post issue on Github in case of any problems.

### Main features

- single-file header-only library, just fetch the file `spaghetti.hpp` and store it on your machine somewhere where your compiler can find it
- C++11, no dependencies if no event loop required.
To get full features, you will need Boost::asio.
- ease of use and performance
- [full manual included](https://github.com/skramm/spaghetti/blob/master/docs/spaghetti_manual.md)
- runtime logging capability
- provided with several sample programs, see folder
[src](https://github.com/skramm/spaghetti/tree/master/src)

### What's in this repo ?

Besides the main file `spaghetti.hpp`, the repo also holds documentation, [FAQ](docs/spaghetti_faq.md), and several samples in `src` folder.
All of this comes with all that is needed to build these on a standard Linux machine (makefile, build folders, ...).
If you clone the repo, just run  `make demo` to build the demo programs (assuming you have Boost installed, as some samples rely on it).
You'll find the corresponding binaries in  the `build/bin` folder.

If you want a quick demo (assuming you have Boost locally available), once you have downloaded the [latest release](https://github.com/skramm/spaghetti/releases) you can try:
```
$ cd spaghetti
$ make demo -j4
$ build/bin/traffic_lights_1
```
(Note: Requires `boost_system` and `boost_thread` libs at link time)

### Short preview 1 (no signal/timeout handling)

#### 1 - State callback function
```C++
void cb_func( bool b )
{
	if( b )
		std::cout << "State: Locked\n";
	else
		std::cout << "State: Unlocked\n";
}
```

#### 2 - Declaring states, events and FSM type
```C++
enum class States { st_Locked, st_Unlocked, NB_STATES };
enum class Events { ev_Push, ev_Coin, NB_EVENTS };

SPAG_DECLARE_FSM_TYPE_NOTIMER( fsm_t, States, Events, bool );
```

#### 3 - Configuring FSM
```C++
int main()
{
	fsm_t fsm;
	fsm.assignTransition( States::st_Locked,   Events::ev_Coin, States::st_Unlocked );
	fsm.assignTransition( States::st_Unlocked, Events::ev_Push, States::st_Locked );

	fsm.assignCallback( States::st_Locked,   cb_func, true );
	fsm.assignCallback( States::st_Unlocked, cb_func, false );
```

#### 4 - Running and event dispatch
```C++
	fsm.start();
	bool quit(false);
	do
	{
		char key;
		std::cout << "Enter command: ";
		std::cin >> key;
		switch( key )
		{
			case 'p':
				std::cout << "Event: push\n";
				fsm.processEvent( Events::ev_Push );
			break;

			case 'c':
				std::cout << "Event: coin\n";
				fsm.processEvent( Events::ev_Coin );
			break;

			case 'q': quit = true; break;
		}
	}
	while( !quit );
}
```

### Short preview 2, with timeout (needs Boost asio)

#### 1 - State callback function
```C++
void callback( std::string v )
{
	std::cout << "cb, value=" << v << '\n';
}
```

#### 2 - Declaring states, events and FSM type
```C++
enum States { st_Red, st_Orange, st_Green, NB_STATES };
enum Events { NB_EVENTS };

SPAG_DECLARE_FSM_TYPE_ASIO( fsm_t, States, Events, std::string );
```

#### 3 - Configuring FSM
```C++
int main()
{
	fsm_t fsm;
	fsm.assignTimeOut( st_Red,    5, st_Green  ); // 5 s. from red to green
	fsm.assignTimeOut( st_Green,  5, st_Orange ); // 5 s. from green to orange
	fsm.assignTimeOut( st_Orange, 1, st_Red   );  // 1 s. from orange to red

	fsm.assignCallback( st_Red,    callback, std::string("RED") );
	fsm.assignCallback( st_Orange, callback, std::string("ORANGE") );
	fsm.assignCallback( st_Green,  callback, std::string("GREEN") );
```

#### 4 - Running and event dispatch
```C++
	fsm.start(); // now blocking function
```


### Future development (?)

What is planned:

- experiment and benchmark with the two provided containers (vectors and arrays)
- with vector as containers, expand API to be able to create states at runtime

