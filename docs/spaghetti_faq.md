# Spaghetti Manual: FAQ


- [Homepage](https://github.com/skramm/spaghetti)
- [Manual](spaghetti_manual.md)


 Q&A:

- **Q**: *What is the timing unit?*<br>
**A**: The values are stored as integer values with an associated `DurUnit` enumeration value, so you can select between seconds, milliseconds, and minutes, and this for each timeout value.<br>
It is up to the Timer class to handle these
(the provided optional timer class `AsioWrapper` does).
The default value is "seconds".<br>
When you add the following configuration line, it will be considered as 5 seconds.
```C++
	fsm.assignTimeOut( st_Red, 5, st_Green );
```
Actually, it uses the current default timer unit, so this is true only if you haven't changed it.
This one can be changed any time, for example to milliseconds, with the following command
(which will **not** change the settings of already assigned timeouts):
```C++
	fsm.setTimerDefaultUnit( spag::DurUnit::ms );
```
Other possible values are `sec`,`min`.
Alternatively, you can also give the units when defining Timeouts. This will for example define a 5 minutes Timeout:
```C++
	fsm.assignTimeOut( st_Red, 5, spag::DurUnit::min, st_Green );
```
If for some reason (templated function in a header, as in `src/traffic_lights_common.hpp`) you don't have access to the `DurUnit` type, you can also use string values:
```C++
	fsm.assignTimeOut( st_Red, 5, "min", st_Green );
```
Internally, the timing is handled through the C++11 `chrono` library
[duration type](http://en.cppreference.com/w/cpp/chrono/duration).

- **Q**: *How does this library differ from the other ones?*<br>
**A**: Most of the other libraries define the states as C++ classes (see below other entry on hierarchical FSM).
While this can have some advantages, it requires you to create a struct/class for each state.
With Spaghetti, you just add an enumerator value and define callback functions and transitions with member function calls.
If you need advanced features of the UML state machine, then this is not for you.

- **Q**: What are the requirements on the event loop/timer class template, if I want to use a different then the one that is provided?<br>
**A**: TODO

- **Q**: *How are runtime errors handled?*<br>
**A**: A lot of stuff is checked at build time but some errors in user code can only be detected at runtime.
Configuration basic errors are handled using exceptions and will throw a
[`std::logic_error`](http://en.cppreference.com/w/cpp/error/logic_error).<br>
For runtime errors (in the sense: "FSM runtime"), due to the inherent nature of the threading mechanism,
critical errors are handled through custom assertions.
This is because throwing an exception from a thread will have other threads call `std::terminate()`, thus
breaking the exception handling procedure:
exception cannot be catched and the user only gets some obscure message
(usually "`terminate called without an active exception`").<br>
Thus it is clearer to trigger an assert that will cleanly exit the program with an appropriate error message written to `std::cerr`.<br>
Other non critical errors will throw a
[`std::runtime_error`](http://en.cppreference.com/w/cpp/error/runtime_error).

- **Q**: *What if I have more that a single argument to pass to my callback function?*<br>
**A**: You'll need to "pack it" in some class, or use a
[`std::pair`](http://en.cppreference.com/w/cpp/utility/pair),
or a [`std::tuple`](http://en.cppreference.com/w/cpp/utility/tuple).

- **Q**: *Can I use a callback function with a void parameter ( `void my_callback()`)*<br>
**A**: No, unfortunately.
This is because `void` is not a type, you can't pass it as template argument.
But you can always use anything, say an integer, and ignore its value.

- **Q**: *Can I pass the FSM object itself as callback argument?*<br>
**A**: No, as the callback argument type is a template parameter of the FSM.
You would get into some infinite template recursion issue...
But you can then make the FSM object global, so the callback functions can access it.
This is demonstrated in sample program
[`src/sample_3.cpp`](../../../tree/master/src/sample_3.cpp).<br>
But **be careful**, because in that case, no checking will be done on what you will change on the configuration of the FSM structure.
You could make the FSM run into some invalid configuration, leading to undefined behavior.<br>

- **Q**: *How can I assign callback argument values in a more convenient way?
I have a lot of states, and assigning these one by one (with `assignCallbackValue()`) is tedious*<br>
**A**: If you have no special needs on the value of the callback arguments (i.e. you only need them to be different values),
then you can use `assignCallbackAutoval( cb )`.
This will assign to all the states the callback function `cb(int)`
and will assign as callback argument value the **index** of the corresponding state.
Of course, this requires that you have defined the FSM type with some kind of integer type as callback argument.
But don't worry, if you don't, you will get a compiler error, this is checked at build time.

- **Q**: *Can I have as callback function a class member function?*<br>
**A**: Sure! This is of course useful so that the callback can handle some data.
The only technical issue is that you cannot store a non-static function into a `std::function` object.
So the workaround is that you will need to use a *binding* trick. This is demonstrated in
[`src/traffic_lights_1c.cpp`](../../../tree/master/src/traffic_lights_1c.cpp):<br>
`   fsm.assignCallback( std::bind( &MyClass::callback, this, std::placeholders::_1 )`<br>
Fortunately, you can use two helper macros to avoid that uglyness:
  * to assign the function `CallbackFunc` belonging to class `Class` to the fsm state `State`:<br>
`SPAG_ASSIGN_MEMBER_CALLBACK( fsm, State, Class, CallbackFunc )`
  * to assign that function to all the states of the fsm:<br>
`SPAG_ASSIGN_MEMBER_CALLBACK_ALL( fsm, ClassName, CallbackFunc )`

- **Q**: *What version of Boost libraries does this require?*<br>
**A**: None, if you do not intend to use the provided Asio Wrapper class.
If you do, then this has been tested as successful against Boost 1.54 to 1.70.
Please post issue in case of trouble with a later Boost release, so it can be fixed.

- **Q**: *How does this library relate to the UML state machine definition
[WP](https://en.wikipedia.org/wiki/UML_state_machine) ?*<br>
**A**: This implements only a small subset of the UML standard.
For example, the concepts of "extended state" and "Guard conditions" are not available.
Nor are hierarchically states implemented (see below).
However, all the basics of FSM are available, most notably it respects the "run to completion" (RTC) execution model.
It also provides the concept of "[Internal events](spaghetti_manual.md#inner_events)".

- **Q**: *Can I use this for a hierarchical FSM?*<br>
**A**: Although it was considered at some point, I think this would completely change the architecture of the library, and change its nature.
Thus if hierarchical features are needed, I would recommend switching to
[boost::msm](https://www.boost.org/doc/libs/release/libs/msm/)
or
[boost::statechart](https://www.boost.org/doc/libs/release/libs/statechart/).

- **Q**: *Can I have two concurrent FSM working at the same time?*<br>
**A**: Yes! See sample program [`src/sample_2.cpp`](../../../tree/master/src/sample_2.cpp) that demonstrates this.
This requires defining the symbol `SPAG_EXTERNAL_EVENT_LOOP`, see [build options](spaghetti_options.md).

- **Q**: *Does this library provide serialization of the configuration of the FSM, so I can easily save it to a file?*<br>
**A**: No, because it holds objects of type `std::function` to store the callback functions.
And this object **can not** be serialized.<br>
All the rest of the configuration could be serialized, but I felt that saving configuration without the callbacks would be useless.
Post an issue if you feel that can be useful, it wouldn't be too hard to add that.

- **Q**: *Can I build a FSM with 0 events, only time outs?*<br>
**A**: Sure, just declare the events as `enum Events { NB_EVENTS };`.
This is demonstrated in `src/sample_3.cpp`.

- **Q**: *I need to track ignored events. How can I do that?*<br>
**A**: First, these are logged (if logging is enable, of course, see [`SPAG_ENABLE_LOGGING`](spaghetti_options.md) ),
and you can print them once your FSM is stopped with `getCounters().print()`.
Second, to handle them during runtime, you can assign a generic callback function that will be called every time an ignored event occurs,
with `assignIgnoredEventsCallback( cbfunc )`.<br>
This callback function needs to have this signature:<br>
`void cb_ign( States, Events )`, with `States` and `Events` the types using to define the FSM.<br>
This will bring information on when and why this happened, for instance you can do something like this:
```C++
void cb_ign( States st, Events ev )
{
   std::cout << "Detected ignored event " << (int)ev << " while on state " << (int)st << '\n';
}
```
This is demonstrated in [`src/traffic_lights_common.hpp`](../../../tree/master/src/traffic_lights_common.hpp):
switching to "warning" mode is only allowed while on regular modes, and if that event occurs while on any other state,
the callback function is triggered.

- **Q**: *What signal does the included event-loop class `AsioWrapper` use ? Can I change it ?*<br>
**A**: The default signal is SIGUSR1,
see [WP Posix signals page](https://en.wikipedia.org/wiki/Signal_(IPC)#POSIX_signals).
If you need another one, just define the symbol SPAG_SIGNAL with the one you need:
```C++
#define SPAG_SIGNAL SIG_XXXX
#include "spaghetti.hpp"
```

- **Q**: *Is it possible to instantiate both a FSm with the embedded asio-timer class and another FSM with another timer-class
(or no timer at all)*<br>
**A**: No, because the build "behavorial" symbols (see [build options](spaghetti_options.md)
will deeply change the nature of the FSM class. So, as these are common, you cannot have in the same program
(actually, in the same compilation unit) two FSM instances of two different types.

- **Q**: *Why that name? Where does that come from?*<br>
**A**: *Naming is hard*. But, lets see: Finite State Machine = FSM = Flying Spaghetti Monster
(see [WP](https://en.wikipedia.org/wiki/Flying_Spaghetti_Monster)).
So you got it.
(and certainly not related to [this](https://en.wikipedia.org/wiki/Spaghetti_code), hopefully!)



--- Copyright S. Kramm - 2018-2020 ---
