# The traditional Vi
This implementation had been derived by Gunnar Ritter
from ex/vi 3.7 of 6/7/85 and the BSD
termcap library, originally from the 2.11BSD distribution.
He had added some useful enhancements,
most notably UTF-8 support.

The last release of *heirloom-ex-vi* had been
version 050325 (4.0).
Until 2007 he added new features, e.g.
dynamically allocated screen buffers for resizing the terminal
and support for files with arbitrary line length.

These changes did introduce an issue to numbered lines mode
which is fixed now.
A simple `./configure` had been added to set curses
as the terminal capabilities access library
on some systems.
## How to install
```
$ ./configure
$ make
$ su
# make install
# exit
$ make mrproper
```
## Usage information
The traditional `vi` reads the file `~/.exrc` at start-up.
For full screen scrolling `vi` uses the keys `^F` and `^B`.
To make &lt;PAGE-DOWN&gt; and &lt;PAGE-UP&gt; work add the following
lines to `.exrc`:
```
map ^[[5~ ^B
map ^[[6~ ^F
```
