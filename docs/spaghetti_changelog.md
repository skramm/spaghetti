## Changelog

2024/03: clarified/cleaned demo build process

2020-XXX:
 - addded macro to change the default signal: `SPAG_SIGNAL`
 - changed licence to Boost 1.0

2020-07-01: v0.9.5
 - fixed bug with Boost 1.70: executor_work_guard needs to be a class member if we want the event loop to stay alive
 - minor fixes (doc)


2019-07-07: v0.9.4
 - API change: renamed `PrintFlags` to `Item`, and cleanout counter access (`getValue()`)
 - API change: added `getStateIndex()` and `size_t getEventIndex()`

2019-07-04: v0.9.3
 - clarified manual on logging capabilities
 - fixed build failure
 - API change: `printLoggedData()`: now use `getCounters()` to get the values and `getCounters().print()`
 - added `clearCounters()`

2019-07-02:
 - fixed bug: when assigning a global timeout, states having already an AAT will not be assigned the timeout.
 - moved dev info into markdown page

2019-07-01: release 0.9.2
 - fixed bug in transmission of transition and authorization matrices
 - renamed `assignEventMatrix()` to `assignEventMat()`







