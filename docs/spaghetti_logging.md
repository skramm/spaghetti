## Logging capabilities

- [Homepage](https://github.com/skramm/spaghetti)
- [Manual](spaghetti_manual.md)

This library can log different things at runtime, in two main ways:
- It can produce per state and per events counters.
- It can produce a csv-style of when a switch to a state occurred, and on what event.

While the first one can be (at present) accessed only once the FSM is stopped, the other one is a file that is continuously updated.
Both of these are enabled only if the build symbol `SPAG_ENABLE LOGGING` has been defined.

### 1 - Counters

If your FSM is able to stop (after a call to `stop()`), you can printout the runtime data with
```C++
fsm.getCounters().print( std::cout );
```
This will print with CSV style and in three different sections:
 - the state counters (how many times the states have been activated).
 - the event counters. This also include the number of timeouts, and the number of "Always Active" transitions that were encountered.
 - the ignored events counters: how many times each event occurred and was ignored (because current state did not take them into account).

 You can pass to this function an optional second parameter of type `Item`, to specify **what** data you want:
 - `ItemStates`        : print state counters
 - `ItemEvents`        : print event counters
 - `ItemIgnoredEvents` : print ignored events counters
 <br>
These flags can be "OR-ed" to have several ones active.
For example:

```C++
fsm.getCounters().print( std::cout, ItemStates | ItemEvents );
```
Please note that if the symbol `SPAG_ENUM_STRINGS` (see [Build options](spaghetti_options.md)) is defined, the strings will appear in this data.

Assuming you ave access to the FSM object (say, you have a separate thread to access it and it is global), you can reset the counter anytime with a call to member function
`fsm.clearCounters()`.
This does not reset the timestamp, only the counters.

You can also fetch at runtime the counter values with a call to `fsm.getCounters()`.
This will return an object of type `Counters`, which hold a copy of the internal values.

Once you have the `Counters` object, its values can be printed using the above `Item` enum.
For example, this:
```C++
std::cout << counters.getValue( ItemStates, 1 )
```
will return the number of times the states having index 1 has been activated.

**Warning** : as this is a independent datatype, no checking is done on index validity.

*Note*: to get the index of a state/event from their name (assuming you enabled the SPAG_ENUM_STRINGS option), you can get these with<br>
 - `getStateIndex( std::string )`
 - `getEventIndex( std::string )`


### 2 - History of events and state changes

At runtime, if `SPAG_ENABLE LOGGING` is defined, a file is automatically created and logs all events and states, along with a time stamp.
The default name is `spaghetti.csv`, but you can change it with `setLogFileName()`.
This is even mandatory in some situations, such as in `src/sample2.cpp`, where two FSM are running concurentely.

This will produce a file holding something like this (if you did not enable the states/events names):

```
# FSM runtime history
#index;time;event-Id;state-Id
000000;0.800368;0;1;
000001;1.30055;0;0;
000002;2.10067;0;1;
000003;2.60078;0;0;
000004;3.4009;0;1;
000005;3.90097;0;0;
```


--- Copyright S. Kramm - 2018-2020 ---
