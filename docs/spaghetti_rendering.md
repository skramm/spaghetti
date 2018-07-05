
### FSM graph rendering

- Homepage: https://github.com/skramm/spaghetti
- Manual: https://github.com/skramm/spaghetti/blob/master/docs/spaghetti_manual.md

This library does not produce graphical rendering of the state machine.
But it provides a function to generate a [standard dot file](https%3A%2F%2Fen.wikipedia.org%2Fwiki%2FDOT_%28graph_description_language%29), that can be used to generate the corresponding image, using [Graphviz](https://www.graphviz.org/).

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
```
If you want to change one of these, instanciate it, change one of the members value, and call the function by adding the options.
For example:

```C++
	spag::DotFileOptions dfo;
	dfo.showActiveState = true;
	fsm.writeDotFile( "myDotFile", dfo );
```


--- Copyright S. Kramm - 2018 ---
