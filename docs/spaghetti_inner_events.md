

## Inner events

In some situations, a FSM has to change its behavior depending on some of its internal characteristics.
For example
- "If state X has been activated 5 times, then switch to state Y instead of state Z"
- "if some class member variable has value 10, then, when on state X, we want to switch to state Y instead of having a timeout leading to state Z"

This is implemented in Spaghetti by using so-called "inner-events", as opposed to other events, that are called "hardware events".
These latter ones are triggered by the user code: when they occur, the user code must call the member function ```processEvent()``` and thats it.
The state switches to the next one and all the actions associated with that state are done:
callback is executed, timeout (if any) is launched, ...

But we cannot rely on this for "inner" event types.
Oh well, actually, we could, by checking when arriving on a state if some condition is met, and if so, change again state, call associated callback function, and so on.
As you get it, there is a high risk of getting into an infinite recursion loop, that will quickly lead to a stack overflow.

So those events are processed differently.

Each state holds a ```InnerTransition``` struct, holding information on what inner event must be handled, and what state it will lead to.
This structure gets assigned during configuration step by member function
```assignInnerTransition()```


At runtime, it is still the user-code responsability to call some member function, but this time it will/may be considered later.<br>
Recall, with the other triggering member function ```processEvent()```, the processing takes place immediately:
the FSM will check if that event is allowed on the current state, and will switch to the next state according to the transition matrix.<br>
Here, we just notify the FSM that some inner event happened, and that it "might" be useful later, when on some other state.
This is done by a call to ```activateInnerEvent()```.

The function will actually only set the flag associated to that inner event to "true", so that it will be indeed processed when we arrive on the state.

So how is this event processed, in a way that will not lead to a potential stack overflow?
The key is using signals.
When we arrive on a state, the function ```runAction()``` is always called.
This function will, depending on the situation, start the timer, and/or run the callback.
Now, it will also check if there is an inner event associated to that state, and if so, it will
**raise a signal**, that will be handled **after completion** of the function, by a dedicated handler function.

The signal handler will then itself call the ```processInnerEvent()``` member function,


--- Copyright S. Kramm - 2018 ---
