# Spaghetti
A C++ library useful for simple and easy Finite State Machine (FSM) building

## Key points
- Status: beta, all the provided sample programs run as expected.
- Licence: GPL v3
- Audience: C++ developper having to implement a FSM
- Webpage: https://github.com/skramm/spaghetti
- Manual : online here : https://github.com/skramm/spaghetti/blob/master/docs/spaghetti_manual.md
- Language: C++11
- OS: any one, as long as you have a C++11 compiler
- Author: Sebastien Kramm, LITIS, University of Rouen, France - 2018.

## General information

### What is this ?
This library provides a container and API functions to easily implement a FSM (see [WP link](https://en.wikipedia.org/wiki/Finite-state_machine)).
However, it does not provide the main event loop.
This is up to the user code.
The rationale behind this is that this loop may vary greatly depending on the requirements
(need for timeout or not, need to acquire hardware external events,...).

This library provides an easy way to specify states and events, and how and when it will switch from one state to another.
It also has some additional facilities, such as logging and easy timeout handling.

It is currently in beta state: everything seems to work as expected, please post issue on Github in case of any problems.

### Main features

- single file header-only library, just fetch the file ```spaghetti.hpp``` and store it on your machine somewhere where your compiler can find it
- C++11, no dependencies other than standard library
- ease of use and performance
- runtime logging capability
- provided with some sample programs, see folder src and src/html/index.html (once ref manual is build)
