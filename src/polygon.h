//
//  polygon.h
//
//  Created by Dmitri Makarov on 16-08-19.
//
//

#ifndef polygon_h
#define polygon_h

#include <vector>

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

  void operator+=(const Point& p)
  {
    x += p.x;
    y += p.y;
  }
};

struct Frame {
  std::vector<Point> vertices;
  int number;
};

struct Polygon {
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

  std::vector<Frame>::size_type get_num_keyframes() const
  {
    return keyframes.size();
  }

  std::vector<Point>::size_type get_num_vertices() const
  {
    return keyframes[0].vertices.size();
  }

  std::vector<Point>& get_vertices(int frame)
  {
    return keyframes[frame].vertices;
  }
  /**
   \brief find_keyframe
   This function will tell you if object <a> has a keyframe at
   frame <frame>, and, if it does, its index in the object's
   keyframe array.
   For example, if an object has keyframes at frames 1, 5, and 10,
   calling find_keyframe(a, 1) will return 0, find_keyframe(a, 10)
   will return 2, and find_keyframe(a, 20) will return -1.
   \param frame - a frame to check whether it's a keyframe
   \return -1 if the object does not have a keyframe at frame,
   otherwise the index of the keyframe in the array of keyframes.
   */
  std::vector<Frame>::iterator
  find_keyframe(int frame)
  {
    return std::find_if(keyframes.begin(), keyframes.end(),
                        [this, frame] (Frame& f) { return f.number == frame; });
  }
  /**
   \brief get_vertices
   This function takes in an object <id>, a frame number and an
   array of Points and fills in that array with the vertices of
   that object at that point in time.  If the passed frame is
   between keyframes, get_vertices will automatically interpolate
   linearly and give you the correct values.
   */
  RGB8 get_vertices(const float frame, std::vector<Point>& vertices);

};

std::ostream& operator<<(std::ostream& os, const Polygon& obj);

#endif /* polygon_h */
