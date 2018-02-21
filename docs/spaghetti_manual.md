# Spaghetti manual

Homepage: https://github.com/skramm/spaghetti

This page demonstrates usage of the library through several showcases, and gives additional details.
All the example are included an runnable in the src folder, just ```make demo```, then run ```bin/program_name```.

For a reference manual, run ```make doc```, then open
```html/index.html``` (needs doxygen).

### Summary
1. [Fundamental concepts](#concepts)
1. [Showcase 1: Hello World for FSM](#showcase1)
1. [Showcase 2: let's use a timer](#showcase2)
1. [Showcase 3 : mixing timeout with hardware events](#showcase3)
1. [Showcase 4 : adding network control](#showcase4)
1. [Concurrent FSM](#concurrent)
1. [Additional stuff](#additional_stuff)
   1. [Configuration](#config)
   1. [Checking configuration](#checks)
   1. [FSM getters and other information](#getters)
1. [Build options](spaghetti_options.md)
1. [FAQ](spaghetti_faq.md)

Before you can get started, all you need is to download and install the file ```spaghetti.hpp``` to a location where your compiler can find it
(On Linux, ```/usr/local/include``` is usually pretty good).
Then you can create a program.

<a name="concepts"></a>
## 1 - Fundamental concepts

Configuration of the state machine is a mix of static and dynamic: the number of states and hardware events is fixed at build time, but it is possible to add transitions at run-time.

States and events are simply defined as enum values:
```C++
enum States { st_1, st_2, st_3, NB_STATES }; // 3 states
enum Events { ev_1, ev_2, ev_3,  NB_EVENTS }; // 3 events
```
Naming is free except for the last value that
**must** be ```NB_STATES``` and ```NB_EVENTS```, respectively.
For the states, the first one (having value 0) will be the initial state.

You can use either classical C++03 enums or C++11 scoped enums (```enum class { st1, st2,...```).
The latter adds of course some type safety.
But all these values are internally casted to integers, so you must not assign values to the enumerators.

Events can be of two types:
- "hardware" events (basically, it can be just a keyboard press): those are the ones you need to define in the enum above.
But you can have none ! In that case, just define the events enum as:
```C++
	enum Events { NB_EVENTS }
```
- "time out" events, when you want to switch from state A to state B after 'x' seconds. There are handled separately.

For the latter case, you need to provide a special "timing" class, that will have some requirements.
You will need to instanciate an object of that class, then assign it to the FSM in the configuration step.
Fortunately, this is made easy for the usual case, no worry.<br>
For the other events, it is up to your code to detect these, and then call some Spaghetti member function.

<a name="showcase1"></a>
## 2 - Showcase 1: Hello World for FSM

In this example, we show the "Hello World" of FSM, which is the "Turnstyle" FSM
(see [WP link)](https://en.wikipedia.org/wiki/Finite-state_machine#Example:_coin-operated_turnstile)).

First, create enums for states and events:

```C++
#include "spaghetti.hpp"
enum States { st_Locked, st_Unlocked, NB_STATES };
enum Events { ev_Push, ev_Coin, NB_EVENTS };
```
Then, create the FSM data type:
```C++
SPAG_DECLARE_FSM_TYPE_NOTIMER( fsm_t, States, Events, bool );
```
This means you create the type ```fsm_t``` (it's a typedef), using ```States``` and ```Events```, with callback functions having a ```bool``` as argument.

Now, you can instanciate the fsm:

```C++
int main()
{
	fsm_t fsm;
```

Next, you need to configure your FSM, that is define what event in what state will trigger switching to what other state.
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
In this case it is the same for all, but you can have a different function for all the states.
You don't even need to provide a callback function, it is always optional:
some states can have one while others won't.
The only constraint is that they must have the same signature.

```C++
	fsm.assignCallback( st_Locked,   cb_func, true );
	fsm.assignCallback( st_Unlocked, cb_func, false );
```

Alternatively (and that is useful if you have a lot of states), you can provide the callback function for all the states, and separately set the argument value:

```C++
	fsm.assignGlobalCallback( cb_func );
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
Now, to run this you need to call ```start()```.

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
while( 1 );
```

That's it!
All of this is given as a sample program,
see file
[src/turnstyle_1.cpp](https://github.com/skramm/spaghetti/blob/master/src/turnstyle_1.cpp)
and/or just clone repo and enter
```make demo -j4``` followed by ```bin/turnstyle_1```.

<a name="showcase2"></a>
## 3 - Showcase 2: let's use a timer

Lets consider another situation: a traffic light going automatically through the three states: Red, Green, Orange. So we define this:
```C++
enum States { st_Red, st_Orange, st_Green, NB_STATES };
enum Events { NB_EVENTS };
```
You need to provide a Timer class that can be used by the FSM, and that provides **asynchronous** timeouts and an event waiting loop.

Oh, wait, we'll talk about this later, fortunately, Spaghetti provides an easy way to handle this. The only requirement is that you must have [Boost Asio](http://www.boost.org/doc/libs/release/libs/asio/) installed on your machine. As this is fairly common these days, lets assume this is okay. If not,
[check here](https://duckduckgo.com/?q=installing+boost+asio).

To use the provided Timer class, you need to pass an option to Spaghetti by defining the symbol ```SPAG_EMBED_ASIO_TIMER```
([see here details about the build options](spaghetti_options.md)).

So now we use the second form of the type declaration macro:
```C++
#define SPAG_EMBED_ASIO_TIMER
#include "spaghetti.hpp"

enum States { st_Red, st_Orange, st_Green, NB_STATES };
enum Events { NB_EVENTS };

SPAG_DECLARE_FSM_TYPE_ASIO( fsm_t, States, Events, std::string );
```
The configuration step will go as follows (assuming the states are names st_Red, st_Green, st_Orange).
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
	fsm.assignGlobalCallback( myCallback );
	fsm.assignCallbackValue( st_Red,    "red" );
	fsm.assignCallbackValue( st_Orange, "Orange" );
	fsm.assignCallbackValue( st_Green,  "Green" );
```
Once configuration is done, just start the FSM. But remember, this is now a **blocking** function!
```C++
	fsm.start();
```
All of this can be found in the runnable example in
[src/traffic_lights_1.cpp](https://github.com/skramm/spaghetti/blob/master/src/traffic_lights_1.cpp).
This sample has an additional "init state", lasting 3s.
Below is the graph produced by Graphviz from the output generated by ```writeDotFile()``` (see below for details).

![traffic_lights_1](traffic_lights_1.svg)

<a name="showcase3"></a>
## 4 - Traffic lights with buttons

Now, lets consider the same system but with added buttons to control it.
We add a "Warning" button to make the system enter a "orange blinking state".
Actually, that won't be a unique state, but two different states,
"Blink On" and "Blink Off".

Oh, and also a "Warning off" button (to return to regular cycle), and a "Reset" button (can be useful).

So we have the following states and events:
```C++
enum EN_States {
	st_Init,
	st_Red,
	st_Orange,
	st_Green,
	st_BlinkOn,
	st_BlinkOff,
	NB_STATES
};
enum EN_Events {
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

	fsm.assignTransitionAlways( ev_Reset,     st_Init ); // if reception of message ev_Reset, then switch to state st_Init, whatever the current state is
	fsm.assignTransitionAlways( ev_WarningOn, st_BlinkOn );
	fsm.assignTransition(       st_BlinkOff,  ev_WarningOff, st_Red );
	fsm.assignTransition(       st_BlinkOn,   ev_WarningOff, st_Red );
```

As the ```start()``` member function is blocking, we need to handle the keyboard events in a different thread.
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

All of this can be found in the runnable example in [src/traffic_lights_2.cpp](https://github.com/skramm/spaghetti/blob/master/src/traffic_lights_2.cpp)
and its companion header file
[src/traffic_lights_common.hpp](
https://github.com/skramm/spaghetti/blob/master/src/traffic_lights_common.hpp).

<a name="showcase4"></a>
## 5 - Network Driven Traffic Lights

Let's say you want to be able to control the lights through a TCP/IP network. This will demonstrate how we can use the event loop both for timeouts and network asynchronous reception, when using Boost::Asio.

First, lets talk about the client. We skip the boring Asio stuff
(that you can find in
[src/traffic_light_client.cpp](https:/github.com/skramm/spaghetti/blob/master/src/traffic_lights_client.cpp)),
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

This will just loop over and over, and send the string to the server, using a UDP socket connect on port 12345..

Now the server. The potential problem we need to deal with is that:
- the server needs to hold the FSM, so that network-received commands can take action on it,
- the server also needs to hold the socket,
- with Boost::asio, to create a socket, we need to provide an "io_service" object,
- if we embed that object inside the FSM, we won't be able to create the socket...

So here, we demonstrate another use-case: we will use the provided asio-based timer class, but we will instanciate it separately, it will not be embedded inside the FSM:
```C++
#define SPAG_USE_ASIO_TIMER
#include "spaghetti.hpp"

SPAG_DECLARE_FSM_TYPE( fsm_t, States, Events, spag::AsioWrapper, std::string );
```

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

And the main() function will instanciate the timer class and **assign it** to the fsm (skipped the parts about the keyboard UI thread):
```C++
int main()
{
	spag::AsioWrapper<States,Events,std::string> asio;  // instanciate Timer class
	MyServer server( asio.get_io_service(), 12345 ); // create udp server with asio
	configureFSM<fsm_t>( server.fsm );
	server.fsm.assignTimer( &asio );
	server.start_receive();           // start reception, see UdpServer
	server.fsm.start();               // blocking !
}
```

For details, check the source file:
[src/traffic_light_3.cpp](https:/github.com/skramm/spaghetti/blob/master/src/traffic_lights_3.cpp).

<a name="concurrent"></a>
## 6 - Running Concurrent FSM

TO BE CONTINUED

For details, check the source file:
[src/sample_2.cpp](https:/github.com/skramm/spaghetti/blob/master/src/sample_2.cpp).

<a name="additional_stuff"></a>
## 7 - Additional facilities

<a name="config"></a>
### 7.1 - Configuration of the FSM
For FSM configuration, you can proceed as described above but it can be tedious for larger situations.
Instead, you can also assign directly a </b>transition matrix</b>, with the events in lines, the states in columns, and each table cell defining the state
to switch to.
This is done with the member function ```assignTransitionMat()```.

For example, say you have a 3 states (```st0,st1,st2```) and 2 events (```ev1,ev2```)
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

This is done with the member function ```assignEventMatrix()```.
For example and with the above code, if we want to disable transitionning from state st2 to st0 when event ev2 occurs, it will be this:
```C++
	std::vector<std::vector<En_States>> eventMat = {
//          st0 - st1 - st2
/* ev1 */  { 1 ,   1 ,   1 },
/* ev2 */  { 1 ,   1 ,   0 }
	};
	fsm.assignEventMatrix( eventMat );
```
As you can see, '1' means transition is allowed, '0' means it is disabled.

Of course in such a situation, it would be simpler to use the following two member functions:
```C++
	fsm.allowAllEvents();
	fsm.allowEvent( ev2, st2, false );
```

The first one allows all events for all the states.
The second one disables event ```ev2``` when on state ```st2```.
Please note that this latter function can also be used to **allow** an event, for example one could write:
```fsm.allowEvent( ev2, st2, true )```
or just
```fsm.allowEvent( ev2, st2 )```.

You can also copy all the configuration from one instance of an FSM to another:
```C++
	fsm_t fsm_1, fsm_2;
// configure fsm_1
	...
// copy config of fsm_1 to fsm_2
	fsm_2.assignConfig( fsm_1 );
```
<a name="checks"></a>
### 7.2 - Checking configuration

At startup (when calling ```fsm.start()```), a general checking is done through a call of  ```fsm.doChecking()```.
This is to make sure nothing wrong will happen.
This function can either throw an error in case of an invalid situation, or just print out a warning.

This function is public, so you may call it yourself, in case you need to make sure everything is correct before running.

A warning is issued in the following situations:
- a state is unreachable, that is it is referenced in the states enum but no transitions leads to it.
- a state is a "Dead-end": once in this state, there is no transition leading to another state: the FSM is "stuck".

These latter situations will not disable running the FSM, because they may occur in developement phases, where everything is not finished but the user wants to test things anyway.

<a name="getters"></a>
### 7.3 - FSM getters and other information
Some self-explaining member function that can be useful in user code:

 - ```nbStates()```: returns nb of states
 - ```nbEvents()```: returns nb of events (only "hardware" ones, not timeouts).
 - ```currentState()```: returns current state
 - ```timeOutDuration( EN_States )```: returns duration of timeout

Other stuff:
- Printing the configuration:
The member function ```printConfig()``` will print the current configuration, for example:
```C++
fsm.printConfig( stsd::cout );
```

- Printing runtime data:
If your FSM is able to stop (after a call to ```stop()```), you can printout the runtime data with
```C++
fsm.printLoggedData( std::cout );
```
This will print out, in a CSV style:
 - the state counters (how many of times they were activated)
 - the event counters. This also include the number of timeouts, and the number of "Always Active" transitions that were encountered.
 - a timed log of the transitions from one state to another.

 You can pass to this function a second parameter, to specify **what** data you want:
 - ```PrintFlags::stateCount``` : print state counters
 - ```PrintFlags::eventCount``` : print event counters
 - ```PrintFlags::history``` : print runtime history
 - ```PrintFlags::all```: all of the above (default value)
<br>
These flags can be "OR-ed" to have several ones active.
<br>
Please note that if the symbol ```SPAG_ENUM_STRINGS``` (see [Build options](spaghetti_options.md) is defined, the strings will appear in this data.
Also see how these functions are used in the provided sample programs.


--- Copyright S. Kramm - 2018 ---
