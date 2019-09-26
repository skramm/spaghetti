## Developper information

- [Homepage](https://github.com/skramm/spaghetti)
- [Manual](spaghetti_manual.md)

This page gives some general details for those who want to dive into the code
(more to come...)

### 0 - Continuous Integration

At present, a very light CI is running, using travis-ci.com, for details see file `.travis.yml`.
All the samples are build and the `make test` is run (see below: Automated testing)


### 1 - Possibly related software

 - Boost MSM: http://www.boost.org/doc/libs/release/libs/msm/
 - Boost Statechart: http://www.boost.org/doc/libs/release/libs/statechart/
 - tinyFSM: https://github.com/digint/tinyfsm


### 2 - Coding style

Most of it is pretty obvious by parsing the code, but here are some additional points:

* TABS for indentation, SPACE for spacing
* Identifiers
  * `camelCaseIsUsed` for functions, variables
  * class/struct member data is prepended with '_' ( `_thisIsADataMember` )
  * Types are `CamelCase` (UpperCase first letter). Example: `ThisIsAType`
* Macros: to avoid name collisions, all the symbols defined here start with `SPAG_`

### 3 - Automated testing

The makefile `test` target will build and launch the test programs, that are located in folder `tests` and have a name starting with `testA_`.

The ones starting with `testB` are just general testing stuff, not automated.

This is very preliminar.
At present, the testing consist in making sure a test program produces an output similar to a given reference
(in the form of a file `tests/XXXX.stdout`).


<a name="inner_events"></a>
### 4 - Inner events handling

In this section, more details about how [Inner events](spaghetti_manual.md#inner_events) are handled internally.

It is not possible to handle inner events in a similar way as regular (external) events.
If we did so (by checking when arriving on a state if some condition is met, and if so, change again state, call associated callback function),
we would quickly enter an infinite recursion loop, that will immediately lead to a stack overflow.

So those events are processed differently.

Each state holds a list of `InnerTransition` struct, that holds information on what inner event must be handled, and what state it will lead to.
This structure gets assigned during configuration step by member function `assignInnerTransition()`.

At runtime, it is still the user-code responsability to call some member function, but this time it will/may be considered later.<br>
Recall, with the other triggering member function `processEvent()`, the processing takes place immediately:
the FSM will check if that event is allowed on the current state, and will switch to the next state according to the transition matrix.<br>

With Inner Events, we just notify the FSM that some inner event happened.
This is done by a call to `activateInnerEvent( EV )`.
This function will only activate a flag associated to that inner event, so that it will be indeed processed when we arrive on the state where it is supposed to trigger something.

So how is this event processed, in a way that will not lead to a potential stack overflow?
The key is using signals.
When we arrive on a state, the function `runAction()` is always called.
This function will, depending on the situation, start the timer, and/or run the callback.
Now, it will also check if there is an inner event associated to that state, and if so, it will
**raise a signal**, that will be handled **after completion** of the function, by a dedicated handler function.

The signal handler will then itself call the `processInnerEvent()` member function,

As this feature require the use of
[signals](https://en.wikipedia.org/wiki/Signal_(IPC)),
thus it is available only if symbol `SPAG_USE_SIGNALS` is defined (see [build options](spaghetti_options.md) ).


