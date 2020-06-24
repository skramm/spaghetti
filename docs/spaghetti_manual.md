# Spaghetti manual

Homepage: https://github.com/skramm/spaghetti


This page demonstrates usage of the library through several showcases, and gives additional details.
All the examples are included and runnable in the src folder, just `make demo` (or `make demo -j4` to make things quicker), then run `bin/program_name`.

For a reference manual, run `make doc`, then open `html/index.html` (needs doxygen).

For a shorter summary of all the API, [read this](spaghetti_summary.md).

In case of any problem, feel free to [post an issue on Github](https://github.com/skramm/spaghetti/issues), feedback always appreciated!

### Summary
1. [Fundamental concepts](#concepts)
1. [Showcase 1: Hello World for FSM](#showcase1)
1. [Showcase 2: let's use a timer](#showcase2)
1. [Showcase 3 : mixing timeout with hardware events](#showcase3)
1. [Showcase 4 : adding network control](#showcase4)
1. [Concurrent FSM](#concurrent)
1. [Using inner events and pass states](#inner_events)
   1. [Inner events](#inner_events)
   1. [Pass states](#pass_states)
1. [Additional stuff](#additional_stuff)
   1. [Configuration](#config)
   1. [Printing Configuration](#printconfig)
   1. [Checking configuration](#checks)
   1. [FSM getters and other information](#getters)
1. [Build options](spaghetti_options.md)
1. [Graphical Rendering of the FSM](spaghetti_rendering.md)
1. [Runtime logging](spaghetti_logging.md)
1. [Developper information](spaghetti_devinfo.md)
1. [FAQ](spaghetti_faq.md)


Before you can get started, all you need is to download and install the file `spaghetti.hpp` to a location where your compiler can find it.
On Linux, `/usr/local/include` is usually pretty good.
You can do that manually, or with `sudo make install`.
Then you can create a program.
Or, you can clone the whole repo to have a look at the provided examples.

<a name="concepts"></a>
## 1 - Fundamental concepts

### 1.1 - Library content
This library is actually made of two parts:
The main part is the FSM main class, that is only a container of all the characteristics of your machine.
You instanciate it and configure it:
you define the states, the events, the callback functions associated to the states, and of course what event may lead from one state to another state.
Then you start it and when an events occur, it is up to your code to call the relevant member function, nothing happens magically.
This part has no dependency other than the standard library.

But a lot of situations require more than just handling of external events.
For example, timeouts.
Thus, the FSM object can work together with another object that will provide all the needed tasks, such as time handling, and generating events at the right moment.
Consider this as some kind of *event loop*.

This class is independent from the main FSM class.
The FSM could theorically work with any user-written class that provides theses services, and that respects certain requirements.
But of course, the Spaghetti library provides such a class, that works seamlessly.
This is the preferred way, but it comes at the price of a dependency on the Boost libraries.
More on this below.

### 1.2 - States and Events
Configuration of the state machine is a mix of static and dynamic: the number of states and hardware events is fixed at build time, but it is possible to add transitions at run-time.

States and events are simply defined as enum values:
```C++
enum States { st_1, st_2, st_3, NB_STATES }; // 3 states
enum Events { ev_1, ev_2, ev_3, NB_EVENTS }; // 3 events
```
Naming is free except for the last value that
**must** be `NB_STATES` and `NB_EVENTS`, respectively.
For the states, the first one (having value 0) will be the initial state.

You can use either classical C++03 enums or C++11 scoped enums (`enum class { st1, st2,...`).
The latter adds of course some type safety.
But all these values are internally casted to integers, so you must not assign values to the enumerators.

Events can be of **three** types:

1. *hardware events* (basically, it can be just a keyboard press).
You need to define them in the enum above.
But you can have none !
In that case, just define the events enum as:
```C++
	enum Events { NB_EVENTS };
```
2. *time out events*, when you want to switch from state A to state B after 'x' seconds.
There are handled separately:
you **must not** add an enumerator for these.
3. *internal events*, that depend on some **inner** condition on your application.
Say something like: "*if the state X is activated 3 times, then, once on state Y, switch immediately to state Z*".
These **must** also be defined in the enum above.

As described above, for the two latter cases you need to provide a special "timing/event handler" class that will have some requirements.
You will need to instanciate an object of that class, then **assign** it to the FSM in the configuration step.
Fortunately, this is made easy for the usual case, no worry.<br>
For the other events, it is up to your code to detect these, and then call some Spaghetti member function.
Take a look at the little sample in next section.

<a name="showcase1"></a>
## 2 - Showcase 1: Hello World for FSM

In this example, we show the "Hello World" of FSM, which is the "Turnstyle" FSM,
see [WP page](https://en.wikipedia.org/wiki/Finite-state_machine#Example:_coin-operated_turnstile).

First, create enums for states and events:

```C++
#include "spaghetti.hpp"
enum States { st_Locked, st_Unlocked, NB_STATES };
enum Events { ev_Push, ev_Coin, NB_EVENTS };
```
Then, create the FSM data type using a conveniency macro:
```C++
SPAG_DECLARE_FSM_TYPE_NOTIMER( fsm_t, States, Events, bool );
```
This means you create the type `fsm_t` (this macro is actually a typedef of the templated class `SpagFSM`), using `States` and `Events`,
with callback functions having a `bool` as argument.

Now, you can instanciate the fsm:

```C++
int main()
{
	fsm_t fsm;
```

Next, you need to configure your FSM: define what event in what state will trigger switching to what other state.
With this simple example, you just do:
```C++
	fsm.assignTransition( st_Locked,   ev_Coin, st_Unlocked );
	fsm.assignTransition( st_Unlocked, ev_Push, st_Locked );
```
This means:
- if a coin is inserted while in "locked" state, then switch to state "unlocked".
- if somebody pushes the gate while in "unlocked" state, then switch to state "locked".

Ok, and also tell the FSM what is to be done when a state gets triggered.
This is done by providing a callback function.
In this case it is the same for all, but you can have a different function for every state.
You don't even need to provide a callback function, it is always optional:
some states can have one while others won't.
The only constraint is that they must have the same signature.
In the present case, we assign to the two states the function `cb_func`, and give the argument value:

```C++
	fsm.assignCallback( st_Locked,   cb_func, true );
	fsm.assignCallback( st_Unlocked, cb_func, false );
```

Alternatively (and that is useful if you have a lot of states), you can provide a common callback function for all the states, and separately set the argument value:

```C++
	fsm.assignCallback( cb_func );
	fsm.assignCallbackValue( st_Unlocked, false );
	fsm.assignCallbackValue( st_Locked,   true );
```

And of course you must provide that callback function:

```C++
void cb_func( bool b )
{
	if( b )
		std::cout << "State: Locked\n";
	else
		std::cout << "State: Unlocked\n";
}
```
Done for configuration.
Now, to run this you need to call `start()`.

Here the events will be triggered by the keyboard, so our event loop will just be this:
```C++
fsm.start();
do
{
	char key;
	std::cout << "Enter command: ";
	std::cin >> key;
	switch( key )
	{
		case 'A':
			std::cout << "Event: push\n";
			fsm.processEvent( ev_Push );
		break;

		case 'B':
			std::cout << "Event: coin\n";
			fsm.processEvent( ev_Coin );
		break;
	}
}
while( true );
```

That's it!
All of this is given as a sample program,
see file
[src/turnstyle_1.cpp](../../../tree/master/src/turnstyle_1.cpp)
and/or just clone repo and enter
`make demo -j4` followed by `bin/turnstyle_1`.

<a name="showcase2"></a>
## 3 - Showcase 2: let's use a timer

Lets consider another situation: a traffic light going automatically through the three states: Red, Green, Orange. So we define this:
```C++
enum States { st_Red, st_Orange, st_Green, NB_STATES };
enum Events { NB_EVENTS };
```
<img align="center" src="LED_Traffic_Light.jpeg" alt="Traffic_light">

You need to provide a Timer class that can be used by the FSM, and that provides **asynchronous** timeouts and an event waiting loop.

You can write your own, but it is obviously easier to use the one provided with this library.
The only requirement is that you must have [Boost Asio](http://www.boost.org/doc/libs/release/libs/asio/) installed on your machine.
As this is fairly common these days, lets assume this is okay.
If not, if you are lucky enough to have a Debian-based OS (Ubuntu and other derivatives), just enter
`sudo apt-get install  libboost-all-dev`.

To use the provided Timer class, you need to pass an option to Spaghetti by defining the symbol `SPAG_EMBED_ASIO_WRAPPER`
([see here details about the build options](spaghetti_options.md)).

So now we use the second form of the type declaration macro:
```C++
#define SPAG_EMBED_ASIO_WRAPPER
#include "spaghetti.hpp"

enum States { st_Red, st_Orange, st_Green, NB_STATES };
enum Events { NB_EVENTS };

SPAG_DECLARE_FSM_TYPE_ASIO( fsm_t, States, Events, std::string );
```
The configuration step will go as follows.
As you can guess, we have here timeouts of 5, 5, and 1 seconds:

```C++
int main()
{
	fsm_t fsm;
	fsm.assignTimeOut( st_Red,    5, st_Green  );
	fsm.assignTimeOut( st_Green,  5, st_Orange );
	fsm.assignTimeOut( st_Orange, 1, st_Red   );
```
For the callback, lets say it will just print out the current color, so we can use a string argument:
```C++
void myCallback( std::string v )
{
	std::cout << "color=" << v << '\n';
}
```
And the configuration will include this:
```C++
	fsm.assignCallback( myCallback );
	fsm.assignCallbackValue( st_Red,    "red" );
	fsm.assignCallbackValue( st_Orange, "Orange" );
	fsm.assignCallbackValue( st_Green,  "Green" );
```
Once configuration is done, just start the FSM. But remember, this is now a **blocking** function!
```C++
	fsm.start();
```
All of this can be found in the runnable example in
[src/traffic_lights_1.cpp](../../../tree/master/src/traffic_lights_1.cpp).
This sample has an additional "init state", lasting 3s.
Below is the graph produced by Graphviz from the output generated by `writeDotFile()`
(more on rendering on [this page](spaghetti_rendering.md))

![traffic_lights_1](traffic_lights_1.svg)

<a name="showcase3"></a>
## 4 - Showcase 3 - mixing timeout with hardware events: Traffic lights with buttons

Now, lets consider the same system but with added buttons to control it.
We add a "Warning" button to make the system enter a "orange blinking state".
Actually, that won't be a unique state, but two different states,
"Blink On" and "Blink Off".

Oh, and also a "Warning off" button (to return to regular cycle), and a "Reset" button (can be useful).

So we have the following states and events:
```C++
enum States {
	st_Init,
	st_Red,
	st_Orange,
	st_Green,
	st_BlinkOn,
	st_BlinkOff,
	NB_STATES
};
enum Events {
	ev_Reset,      ///< reset button
	ev_WarningOn,  ///< blinking mode on
	ev_WarningOff, ///< blinking mode off
	NB_EVENTS
};
```

Configuration of the FSM will be as previously, we just add this (self-explanatory):
```C++
	fsm.assignTimeOut( st_BlinkOn,  1, st_BlinkOff );
	fsm.assignTimeOut( st_BlinkOff, 1, st_BlinkOn );

	fsm.assignTransition( ev_Reset,     st_Init ); // if reception of message ev_Reset, then switch to state st_Init, whatever the current state is
	fsm.assignTransition( ev_WarningOn, st_BlinkOn );
	fsm.assignTransition( st_BlinkOff,  ev_WarningOff, st_Red );
	fsm.assignTransition( st_BlinkOn,   ev_WarningOff, st_Red );
```

As the `start()` member function is blocking, we need to handle the keyboard events in a different thread.
So we define a "user interface" function, templated by the FSM type:

```C++
template<typename FSM>
void UI_thread( const FSM* fsm )
{
	bool quit(false);
	do
	{
		char key;
		std::cin >> key;
		switch( key )
		{
			case 'a': fsm->processEvent( ev_WarningOn );  break;
			case 'b': fsm->processEvent( ev_WarningOff ); break;
			case 'c': fsm->processEvent( ev_Reset );      break;
			case 'q': fsm->stop(); quit = true;           break;
		}
	}
	while( !quit );
}
```

And we start that thread **before** starting the FSM:
```C++
	std::thread thread_ui( UI_thread<fsm_t>, &fsm );
	fsm.start();  // blocking !
	thread_ui.join();
```

All of this can be found in the runnable example in [src/traffic_lights_2.cpp](../../../tree/master/src/traffic_lights_2.cpp)
and its companion header file
[src/traffic_lights_common.hpp](../../../tree/master/src/traffic_lights_common.hpp).

<a name="showcase4"></a>
## 5 - Network Driven Traffic Lights

Let's say you want to be able to control the lights through a TCP/IP network. This will demonstrate how we can use the event loop both for timeouts and network asynchronous reception, when using Boost::Asio.

First, lets talk about the client. We skip the boring Asio stuff
(that you can find in
[src/traffic_light_client.cpp](../../../tree/master/src/traffic_lights_client.cpp)),
and get to the core part of the client:
```C++
	std::cout << "Enter key: (a:warning on, b:warning off, c:reset): ";
	do
	{
		std::string str;
		std::cin >> str;
		socket.send_to(                 // blocking data send
			boost::asio::buffer( str ),
			endpoint
		);
	}
	while(1);
```

This will just loop over and over and send the string to the server, using a UDP socket connected on port 12345.

Now the server. The potential problem we need to deal with is that:
- the server needs to hold the FSM, so that network-received commands can take action on it,
- the server also needs to hold the socket,
- with Boost::asio, to create a socket, we need to provide an "io_service" object,
- if we embed that object inside the FSM (as it is done in the previous example), we won't be able to create the socket...

So here, we demonstrate another use case: we will use the provided asio-based timer class, but we will instanciate it separately, it will **not** be embedded inside the FSM:
```C++
#define SPAG_USE_ASIO_WRAPPER
#include "spaghetti.hpp"

SPAG_DECLARE_FSM_TYPE_ASIO( fsm_t, States, Events, std::string );
```
This macro also defines the class `AsioEL` ("boost::Asio Event Loop") that you will need to instanciate (see below).

The server will inherit from some generic UDP server (also included):
```C++
struct MyServer : public UdpServer<1024>
{
	MyServer( boost::asio::io_service& io_service, int port_no )
		: UdpServer( io_service, port_no )
	{}

	std::vector<BYTE> getResponse( const Buffer_t& buffer, std::size_t nb_bytes ) const
	{
		std::cout << "received " << nb_bytes << " bytes\n";
		switch( buffer.at(0) )
		{
			case 'a':
				fsm.processEvent( ev_WarningOn );
			break;
			case 'b':
				fsm.processEvent( ev_WarningOff );
			break;
			case 'c':
				fsm.processEvent( ev_Reset );
			break;
			default:
				std::cout << "Error: invalid message received !\n";
		}
		return std::vector<BYTE>(); // return empty vector
	}
	fsm_t fsm;
};
```

And the `main()` function will instanciate the timer class and **assign it** to the fsm
(skipped the parts about the keyboard UI thread):
```C++
int main()
{
// instanciate Timer/Event loop
	spag::AsioEL asio;

// create udp server with asio, listening on port 12345
	MyServer server( asio.get_io_service(), 12345 );

	configureFSM<fsm_t>( server.fsm );
	server.fsm.assignEventHandler( &asio );

	server.start_receive();  // start reception, see UdpServer
	server.fsm.start();      // blocking !
}
```

For details, check the source file:
[src/traffic_light_3.cpp](../../../tree/master/src/traffic_lights_3.cpp).

<a name="concurrent"></a>
## 6 - Running Concurrent FSM

When using a timer associated with the FSM, the default behavior is that starting the FSM will be a blocking function.
If you need to run several FSMs concurrently, then you need to handle the event loop separately, and the `start()` function must not be blocking.
This implies that the Timer class must not hold the timer.

To use the provided Event loop class `AsioEL` in this situation, you need to define the symbol `SPAG_EXTERNAL_EVENT_LOOP`.
This will change the class behavior, it will not embed the Boost::asio event loop structure (aka "io_service" or "io_context"),
it is now up to your code to provide it.
You can then start all the needed FSM, then eventually start the event loop, usually with something looking like: `io_service.run()`

This is demonstrated in sample program [src/sample_2.cpp](../../../tree/master/src/sample_2.cpp).<br>

<a name="inner_events"></a>
## 7 - Using inner events and pass states

### 7.1 Inner events

#### Introduction

In some situations, a FSM has to change its behavior depending on some internal event.
For example
- "If state X has been activated 5 times, then switch to state Y instead of state Z"
- "if some class member variable has value 10, then, when on state X, we want to switch to state Y instead of having a timeout leading to state Z"

This is implemented in Spaghetti by using so-called "inner-events", as opposed to other events, that are called "external events".
These are identified as enum values, and must be part of the "Events" enum, just as the others.

This feature requires the use of
[signals](https://en.wikipedia.org/wiki/Signal_(IPC)),
thus it is available only if symbol `SPAG_USE_SIGNALS` is defined (see [build options](spaghetti_options.md) ).

#### Usage

Inner event are declared as regular events, they must be part of the "events" enum.

To **configure** inner events, you may use these fsm member functions:

- `void assignInnerTransition( ST st1, EV iev, ST st2 )`<br>
This means:  *when we are on state `st1` and if event `iev` has occurred, then switch to state `st2`*.

- `void assignInnerTransition( EV iev, ST st )`<br>
This means:  *whatever state is active, if event `iev` has occurred, then switch to state `st`*.

- `void disableInnerTransition( EV ev, ST st_from )`<br>
This can be used to disable the inner event `ev` transition that may have been assigned to state `st_from`.

To **trigger** an inner event `ev`:
```
	fsm.activateInnerEvent( ev );
```
In some situations, you might need to **de-activate** an inner event, this can be done with:
```
	fsm.clearInternalEvent( ev );
```

More details [here](spaghetti_devinfo.md#inner_events) .

<a name="pass_states"></a>
### 7.2 - Pass states

Pass states are states having a single transition to the next state, with that transition being always active.
It is also handled using signals, so the symbol `SPAG_USE_SIGNALS` must also be defined.

These transitions appear in the config function output and on the graph with the string "AAT", meaning "Always Active Transition".

They are defined using the member function `assignAAT( st1, st2 );`.<br>
This will disable all other transitions that *may* have been assigned previously between these two states.

See for example [src/sample_1b.cpp]( ../../../tree/master/src/sample_1b.cpp)

<a name="additional_stuff"></a>
## 8 - Additional features

<a name="config"></a>
### 8.1 - Configuration of the FSM
For FSM configuration, you can proceed as described above but it can be tedious for larger situations.
Instead, you can also assign directly a </b>transition matrix</b>, with the events in lines, the states in columns, and each table cell defining the state
to switch to.
This is done with the member function `assignTransitionMat()`.

For example, say you have a 3 states (`st0,st1,st2`) and 2 events (`ev1,ev2`)
and you want to switch from each of the states to the next one if the "event 1" occurs, and switch back to initial state if "event 2" occurs.
You can build a "matrix" (vector of vector) holding that information and assign it to the FSM.

```C++
	std::vector<std::vector<En_States>> trMat = {
//           st0 - st1 - st2
/* ev1 */  { st1 , st2 , st0 },
/* ev2 */  { st0 , st0 , st0 }
	};
	fsm.assignTransitionMat( trMat );
```
However, this doesn't take into account the fact that some of the transitions from one state to another may or may not be allowed.
So you also need to provide an **authorization matrix**, that defines what can and what cannot be done.

This is done with the member function `assignEventMat()`.
For example and with the above code, if we want to disable transitioning from state st2 to st0 when event ev2 occurs, it will be this:
```C++
	std::vector<std::vector<char>> eventMat = {
//          st0 - st1 - st2
/* ev1 */  { 1 ,   1 ,   1 },
/* ev2 */  { 1 ,   1 ,   0 }
	};
	fsm.assignEventMat( eventMat );
```
As you expect, '1' (or any other non-null value) means transition is allowed, '0' means it is disabled.

This will produce this state machine:

![sample_FSM](doc_fig1.svg)

However, in such a situation, it would be simpler to use the following two member functions:
```C++
	fsm.allowAllEvents();
	fsm.allowEvent( ev2, st2, false );
```

The first one allows all events for all the states.
The second one disables event `ev2` when on state `st2`.
Please note that this latter function can also be used to **allow** an event, for example one could write:
`fsm.allowEvent(ev2, st2, true)`
or just
`fsm.allowEvent(ev2, st2)`.

You can also copy all the configuration from one instance of an FSM to another:
```C++
	fsm_t fsm_1, fsm_2;
// configure fsm_1
	...
// copy config of fsm_1 to fsm_2
	fsm_2.assignConfig( fsm_1 );
```

<a name="printconfig"></a>
### 8.2 - Printing Configuration of the FSM

You can printout the whole configuration of the FSM with a member function:
```C++
	fsm_t fsm;
	 // do the configuration
	fsm.printConfig( std::cout, "my Config" );
```
The second argument is optional.

This will print out both the transition table and the state information.
For example, consider this, produced with
`$ bin/testA_2`:

```
* FSM Configuration:
 - Transition table:
                 STATES:
EVENTS           S00 S01 S02 S03 S04 S05 S06
---------------|----------------------------
my_event   E00 | S01  .  S03  .   .   .   .
Ev-1       E01 |  .   .   .   .   .   .  S00
*Timeout*   TO | S02  .  S04 S04 S00 S00 S00
*  AAT  *  AAT |  .  S02  .   .   .   .   .

 - State info:
S00:init state| TO: 1500 ms => S02 (state_2)
   :          | IT (I): E00 (my_event) => S01 (St-1)
S01:St-1      | AAT: => S02 (state_2)
S02:state_2   | TO: 600 ms => S04 (St-4)
   :          | IT (I): E00 (my_event) => S03 (St-3)
S03:St-3      | TO: 600 ms => S04 (St-4)
S04:St-4      | TO: 600 ms => S00 (init state)
S05:St-5      | TO: 600 ms => S00 (init state)
S06:St-6      | TO: 600 ms => S00 (init state)
---------------------
Spaghetti: Warning, state S05 (St-5) is unreachable
Spaghetti: Warning, state S06 (St-6) is unreachable
```

The transition table is pretty much simple to understand:
for each state (columns), it shows the next state, depending on the event (lines).
It has a line for Timeouts, where we can see that all the states have a timeout assigned, except for state S01, that has an "Always Active" transition.

The second part shows, for each state, the "special events"
(Time-outs, Internal events, Always Active Transitions).
We can see that on state S00 (named here "init state"), a timeout will occur after 1.5 s. and switch to state S02.
But this state also has an Internal Transition (IT), currently Inactive (thus the (I)).
It will be triggered by the internal event E00 (named "my_event"), and switching to state S01 will occur.

State S01 is a "Pass State": it has a "Always Active Transition" (AAT), so when activated, it will switch immediately to state S02.

<a name="checks"></a>
### 8.3 - Checking configuration

At startup (when calling `fsm.start()`), a general checking is done through a call of  `fsm.doChecking()`.
This is to make sure nothing wrong will happen.
This function can either throw an error in case of an invalid situation, or just print out a warning.

This function is public, so you may call it yourself, in case you need to make sure everything is correct before running.

A warning is issued in the following situations:
- a state is unreachable: it is referenced in the states enum but no transitions leads to it.
- a state is a "Dead-end": once in this state, there is no transition leading to another state: the FSM is "stuck".

These latter situations will not disable running the FSM, because they may occur in developement phases,
where everything is not finished but the user wants to test things anyway.

<a name="getters"></a>
### 8.4 - FSM getters and other information
Some self-explaining member function that can be useful in user code:

 - `size_t nbStates()`: returns nb of states
 - `size_t nbEvents()`: returns nb of events (only "hardware" and "inner" ones, not timeouts)
 - `States currentState()`: returns current state (`States` being the enum you have used to declare the FSM type)
 - `States previousState()`: returns previous state
 - `size_t getStateIndex( std::string s )`: returns internal index of state with assigned string `s`
 - `size_t getEventIndex( std::string s )`: returns internal index of event with assigned string `s`
 - `timeOutDuration( States st )`: returns duration of timeout on state `st`, as a `std::pair (Duration, DurUnit)`.
 First element will be 0 if no timeout assigned to that state.

 *Note:* `getStateIndex()` and `getEventIndex()`:
 - are only available if build option `SPAG_ENUM_STRINGS` build option is activated, see [build options](spaghetti_options.md)
 - will throw if string not found

Other stuff:
- The version of the library is in the symbol `SPAG_VERSION`, can be printed with:
```C++
std::cout << "version=" << SPAG_VERSION << '\n';
```


--- Copyright S. Kramm - 2018-2020 ---
