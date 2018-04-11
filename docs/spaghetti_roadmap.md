

## Spaghetti: Roadmap

- Homepage: https://github.com/skramm/spaghetti
- Manual: https://github.com/skramm/spaghetti/blob/master/docs/spaghetti_manual.md


### Releases:
 - v0.63, released in april 2018 is the latest stable release, but is no more active.
 A design flaw with pass-states has been encountered, but is still usable. Development is now focused on v0.7.

 (The callback of states following pass states are called before the event processing function of the pass state is completed).

 - v0.7x: current dev branch (named "add_next_transition"), still in dev. stage, expected release in a few weeks.
  - Internal changes: will use OS signalling to trigger internal events.
  - External changes on API: not much, but will be enhanced with new features (run-time events).
