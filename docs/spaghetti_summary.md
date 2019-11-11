## Usage summary

- [Homepage](https://github.com/skramm/spaghetti)
- [Manual](spaghetti_manual.md)


This page gives a short summary of the library usage.
For more details, please check manual or off-line Doxygen-generated pages.


Page content:
1. [Declaring the type of the FSM](fsm_type)
1. [Configuring the FSM](config)
1. [Running the FSM](running)

<a name="fsm_type"></a>
### 1 - Declaring the type of the FSM

These are some conveniency macros, are actually expanded into typedefs.
For each of these, you need to provide some types.
First argument (`fsm_t`) will be the name of the created type.
The arguments `st` and `ev` will be the enums describing the states and events, respectively.
The argument `cbarg` is the type of the argument of the callback functions.

* Creating FSM type with no timer/event loop involved:<br>
`SPAG_DECLARE_FSM_TYPE_NOTIMER( fsm_t, st, ev, cbarg );`

* Creating FSM type with some random timer/event loop class `tim`:<br>
`SPAG_DECLARE_FSM_TYPE( fsm_t, st, ev, tim, cbarg );`

* Creating FSM type with the provided timer/event loop class.
Requires [Boost::asio](https://www.boost.org/doc/libs/release/libs/asio/) and the use of either
`SPAG_USE_ASIO_WRAPPER` or `SPAG_EMBED_ASIO_WRAPPER` (see [build options](spaghetti_options.md)):<br>
`SPAG_DECLARE_FSM_TYPE_ASIO( fsm_t, st, ev, cbarg );`

<a name="config"></a>
### 2 - Configuring the FSM

After you have created the FSM with
`fsm_t fsm;`
you need to configure it.

#### 2.1 - Setting transitions from events

* `fsm.assignTransition( st1, ev1, st2 );`<br>
assigns event `ev1` as an allowed transition between states `st1` and `st2`

* `fsm.assignTransitionMat( mat );`<br>
assigns allowed event matrix to the FSM.
The allowed types for `mat` are:
  * `std::vector<std::vector<A>>`
  * `std::array<std::array<A,N1>,N2>`, with `N1` the number of states and `N2` the number of events<br>
Type `A` must be convertible to a `bool`


The functions below are only available when option/symbol `SPAG_USE_SIGNALS` is defined, see manual.

* `fsm.assignAAT( st1, st2 );`<br>
assigns an "Always Active Transition" to a "pass-state" (AAT):
once on state `st1`, the FSM will switch right away to `st2`.

* `fsm.assignInnerTransition( st1, iev, st2 );`<br>
Assigns a inner transition between `st1` and `st2`, triggered by internal event `iev`

* `fsm.assignInnerTransition( iev, st );`<br>
Whatever state we are on, when internal event `iev` occurs, we will switch to state `st` (except if we are already on that state)

* `fsm.clearInternalEvent( iev );`<br>
This will clear the (potentially activated) inner event `iev` (i.e. as if it didn't happen).

* `fsm.disableInnerTransition( ev, st_from );`<br>
Removes inner transition `ev` that is assigned on state `st_from`


#### 2.2 - Assigning callback functions and callback values

* `fsm.assignCallback( st1, cb_func );`<br>
assigns callback function `cb_func` to the state `st1`

* `fsm.assignCallback( st1, cb_func, some_value );`<br>
assigns callback function `cb_func` to the state `st1`, will be passed value `some_value`

* `fsm.assignCallbackValue( st1, some_value );`<br>
assigns value `some_value` to the callback function assigned to state `st1`

* `fsm.assignCallbackAutoval( cb_func );`<br>
assigns the function `cb_func` as callback to all the states, with argument value being the state value/index, converted to an `int`.
Requires that argument type is an "integer" type, see https://en.cppreference.com/w/cpp/types/numeric_limits/is_integer

* `fsm.assignIgnoredEventsCallback( func );`<br>
assigns the function `func` that will be called when an ignored event occurs.
The function MUST have the following signature:
`std::function<void(ST,EV)>`<br>
This will allow the function to determine wich state and event lead to its calling.

* `fsm.assignCBValuesStrings();`<br>
assigns to callback functions an argument value that is the state name (requires that callback argument is a string, [see below](#names). The callback argument type must be a string.

#### 2.3 - Assigning time outs

All these function are only available if an event-handler class has been assigned.

Some function use default values and units, that itself can be configured.
Timing values are integers.
Timing units must be either a member of enum `spag::DurUnit` (`DurUnit::ms`, `DurUnit::sec`, `DurUnit::min`),
or a string among these values: "ms" or "msec" for milliseconds, "s" or "sec" for seconds, or "mn" or "min" for minutes.

##### Assign a Timeout on a single state

* `fsm.assignTimeOut( st_curr, st_next );`<br>
Assigns a timeout event on state `st_curr`, will switch to event `st_next`.
Duration value will depend on the situation:
 - if a timeout has been previously assigned to `st_curr`, then its value will be retained.
 - if not, the default value and units will be used.

* `fsm.assignTimeOut( st_curr, dur, st_next );`<br>
Assigns a timeout event on state `st_curr`, will switch to event `st_next`.
Duration will be `dur`, with the current default unit.

* `fsm.assignTimeOut( st_curr, dur, unit, st_next );`<br>
Assigns a timeout event of duration `dur` with unit `unit` on state `st_curr`, will switch to event `st_next`.


##### Handling Timeouts on several states

* `fsm.clearTimeOuts();`<br>
Removes all the timeouts that may have been assigned.

The following member functions will assign a timeout to all the states except one:

* `fsm.assignGlobalTimeOut( st_final );`<br>
Assigns a timeout event leading to state `st_final` on all states except `st_final`, using default timer unit and default timer duration value.

* `fsm.assignGlobalTimeOut( dur, st_final );`<br>
Assigns a timeout event on all states except `st_final`, using duration `dur` and default timer unit.


* `fsm.assignGlobalTimeOut( dur, unit, st_final );`<br>
Assigns a timeout event on all states except `st_final`, using duration `dur` and unit `unit`.


##### Timer default values

* `fsm.setTimerDefaultValue( val );`<br>
Assigns the (integer) value `val` as default timer value, will be used for all further timer configuration not specifying a value.

* `fsm.setTimerDefaultUnit( unit )`<br>
Assign `unit` as default timer unit for all further timer configuration not specifying a unit.


<a name="names"></a>
#### 2.4 - Assigning names to events/states

This is possible only if you have activated build option `SPAG_ENUM_STRINGS`.
However, if not, then these functions will just do nothing (no build error).

* `fsm.assignString2State( st, some_name );`<br>
assigns string `some_name` to state `st`.

* `fsm.assignString2Event( ev, some_name );`<br>
assigns string `some_name` to event `ev`.

* `fsm.assignStrings2States( names );`<br>
assigns `names` to the FSM states.
The argument can be either a vector of pairs (states_enum,string):<br>
`std::vector<std::pair<ST,std::string>>` <br>
or a `std::map` with the states as key type.

* `fsm.assignStrings2Events( names );`<br>
assigns `names` to the FSM events.
The argument can be either a vector of pairs (events_enum,string):<br>
`std::vector<std::pair<EV,std::string>>` <br>
or a `std::map` with the events as key type.

Once configuration has been done, you can access the string values of any state/event with<br>
`fsm.getString( state );`
and
`fsm.getString( event );`


#### 2.5 - Miscellaneous

The configuration of the FSM can be copied to another variable, assuming they are of the same type:
```
fsm_t fsm1, fsm2;
// .. do some config on fsm1
fsm1. assignConfig( fsm2 );
```

<a name="running"></a>
### 3 - Running the FSM

#### 3.1 - Starting/stopping the FSM

* `fsm.start();`<br>
Starts the FSM. This function can be blocking or not, depending on how the FSM type has been created (see above).

* `fsm.stop();`<br>
Stops the FSM.

####  3.2 - Triggering events

* Handling hardware/external events:
`fsm.processEvent( eev );`

* Activating internal events:
`fsm.activateInnerEvent( iev );`



--- Copyright S. Kramm - 2018-2019 ---
