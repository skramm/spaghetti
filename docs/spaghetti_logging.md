## Logging capabilities

- [Homepage](https://github.com/skramm/spaghetti)
- [Manual](spaghetti_manual.md)

This library can log different things at runtime, in two main ways:
- It can produce per state and per events counters.
- It can produce a csv-style of when a switch to a state occurred, and on what event.

While the first one can be (at present) accessed only once the FSM is stopped, the other one is a file that is continuously updated.
Both of these are enabled only if the build symbol `SPAG_ENABLE LOGGING` has been defined.

### Counters

If your FSM is able to stop (after a call to `stop()`), you can printout the runtime data with
```C++
fsm.printLoggedData( std::cout );
```
This will print out, in a CSV style and in three different sections:
 - the state counters (how many times the states have been activated).
 - the event counters. This also include the number of timeouts, and the number of "Always Active" transitions that were encountered.
 - the ignored events counters: how many times each event occurred and was ignored (because current state did not take them into account).

 You can pass to this function an optional second parameter, to specify **what** data you want:
 - `PrintFlags::stateCount` : print state counters
 - `PrintFlags::eventCount` : print event counters
 - `PrintFlags::ignoredEvents` : print ignored counters
 - `PrintFlags::all`: all of the above (default value)
<br>
These flags can be "OR-ed" to have several ones active.
For example:

```C++
fsm.printLoggedData( std::cout, PrintFlags::stateCount | PrintFlags::eventCount );
```
Please note that if the symbol `SPAG_ENUM_STRINGS` (see [Build options](spaghetti_options.md)) is defined, the strings will appear in this data.
Also see how these functions are used in the provided sample programs.


### History of events and state changes

At runtime, if `SPAG_ENABLE LOGGING` is defined, a file is automatically created and logs all events and states, along with a time stamp.
The default name is `spaghetti.csv`.

(TO BE CONTINUED)



