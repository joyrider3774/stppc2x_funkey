                                 MAZE 3D GAME

               By Edward Macnaghten (eddy AT edsystems DOT com)
                                12 January 2006

INSTALLATION

This game is written using Simon Tatham's Portable Puzzle environmen.  The web
site for that (at time of writing) is
http://www.chiark.greenend.org.uk/~sgtatham/puzzles/ and describe the
pre-requisits needed.

Also, at time of writing, the web-site for Maze3d is
http://edlsystems.com/~eddy/maze3d and how to download the Maze3d source and
some binaries is explained there, but as you are reading this you probably have
got the appropriate files already.

If you will need to (re)compile, then the Portable Puzzle's source files are
required, then in the directory that is copied to you need to:

1   Copy the Maze3d source files (maze3dc.c, maze3dc.h and maze3d.c) to the same
    directory.

2   Replace the existing "Recipe" file with "Recipe.maze3d" (after backing up the
    original of course).

3   Run the "mkfiles.pl" script.

4   Enter "make maze3d" - and the binary should be generated.


RUNNING

To run the thing simply run the "maze3d" executeable, (or "amze3d.exe" in
MS-WINDOWS).  As this is written in the "Portable Puzzle's" environment it will
have the same generic issues as the others there do.  Please see the
documentation there for details of those.

It can optionally take one parameter describing the type of maze to be
generated.  This parameter can be quite simple or complex, and I will go
through the combination.

TYPE         EXAMPLE        DESCRIPTION

Number       4              This would generate a cubic 3 dimensional maze of the
                            size specified.  Size, here as in the rest of the
                            documentation, is measured in "cells", or "rooms"
                            which you travel through in the maze.  The example
                            eould generate a maze 4 rooms long, 4 wide and 4
                            high.  The range for this is from 3 to 20 itnclusive.
                            The default being 4.

No x No      5x10           This generates a 2 dimensional maze of he specified
                            dimensions.  That is it is only 1 unit "high".  The
                            example ill generate a 5 by 10 maze.  Ranges can go
                            from a 3 x 2 up to 20 x 20.

No x No x No 3x4x5          A specified 3 dimensional maze.  The example would
                            generate one 3 rooms long, 4 wide and 5 high.  Valid
                            ranges are from 3 x 2 x 1 to 20 x 20 x 20.  (Worth
                            noting that if the maze is 1 high it is a 2
                            dimensional one and could have been defined in the
                            format above).

...Target    3x4x5c         After the dimensions a "Target" letter can be
                            specified.  This is either "C", "E", "F" or "A" (case
                            insensitive), This specifies where a target can go.
                            "C" specifies it will be placed in the corner of the
                            maze, and "E", "F" and "A" mean on the edge, on a
                            face and anywhere respectively.  This defaults to
                            "C".

...Doubles   3x4x5cs        Also a letter specifying if any "double routes" can
                            be generated.  "S" means there are none (Single
                            routes only), "D" means there are.  A double route is
                            when there may be more than one way to reach the
                            target from certain points of the maze.  This usually
                            means it is harder to solve.  Mazes that have less
                            than a total of 30 rooms never produce double routes
                            and this patameter is ignored for those.  The
                            defaultis not to have double routes.

...Display   3x4x5csw       There is a choice of hints what to display on the
                            Status Panel.  These consist of "N" for no hints at
                            all, "L" for the current location where you are and
                            "W" for the aprpoximate direction and distance where
                            the target is, which is the default.

...Seed      3x4x5cs1234    At the end of the parameters you can place (up to) a
                            nine digit seed.  The same seed will generate the
                            same maze (as long as all the other parameters are
                            also the same).  So if you can have a competition ith
                            friends to see who can find the target the quickest!

                            It maybe worth mentioning thaat the Portable Puzzle
                            Environment also has a similar type of seed for all
                            games.  That can be used in Maze3d too, but if the
                            seed mentioned here is specified then that takes
                            precedence.

It may be worth noting that a beginner should stay with the small mazes.  A 3 x
3 x 3 maze generates 27 cells and will probably be challenging enough,
alternatively you can start with the two dimensional ones (those having a
height of 1).

The smallest maze you can create is one of 3 x 2 x 1 size.  This has a
staggering 6 rooms in it, and, quite frankly, not challenging (sometimes there
are no deviations at all between the start and the target).  It is useful
creating one this size if you want to know what the target looks like :-).  If
you try and create one smaller then it will alter your parameters to be 3 x 2 x
1.


PLAYING

The game presents itelf as though through a camera from the inside of the maze.
You control the movements of this camera and the object is to find the target.
You can face and travel in any direction, including up and down.  Gravity also
does not exist in the maze.

When you start the game you are presented with a diagram of a "view" of the
maze from the inside, that is the view from the starting point.  You can move
forward by clicking with the left mouse button the middle of the diagram - near
the center.  You cannot move forward if there is a wall in front of you though.
You can rotate yourself by clicking the edge of the maze.  Clicking the top and
bottom will pitch you by 90 degrees in the appropriate direction, and clicking
the left and right will yaw you.  You can also roll, that is stay facing the
same direction spin yourself to the left or right by 90 degrees.  This is
useful in re-orienting yourself.  You roll clockwise by clicking the top-right
or bottom-left corners, or anti clockwise by clicking the top-left or
bottom-right.

It is also possible to use the cursor keys.  The left, rightm up and down
arrows will rotate (pitch and yaw) you in the appropriate direction, and the
space bar will move you forward.

As well as that you can use the numeric keypad.  The nine digits from 1 to 9
represent the appropriate area of the screen.  So 5 would make you go forward.
8 and 2 will make you pitch upwards and downwards, 4 and 6 will make you yaw
left and right, and 9 ad 1 will roll you clockwise whereas 7 and 3 will roll
you anti-clockwise.

There are two other movements that can be performed.  A "B", or the "-" key
will move you backwards (so long as there is not a wall there), and a "T" or
the "*" key will turn you around in one movement (same as yawing left or right
twice).

At any time you can place a mark on the room you are in.  This is achieved by
right-clicking, or by pressing the "0" (zero), or "M" keys.  It an be removed
by performing the same operation.  Marks are provided to assist you around the
maze.

To sumarize:

Move forward        Click on center of diagram
                    Press SPACE bar Press 5 or "+" on numeric keypad

Rotate Left (Yaw)   Click on left of diagram
                    Press LEFT cursor key
                    Press 4 on numeric keypad

Rotate right (Yaw)  Click on right of diagram
                    Press RIGHT cursor key
                    Press 6 on numeric keypad

Rotate up (Pitch)   Click on top of diagram
                    Press UP cursor key
                    Press 8 on numeric keypad

Rotate down (Pitch) Click on bottom of diagram
                    Press DOWN cursor key
                    Press 2 on numeric keypad

Spin anti-clockwise Click on top-left or bottom-right corner of diagram
(Roll)              Press "A" key Press 7 or 3 on numeric keypad

Spin clockwise      Click on top-right or bottom-left corner of diagram
(Roll)              Press "C" key Press 9 or 1 on numeric keypad

Move backwards      Press "B" key
                    Press "-" on the numeric keypad

Turn around         Press "T" key
                    Press "*" on the numeric keypad

(Un)place marks     Right click anywhere on diagram
                    Press "M" key
                    Press "0" on the numeric keypad


STATUS PANEL

To the right there is a status panel giving you various pieces of information.
After some (extraordinary short) instructions on how to navigate using the
mouse (described above in more detail) it gives the following information.

Moves       The number of movements you have performed to date.

Turns       The number of rotations you have performed to date.  These just
            include pitching and yawing, not rolling.

Size        The size of the maze.  You probably know that already as you
            specified the parameters when starting the game, but I find having
            the size there useful so I guess maybe others will.

Marks       This is the number of marks placed.  When you place a mark this
            increments, and it detriments when you take the mark away.  There is
            not limit to the amount of marks you can place (at time of writing)
            but it may be of interest to some players in unusual circumstances.

Distance    This only appears if the "Direction/Distance" parameter was set at
            the start.  it is is the distance to the target.  More specifically
            it is the distance the crow flies, or more accurately a super powered
            lazer charge that could smash through all the walls.  It is not the
            distance that you have to go to get there, that is usually much
            higher (it is never lower).

Direction   This only appears if the "Direction/Distance" parameter was set at
            the start.  This is the compass direction of the target, more or less
            (ish).  It is assumed that the maze itself is in a particular place,
            or virtual building, and one side faces north, and the others east,
            south and west.  Being three dimensional there is also an up and
            down.  The compass directions are signified by the first letter (N,
            S, W, E, U, D for Morth, South, West, East, Up, Down respectively).
            These letters are used to signify the direction the target is from
            where you are at.

            A compass point does not appear unless you are within 60 degrees of
            it, so it is more of a rough guideline of the direction of the target
            rather than a precise measurement.  The same also applies here as it
            does above, it is the direction directly (ignoring walls) to it.  Not
            the direction you neccessarily have to go at that point to get there.

Location    This only appears if the "Location" parameter was chozen at the
            start.  It is the co-ordinates of your current location being the
            number of rooms north - the the number west and finally down from the
            south-east-up most room of the maze.

Compass     The weird cross thing at the bottom is a compass that tells you which
            direction you are facing, and which way up you are.  Ignoring the
            diagonal line there for now, the field on the left of the "cross"
            displays the compass point letter that represents the direction to
            your left, the same goes for the ones on the right, top and bottom.
            As far as the diagonal one is concerned, the top-left informs you the
            direction you are facing and the bottom right is the direction behind
            you.


PART OF THE FAMILY

As mentioned, this manifestation of this game is designed to be a part of the
Portable Puzzle family (if Simon Tatham is happy about it).  As far as that is
concerned the generic functions that come with that family apply here.  This
includes the undo (Key "U") and redo (Key "R") functions, and also to start a
new key by pressing "N" and to quit by pressing "Q", both those have no
confirmation so beware! These functions and others others are available from
the "Game" menu.  The other functions there include the ability to enter a
specific runtime parameter (saving you from having to enter it in on the
command line, which can be awkward for GUI systems) and the ability to find out
the complete parameter you are playing including the "seed" mentioned above.

Maze3d supports the Portable Puzzle's operations for of saveing and restoring
games to a file.

There is also the ability to have the system "Solve" the maze for you, which I
go into in a following section below.

The "Type" menu lists a number of maze dimensions and parameters that I thought
are suitable to include there, and a "Custom" choice for you to enter your own.


STRATERGY

Keep the mazes small.  I know I have mentioned this above, but for a beginner
even a 3 by 3 by 3 maze will be a bit of a challenge.  That has 27 rooms, a 4 x
4 x 4 has 64, more than twice the amount of a 3 x 3 x 3 one.  Going up, a 5 x 5
x 5 has 125 - nearly twice the amount as a cube of 4.

The maximum size, 20 x 20 x 20 has 8,000 rooms.  This takes a lllloooonnnngggg
ttttiiimmmmeeee to solve, believe me.  Good family fun can be had by a 3 x 3 x
3 one, though my preference is for a 4 x 4 x 4 maze as it required a little
skill to solve.  As that is my personal preference then that is the default.

The maze will seem far less complicated if you remain the right way up, however
what with pitching and yawing, with no sense of gravity, you soon find yourself
on your side or upside down, even when travelling horizontally.  In this case
it is always best to re-orient yourself the right way up using the "spin" or
"roll" commands (clicking on the corner of the diagram).

Another pointer to solving these mazes is to mark a room just before you leave
it when you know for certain the target is not behind you, such as coming from
a dead-end or similar.  Then you never cross a mark in your search, in fact,
treat them as walls.  The "unmarked" area will then become less and less and
you will eventually stumble across the target.

However, if you have selected the "Double Route" option the above is not
guaranteed to work.  You will get times when you are going around in circles in
these "Double Routes" and you will never be certain the target is not behind
you.  Here you will have to guess at the appropriate place to mark to break
these circles.


CHEATING (SYSTEM SOLVING)

Like many of the other Portable Puzzle programs Maze3d has a "Solve" option
from the "Games" menu.  When this is selected then the direction you need to go
to in order to get to the target appears highlighted in the compass in the
bottom of the Status Panel, and a "Thread" appears on the screen that leads you
to the target (unless you are facing in the oposite direction, in which case
turn).

There is a small caveat thoough.  If you have selected "Double Routes" then you
may see another "thread" on the screen that are not yours.  This is because the
other thread will ALSO lead you to the target (that what double routes are).
If you choose to follow the other thread (that is not on your square) then it
may take less time.  However it may also take longer, and continually changing
them may mean you are going round in circles.  If you follow the thread that is
in your room then you will deffinately get to the target eventually.

Selecting "Solve" again swithec the thread off.


PROGRAMME'S STUFF

The Maze32 source file suite comes in three files, "maze3d.c" and "maze3dc.c"
and maze3dc.h.  Each performs the following:

maze3d.c    This is the program that employs the Portable Puzzle's API, and
            executes the user interface of that.  This is in fact a derivative of
            that suite's "nullgame.c", which was created for the purpose of
            building new games.  It is very much a "Portable Puzzles" program.
            It creates an instance of the class coded in "maze3dc.c" then uses
            that object to handle the (complex) maze-play back-end functionality.
            Being built in the "Portable Puzzles" environment this also handles
            the "Save" and "Restore", despite the fact that the "maze3dc.c" class
            has it's own (which, although unused, is still in there if someone
            else needs it).

maze3dc.c   This is a class C source file.  It generates an object that handles
            the maze generation, validation of moves, orientation algorithms when
            rotating.  As well as that it sorts out where things are and what can
            be seen by the "camera" at any given time where it works out the
            orientation of things - such as which way is "up" - and presents data
            to a front end (like maze3d.c) to be displayed.  It does not handle
            any of the displaying as such though.  This is portable and does not
            rely on any Portable Puzzle's functionality (or anyone else, apart
            from standard C libraries).  There is no reason why it could not be
            implemented using different front ends if required.

maze3dc.h   This is the header file for "maze3dc.c".  It contains function and
            data deffinitions needed to interface with "maze3dc" class's methods
            and functions.  The documentation of these exist here as comments
            too.

LICENSE

The software and documentation is released under the same license as Simon's
Tatham's Portable Puzzles environment , the MIT license, text as follows:

This software is copyright 2006 Edward Macnaghten

maze3d.c also contain work copyrighted to Simon Tatham

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

