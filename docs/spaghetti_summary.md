## Usage summary

- [Homepage](https://github.com/skramm/spaghetti)
- [Manual](spaghetti_manual.md)


This page gives a short summary of the library usage.
For more details, please check manual or off-line Doxygen-generated pages.

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

### 2 - Configuring the FSM

After you have created the FSM with
`fsm_t fsm;`
you need to configure it.

#### 2.1 - Configure allowed transitions

* `fsm.assignTransition( st1, ev1, st2 );`<br>
assigns event `ev1` as an allowed transition between states `st1` and `st2`



#### 2.2 - Assigning callback functions and callback values

* `fsm.assignCallback( st1, cb_func );`<br>
assigns callback function `cb_func` to the state `st1`


* `fsm.assignCallback( st1, cb_func, some_value );`<br>
assigns callback function `cb_func` to the state `st1`, will be passed value `some_value`


* `fsm.assignCallbackValue( st1, some_value );`<br>
assigns value `some_value` to the callback function assigned to state `st1`


#### 2.3 - Assigning time outs

(This is detailed on a [separate page](spaghetti_timeout.md)).

(TO BE CONTINUED)

### 3 - Starting/stopping the FSM

* `fsm.start();`<br>
Starts the FSM. This function can be blocking or not, depending on how the FSM type has been created (see above).

* `fsm.stop();`<br>
Stops the FSM.

###  Triggering events

* Handling hardware/external events:
`fsm.processEvent( eev );`

* Activating internal events:
`fsm.activateInnerEvent( iev );`



(TO BE CONTINUED)

--- Copyright S. Kramm - 2018-2019 ---
