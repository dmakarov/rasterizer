/**
   \file rasterizer.h

   Created by Dmitri Makarov on 16-05-31.
   Copyright © 2016 Dmitri Makarov. All rights reserved.
*/

#ifndef rasterizer_h
#define rasterizer_h

#include "observer.h"

#include <algorithm>
#include <cassert>
#include <functional>
#include <iostream>
#include <memory>
#include <vector>

enum SHIFT_MODE_TYPE {
  GRID = 0,
  RANDOM
};

enum WEIGHT_FUNC_TYPE {
  BOX = 0,
  BARTLETT };

struct RGB8 {
  unsigned int pixel;

  RGB8() : pixel(0)
  {}

  RGB8(unsigned int pixel) : pixel(pixel)
  {}

  RGB8(const RGB8& pixel) : pixel(pixel.pixel)
  {}

  float get_red() const
  {
    return ((float) (pixel & 0xff)) / 255.0f;
  }

  void set_red(unsigned int c)
  {
    pixel = (pixel & 0xffffff00) | (c & 0xff);
  }

  float get_green() const
  {
    return ((float) ((pixel >> 8) & 0xff)) / 255.0f;
  }

  void set_green(unsigned int c)
  {
    pixel = (pixel & 0xffff00ff) | ((c << 8) & 0xff00);
  }

  float get_blue() const
  {
    return ((float) ((pixel >> 16) & 0xff)) / 255.0f;
  }

  void set_blue(unsigned int c)
  {
    pixel = (pixel & 0xff00ffff) | ((c << 16) & 0xff0000);
  }

  operator bool() const
  {
    return pixel != 0;
  }

};

struct RGB32 {
  unsigned int r, g, b;

  RGB32() : r(0), g(0), b(0)
  {}

  RGB32(const RGB8& c)
    : r(c.pixel & 0xff)
    , g((c.pixel & 0xff00) >> 8)
    , b((c.pixel & 0xff0000) >> 16)
  {}

  RGB8 get(unsigned int k) const
  {
    return r / k + ((g / k) << 8) + ((b / k) << 16);
  }

  void operator+=(const RGB32& a)
  {
    r += a.r;
    g += a.g;
    b += a.b;
  }

  RGB32& operator*(unsigned int w)
  {
    r *= w;
    g *= w;
    b *= w;
    return *this;
  }
};

struct Point {
  float x, y;

  Point() : x(0.0f), y(0.0f)
  {}

  Point(std::initializer_list<float> il)
  {
    auto it = il.begin();
    x = *it++;
    y = *it;
  }
};

struct Frame {
  std::vector<Point> vertices;
  int number;
};

struct Animation {
  std::vector<Frame> keyframes;
  unsigned int r, g, b;

  RGB8 get_color() const
  {
    return (0xff0000 & ((0xff & b) << 16)) |
           (0x00ff00 & ((0xff & g) <<  8)) |
                        (0xff & r);
  }

  void set_color(unsigned int R, unsigned int G, unsigned int B)
  {
    r = R; g = G; b = B;
  }

  std::vector<Point>::size_type get_num_vertices() const
  {
    return keyframes[0].vertices.size();
  }

  std::vector<Point>& get_vertices(int frame)
  {
    return keyframes[frame].vertices;
  }

};

std::ostream& operator<<(std::ostream& os, const Animation& obj);

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

/**
  \brief Abuffer holds canvas pixels with 32 bits per RGB component.
*/
struct Abuffer {
  size_t size;
  std::unique_ptr<RGB32[]> pixels;

  Abuffer(size_t w = 0, size_t h = 0) : size(w * h), pixels(new RGB32[size])
  {}

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

/**
  \class Rasterizer manages the objects to be rendered and implements the
  rasterization algorithm on canvas with the objects.
*/
class Rasterizer : public Subject {

  using ObjSP = std::shared_ptr<Animation>;
  using ObjSz = std::vector<ObjSP>::size_type;

  std::vector<ObjSP> objects;
  std::unique_ptr<RGB8[]> pixels;
  int width;
  int height;
  Point original;
  ObjSz selected_object;
  ObjSz selected_vertex;
  bool is_object_selected = false;

public:

  static const auto MAX_SAMPLES = 64;

  Rasterizer(int w = 0, int h = 0) : pixels(new RGB8[w * h]), width(w), height(h)
  {
    clear();
  }

  int get_width() const
  {
    return width;
  }

  int get_height() const
  {
    return height;
  }

  std::vector<ObjSP>& get_objects()
  {
    return objects;
  }

  void add_object(ObjSP& object)
  {
    objects.push_back(object);
  }

  /**
     \returns false if no object has keyframe at <frame> and true otherwise.
  */
  bool any_keyframe(int frame) const
  {
    auto E = objects.end();
    return E !=
      std::find_if(objects.begin(), E, [this, frame] (const ObjSP& a) {
          return find_keyframe(a, frame) != a->keyframes.end(); });
  }

  std::vector<Point>::size_type get_num_vertices(ObjSz ix) const
  {
    return objects[ix]->get_num_vertices();
  }

  std::vector<Frame>::size_type get_num_keyframes(ObjSz ix) const
  {
    return objects[ix]->keyframes.size();
  }

  ObjSz get_selected_object() const
  {
    return is_object_selected ? selected_object : objects.size();
  }

  auto get_selected_vertex() const -> decltype(selected_vertex)
  {
    return selected_vertex;
  }

  bool is_selected() const
  {
    return is_object_selected;
  }

  void delete_selected_object()
  {
    if (!is_object_selected) {
      return;
    }
    objects.erase(objects.begin() + selected_object);
    is_object_selected = false;
    notify();
  }

  void delete_keyframe(int frame)
  {
    if (frame == 1 || !any_keyframe(frame)) {
      return;
    }
    for (auto o : objects) {
      auto k = find_keyframe(o, frame);
      auto E = o->keyframes.end();
      if (k != E) {
        for (auto j = k; j != E - 1; ++j) {
          std::copy(j + 1, j + 2, j);
        }
        o->keyframes.pop_back();
      }
    }
  }

  /**
    This function takes a frame number, and a bunch of arguments
    showing how the frame should be rasterized.  By the time
    rasterize() completes, the canvas should be filled.
  */
  void rasterize(int frame,
                 bool aa_enabled, int num_aa_samples,
                 bool mb_enabled, int num_mb_samples,
                 const std::string& aa_filter) const;
  void render_to_file(const std::vector<std::string>& args);
  bool load_objects(const std::string& filename);
  void save_objects(const std::string& filename) const;
  void save_image(const std::string& filename) const;
  bool select_object(const int frame, const float mx, const float my);
  unsigned char* getPixelsAsRGB() const;
  /**
   \brief get_vertices
   This function takes in an object <id>, a frame number and an
   array of Points and fills in that array with the vertices of
   that object at that point in time.  If the passed frame is
   between keyframes, get_vertices will automatically interpolate
   linearly and give you the correct values.
  */
  RGB8 get_vertices(const ObjSz id, const float frame,
                    std::vector<Point>& vertices) const;

private:

  void clear() const {
    std::fill(pixels.get(), pixels.get() + width * height, 0);
  }

  /**
   \brief find_keyframe
   This function will tell you if object <a> has a keyframe at
   frame <frame>, and, if it does, its index in the object's
   keyframe array.
   For example, if an object has keyframes at frames 1, 5, and 10,
   calling find_keyframe(a, 1) will return 0, find_keyframe(a, 10)
   will return 2, and find_keyframe(a, 20) will return -1.
   \param a - a pointer to an Animation object
   \param frame - a frame to check whether it's a keyframe
   \return -1 if the object does not have a keyframe at frame,
           otherwise the index of the keyframe in the array of keyframes.
  */
  std::vector<Frame>::iterator
  find_keyframe(const ObjSP& a, int frame) const
  {
    return std::find_if(a->keyframes.begin(), a->keyframes.end(),
           [this, frame] (const Frame& f) { return f.number == frame; });
  }

  void scan_convert(std::vector<Point>& vertices, RGB8 color) const;
};

#endif /* rasterizer_h */

// Local Variables:
// mode: c++
// End:
