#ifndef _CANVAS_HPP
#define _CANVAS_HPP

#include <algorithm>
#include <functional>
#include <memory>
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
#define SET_RED(P,C) (P = (((P) & 0xFFFFFF00) | (C)))

typedef unsigned int RGB8;

/* These are constants. You can increase them if your animation requires
   it, but do not decrease them, so that we can have data sets that will
   always work on them.*/
#define MAX_KEYFRAMES     10
#define MAX_VERTICES     100
#define MAX_FRAMES       100
#define MAX_ALIAS_SAMPLES 64
#define MAX_BLUR_SAMPLES  64

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
  Edge(float y = 0, float x = 0, float k = 0) : yy(y), xx(x), kk(k)
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

class Canvas {
  int Width;
  int Height;
  int originalX;
  int originalY;
  int prev_rotationX;
  int prev_rotationY;
  int rotation_centerX = -1;
  int rotation_centerY;
  int selectedVertex = -1;
  bool rotate_polygon = false;
  bool scale_polygon = false;
  bool draw_curve = false;
  RGB8* Pixels;
  AnimObject* currObject = nullptr;
  std::vector<AnimObject*> objects;

public:
  Canvas(int w = 0, int h = 0) : Width(w), Height(h)
  {
    Pixels = new RGB8[Width * Height];
    init(0);
  }
  ~Canvas()
  {
    delete [] Pixels;
  }
  void render();
  void display(int frame, int selected_object) const;
  void save(const char* filename) const;
  void load_objects(const char* filename);
  void save_objects(const char* filename);
  void delete_keyframe(int id, int frame);
  bool delete_object(int id);
  bool motion(int mx, int my, int frame, int selected_object);
  bool mouse(int button, int state, int mx, int my, int frame, int& selected_object);
  /**
   *  This function takes a frame number, and a bunch of arguments
   *  showing how the frame should be rasterized.  By the time
   *  rasterize() completes, the canvas should be filled.
   */
  void rasterize(int frame, bool antiAlias, int numAliasSamples, bool motionBlur, int numBlurSamples, const char* aafilter_function);
  /**
   *  @returns false if no objects have a keyframe
   *  at <frame> and true otherwise.
   */
  bool any_keyframe(int frame) const
  {
    auto it = std::find_if(objects.begin(), objects.end(), [frame, this] (const AnimObject* a) {return find_keyframe(a, frame) != -1;});
    return it != objects.end();
  }
  int get_num_vertices(int object_num) const
  {
    return objects[object_num]->numVertices;
  }
  int get_num_keyframes(int object_num) const
  {
    return objects[object_num]->numKeyframes;
  }

private:
  void init(RGB8 color)
  {
    std::fill(Pixels, Pixels + Width * Height, color);
  }
  /** Function: get_vertices
      ---------------------
      This function takes in an object <id>, a frame number and an
      array of Points and fills in that array with the vertices of
      that object at that point in time.  If the passed frame is
      between keyframes, get_vertices will automatically interpolate
      linearly and give you the correct values.
  */
  unsigned int get_vertices(int id, float frame, Point* holderFrame) const;
  /** Function: find_keyframe
      ----------------------
      This function will tell you if object <a> has a keyframe at
      frame <frame>, and, if it does, its index in the object's
      keyframe array.  find_keyframe will return -1 if the object does
      not have a keyframe at that frame.  For example, if object a has
      keyframes at frames 1, 5, and 10, calling find_keyframe(a, 1)
      will return 0, find_keyframe(a, 10) will return 2, and
      find_keyframe(a, 20) will return -1.
  */
  int find_keyframe(const AnimObject* a, int frame) const;
  void scan_convert(Point* vertex, int vertno, RGB8 color);
  void polygon_scaling(int mx, int my, int frame, int selected_object);
  void polygon_rotation(int mx, int my, int frame, int selected_object);
  bool select_object(int button, int mx, int my, int frame, int& selected_object);
};

/**
 *  Abuffer holds canvas pixels with 32 bits per RGB component.
 */
struct Abuffer {
  struct RGB32 {
    unsigned int r, g, b;
    RGB32() : r(0), g(0), b(0) {}
    RGB32(const RGB8& c) : r(0xff & c), g(0xff & (c >> 8)), b(0xff & (c >> 16)) {}
    RGB8 get(unsigned int k) { return r / k + ((g / k) << 8) + ((b / k) << 16); }
    void operator+=(const RGB32& a) { r += a.r; g += a.g; b += a.b; }
    RGB32& operator*(unsigned int w) { r *= w; g *= w; b *= w; return *this; }
  };
  size_t size;
  std::unique_ptr<RGB32[]> pixels;
  Abuffer(size_t w = 0, size_t h = 0) : size(w * h), pixels(new RGB32[size]) {}
  void add(size_t x, RGB8 color, unsigned int weight = 1)
  {
    assert(x < size);
    pixels[x] += RGB32(color) * weight;
  }
  RGB8 get(size_t x, unsigned int k)
  {
    assert(x < size);
    return pixels[x].get(k);
  }
};

#endif
