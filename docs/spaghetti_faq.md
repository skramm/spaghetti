# Spaghetti Manual: FAQ

 - Homepage: https://github.com/skramm/spaghetti
 - Manual: https://github.com/skramm/spaghetti/blob/master/docs/spaghetti_manual.md

- Q: *What is the timing unit?*<br/>
A: The values are stored as integer values with an associated ```DurUnit``` enumeration value, so you can select between seconds, milliseconds, and minutes, and this for each timeout value.<br>
It is up to the Timer class to handle these (the provided optional timer class ```AsioWrapper``` does).
The default value is "seconds".<br>
When you add the following configuration line, it will be considered as 5 seconds.
```C++
	fsm.assignTimeOut( st_Red, 5, st_Green  );
```
Actually, it uses the current default timer unit.
This one can be changed any time, for example to milliseconds, with the following command (which will **not** change the settings of already assigned timeouts):
```C++
	fsm.setTimerDefaultUnit( spag::DurUnit::ms );
```
Other possible values are ```sec```,```min```.
Alternatively, you can also give the units when defining Timeouts. This will for example define a 5 minutes Timeout:
```C++
	fsm.assignTimeOut( st_Red, 5, spag::DurUnit::min, st_Green  );
```
Oh, and if for some reason (templated function in a header, as in ```src/traffic_lights_common.hpp```) you don't have access to the ```DurUnit``` type, you can also use string values:
```C++
	fsm.assignTimeOut( st_Red, 5, "min", st_Green  );
```
Internally, the timing is handled through the C++11 ```chrono``` library
[duration type](http://en.cppreference.com/w/cpp/chrono/duration).

- Q: *How does this library differ from the other ones?*<br/>
A: Most of the other libraries define the states as C++ classes.
While this can have some advantages, it requires you to create a class for each state.
With Spaghetti, you just add an enumerator value.

- Q: *How are runtime errors handled?*<br/>
A: A lot of stuff is checked at build time but some errors can only be detected at runtime.
These are handled using exceptions.
Configuration errors will throw a
[```std::logic_error```](http://en.cppreference.com/w/cpp/error/logic_error)
and runtime errors (in the sense: "FSM runtime") will throw a
[```std::runtime_error```](http://en.cppreference.com/w/cpp/error/runtime_error)

- Q: *Why are certain functions (for example: ```writeDotFile()```) not always enabled? Why do I have to pass a build option to "activate" them?*<br/>
A: The rationale is that this doesn't require editing your source code between "building up and testing" and "production" phases.
In the first phase, you may need to produce that information, and in the second phase, you could want reduced memory footprint.
So just disable the build option and the optional functions get reduced automatically to nothing.

- Q: *What if I have more that a single argument to pass to my callback function?*<br/>
A: You'll need to "pack it" in some class, or use a
[```std::pair```](http://en.cppreference.com/w/cpp/utility/pair),
or a [```std::tuple```](http://en.cppreference.com/w/cpp/utility/tuple).

- Q: *Can I use a callback function with a void parameter ( ```void my_callback()```)*<br/>
A: No, unfortunately. This is because void is not a type, you can't pass it as template argument. But you can always use anything, say an integer, and ignore its value.

- Q: *What version of Boost libraries does this require?*<br>
A: None, if you do not intend to use the provided Asio Wrapper class.
If you do, then this has been tested as successful against Boost 1.54.
But it *should* be okay with todays current release (1.66 at the time of writing), please post issue in case of trouble so it can be fixed.

- Q: *Can I use this for a hierarchical FSM?*<br/>
A: at present, no, but this is considered for future releases

- Q: *Can I have two concurrent FSM working at the same time?*<br/>
A: Yes! See sample program [src/sample_2.cpp](../../../tree/master/src/sample_2.cpp) that demonstrates this.
This requires defining the symbol ```SPAG_EXTERNAL_EVENT_LOOP```, see [build options](spaghetti_options.md).


- Q: *Why that name? Where does that come from?*<br/>
A: Naming is hard. But, lets see: Finite State Machine = FSM = Flying Spaghetti Monster
(see [WP](https://en.wikipedia.org/wiki/Flying_Spaghetti_Monster)).
So you got it.
(and certainly not related to [this](https://en.wikipedia.org/wiki/Spaghetti_code), hopefully!)

--- Copyright S. Kramm - 2018 ---
