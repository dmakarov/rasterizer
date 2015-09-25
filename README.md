# rasterizer

[<img src="https://travis-ci.org/dmakarov/rasterizer.png?branch=master">](https://travis-ci.org/dmakarov/rasterizer)

Prerequisites:

On Mac OS X install ```glui``` (e.g. ```brew install glui```).

Functionality:

This program implements a rasterizer.  The rasterizer is capable of
scan-converting polygons of any geometry, specified by their vertices
coordinates on the canvas.  In addition, the rasterizer uses spatial
supersampling and motion blur to reduce aliasing artifacts in space and time.

The polygons can be drawn on the edit canvas by setting their vertices with
mouse clicks or by holding the shift-key and mouse left button down and moving
the mouse pointer on the edit window canvas.  The latter allows the user to
enter curves approximated by sets of lines.

The user interface allows the user to rotate existing polygons and scale them
relative to the selected center point on canvas.  To select the center point the
user clicks the left button of the mouse while holding the ctrl key down.  After
the center point is set, the user can rotate the polygon by pulling the polygon
with mouse for one of its vertices while holding the ctrl key down.  Or the user
can scale the polygon relative to the previously selected center point by
pulling the polygon for one of its vertices while holding the ctrl and shit keys
down.

The user can specify the filter kernel by typing commands in filter text box of
the GUI.  Currently accepted commands are box, grid, random and bartlett.  These
commands determined how the supersampling will be performed.

Implementation details:

The scan conversion, supersampling and motion blur is implemented as specified
by the accumulation buffer algorithm given to us.  The default filter for
supersampling is a box on a grid of subpixels with random placement of samples
within each individual subpixel.  Scan conversion is an implementation of the
FvD book section 3.6 algorithm.  One notable exception is that the range of Y
coordinates of the vertices in the current scene is determined prior to the edge
table construction.  Then the edge table allocated to accommodate just for the
computed range of Y coordinates. For example, if the minimum Y coordinate on the
scene is 15 and the maximum Y coordinate is 385, then the edge table will have
buckets 0 to 370 only.  We'll compute Y coordinate of each line during the line
conversion by adding 15 to whatever Ymin coordinate we determine from the edge
table.  In other words if the edge is located in bucket 0 of the edge table, its
Ymin is 15 etc.  This works for negative coordinates as well.  It slows down the
implementation because we need to reallocate the edge table before each
scan-conversion run, which probably can be avoided.  At the same time it makes
the implementation safe for any scenes, no matter how far beyond the canvas
boundaries the scene vertices are located.

The application was tested by running it and using different combinations of
polygons, trying to hit all the possible corner case.  For example, by scaling a
polygon far beyond the boundaries of the canvas, by enabling the maximum
allowable number of AA and MB samples, by specifying different last keyframes
for different polygons in a scene etc.

* Files

# Files in the package, what they do, and how to build them.

# IMPORTANT FILES AND DIRECTORIES


README.files (this file)

README.animgui : Basic instructions on how to use the initial program we give
  you. Describes how to add vertex points, how to render and save images,
  and how to create animations and view them. You should definitely read this!

/src/animgui.cpp : The main module of the program that creates the GUI
  and sets up the editing and rendering canvases. You won't have to change this
  file unless you want to change the interface, which is not required for this
  assignment.

/src/canvas.cpp, canvas.h : Provides you with functions that modify and
  read values from the rendering canvas in a way that is similar to project 1.

/src/objects.cpp, objects.h : The heart of the rendering functions for
  the program. This is where the Rasterize() function lives, where most of your
  work will be focused. Read the .h file for a detailed description of the
  provided functions.

/samples/README.samples : Check out this file for information on the sample
  .obs files and example results (in /samples/examples).

/docs : Documentation on the GLUI toolkit, if you need it.


# BUILDING THE PROGRAM
#

On Linux:

The executable for this program is called 'animgui' and it sits in the
/bin directory of the project. The source code you will be touching sits
in /src, and these files will be described shortly. To build the project,
type 'make' in the /src directory.

The program depends on GL and glut, which are usually installed by default 
on most linux systems, and glui (see http://glui.sourceforge.net/).

* GUI

1. How to use the GUI

1.1 Startin' her up

Start up the GUI by typing in "animgui", located in the "bin" directory of
your assignment, with no command-line parameters. You should be presented
with an amazingly elegant, yet mind-blowingly functional GUI where you can
specify polygons and how they change over time.

1.2 Specifying polygons

Shift-click on the main canvas (the Edit Window) to begin defining your
polygon. Additional shift-clicks will add more vertices at the specified
locations to your polygon. As soon as you do a normal click, you close the
polygon and cannot add any more vertices to it (unless, of course, you
want some extra credit). If you only bestow 1 or 2 points unto your
polygon, it is discarded. You can specify up to MAX_OBJECTS (defined in
objects.h) on your canvas.

1.3 Editing polygons

If your polygon offends you with its bold shape and sharp corners, you can
modify the location of any (or all) of the vertices. Simply click (as
opposed to shift-clicking) near the vertex you wish to move, then drag it
to its new location. You can also move entire polygons by right-clicking on a
polygon vertex and dragging it around.

1.4 Deleting polygons

If no amount of vertex modification will improve your polygon, select it
by clicking on one of its vertices and hit the "Del" key or the
"Backspace" key. Both of these should cause the offending polygon to
disappear forever.

1.5 Keyframes

A keyframe is an exact specification of a polygon at a given moment in
time. For instance, suppose you wanted to smoothly animate a square
turning into a house turning back into a square. You could individually
animate each frame, incrementally changing your square to look more and
more like a house (this is how traditional animation like Disney works),
but that is a pain in the behind, especially if you wish to also have a
life. The computer animation approach is to set keyframes:

Frame 1: Polygon looks like a square
Frame 15: Polygon looks like a house
Frame 30: Polygon looks like a square again

and the computer will fill in frames 2-14 and 16-29, by doing all of the
boring incrementing for you. 

The GUI provided allows you to easily set keyframes. Use the frame
spinner to select which frame you wish to set a keyframe at. (WARNING: If
you input a frame number using the text input box, be sure and press Enter
to register the change before continuing) Then edit the object you wish to
change. Voila! A red border appears, indicating that you have a keyframe
at that position. Now, if you browse the frames using the spinner, you
will see that the object is automatically interpolating between frames. By 
default, when you create an object, a keyframe is automatically created at 
frame 1 with that configuration.

1.6 Deleting Keyframes

What could be more simple? Spin to the offending keyframe and hit "Delete
Keyframe". The red border should go away, and any objects that had
keyframes at that frame will lose a little piece of themselves. Note that
you cannot delete keyframe 1; think of keyframe 1 as the "existence
keyframe", without which objects cannot survive.

1.7 Saving/Loading Object Files

To save an animation you have created, type in the name ("wombat", for
example) in the "Filename" field in the Save/Load panel and click "Save
Objects". The GUI will create a file "wombat.obs" that holds all of your
polygon and keyframe information. To load this file back in, type "wombat"
in the filename field and click Load -- you don't need to add the ".obs"
file extension.

1.8 Rendering

Now for the fun part. The first step is to set your antialias and motion
blur settings. These are pretty straightforward; click the checkbox to
enable a particular feature, then input the number of samples you want. 

To render the current frame of your animation, check the "This Frame Only"
radio button and hit "Render". If all goes well, your newly implemented 
rasterizer should display a filled, motion-blurred, antialiased,
museum-quality version of the pathetic line drawing in the edit
canvas. If you put text ("kangaroo") in the "Render Out" field, a
kangaroo.ppm file with your masterpiece will also be generated.

To render multiple frames, check the "Multiple Frames" radio button and
input the frame range you wish to render. In this case, you definitely
should input text into the "Render Out" field ("images/platypus", for
example), and the GUI will generate, in this case in the images/ directory:

platypus.5.ppm <- frame 5
platypus.6.ppm 
...
platypus.60.ppm <- frame 60
platypus.list (more on this later)

Warning: The .ppm files weigh in at about 700K each, so don't use up your
quota rendering 1000 frame animations.

Warning, part 2: Closing the Render Window will quit your program. Once it
gets popped up, just leave it alone. 

2. Command Line Arguments

One of the things that sucks about the GUI is that, in most cases, you
won't be able to use an emulator from your dorm room or whatnot to work on
it, because it displays OpenGL windows that most emulators don't
support. Not to worry! If the Sweet Hall labs are crowded, or if you are
just plain lazy, we have provided an alternative way for you to test your
rasterizer. For this, you will need:

one (1) .obs file

That's it! Once you have your .obs file (we'll provide you with some, or
you can create one with the GUI), invoke the animgui with the following
arguments:

animgui [-a<# of samples>] [-m<# of samples>] <start frame> <end frame>
<input OBS file> <output label>

So, if we wanted to make a Tazmanian devil animation, we might do
something like:

animgui -a4 -m6 1 50 tdevil.obs tdevil

This would read in tdevil.obs, render frames 1 through 50 with
antialiasing on (4 samples) and motion blurring on (6 samples), and export
the lot to tdevil.1.ppm, tdevil.2.ppm, etc. It will also generate a tdevil.list
file (which I'll get to in a bit).

animgui 5 5 tdevil.obs tdevil

Same as above, but with no antialiasing or motion blurring, and only
rendering frame 5.

3. Support Utilities

OK, so now you have the .ppms and a .list file, so what do you do with it?
Well, in the /usr/class/cs248/support/bin/i386-linux directory, there are
two utilities: ppm2fli and xanim. Grab them both.

Now, after switching to the directory with your .list file, run:

ppm2fli tdevil.list tdevil.flc

This utility will take all of the ppms found in your .list file and
compress them into an animation. After it does this, you can watch your
animation by typing:

xanim tdevil.flc

When you're giving your speech to the Academy, remember to mention your
wonderful CS248 TAs.

That should be it! Check objects.h for more implementation-specific
details.





