# rasterizer

[<img src="https://travis-ci.org/dmakarov/paintbrush.png?branch=master">](https://travis-ci.org/dmakarov/paintbrush)

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
