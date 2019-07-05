## Changelog

-v 0.6
- removed logging history, instead events gets logged to a logfile, default name is spaghetti.csv

2019-07-01: release 0.9.2
 - fixed bug in transmission of transition and authorization matrices
 - renamed `assignEventMatrix()` to `assignEventMat()`

2019-07-02:
  - fixed bug: when assigning a global timeout, states having already an AAT will not be assigned the timeout.
  - moved dev info into markdown page

2019-07-04:
	- clarified manual on logging capabilities
	- fixed build failure
	- API change: `printLoggedData()`: now use `getCounters()` to get the values and `getCounters().print()`
	- added `clearCounters()`
