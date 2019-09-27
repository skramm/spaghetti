## Usage summary

- [Homepage](https://github.com/skramm/spaghetti)
- [Manual](spaghetti_manual.md)


This page gives a short summary of the library usage.
For more details, please check manual or off-line Doxygen-generated pages.

### Declaring the type of the FSM

These are some conveniency macros, are actually expanded into typedefs.
For each of these, you need to provide some types.
First argument (`type`) will be the name of the created type.
The arguments `st` and `ev` will be the enums describing the states and events, respectively.
The argument `cbarg` is the type of the argument of the callback functions.

* Creating FSM type with no timer/event loop involved:<br>
`SPAG_DECLARE_FSM_TYPE_NOTIMER( type, st, ev, cbarg );`

* Creating FSM type with some random timer/event loop class `tim`:<br>
`SPAG_DECLARE_FSM_TYPE( type, st, ev, tim, cbarg );`

* Creating FSM type with the provided timer/event loop class.
Requires [Boost::asio](https://www.boost.org/doc/libs/release/libs/asio/) and the use of either
`SPAG_USE_ASIO_WRAPPER` or `SPAG_EMBED_ASIO_WRAPPER` (see [build options](spaghetti_options.md)):<br>
`SPAG_DECLARE_FSM_TYPE_ASIO( type, st, ev, cbarg );`

### Configuring the FSM

After you have created the FSM with
```
fsm_t fsm;
```
you need to configure it.

(TO BE CONTINUED)

### Starting FSM

This is done with:
```
fsm.start();
```

This function can be blocking or not, depending on how the FSM type has been created (see above).
###  Triggering events

* Handling hardware/external events:
`fsm.processEvent( eev );`

* Activating internal events:
`fsm.activateInnerEvent( iev );`



(TO BE CONTINUED)

--- Copyright S. Kramm - 2018-2019 ---
