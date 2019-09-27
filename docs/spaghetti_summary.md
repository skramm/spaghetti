## Usage summary

- [Homepage](https://github.com/skramm/spaghetti)
- [Manual](spaghetti_manual.md)

### Declaring the type of the FSM

These are some conveniency macros, are actually expanded into typedefs.
For each of these, you need to provide some types.
First argument (`type`) will be the name of the created type.
The arguments `st` and `ev` will be the enums describing the states and events, respectively.
The argument `cbarg` is the type of the argument of the callback functions.

* Creating FSM type with no timer/event loop involved:<br>
`SPAG_DECLARE_FSM_TYPE_NOTIMER( type, st, ev, cbarg );`

* Creating FSM type with a some timer/event class:<br>
`SPAG_DECLARE_FSM_TYPE( type, st, ev, timer, cbarg );`

* Creating FSM type with the provided timer/event class.
Requires (Boost::asio)[https://www.boost.org/doc/libs/release/libs/asio/]) and the use of either
`SPAG_USE_ASIO_WRAPPER` or `SPAG_EMBED_ASIO_WRAPPER`, see [build options](spaghetti_options.md).<br>
`SPAG_DECLARE_FSM_TYPE_ASIO( type, st, ev, cbarg );`

### Configuring the FSM

After you have created the FSM with
`fsm_t fsm;`
you need to configure it.

(TO BE CONTINUED)


###  Triggering events

* Handling hardware/external events:
`fsm.processEvent( eev );`

* Activating internal events:
`fsm.activateInnerEvent( iev );`



(TO BE CONTINUED)

--- Copyright S. Kramm - 2018-2019 ---
