## Time out related functions

- [Homepage](https://github.com/skramm/spaghetti)
- [Manual](spaghetti_manual.md)

This page describes the member functions that can be used to configure timeouts.
This implies that an event-handler class is available, see [manual](spaghetti_manual.md) and [build options](spaghetti_options.md).

The types used here are:
- ST : the enumerator used for states
- Duration : unsigned integer value
- DurUnit : an enum holding three values:
`DurUnit::ms`, `DurUnit::sec`, `DurUnit::min`

The duration unit can also be expressed as a string, the allowed values are "ms", "sec" and "min".

Please read [this for more info on how to use timeouts](spaghetti_manual.md#showcase2).

### List of functions

### 1 - Single Timeout on a state

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


### 2 - Global operations

* `fsm.clearTimeOuts();`<br>
Removes all the timeouts that may have been assigned.

The following member functions will assign a timeout to all the states except one.

* `fsm.assignGlobalTimeOut( st_final );`<br>
Assigns a timeout event leading to state `st_final` on all states except `st_final`, using default timer unit and default timer duration value.

* `fsm.assignGlobalTimeOut( dur, st_final );`<br>
Assigns a timeout event on all states except `st_final`, using duration `dur` and default timer unit.


* `fsm.assignGlobalTimeOut( dur, unit, st_final );`<br>
Assigns a timeout event on all states except `st_final`, using duration `dur` and unit `unit`.


## 3 - Timer default values

* `fsm.setTimerDefaultValue( val );`<br>
Assigns the (integer) value `val` as default timer value, will be used for all further timer configuration not specifying a value.

* `fsm.setTimerDefaultUnit( unit )`<br>
Assign `unit` as default timer unit for all further timer configuration not specifying a unit.
Value `unit` must be either
a member of enum spag::DurUnit (see top of page),
or a string among these values: "ms" or "msec" for milliseconds, "s" or "sec" for seconds, or "mn" or "min" for minutes.


--- Copyright S. Kramm - 2018-2019 ---
