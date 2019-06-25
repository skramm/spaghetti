## Time out related functions

- Homepage: https://github.com/skramm/spaghetti
- Manual: https://github.com/skramm/spaghetti/blob/master/docs/spaghetti_manual.md

These member functions will assign a timeout to all the states except one.
The types used here are:
- ST : the enumerator used for states
- Duration : unsigned integer value
- DurUnit : an enum holding three values:
```DurUnit::ms```, ```DurUnit::sec```, ```DurUnit::min```

### List of functions

```C++
void assignGlobalTimeOut( ST st_final )
```
Assigns a timeout event leading to state ```st_final```, on **all** states except ```st_final```,
using default timer unit and default timer duration value.

```C++
void assignGlobalTimeOut( ST st_final );
```
Assigns a timeout event leading to state ```st_final``` on **all** states except ```st_final```, using default timer unit and default timer duration value.

```C++
void assignGlobalTimeOut( Duration dur, ST st_final );
```
Assigns a timeout event on **all** states except ```st_final```, using duration ```dur``` and default timer unit.

```C++
void assignGlobalTimeOut( Duration dur, std::string durUnit, ST st_final );
```
Assigns a timeout event on **all** states except ```st_final```, using duration ```dur``` and unit ```durUnit``` (expressed as a string value).


```C++
void assignGlobalTimeOut( Duration dur, DurUnit durUnit, ST st_final );
```
Assigns a timeout event on **all** states except ```st_final```, using duration ```dur``` and unit ```durUnit```.
