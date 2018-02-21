# Spaghetti Manual: FAQ

 - Homepage: https://github.com/skramm/spaghetti
 - Manual: https://github.com/skramm/spaghetti/blob/master/docs/spaghetti_manual.md

- Q: *What is the timer unit?*<br/>
A: There are no timer units. Timing is stored as untyped integer value, it is up to the timer class you define to handle the considered unit.

- Q: *How does this library differ from the other ones?*<br/>
A: Most of the other libraries define the states as C++ classes.
While this can have some advantages, it requires you to create a class for each state.
With Spaghetti, you just add an enumerator value.

- Q: *How are runtime errors handled?*<br/>
A: Using exceptions. Configuration errors will throw a
[```std::logic_error```](http://en.cppreference.com/w/cpp/error/logic_error)
and runtime errors (in the sense: "FSM runtime") will throw a
[```std::runtime_error```](http://en.cppreference.com/w/cpp/error/runtime_error)

- Q: *Why are certain functions (for example: ```writeDotFile()```) not always enabled? Why do I have to pass a build option to "activate" them?*<br/>
A: The rationale is that this doesn't require editing your source code between "building up and testing" and "production" phases.
In the first phase, you may need to produce that information, and in the second phase, you could want reduced memory footprint.
So just disable the build option and the optional functions get reduced automatically to nothing.

- Q: *What if I have more that a single argument to pass to my callback function?*<br/>
A: You'll need to "pack it" in some class, or use a
[```std::pair```](http://en.cppreference.com/w/cpp/utility/pair),
or a [```std::tuple```](http://en.cppreference.com/w/cpp/utility/tuple).

- Q: *Can I use a callback function with a void parameter ( ```void my_callback()```)*<br/>
A: No, unfortunately. This is because void is not a type, you can't pass it as template argument. But you can always use anything, say an integer, and ignore its value.

- Q: *Can I use this for a hierarchical FSM?*<br/>
A: at present, no, but this is considered for future releases

- Q: *Can I have two concurrent FSM working at the same time?*<br/>
A: Yes! See sample program [src/sample_2.cpp](../../../tree/master/src/sample_2.cpp) that demonstrates this.
This need symbol ```SPAG_EXTERNAL_EVENT_LOOP```, see [build options](spaghetti_options.md).


- Q: *Why that name? Where does that come from?*<br/>
A: Naming is hard. But, lets see: Finite State Machine = FSM = Flying Spaghetti Monster
(see [WP](https://en.wikipedia.org/wiki/Flying_Spaghetti_Monster)).
So you got it.
(and certainly not related to [this](https://en.wikipedia.org/wiki/Spaghetti_code), hopefully!)

--- Copyright S. Kramm - 2018 ---
