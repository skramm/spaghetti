# Spaghetti

[![Build Status](https://travis-ci.org/skramm/spaghetti.svg?branch=master)](https://travis-ci.org/skramm/spaghetti)

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

![FSM gif](https://github.com/skramm/spaghetti/blob/master/docs/out.gif)

## General information

### What is this ?
This library provides a container and API functions to easily implement an event-driven Finite State Machine
(see [WP link](https://en.wikipedia.org/wiki/Finite-state_machine)).
It can be used for FSM requiring the handling of hardware events, or time out situations.
It has been designed with good "Modern C++" practices, but provides an API as simple as possible to the developper.

This library provides an easy way to specify states and events, and how and when it will switch from one state to another.
It also has some additional features, such as logging the execution history.

The manual describes many uses cases, all of them are included in source tree and directly runnable for testing.

A lot of efforts has been put on safety: most of the potential errors are detected at build time, and everything is checked at runtime.

Feedback welcome, please post issue on Github in case of any problems.

### Main features

- single-file header-only library, just fetch the file ```spaghetti.hpp``` and store it on your machine somewhere where your compiler can find it
- C++11, no dependencies other than standard library
- ease of use and performance
- [full manual included](https://github.com/skramm/spaghetti/blob/master/docs/spaghetti_manual.md)
- runtime logging capability
- provided with some sample programs, see folder
[src](https://github.com/skramm/spaghetti/tree/master/src)
and src/html/index.html (once ref manual is build)

### What's in this repo ?

Besides the main file ```spaghetti.hpp```, the repo also holds documentation, FAQ, and several samples in ```src``` folder.
All of this comes with all that is needed to build these on a standard Linux machine (makefile, build folders, ...).
If you clone the repo, just run  ```make demo``` to build the programs (assuming you have Boost installed, as some samples rely on it).
You'll find the corresponding binaries in  the ```bin``` folder.
