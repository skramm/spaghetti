# Spaghetti manual

homepage: https://github.com/skramm/spaghetti

This page demonstrates usage of the library through several showcases, and gives additional details.
All the example are included an runnable in the src folder, just ```make demo```, then run ```bin/program_name```.

For a reference manual, run ```make doc```, then open
```html/index.html``` (needs doxygen).

- [Fundamental concepts](#concepts)
- [Showcase 1: Hello World for FSM](#showcase1)
- [Showcase 2: let's use a timer](#showcase2)
- [Showcase 3 : mixing timeout with hardware events](#showcase3)
- [Additional stuff](#additional_stuff)
- [Build options](#build_options)
- [FAQ](#faq)

<a name="concepts"></a>
## Fundamental concepts

All you need to get this running is to download and install the file ```spaghetti.hpp```.
then you can create a program.

States and events are simply defined as enum values:
```C++
enum States { st_1, st_2, st_3, NB_STATES };
enum Events { ev_1, ev_2, ev_3,  NB_EVENTS };
```
Naming is free except for the last value that
**must** be ```NB_STATES``` and ```NB_EVENTS```, respectively.
For the states, the first one (having value 0) will be the initial state.

Events can be of two types:
 - either "hardware" events (basically, it can be just a keyboard press).
 - or "time outs", when you want to switch from state A to state B after 'x' seconds

For the latter case, you need to provide a special "timing" class, that will have some requirements (see below).
you will need to "assign" the timer to the FSM in the configuration step.
For the other events, it is up to your code to detect these, and then call some member function.

<a name="showcase1"></a>
## Showcase 1: Hello World for FSM

In this example, we show the Hello World" of FSM, which is the "Turnstyle" FSM
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
This means you declare the type ```fsm_t```, using ```States``` and ```Events```, with callback functions having a bool as an argument.
(Alternatively, you could also use a ```typedef```, but lets say this is easier at present.)

Now, you can instanciate the fsm:

```C++
int main()
{
	fsm_t fsm;
```

Now, you need to configure your FSM, that is define what event in what state will trigger switching to what state.
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
Now, how to run this. (Here, you don't really need to because there are no timeout, but its recommended to do so, to avoid forgetting it afterwards. Just call ```start()``` ).

Here the events will be triggered by the keyboard, so lets do this:
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
see file ```src/turnstyle_1.cpp``` and/or just clone repo and enter
```make demo -j4``` followed by ```bin/turnstyle_1```.

<a name="showcase2"></a>
## Showcase 2: let's use a timer

Lets consider another situation: a traffic light going automatically through the three states: Red, Green, Orange.
You need to provide a Timer class that can be used by the FSM, and that provides **asynchronous** timeouts and
an event waiting loop.
To be usable here, this class needs to provide these three functions:
- ```timerInit()```: initialize the timer
- ```timerStart()```: start a timeout
- ```timerCancel()```: cancel the timer

Any timing class can be used, in the provided sample ```src/traffic_lights.cpp``` we demonstrate the use of [boost::asio](http://www.boost.org/doc/libs/release/libs/asio/):
```C++
template<typename ST, typename EV, typename CBA>
struct AsioWrapper
{
	boost::asio::io_service io_service;
	std::unique_ptr<boost::asio::deadline_timer> ptimer;

	AsioWrapper()
	{
		ptimer = std::unique_ptr<boost::asio::deadline_timer>( new boost::asio::deadline_timer(io_service) );
	}
...
```
(see full code for details)
The three type parameters are the states, the events, and the type of the argument of the callback function.

Once you have declared this class, the declaration of the data type of the FSM will be done with the second form of the macro:
```C++
SPAG_DECLARE_FSM_TYPE( fsm_t, States, Events, AsioWrapper, bool );
```
The configuration step will go as follows (assuming the states are names st_Red, st_Green, st_Orange).
As you can guess, we have here timeouts of 5, 5, and 1 seconds:

```C++
	fsm.assignTimeOut( st_Red,    5, st_Green  );
	fsm.assignTimeOut( st_Green,  5, st_Orange );
	fsm.assignTimeOut( st_Orange, 1, st_Red   );
```

Once configuration is done, you need to instanciate the timer, assign it to the FSM, and start it:

```C++
	AsioWrapper<EN_States,EVENT,std::string> asio;
	fsm.assignTimer( &asio );
	fsm.start();
```

Done !
Remember: here the ```fsm.start()``` call needs to be the **last** one, as it is now a blocking function.

All of this can be found in the runnable example in ```src/traffic_lights_1.cpp```.

<a name="showcase3"></a>
## Traffic lights with buttons

Now, lets consider the same system but with added buttons to control it.
We add a "Warning" button to make the system enter a "orange blinking state".
Actually, that won't be a unique state, but two different states,
"Blink On" and "Blink Off".

Oh, and also a "Warning off" button (to return to regular cycle), and a "Reset" button (can be useful).

So we have now the following states and events:
```C++
enum EN_States { st_Init=0, st_Red, st_Orange, st_Green, st_BlinkOn, st_BlinkOff, NB_STATES };
enum EN_Events {
	ev_Reset=0,    ///< reset button
	ev_WarningOn,  ///< blinking mode on
	ev_WarningOff, ///< blinking mode off
	NB_EVENTS
};
```

Confiuration of the FSM will be as previously, we just add this:
```
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

And we start that thread before starting the FSM:
```C++
	std::thread thread_ui( UI_thread<fsm_t>, &fsm );
	fsm.start();  // blocking !
	thread_ui.join();
```

All of this can be found in the runnable example in ```src/traffic_lights_2.cpp```
(and its companion header file ```src/traffic_lights_common.hpp```).

Once you have trid this, you can also try
```bin/traffic_light_3```.
It is the same but with an added udp network capability:
By running the  program ```bin/traffic_lights_client``` in another shell window
(```bin/traffic_lights_client localhost```) or even on another machine, you can trigger the events using the network.

<a name="additional_stuff"></a>
## Additional facilities

Some self-explaining member function that can be useful in user code:

 - ```nbStates()```
 - ```nbEvents()```
 - ```currentState()```
 - ```timeOutDuration( EN_States )```

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
This will print out
 - the state counters (how many of times they were activated)
 - the event counters. This also include the number of timeouts, adn the bnumber of "Always Active" transitions that were encountered.
 - a timed log of the transitions from one state to another.

Please note that if the symbol ```SPAG_ENUM_STRINGS``` (see below) is defined, the strings will appear in this data.
Also see how these functions are used in the provided sample programs.


<a name="build_options"></a>
## Build options

Several symbols can change the behavior of the library and/or add additional capabilities, you can define them either by adding them in your makefile
(with GCC, its ```-DSPAG_SOME_SYMBOL``` ), or by hardcoding in your program, BEFORE including the library file, like this:

```C++
#define SPAG_SOME_SYMBOL
#include "spaghetti.hpp"
```

They all start with these 5 characters: ```SPAG_```

You can printout at runtime the build options with this static function:
```cout << fsm_t::buildOptions()```
(once you have defined the type ```fsm_t```).

The available options/symbols are:
- ```SPAG_PRINT_STATES``` : will print on stdout the steps, useful only for debugging your SpagFSM
- ```SPAG_ENABLE_LOGGING``` : will enable logging of dynamic data (see spag::SpagFSM::printLoggedData() )
- ```SPAG_FRIENDLY_CHECKING```: A lot of checking is done to ensure no nasty bug will crash your program.
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
If this symbol is not defined, regular checking is done with the classical ```assert()```.
As usual, this checking can be removed by defining the symbol ```NDEBUG```.

- ```SPAG_ENUM_STRINGS``` : this enables the usage of enum-string mapping, for events only.
You can provide a string either individually with
```C++
	fsm.assignString2Event( std::make_pair(EV_MyEvent, "something happened" );
```
or globally, by providing a vector of pairs(enum values, string). For example:
```C++
	std::vector<std::pair<EVENT,std::string>> v_str = {
		{ ev_Reset,       "Reset" },
		{ ev_WarningOn,  "Warning On" },
		{ ev_WarningOff, "Warning Off" }
	};
	fsm.assignStrings2Events( v_str );
```
These strings will then be printed out when calling the ```printConfig()``` and ```printData()``` member function.


<a name="faq"></a>
## FAQ

- Q: What is the timer unit ?
- A: There are no timer units. Timing is stored as untyped integer value, it is up to the timer class you define to handle the considered unit.

- Q: What if I have more that a single argument to pass to my callback function ?
- A: then, you'll need to "pack it" in some class, or use a ```std::pair```, or ```std::tuple```.

- Q: can I use a callback function with a void parameter ( ```void my_callback()```)
- A: No, unfortunately. This is because void is not a type, you can't pass it as template argument. But you can always use anything, say an integer, and ignore its value.

- Q: Why that name ? Where does that come from ?
- A: Naming is hard. But, lets see: Finite State Machine = FSM = Flying Spaghetti Monster
(see [WP](https://en.wikipedia.org/wiki/Flying_Spaghetti_Monster)).
So you got it.
(and certainly not related to [this](https://en.wikipedia.org/wiki/Spaghetti_code), hopefully!)

