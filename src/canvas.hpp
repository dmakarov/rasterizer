#ifndef _CANVAS_HPP
#define _CANVAS_HPP

#include <algorithm>
#include <functional>
#include <vector>

/* MACROS. */

/* The following macros retrieve the primary color components from one
   canvas pixel P. */

#define GET_RED(P)   (((P)      ) & 0xFF)
#define GET_GREEN(P) (((P) >>  8) & 0xFF)
#define GET_BLUE(P)  (((P) >> 16) & 0xFF)

/* The following macros set the primary color components of the canvas
   pixel P to C. C must lie between 0 and 255 (both endpoints
   inclusive). */

#define SET_RED(P,C)   (P = (((P) & 0xFFFFFF00) | ((C)      )))
#define SET_GREEN(P,C) (P = (((P) & 0xFFFF00FF) | ((C) <<  8)))
#define SET_BLUE(P,C)  (P = (((P) & 0xFF00FFFF) | ((C) << 16)))

/* The following macro retrieves the pixel at coordinates (X,Y) of the
   canvas C (where C is a pointer to a canvas structure). (0,0) are the
   coordinates of the top left corner of the canvas, while the bottom
   right corner is located at (C->Width-1,C->Height-1). */

#define PIXEL(C,X,Y) ((C)->Pixels[(Y)*(C)->Width+(X)])

typedef unsigned int RGB8;

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

struct Canvas {
  int Width;
  int Height;
  RGB8 *Pixels;
  AnimObject* currObject;
  std::vector<AnimObject*> objects;

  Canvas(int ww = 0, int hh = 0) : Width(ww), Height(hh), Pixels(0), currObject(nullptr)
  {
    Pixels = new unsigned int[Width * Height];
    std::fill(Pixels, Pixels + Width * Height, 0);
  }
  void init(RGB8 color)
  {
    std::fill(Pixels, Pixels + Width * Height, color);
  }
  RGB8 get(int xx, int yy)
  {
    return (Pixels[Width * yy + xx]);
  }
  void load_objects(char* filename);
  void save_objects(char* filename);
  void delete_object(int id);
  void delete_keyframe(int id);
  void edit_screen_display();
  void render();
  void save(const char* filename) const;
  void polygon_scaling(int mx, int my);
  void polygon_rotation(int mx, int my);
  bool mouse(int button, int state, int mx, int my);
  bool motion(int mx, int my);
  bool select_object(int button, int mx, int my);

  /** Function: Rasterize
   -------------------
   This function takes in a pointer to a canvas, a frame number, and a
   bunch of arguments showing how the frame should be rasterized. By the
   time Rasterize() completes, the canvas should be filled.
  */

  void rasterize(int frameNumber, bool antiAlias, int numAliasSamples, bool motionBlur, int numBlurSamples);
  void scan_convert(Point* vertex, int vertno, RGB8 color);

  /** Function: FindKeyframe
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

  /** Function: AnyKeyframe
      ---------------------
      This function just returns 0 if no objects have a keyframe at
      <frameNumber> and 1 otherwise.
  */
  int AnyKeyframe(int frameNumber);

  /** Function: GetVertices
      ---------------------
      This function takes in an object ID, a frame number and an array of
      Points and fills in that array with the vertices of that object at
      that point in time. If the passed frameNumber is between keyframes,
      GetVertices will automatically interpolate linearly and give you the
      correct values.
  */
  unsigned int GetVertices(int id, float frameNumber, Point* holderFrame);
};

// This is a structure to hold canvas pixels with 32 bit per RGB component.

typedef struct {
  unsigned int rr, gg, bb;
} RGB32;

typedef struct abuffer_struct {
  int Width, Height;
  RGB32* pixel;
  abuffer_struct(int ww = 0, int hh = 0)
    : Width(ww), Height(hh), pixel(0)
  {}
  void init()
  {
    if (!pixel) return;
    for (int jj = 0; jj < Height; ++jj)
      for (int ii = 0; ii < Width; ++ii)
        pixel[Width * jj + ii].rr =
          pixel[Width * jj + ii].gg =
          pixel[Width * jj + ii].bb = 0;
  }
  void add(int xx, int yy, RGB8 color, int weight = 1)
  {
    pixel[Width * yy + xx].rr += (0xFF & (color      )) * weight;
    pixel[Width * yy + xx].gg += (0xFF & (color >>  8)) * weight;
    pixel[Width * yy + xx].bb += (0xFF & (color >> 16)) * weight;
  }
  RGB8 get(int xx, int yy, int kk)
  {
    RGB8 rr = pixel[Width * yy + xx].rr / kk;
    RGB8 gg = pixel[Width * yy + xx].gg / kk;
    RGB8 bb = pixel[Width * yy + xx].bb / kk;
    return (rr + (gg << 8) + (bb << 16));
  }
} Abuffer;

#endif

// Local Variables:
// mode: c++
// End:
