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

```C++
void assignTimeOut( ST st_curr, ST st_next );
```
Assigns a timeout event on state `st_curr`, will switch to event `st_next`.
Duration value will depend on the situation:
 - if a timeout has been previously assigned to `st_curr`, then its value will be retained.
 - if not, the default value and units will be used.

See `setTimerDefaultValue()` and `setTimerDefaultUnit()`

```C++
void assignTimeOut( ST st_curr, Duration dur, ST st_next );
```
Assigns a timeout event on state `st_curr`, will switch to event `st_next`.
Duration will be `dur`, with the current default unit.

```C++
void assignTimeOut( ST st_curr, Duration dur, DurUnit unit, ST st_next );
```
Assigns a timeout event of duration `dur` with unit `unit` on state `st_curr`, will switch to event `st_next`.

```C++
void assignTimeOut( ST st_curr, Duration dur, std::string unit, ST st_next );
```
Assigns a timeout event of duration `dur` with unit `unit` expressed as a string on state `st_curr`, will switch to event `st_next`.


### 2 - Global operations

```C++
void clearTimeOuts();
```
Removes all the timeouts

The following member functions will assign a timeout to all the states except one.

```C++
void assignGlobalTimeOut( ST st_final );
```
Assigns a timeout event leading to state `st_final` on all states except `st_final`, using default timer unit and default timer duration value.

```C++
void assignGlobalTimeOut( Duration dur, ST st_final );
```
Assigns a timeout event on all states except `st_final`, using duration `dur` and default timer unit.

```C++
void assignGlobalTimeOut( Duration dur, std::string durUnit, ST st_final );
```
Assigns a timeout event on all states except `st_final`, using duration `dur` and unit `durUnit` (expressed as a string value).


```C++
void assignGlobalTimeOut( Duration dur, DurUnit durUnit, ST st_final );
```
Assigns a timeout event on all states except `st_final`, using duration `dur` and unit `durUnit`.

--- Copyright S. Kramm - 2018-2019 ---
