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

enum class SHIFT_MODE { GRID, RANDOM };
enum class WEIGHT_FUN { BOX, BARTLETT };

struct Edge {
  float yy, xx, kk;

  Edge(float y = 0, float x = 0, float k = 0) : yy(y), xx(x), kk(k)
  {}

  bool operator<(const Edge& edge) const {
    return (xx < edge.xx);
  }
};

class edge_ymax_le : public std::unary_function<Edge, bool>
{
  float ymax;
public:
  explicit edge_ymax_le(float yy) : ymax(yy)
  {}

  bool operator()(const Edge& edge) const {
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

  void add(const RGB8* colors, size_t size, unsigned int weight = 1) {
    assert(size <= this->size);
    for (int x = 0; x < size; ++x) {
      pixels[x] += RGB32(colors[x]) * weight;
    }
  }

  void get(RGB8* p, size_t size, unsigned int k) {
    assert(size <= this->size);
    for (int x = 0; x < size; ++x) {
      p[x] = pixels[x].get(k);
    }
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
    : pixels(new RGB8[w * h]), width(w), height(h) {
    clear();
  }

  int getWidth() const {
    return width;
  }

  int getHeight() const {
    return height;
  }

  void resize(int w, int h) {
    width = w;
    height = h;
    pixels.reset(new RGB8[w * h]);
  }

  /**
     \brief takes a frame number, and a bunch of arguments showing how the frame
            should be rasterized.
  */
  void run(const std::vector<std::shared_ptr<Polygon>>& polygons,
           int frame,
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

  void scanConvert(std::vector<Point>& vertices, RGB8 color) const;
};

#endif /* rasterizer_h */

// Local Variables:
// mode: c++
// End:
