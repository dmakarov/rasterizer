/**
   \file rasterizer.cpp

   Created by Dmitri Makarov on 16-05-31.
   Copyright © 2016 Dmitri Makarov. All rights reserved.
 */

#include "rasterizer.h"
#include <cassert>
#include <cmath>
#include <fstream>
#include <list>
#include <ostream>
#include <random>
#include <sstream>
#include <string>

static SHIFT_MODE_TYPE shift_mode = RANDOM;
static WEIGHT_FUNC_TYPE weight_mode = BOX;
static std::uniform_real_distribution<float> urf(-0.5f, 0.5f);
static std::default_random_engine e;

#ifdef DEBUG_RASTERIZER
bool validate_aet(std::list<edge_type>& el)
{
  if (0 == el.size())
    return true;
  auto prev = el.begin()->xx;
  for (auto li = el.begin() + 1; li != el.end(); ++li) {
    auto curr = li->xx;
    if (prev > curr)
      return false;
    prev = curr;
  }
  return true;
} // validate_aet
#endif

// Bartlett filter implementation:
inline int bartlett(int sample, int total)
{
  if (BOX == weight_mode || (sample == total / 2 && 0 == total % 2))
    return 0;
  return (sample < total / 2 + total % 2) ? 1 : - 1;
} // bartlett

static float shift_function(int mode)
{
  return (RANDOM == mode) ? urf(e) : 0.0f;
} // shift_function

// Build a table of coordinate shifts within a pixel boundaries.
// Every value in data array is in the range [-0.5, 0.5].
static void precompute_shifts(Point data[8][8], int dim)
{
  if (1 == dim) {
    data[0][0].x = 0.0;
    data[0][0].y = 0.0;
    return;
  }

  if (dim > 8) dim = 8;
  float shift = 1.0 / (float)dim;
  float start = shift / 2.0 - 0.5;

  for (int ii = 0; ii < dim; ++ii) {
    for (int jj = 0; jj < dim; ++jj) {
      float incell = shift_function(shift_mode);
      data[ii][jj].x = start + ii * shift + incell * shift;
      data[ii][jj].y = start + jj * shift + incell * shift;
    }
  }
  if (1 == dim % 2) {
    data[dim/2][dim/2].x = 0.0;
    data[dim/2][dim/2].y = 0.0;
  }
} // precompute_shifts

static void parse_aafilter_func(const std::string& expr)
{
  if (std::string::npos != expr.find("rand")) {
    shift_mode = RANDOM;
  }
  if (std::string::npos != expr.find("grid")) {
    shift_mode = GRID;
  }
  if (std::string::npos != expr.find("bart")) {
    weight_mode = BARTLETT;
  }
  if (std::string::npos != expr.find("box")) {
    weight_mode = BOX;
  }
}

static void add_edge(std::unique_ptr<std::list<Edge>[]>& et,
                     Point* lo, Point* hi, int bias)
{
  float slope = (hi->x - lo->x) / (hi->y - lo->y);
  float ymin = ceilf(lo->y);
  float xmin = lo->x + slope * (ymin - lo->y);
  if ((slope < 0.0f && xmin < hi->x) || (slope > 0.0f && xmin > hi->x)) {
    xmin = hi->x;
  }
  int bucket = ymin - bias;
  et[bucket].emplace_back(Edge(hi->y, xmin, slope));
} // add_edge

void Rasterizer::save(const std::string& filename) const
{
  std::ofstream output(filename, std::ios::binary);
  assert(output);
  // print header
  output << "P6\n# Comment Line\n" << width << " " << height << "\n255\n";
  // output every pixel as 3 bytes
  auto* buffer = pixels.get();
  for (auto i = 0; i < width * height; ++i, ++buffer) {
    output.write(reinterpret_cast<char*>(buffer), 3);
  }
}

/**
   \brief drive the rasterization of a frame
 */
void Rasterizer::run(const std::vector<std::shared_ptr<Polygon>>& polygons,
                     int frame_num,
                     bool aa_enabled, int num_aa_samples,
                     bool mb_enabled, int num_mb_samples,
                     const std::string& aa_filter) const
{
  // set up the accumulation buffer and a scratch pad canvas;
  Abuffer abuf(width, height);
  Rasterizer pad(width, height);
  // precomputed shift distances for vertices in AA.
  Point aajitter[8][8];
  int tiles = 1;
  float frame_shift = 0.0;
  float frame_offset = 0.0;

  if (!aa_enabled) {
    num_aa_samples = 1;
  }
  if (num_aa_samples > 1) {
    // don't do more than MAX_SAMPLES:
    tiles = ceil(sqrt(num_aa_samples < MAX_SAMPLES ?
                      num_aa_samples : MAX_SAMPLES));
  }
  if (!aa_filter.empty()) {
    parse_aafilter_func(aa_filter);
  }
  if (!mb_enabled) {
    num_mb_samples = 1;
  }
  if (num_mb_samples > 1) {
    frame_offset = 1.0 / (2.0 * (float)num_mb_samples) - 0.5;
    frame_shift = 1.0 / (float)num_mb_samples;
  }
  precompute_shifts(aajitter, tiles);

  int samples = 0;
  int yyfilt = 1;
  int scans = 0;

  for (int jj = 0; jj < tiles; ++jj)
  {
    int xxfilt = 1;
    for (int ii = 0; ii < tiles; ++ii)
    {
      int aafilt = yyfilt * xxfilt;
      int mbfilt = 1;
      for (int mov = 0; mov < num_mb_samples; ++mov)
      {
        float frame = (float)frame_num + frame_offset + frame_shift * mov;
        if (frame < 1.0)
        {
          mbfilt += bartlett(mov + 1, num_mb_samples);
          continue;
        }
        pad.clear();
        for (auto& p : polygons)
        {
          // make sure it hasn't gone beyond the last frame
          float max_frame = (p->keyframes.end() - 1)->number;
          float adj_frame = (frame > max_frame) ? max_frame : frame;
          // Here we grab the vertices for this object at this snapshot in time
          std::vector<Point> vertices;
          RGB8 color = p->get_vertices(adj_frame, vertices);

          // shift vertices
          for (auto& v : vertices) {
            v += aajitter[ii][jj];
          }

          //printf("OBJECT #%2d [%d,%d] sample: %2d vertices\n", (int) obj, ii, jj, vertno);

          pad.scan_convert(vertices, color);
          ++scans;
        }
        // accumulate:
        for (int x = 0; x < height * width; ++x)
        {
          abuf.add(x, pad.pixels[x], mbfilt * aafilt);
        }
        // done with another sample:
        samples += mbfilt * aafilt;
        mbfilt += bartlett(mov + 1, num_mb_samples);
      }
      xxfilt += bartlett(ii + 1, tiles);
    }
    yyfilt += bartlett(jj + 1, tiles);
  }

  assert(samples != 0);

  clear();
  // convert accumulation buffer to RGB8 and copy to the render
  // canvas.
  for (int x = 0; x < height * width; ++x)
  {
    auto v = abuf.get(x, samples);
    //if (v) std::cout << std::dec << "P(" << x / width << ":" << x % width << ") " << v << std::endl;
    pixels[x] = v;
  }
} // Rasterize

void Rasterizer::scan_convert(std::vector<Point>& vertex, RGB8 color) const
{
  auto vertno = vertex.size();

#ifdef DEBUG_RASTERIZER
  std::cout << "color " << color << std::endl;
  for (int ii = 0; ii < vertno; ++ii)
  {
    printf( "  VERTEX #%2d: <%g, %g>\n", ii, vertex[ii].x, vertex[ii].y );
    //
    SET_RED(Pixels[vertex[ii].x-1 + (int)vertex[ii].y * Width], 255);
    SET_RED(Pixels[vertex[ii].x+0 + (int)vertex[ii].y * Width], 255);
    SET_RED(Pixels[vertex[ii].x+1 + (int)vertex[ii].y * Width], 255);
  }
#endif

  if (0 >= vertno)
  {
    //std::cout << "NO VERTICES TO SCAN %d" << vertno << std::endl;
    return;
  }

  // find the range of y coordinates:
  float ymax = vertex[0].y;
  float ymin = ymax;

  for (decltype(vertno) ii = 1; ii < vertno; ++ii)
  {
    if (ymax < vertex[ii].y) ymax = vertex[ii].y;
    if (ymin > vertex[ii].y) ymin = vertex[ii].y;
  }

  int bias = (int)ceilf(ymin);
  int range = (int)ceilf(ymax) - bias + 1;

  //printf("THE EDGE TABLE [%f,%f] size %d, bias %d\n", ymin, ymax, range, bias);

  // build the edge table
  std::unique_ptr<std::list<Edge>[]> edge_table(new std::list<Edge>[range]);

  for (decltype(vertno) ii = 0; ii < vertno; ++ii)
  {
    // do not add horizontal edges to the edge table.
    auto jj = (ii + 1) % vertno;
    if (vertex[ii].y < vertex[jj].y)
    {
      add_edge(edge_table, &vertex[ii], &vertex[jj], bias);
    }
    else if (vertex[ii].y > vertex[jj].y)
    {
      add_edge(edge_table, &vertex[jj], &vertex[ii], bias);
    }
  }

#ifdef DEBUG_RASTERIZER
  print_et( edge_table, range, bias );
#endif

  // initialize active edge table

  std::list<Edge> aet;

  // while not empty AET and ET

  for (int jj = 0; jj < range; ++jj)
  {
    int line = jj + bias;

    // move from ET to AET y_min == y edges

    std::list<Edge>::iterator li;
    for (li = edge_table[jj].begin(); li != edge_table[jj].end(); ++li)
    {
      aet.push_back(*li);
    }

#ifdef DEBUG_RASTERIZER
    if (edge_table[jj].begin() != edge_table[jj].end())
    {
      DOUT(( "AET @ %3d after adding:", line ));
      print_edge_list( aet );
    }
    li = std::find_if( aet.begin(), aet.end(), edge_ymax_le( line ) );
#endif

    // delete from AET y_max == y edges

    aet.remove_if(edge_ymax_le(line));

#ifdef DEBUG_RASTERIZER
    if ( li != aet.end() )
    {
      DOUT(( "AET @ %3d after remove:", line ));
      print_edge_list( aet );
    }
#endif
    if (1 == aet.size())
    {
      std::cerr << "INTERNAL ERROR: INVALID AET\n";
    }

    aet.sort();

#ifdef DEBUG_RASTERIZER
    if ( !validate_aet( aet ) )
    {
      DOUT(( "AET is invalid after sorting.\n" ));
    }
#endif
    // fill in scan line by going through AET

    bool parity = true;

    for (li = aet.begin(); li != aet.end(); ++li)
    {
      if (parity)
      {
        // scissor
        if (0 <= line && line < height)
        {
          std::list<Edge>::iterator lj = li;
          ++lj;

          //DOUT(("SPAN %3d: %d <-> %d\n",
          //      line, (int)ceilf(li->xx), (int)lj->xx));

          for (int xx = (int)ceilf(li->xx); xx <= lj->xx; ++xx)
          {
            // scissor
            if (0 <= xx && xx < width)
            {
              pixels[xx + line * width] = color;
            }
          }
        }
        parity = false;
      }
      else
      {
        parity = true;
      }

      // for each edge in AET update x for the new y.

      li->xx += li->kk;
    }
  }
} // scan_convert

/**
   \brief Copy the pixel data to an unsigned char array dynamically allocated.

   This function assumes little endian representation of integers,
   and the address of a block returned by malloc is always a multiple of eight
   (or sixteen on 64-bit systems).
 */
unsigned char* Rasterizer::getPixelsAsRGB() const
{
  auto* data = static_cast<unsigned char*>(malloc(width * height * 3));
  auto* t = reinterpret_cast<unsigned int*>(data);
  auto i = 0;
  for (; i < width * height; i += 4) {
    auto p0 = pixels[i + 0].pixel;
    auto p1 = pixels[i + 1].pixel;
    auto p2 = pixels[i + 2].pixel;
    auto p3 = pixels[i + 3].pixel;
    t[0] = ((p1 << 24) & 0xff000000) | ( p0        & 0xffffff);
    t[1] = ((p2 << 16) & 0xffff0000) | ((p1 >>  8) & 0x00ffff);
    t[2] = ((p3 <<  8) & 0xffffff00) | ((p2 >> 16) & 0x0000ff);
    t += 3;
  }
  if (i != width * height) {
    for (i -= 3; i < width * height; ++i) {
      auto p = pixels[i].pixel;
      auto* t = data + 3 * i;
      t[0] = (p & 0x0000ff);
      t[1] = (p & 0x00ff00) >> 0x08;
      t[2] = (p & 0xff0000) >> 0x10;
    }
  }
  return data;
}
