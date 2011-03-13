/* objects.h
 * Defines the data structures and objects used in the CS248
 * animation/rasterizer assignment
 * written by Yar Woo (ywoo@cs)
 */

#ifndef _OBJECTS_H
#define _OBJECTS_H

#include "canvas.h"

#include <functional>

/* These are constants. You can increase them if your animation requires
   it, but do not decrease them, so that we can have data sets that will
   always work on them.*/

#define MAX_VERTICES 100
#define MAX_OBJECTS 10
#define MAX_KEYFRAMES 10
#define MAX_FRAMES 100
#define MAX_ALIAS_SAMPLES 64
#define MAX_BLUR_SAMPLES 64

typedef struct {
    float x;
    float y;
} Point;

typedef struct {
    int frameNumber;
    Point vertices[MAX_VERTICES];
} FrameData;

typedef struct {
    int numVertices;
    FrameData keyframes[MAX_KEYFRAMES];
    int numKeyframes;
    int r, g, b;
} AnimObject;

typedef struct edge_struct {
    float yy, xx, kk;
    edge_struct( float yy_ = 0, float xx_ = 0, float kk_ = 0 )
        :
        yy( yy_ ),
        xx( xx_ ),
        kk( kk_ )
    {}
    bool operator<( const edge_struct& edge ) const
    {
        return ( xx < edge.xx );
    }
} edge_type;

class edge_ymax_le : public std::unary_function< edge_type, bool >
{
    float ymax;
public:
    explicit edge_ymax_le( float yy ) : ymax( yy ) 
    {}
    bool operator()( const edge_type& edge ) const
    {
        return ( edge.yy <= ymax );
    }
};

extern int numObjects;
extern AnimObject* objects[MAX_OBJECTS];

/* Function: GetVertices 
   --------------------- 
   This function takes in an object ID, a frame number and an array of
   Points and fills in that array with the vertices of that object at
   that point in time. If the passed frameNumber is between keyframes, 
   GetVertices will automatically interpolate linearly and give you the
   correct values.
*/
 
unsigned long GetVertices( int id, float frameNumber, Point* holderFrame );

/* Function: FindKeyframe
   ----------------------
   This function will tell you if object <id> has a keyframe at frame
   <frameNumber>, and, if it does, its index in the object's keyframe
   array. Findkeyframe will return -1 if the object does not have a
   keyframe at that frame. For example, if object 0 has keyframes at
   frames 1, 5, and 10, calling FindKeyframe(0, 1) will return 0,
   FindKeyframe(0, 10) will return 2, and FindKeyframe(0, 20) will return
   -1.
*/

int FindKeyframe(int id, int frameNumber);

/* Function: AnyKeyframe
   ---------------------
   This function just returns 0 if no objects have a keyframe at
   <frameNumber> and 1 otherwise.
*/

int AnyKeyframe(int frameNumber);

/* Function: Rasterize
   -------------------
   This function takes in a pointer to a canvas, a frame number, and a
   bunch of arguments showing how the frame should be rasterized. By the
   time Rasterize() completes, the canvas should be filled.
*/

void Rasterize(Canvas* renderCanvas, int frameNumber, bool antiAlias,
	       int numAliasSamples, bool motionBlur, int numBlurSamples);

#endif








