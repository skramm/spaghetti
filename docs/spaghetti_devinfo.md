## Developper information

- [Homepage](https://github.com/skramm/spaghetti)
- [Manual](spaghetti_manual.md)

This page gives some general details for those who want to dive into the code
(more to come...)

### 1 - Possibly related software

 - Boost MSM: http://www.boost.org/doc/libs/release/libs/msm/
 - Boost Statechart: http://www.boost.org/doc/libs/release/libs/statechart/
 - tinyFSM: https://github.com/digint/tinyfsm


### 2 - Coding style

Most of it is pretty obvious by parsing the code, but here are some additional points:

- TABS for indentation, SPACE for spacing
- Identifiers
 - `camelCaseIsUsed` for functions, variables
 - class/struct member data is prepended with '_' ( `_thisIsADataMember` )
 - Types are `CamelCase` (UpperCase first letter). Example: `ThisIsAType`
 - To avoid name collisions, all the symbols defined here start with `SPAG_`

### 3 - Automated testing

The makefile `test` target will build and launch the test programs, that are located in folder `tests` and have a name starting with `testA_`.

The ones starting with `testB` are just general testing stuff, not automated.

This is very preliminar.
At present, the testing consist in making sure a test program produces an output similar to a given reference
(in the form of a file `tests/XXXX.stdout`).

