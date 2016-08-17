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

std::ostream& operator<<(std::ostream& os, const RGB8& obj)
{
  os << std::hex << obj.pixel;
  return os;
}

std::ostream& operator<<(std::ostream& os, const RGB32& obj)
{
  os << '(' << obj.r << ',' << obj.g << ',' << obj.b << ')';
  return os;
}

std::ostream& operator<<(std::ostream& os, const Animation& obj)
{
  os << "vertices " << obj.get_num_vertices()
     << ", frames " << obj.keyframes.size()
     << ", color " << obj.get_color();
  return os;
}

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

void Rasterizer::save_image(const std::string& filename) const
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
void Rasterizer::rasterize(int frame_num,
                           bool aa_enabled, int num_aa_samples,
                           bool mb_enabled, int num_mb_samples,
                           const std::string& aa_filter) const
{
  // set up the accumulation buffer and a scratch pad canvas;

  Abuffer abuf(width, height);
  Rasterizer pad(width, height);

  // precomputed shift distances for vertices in AA.
  Point aajitter[8][8];

  if (!aa_filter.empty())
  {
    parse_aafilter_func(aa_filter);
  }

  int tiles = 1;
  float frame_shift = 0.0;
  float frame_offset = 0.0;

  if (!aa_enabled)
  {
    num_aa_samples = 1;
  }
  if (! mb_enabled)
  {
    num_mb_samples = 1;
  }
  if (num_aa_samples > 1)
  {
    // don't do more than MAX_ALIAS_SAMPLES:
    float froot = (num_aa_samples < MAX_ALIAS_SAMPLES)
                ? sqrt(num_aa_samples) : sqrt(MAX_ALIAS_SAMPLES);
    int iroot = froot;
    tiles = (froot - (float)iroot > 0.0) ? iroot + 1 : iroot;
  }
  precompute_shifts(aajitter, tiles);
  if (num_mb_samples > 1)
  {
    frame_offset = 1.0 / (2.0 * (float)num_mb_samples) - 0.5;
    frame_shift = 1.0 / (float)num_mb_samples;
  }
  // frame
  //printf("FRAME #%2d: %dx%d AA + %d MS\n", frame_num, tiles, tiles, num_mb_samples);

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
        for (std::vector<std::shared_ptr<Animation>>::size_type obj = 0;
             obj < objects.size(); ++obj)
        {
          // make sure it hasn't gone beyond the last frame
          float max_frame = (objects[obj]->keyframes.end() - 1)->number;
          float adj_frame = (frame > max_frame) ? max_frame : frame;
          // Here we grab the vertices for this object at this snapshot in time
          std::vector<Point> vertices;
          RGB8 color = get_vertices(obj, adj_frame, vertices);
          auto vertno = objects[obj]->get_num_vertices();

          // shift vertices
          for (decltype(vertno) vv = 0; vv < vertno; ++vv)
          {
            vertices[vv].x += aajitter[ii][jj].x;
            vertices[vv].y += aajitter[ii][jj].y;
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
 *  This function returns the set of vertices for the passed object in
 *  the current frame
 */
RGB8 Rasterizer::get_vertices(objects_size_type id, float frame,
                              std::vector<Point>& vertices) const
{
  float prev_keyframe = -1.0f, next_keyframe = -1.0f;
  auto prev_frame = objects[id]->keyframes.end();
  auto next_frame = objects[id]->keyframes.end();

  for (int i = (int)frame; i >= 0; --i)
    if ((prev_frame = find_keyframe(objects[id], i)) != objects[id]->keyframes.end())
    {
      prev_keyframe = i;
      break;
    }
  // there should always be a keyframe at frame 1
  if (prev_frame == objects[id]->keyframes.end())
    prev_frame = objects[id]->keyframes.begin() + 1;

  for (int i = ((int)frame + 1); i <= (objects[id]->keyframes.end() - 1)->number; ++i)
    if ((next_frame = find_keyframe(objects[id], i)) != objects[id]->keyframes.end())
    {
      next_keyframe = i;
      break;
    }
  auto& prev_keyframe_vertices = prev_frame->vertices;
  if (next_frame == objects[id]->keyframes.end())
  { // if there are no more keyframes, just go with the last frame
    vertices.resize(prev_keyframe_vertices.size());
    std::copy(prev_keyframe_vertices.begin(), prev_keyframe_vertices.end(),
              vertices.begin());
  }
  else // here we do the interpolation
  {
    auto& next_keyframe_vertices = next_frame->vertices;
    float percent = (frame - prev_keyframe) / (next_keyframe - prev_keyframe);
    for (vertex_id_type i = 0; i < objects[id]->get_num_vertices(); ++i)
    {
      auto x = (1 - percent) * prev_keyframe_vertices[i].x
                  + percent  * next_keyframe_vertices[i].x;
      auto y = (1 - percent) * prev_keyframe_vertices[i].y
                  + percent  * next_keyframe_vertices[i].y;
      vertices.emplace_back(Point{x, y});
    }
  }
  return objects[id]->get_color();
}

void Rasterizer::delete_selected_object()
{
  if (!is_object_selected) {
    return;
  }
  objects.erase(objects.begin() + selected_object);
  is_object_selected = false;
  notify();
}

void Rasterizer::scale(int mx, int my, int frame)
{
  if (!is_object_selected) return;

  mx -= rotation_centerX;
  my -= rotation_centerY;

  if (std::abs(prev_rotationX - mx) < 10 && std::abs(prev_rotationY - my) < 10)
    return;

  float dm = sqrt(mx*mx + my*my);
  float dp = sqrt(prev_rotationX*prev_rotationX + prev_rotationY*prev_rotationY);

  auto& vert = find_keyframe(objects[selected_object], frame)->vertices;
  auto vertno = objects[selected_object]->get_num_vertices();

  float sx = (dm > dp) ? 1.2 : 0.8;
  float sy = (dm > dp) ? 1.2 : 0.8;

  for (decltype(vertno) ii = 0; ii < vertno; ++ii)
  {
    vert[ii].x -= rotation_centerX;
    vert[ii].y -= rotation_centerY;
    vert[ii].x = sx * vert[ii].x;
    vert[ii].y = sy * vert[ii].y;
    vert[ii].x += rotation_centerX;
    vert[ii].y += rotation_centerY;
  }
  prev_rotationX = mx;
  prev_rotationY = my;
}

void Rasterizer::rotate(int mx, int my, int frame)
{
  if (!is_object_selected) return;

  mx -= rotation_centerX;
  my -= rotation_centerY;

  if (std::abs(prev_rotationX - mx) < 10 && std::abs(prev_rotationY - my) < 10)
    return;
  if ((mx < 0 && prev_rotationX > 0) || (mx > 0 && prev_rotationX < 0))
  {
    prev_rotationX = mx;
    prev_rotationY = my;
    return;
  }

  float ro = sqrtf(mx*mx + my*my);
  float al = asinf(my / ro);
  ro = sqrtf(prev_rotationX*prev_rotationX + prev_rotationY*prev_rotationY);
  float be = asinf(prev_rotationY / ro);
  float sign = ((al > be && mx > 0) || (al < be && mx < 0)) ? 1.0 : -1.0;
  float te = sign * 3.14159 / 18;
  float sn = sinf(te);
  float cs = cosf(te);

  auto& vert = find_keyframe(objects[selected_object], frame)->vertices;
  auto vertno = objects[selected_object]->get_num_vertices();

  for (decltype(vertno) ii = 0; ii < vertno; ++ii)
  {
    vert[ii].x -= rotation_centerX;
    vert[ii].y -= rotation_centerY;
    float xx = cs * vert[ii].x - sn * vert[ii].y;
    float yy = sn * vert[ii].x + cs * vert[ii].y;
    vert[ii].x = xx + rotation_centerX;
    vert[ii].y = yy + rotation_centerY;
  }
  prev_rotationX = mx;
  prev_rotationY = my;
}

bool Rasterizer::move(float mx, float my, int frame)
{
  if (draw_curve)
  {
    auto px = active_object->keyframes[0].vertices.back().x;
    auto py = active_object->keyframes[0].vertices.back().y;
    if (std::abs(px - mx) > 7 || std::abs(py - my) > 7)
    { // sqrt((px-mx)*(px-mx) + (py-my)*(py-my)) > 5)
      active_object->keyframes[0].vertices.push_back(Point{mx, my});
    }
    return true;
  }

  if (!is_object_selected) {
    return false;
  }
  // look for a keyframe at this frame
  // if we don't find it, then create a new one in the right place
  auto keyframe = find_keyframe(objects[selected_object], frame);
  if (keyframe == objects[selected_object]->keyframes.end()) {
    std::vector<Point> vertices;
    get_vertices(selected_object, frame, vertices);

    decltype(get_num_keyframes(selected_object)) insertLoc;
    for (insertLoc = 0; insertLoc < get_num_keyframes(selected_object); ++insertLoc)
      if (frame < objects[selected_object]->keyframes[insertLoc].number)
        break;

    objects[selected_object]->keyframes.push_back(objects[selected_object]->keyframes.back());
    for (auto i = get_num_keyframes(selected_object) - 2; i >= insertLoc; --i)
      std::copy(objects[selected_object]->keyframes.begin() + i,
                objects[selected_object]->keyframes.begin() + i + 1,
                objects[selected_object]->keyframes.begin() + i + 1);

    std::copy(vertices.begin(), vertices.end(),
              objects[selected_object]->keyframes[insertLoc].vertices.begin());

    objects[selected_object]->keyframes[insertLoc].number = frame;
    keyframe = find_keyframe(objects[selected_object], frame);
    assert(keyframe != objects[selected_object]->keyframes.end());
  }

  if (scale_polygon) {
    scale(mx, my, frame);
    return true;
  }
  if (rotate_polygon) {
    rotate(mx, my, frame);
    return true;
  }

  rotation_centerX = -1;

  if (!is_object_selected) {
    keyframe->vertices[selected_vertex].x = mx;
    keyframe->vertices[selected_vertex].y = my;
  }
  else {
    for (auto& v : keyframe->vertices) {
      v.x += (mx - original.x);
      v.y += (my - original.y);
    }
    original = Point{mx, my};
  }
  return true;
}

bool Rasterizer::select_object(float mx, float my, int frame,
                               bool is_right_click)
{
  bool foundVertex = false;
  for (objects_size_type i = 0; !foundVertex && i < objects.size(); ++i) {
    std::vector<Point> vertices;
    get_vertices(i, frame, vertices);
    auto j = 0;
    for (auto& v : vertices) {
      // check for proximity
      if (fabs(v.x - mx) < 5 && fabs(v.y - my) < 5) {
        foundVertex = true;
        selected_object = i;
        selected_vertex = j;
        //implement right-click drag
        if (is_right_click) {
          selected_vertex = vertices.size();
          original = Point{mx, my};
        }
        break;
      }
      ++j;
    }
  }
  is_object_selected = foundVertex;
  notify();
  return foundVertex;
}

void Rasterizer::delete_keyframe(int frame)
{
  if (frame == 1 || !any_keyframe(frame)) {
    return;
  }
  for (auto o : objects) {
    auto k = find_keyframe(o, frame);
    if (k != o->keyframes.end()) {
      for (auto j = k, E = o->keyframes.end() - 1; j != E; ++j) {
        std::copy(j + 1, j + 2, j);
      }
      o->keyframes.pop_back();
    }
  }
}

bool Rasterizer::load_objects(const std::string& filename)
{
  // check if there's something in the filename field
  std::ifstream infile(filename);
  if (!infile)
  {
    std::cerr << "Can't load file " << filename << "\n";
    return false;
  }
  active_object = nullptr;
  objects.clear();

  int num_of_objects;
  std::string line;
  std::getline(infile, line);
  std::istringstream iss(line.substr(line.find_last_of(":") + 1));
  iss >> num_of_objects;

  for (int i = 0; i < num_of_objects; ++i)
  {
    int num_vertices;
    unsigned int r, g, b;
    auto obj = std::make_shared<Animation>();
    // "%[^\n]\n"
    std::getline(infile, line);
    // "Color: r: %d, g: %d, b: %d\n"
    std::getline(infile, line);
    line = line.substr(line.find_first_of(":") + 1);
    line = line.substr(line.find_first_of(":") + 1);
    iss.clear();
    iss.str(line);
    iss >> r;
    line = line.substr(line.find_first_of(":") + 1);
    iss.clear();
    iss.str(line);
    iss >> g;
    line = line.substr(line.find_first_of(":") + 1);
    iss.clear();
    iss.str(line);
    iss >> b;
    obj->set_color(r, g, b);
    // "Number of Vertices: %d\n"
    std::getline(infile, line);
    iss.clear();
    iss.str(line.substr(line.find_first_of(":") + 1));
    iss >> num_vertices;
    // "Number of Keyframes: %d\n"
    std::getline(infile, line);
    iss.clear();
    iss.str(line.substr(line.find_first_of(":") + 1));
    int num_keyframes;
    iss >> num_keyframes;
    obj->keyframes.resize(num_keyframes);
    for (int j = 0; j < num_keyframes; ++j)
    {
      // "Keyframe for Frame %d\n"
      std::getline(infile, line);
      iss.clear();
      iss.str(line.substr(line.find_last_of(" ") + 1));
      iss >> obj->keyframes[j].number;
      for (int k = 0; k < num_vertices; ++k)
      {
        // "Vertex %d, x: %g, y: %g\n"
        std::getline(infile, line);
        int t;
        line = line.substr(line.find_first_of(":") + 1);
        iss.clear();
        iss.str(line);
        iss >> t;
        float x = t;
        line = line.substr(line.find_first_of(":") + 1);
        iss.clear();
        iss.str(line);
        iss >> t;
        float y = t;
        obj->keyframes[j].vertices.emplace_back(Point{x, y});
      }
    }
    objects.emplace_back(obj);
  }
  return true;
}

void Rasterizer::save_objects(const std::string& filename) const
{
  // check if there's something in the filename field
  std::ofstream ofs(filename);
  if (!ofs) return;
  // write out the number of objects
  ofs << "Number of Objects: " << objects.size() << "\n";
  // write out each object
  auto i = 0;
  for (auto& o : objects) {
    ofs << "Object Number: " << i++ << "\n";
    ofs << "Color: r: " << o->r << ", g: " << o->g << ", b: " << o->b << "\n";
    ofs << "Number of Vertices: " << o->get_num_vertices() << "\n";
    ofs << "Number of Keyframes: " << get_num_keyframes(i) << "\n";
    for (auto& f : o->keyframes) {
      ofs << "Keyframe for Frame " << f.number << "\n";
      auto k = 0;
      for (auto& v : f.vertices) {
        ofs << "Vertex " << k++ << ", x: " << v.x << ", y: " << v.y << "\n";
      }
    }
  }
}

void Rasterizer::render_to_file(const std::vector<std::string>& args)
{
  std::string infile;
  std::string outfile;
  std::string basename{"basename"};
  unsigned int first_frame;
  unsigned int final_frame;
  unsigned int num_aa_samples = 0;
  unsigned int num_mb_samples = 0;
  bool aa_enabled = false;
  bool mb_enabled = false;
  std::istringstream iss;

  if (args.size() < 4 || args[0] == "-help")
  {
    std::cout << "Usage: rasterizer [-a<#samples>] [-m<#samples>]"
              << " <first frame> <last frame> <infile> <outfile>\n";
    return;
  }

  auto arg = args.crbegin();
  outfile = *arg++;
  infile = *arg++;
  iss.str(*arg++);
  iss >> final_frame;
  iss.clear();
  iss.str(*arg++);
  iss >> first_frame;
  if (first_frame == 0 || final_frame == 0)
  {
    std::cerr << "Incorrect arguments. Type 'rasterizer -help' for more info\n";
    return;
  }
  /*
  if (oss.str().substr(0, 2) == "-a")
  {
    aa_enabled = true;
    std::istringstream iss(oss.str().substr(2));
    iss >> num_aa_samples;
    if (num_aa_samples == 0)
    {
      std::cerr << "Incorrect arguments. Type 'rasterizer -help' for more info\n";
    }
    ++i;
  }

  if (strncmp(argv[i], "-m", 2) == 0)
  {
    mb_enabled = true;
    if (sscanf(argv[i], "-m%d", &num_mb_samples) == 0)
    {
      std::cerr << "Incorrect arguments. Type 'rasterizer -help' for more info\n";
    }
    ++i;
  }

  //there should be four more arguments after the optional switches
  if (i != argc - 4)
  {
    std::cerr << "Incorrect number of arguments."
              << " Type 'rasterizer -help' for more info\n";
    return;
  }
  */
  load_objects(infile);
  //ParseFilename(outputFile, pathless);
  std::ofstream listfile(outfile + ".list");
  assert(listfile);

  for (auto frame = first_frame; frame <= final_frame; ++frame)
  {
    rasterize(frame, aa_enabled, num_aa_samples, mb_enabled, num_mb_samples, "");
    std::ostringstream oss;
    oss << outfile << "." << frame << ".ppm";
    save_image(oss.str());
    listfile << basename << "." << frame << ".ppm" << "\n";
  }
}

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
