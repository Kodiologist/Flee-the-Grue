Flee the Grue is a small console game that I wrote in the summer of 2008 to brush up on my C. I've belatedly published it less as an example of good code and more as an amusement.

The only dependency is `GLib <https://developer.gnome.org/glib/>`_. To compile the program with GCC, use the command ``gcc -x c -std=c99 -pedantic-errors -Wall flee-the-grue.c `pkg-config --cflags --libs glib-2.0` -o flee-the-grue``. A VT100-compatible terminal emulator with Unicode support is assumed.

The object of the game is to `kill the wumpus <https://en.wikipedia.org/wiki/Hunt_the_Wumpus>`_ with your bow and arrows and then reach the exit. You lose if you run out of arrows before killing the wumpus, or if you get killed by the wumpus or the grue. The wumpus rarely moves, whereas the grue attempts to hunt you. The grue is merely stunned when hit by an arrow. The game map is a hexagonal grid where each vertex is a room. The map doesn't wrap around.

Arrows fork: when you shoot an arrow, if it doesn't hit anything in the first room it reaches, it splits into two and goes into the two adjacent rooms that you aren't in (if they exist).

For keyboard commands, see ``definitions.h``.

License
============================================================

This program is copyright 2008, 2019 Kodi B. Arfer.

This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the `GNU General Public License`_ for more details.

.. _`GNU General Public License`: http://www.gnu.org/licenses/

