/**
   \file rasterizer.h

   Created by Dmitri Makarov on 16-05-31.
   Copyright © 2016 Dmitri Makarov. All rights reserved.
*/

#ifndef rasterizer_h
#define rasterizer_h

#include "polygon.h"

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
  \class implements the rasterization algorithm on canvas with the objects.
*/
class Rasterizer {

  std::unique_ptr<RGB8[]> pixels;
  int width;
  int height;

public:

  static const auto MAX_SAMPLES = 64;

  Rasterizer(int w = 500, int h = 500)
  : pixels(new RGB8[w * h]), width(w), height(h)
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

  /**
    This function takes a frame number, and a bunch of arguments
    showing how the frame should be rasterized.  By the time
    rasterize() completes, the canvas should be filled.
  */
  void run(const std::vector<std::shared_ptr<Polygon>>& polygons, int frame,
           bool aa_enabled, int num_aa_samples,
           bool mb_enabled, int num_mb_samples,
           const std::string& aa_filter) const;
  void save(const std::string& filename) const;
  unsigned char* getPixelsAsRGB() const;

private:

  void clear() const {
    auto* p = pixels.get();
    std::fill(p, p + width * height, 0);
  }

  void scan_convert(std::vector<Point>& vertices, RGB8 color) const;
};

#endif /* rasterizer_h */

// Local Variables:
// mode: c++
// End:
