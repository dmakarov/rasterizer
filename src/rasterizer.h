//
//  rasterizer.h
//  rasterizer
//
//  Created by Dmitri Makarov on 16-05-31.
//  Copyright © 2016 Dmitri Makarov. All rights reserved.
//

#ifndef rasterizer_h
#define rasterizer_h

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
};

struct RGB32 {
  unsigned int r, g, b;

  RGB32() : r(0), g(0), b(0)
  {}

  RGB32(const RGB8& c) : r(c.get_red()), g(c.get_green()), b(c.get_blue())
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
  int numVertices;
  unsigned int r, g, b;

  RGB8 get_color() const
  {
    return r + (g << 8) + (b << 16);
  }

  void set_color(unsigned int R, unsigned int G, unsigned int B)
  {
    r = R; g = G; b = B;
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
  \class Rasterizer
*/
class Rasterizer {
  static const auto MAX_ALIAS_SAMPLES = 64;
  static const auto MAX_BLUR_SAMPLES = 64;
  int width;
  int height;
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
  std::unique_ptr<RGB8[]> pixels;
  std::vector<std::shared_ptr<Animation>> objects;
  std::shared_ptr<Animation> active_object;

public:
  Rasterizer(int w = 0, int h = 0) : width(w), height(h), pixels(new RGB8[w * h])
  {
    clear();
  }

  void clear() const
  {
    std::fill(pixels.get(), pixels.get() + width * height, 0);
  }
  /**
    This function takes a frame number, and a bunch of arguments
    showing how the frame should be rasterized.  By the time
    rasterize() completes, the canvas should be filled.
  */
  void rasterize(int frame, bool aa_enabled, int num_aa_samples,
                 bool mb_enabled, int num_mb_samples,
                 const std::string& aa_filter) const;
  void render() const;
  void render_to_file(std::vector<std::string>& args);
  bool load_objects(const std::string& filename);
  void save_objects(const std::string& filename) const;
  void save_image(const std::string& filename) const;
  void delete_keyframe(int frame);
  bool delete_object(int id);
  bool motion(int mx, int my, int frame, int selected_object);
  /**
   \brief get_vertices
   This function takes in an object <id>, a frame number and an
   array of Points and fills in that array with the vertices of
   that object at that point in time.  If the passed frame is
   between keyframes, get_vertices will automatically interpolate
   linearly and give you the correct values.
  */
  RGB8 get_vertices(std::vector<std::shared_ptr<Animation>>::size_type id,
                    float frame, std::vector<Point>& holderFrame) const;

  std::vector<std::shared_ptr<Animation>>& get_objects()
  {
    return objects;
  }

  /**
    \returns false if no objects have a keyframe
    at <frame> and true otherwise.
  */
  bool any_keyframe(int frame) const
  {
    auto it = std::find_if(objects.begin(), objects.end(),
                           [this, frame] (const std::shared_ptr<Animation>& a)
                           {
                             return find_keyframe(a, frame) != a->keyframes.end();
                           });
    return it != objects.end();
  }

  int get_num_vertices(std::vector<std::shared_ptr<Animation>>::size_type ix) const
  {
    return objects[ix]->numVertices;
  }

  decltype(objects[0]->keyframes.size())
  get_num_keyframes(std::vector<std::shared_ptr<Animation>>::size_type ix) const
  {
    return objects[ix]->keyframes.size();
  }

  void dump() const
  {
    for (auto obj : objects)
    {
      std::cout << *obj << "\n";
    }
  }

private:
  /**
   \brief find_keyframe
   This function will tell you if object <a> has a keyframe at
   frame <frame>, and, if it does, its index in the object's
   keyframe array.  find_keyframe will return -1 if the object does
   not have a keyframe at that frame.  For example, if object a has
   keyframes at frames 1, 5, and 10, calling find_keyframe(a, 1)
   will return 0, find_keyframe(a, 10) will return 2, and
   find_keyframe(a, 20) will return -1.
  */
  std::vector<Frame>::iterator
  find_keyframe(const std::shared_ptr<Animation>& a, int frame) const
  {
    return std::find_if(a->keyframes.begin(), a->keyframes.end(),
                        [this, frame] (const Frame& f)
                        {
                          return f.number == frame;
                        });
  }
  void scan_convert(std::vector<Point>& vertices, int vertno, RGB8 color) const;
  bool select_object(int mx, int my, int frame, bool is_right_click, int& selected_object);
  void polygon_scaling(int mx, int my, int frame, int selected_object);
  void polygon_rotation(int mx, int my, int frame, int selected_object);
};

#endif /* rasterizer_h */

// Local Variables:
// mode: c++
// End: