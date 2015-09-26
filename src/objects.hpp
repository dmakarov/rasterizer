/**
 * Defines the data structures and objects used in the CS248
 * animation/rasterizer assignment
 */

#ifndef _OBJECTS_HPP
#define _OBJECTS_HPP

#include <functional>

/* These are constants. You can increase them if your animation requires
   it, but do not decrease them, so that we can have data sets that will
   always work on them.*/

#define MAX_VERTICES 100
#define MAX_KEYFRAMES 10
#define MAX_FRAMES 100
#define MAX_ALIAS_SAMPLES 64
#define MAX_BLUR_SAMPLES 64

struct Point {
  float x;
  float y;
};

struct FrameData {
  Point vertices[MAX_VERTICES];
  int frameNumber;
};

struct AnimObject {
  FrameData keyframes[MAX_KEYFRAMES];
  int numVertices;
  int numKeyframes;
  int r, g, b;
};

struct Edge {
  float yy, xx, kk;
  Edge(float yy_ = 0, float xx_ = 0, float kk_ = 0) : yy(yy_), xx(xx_), kk(kk_)
  {}
  bool operator<(const Edge& edge) const
  {
    return (xx < edge.xx);
  }
};

class edge_ymax_le : public std::unary_function<Edge, bool>
{
  float ymax;
public:
  explicit edge_ymax_le(float yy) : ymax(yy)
  {}
  bool operator()(const Edge& edge) const
  {
    return (edge.yy <= ymax);
  }
};

#endif
