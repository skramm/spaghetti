# Demo files

| File | Comment |
|-----|-----|
| [`turnstyle_1.cpp`](turnstyle_1.cpp) | Turnstyle example: FSM without timer and with a custom callback function type. |
| [`turnstyle_2.cpp`](turnstyle_2.cpp) | Turnstyle example: similar to `turnstyle_1.cpp` but with an added timeout. |
| [`sample_0.cpp`](sample_0.cpp)| Demo of a running 2-states FSM with 1 hardware (keyboard) event to switch from initial state to other state (handled from a thread), and 1 timeout event to switch from second state to initial state |
| [`sample_1b.cpp`](sample_1b.cpp) | Similar to sample_1.cpp but with a timer (and subsequently, the UI in a thread). simple FSM with 2 events (key press), 5 states, 1 timeout and 1 pass state.<br> Also demonstrates how string states values can be used as callback arguments |
| [`sample_1.cpp`](sample_1.cpp) | Demo program of a simple FSM with 3 events (key press) and 5 states. No Timer  |
| [`sample_2.cpp`](sample_2.cpp) | Demo program of concurrent FSM, each with its own timeouts, using boost::asio. Uses the same enum for events and states. Needs symbol `SPAG_EXTERNAL_EVENT_LOOP` |
| [`sample_3.cpp`](sample_3.cpp) | Demo program of a simple FSM: 5 states with 1, 2, 3... ms between each, and with FSM as a global variable. Also demonstrates how user code can stop the running FSM (without using signals). |
| [`sample_3b.cpp`](sample_3b.cpp) | Demo program of a simple FSM: 5 states with 1, 2, 3... ms between each, and with FSM as a global variable. Also demonstrates how user code can stop the running FSM, using signals. |
| [`sample_3c.cpp`](sample_3c.cpp) | Similar to `sample_3b.cpp`, but with external event handler |
| [`traffic_lights_1.cpp`](.cpp) | A simple traffic light example, build using the embedded boost::asio timer |
| [`traffic_lights_1b.cpp`](.cpp) | Same as `traffic_lights_1.cpp` (traffic light), but with the boost.Asio Wrapper **not** embedded inside the FSM, provided separately |
| [`traffic_lights_1c.cpp`](.cpp) | same as `traffic_lights_1.cpp` but with the fsm and the callback function inside a class.<br>Also demonstrates: - how a inner event can be activated from a callback function: here, once the  |
| [`traffic_lights_2.cpp`](.cpp) | A simple traffic light example, build using boost::asio. Similar to version 1, with an added keyboard user interface as a separate thread<br>See companion file: [`traffic_lights_common.hpp`](traffic_lights_common.hpp) |
| [`traffic_lights_3.cpp`](.cpp) | A simple traffic light example. Similar to version 2, with an added UDP server part. Besides having a separate thread handling keyboard input, it can receive data from `traffic_lights_client.cpp` |
| [`traffic_lights_client.cpp`](.cpp) | Client-side for `traffic_lights_3.cpp`.<br>Sends UDP frames to the server, using port 12345 |

