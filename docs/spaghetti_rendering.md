
### FSM graph rendering

- Homepage: https://github.com/skramm/spaghetti
- Manual: https://github.com/skramm/spaghetti/blob/master/docs/spaghetti_manual.md

This library does not produce graphical rendering of the state machine.
But it provides a function to generate a [standard dot file](https://en.wikipedia.org/wiki/DOT_%28graph_description_language%29), that can be used to generate the corresponding image, using [Graphviz](https://www.graphviz.org/).

To use it with default options:
```C++
	fsm_t fsm;
	...
	fsm.writeDotFile( "myDotFile" );
```
This will produce in the current folder the file ```myDotFile.dot```.
You can render into an svg image with:
```
$ dot -Tsvg myDotFile.dot > myGraph.svg
```
or
```
$ dot -Tpng myDotFile.dot > myGraph.png
```
for a png image.

Several options can be used, they are embedded in the ```spag::DotFileOptions``` data type.

The available options and default values are:

```
showActiveState = false
showTimeOuts    = true
showInnerEvents = true
showAAT         = true
showStateIndex  = true
showStateString = true
showEventIndex  = true
showEventString = true
showUnreachableStates = true
fixedNodeWidth = false
useColorsEventType = true
```
If you want to change one of these, instanciate this object, change one of the members value, and call the function by adding the options.
For example:

```C++
	spag::DotFileOptions dfo;
	dfo.showActiveState = true;
	fsm.writeDotFile( "myDotFile", dfo );
```

The included test file ```tests/testA_2.cpp``` does a demo of these options.
It has unreachable states to show how these can be removed from the rendering.
This can be useful when handling a complex situation where you would want to printout a rendering while FSM is partially configured.
The table below shows for the same FSM different renderings, when one option changes from the default value.

| Option |  Rendering |
|-----------------------|----------------------------|
| Default               | ![test2_00](test_2_00.svg) |
| showActiveState=true  | ![test2_01](test_2_01.svg) |
| showInnerEvents=false | ![test2_02](test_2_02.svg) |
| showAAT=false         | ![test2_03](test_2_03.svg) |
| showTimeOuts=false    | ![test2_04](test_2_04.svg) |
| showEventIndex=false  | ![test2_05](test_2_05.svg) |
| showEventString=false | ![test2_06](test_2_06.svg) |
| showStateIndex=false  | ![test2_07](test_2_07.svg) |
| showStateString=false | ![test2_08](test_2_08.svg) |
| showUnreachableStates=false | ![test2_09](test_2_09.svg) |
| fixedNodeWidth=true         | ![test2_10](test_2_10.svg) |
| useColorsEventType=false    | ![test2_11](test_2_11.svg) |

When rendering with Graphviz, the size of the nodes are set automatically.
If you prefer a fixed size, then you can set `fixedNodeWidth` to true, and assign the requested size (in inches) with `nodeWidth`.
The default value is "1.5".
For example:

```C++
	spag::DotFileOptions dfo;
	dfo.fixedNodeWidth = true;
	dfo.nodeWidth = "1.2";
	fsm.writeDotFile( "myDotFile", dfo );
```

The edges are colored depending on their type
- external event: black
- time out event: blue
- Internal event: red
- Always Active Transition (AAT): green

This can be disabled by setting `useColorsEventType` to `false`.

--- Copyright S. Kramm - 2018 ---
