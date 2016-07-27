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

These changes did introduce an issue to numbered lines mode.
This and other found bugs are fixed now.
A simple `./configure` had been added to set curses
as the terminal capabilities access library
on some systems.
## How to install
The embedded termcap library may cause problems on systems where
there is no `/etc/termcap` and `$TERMCAP` of the user that runs
`./configure` and other users (e.g. `root`) differs.
Therefore using termcap has lowest priority during auto-configure.
To prefere termcap put the corresponding line before the curses
entries in
[configure](https://github.com/n-t-roff/heirloom-ex-vi/blob/master/configure).
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
