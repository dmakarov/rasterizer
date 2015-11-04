#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <list>
#include <random>
#include <string>

#ifdef __MACH__
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif

#include "canvas.hpp"

static SHIFT_MODE_TYPE shift_mode = RANDOM;
static WEIGHT_FUNC_TYPE weight_mode = BOX;
static std::uniform_real_distribution<float> urf(-0.5f, 0.5f);
static std::uniform_int_distribution<unsigned int> uiu(0, 255);
static std::default_random_engine e;

#ifdef DEBUG_RASTERIZER
void print_edge_list(std::list<edge_type>& el)
{
  for (auto li = el.begin(); li != el.end(); ++li)
  {
    DOUT(( " -> < %f, %f, %f >", li->yy, li->xx, li->kk ));
  }
  DOUT(( "\n" ));
} // print_edge_list

void print_et(std::list<edge_type>* et, int range, int bias)
{
  for (int ii = 0; ii < range; ++ii)
  {
    if (!et[ii].empty())
    {
      DOUT(( "CELL %2d:", ii + bias ));
      print_edge_list(et[ii]);
    }
  }
} // print_et

bool validate_aet(std::list<edge_type>& el)
{
  auto li = el.begin();
  if (li == el.end()) return true;
  auto xprev = (li++)->xx;
  for (; li != el.end(); ++li)
  {
    auto xcurr = li->xx;
    if (xprev > xcurr) return false;
    xprev = xcurr;
  }
  return true;
} // validate_aet
#endif

// Bartlett filter implementation:
inline int bartlett(int sample, int total)
{
  if (BOX == weight_mode) return 0;
  if (sample == total / 2 && 0 == total % 2) return 0;
  return (sample < total / 2 + total % 2) ? 1 : - 1;
} // bartlett

static float shift_function(int mode)
{
  switch (mode)
  {
  case RANDOM: return urf(e);
  case GRID:   return 0;
  }
  return 0;
} // shift_function

// Build a table of coordinate shifts within a pixel boundaries.
// Every value in data array is in the range [-0.5, 0.5].
static void precompute_shifts(Point data[8][8], int dim)
{
  if (1 == dim)
  {
    data[0][0].x = 0.0;
    data[0][0].y = 0.0;
    return;
  }

  if (dim > 8) dim = 8;
  float shift = 1.0 / (float)dim;
  float start = shift / 2.0 - 0.5;

  for (int ii = 0; ii < dim; ++ii)
  {
    for (int jj = 0; jj < dim; ++jj)
    {
      float incell = shift_function(shift_mode);
      data[ii][jj].x = start + ii * shift + incell * shift;
      data[ii][jj].y = start + jj * shift + incell * shift;
    }
  }
  if (1 == dim % 2)
  {
    data[dim/2][dim/2].x = 0.0;
    data[dim/2][dim/2].y = 0.0;
  }
} // precompute_shifts

static void parse_aafilter_func(std::string& expr)
{
  if (std::string::npos != expr.find("rand"))
  {
    shift_mode = RANDOM;
  }
  if (std::string::npos != expr.find("grid"))
  {
    shift_mode = GRID;
  }
  if (std::string::npos != expr.find("bart"))
  {
    weight_mode = BARTLETT;
  }
  if (std::string::npos != expr.find("box"))
  {
    weight_mode = BOX;
  }
}

static void add_edge(std::unique_ptr<std::list<Edge>[]>& et,
                     Point* lo, Point* hi, int bias)
{
  float slope = (hi->x - lo->x) / (hi->y - lo->y);
  float ymin = ceilf(lo->y);
  float xmin = lo->x + slope * (ymin - lo->y);
  if ((slope < 0.0f && xmin < hi->x) || (slope > 0.0f && xmin > hi->x))
  {
    xmin = hi->x;
  }
  int bucket = ymin - bias;
  et[bucket].emplace_back(Edge(hi->y, xmin, slope));

  DOUT(("EDGE (%6.2f, %6.2f)--(%6.2f, %6.2f) @ %d\n",
        lo->x, lo->y, hi->x, hi->y, bucket));
} // add_edge

// now assign the thing a random color (not too dark)
static void assign_random_color(AnimObject* obj)
{
  static int colorToPick = 0;
  colorToPick++;
  switch(colorToPick % 3)
  {
  case 0: obj->set_color(255, uiu(e), uiu(e)); break;
  case 1: obj->set_color(uiu(e), 255, uiu(e)); break;
  case 2: obj->set_color(uiu(e), uiu(e), 255); break;
  }
}

void Canvas::render()
{
  glClearColor(0.f, 0.f, 0.f, 0.f);
  glClear(GL_COLOR_BUFFER_BIT);
  glRasterPos2s(0, 0);
  glPixelZoom(1.0, -1.0);
  glDrawPixels(Width, Height, GL_RGBA, GL_UNSIGNED_BYTE, Pixels);
  glutSwapBuffers();
}

void Canvas::save(const char* filename) const
{
  FILE* output = fopen(filename, "wb");
  assert(output);

  /* Print header. */
  fprintf(output, "P6\n");
  fprintf(output, "# Comment Line\n");
  fprintf(output, "%d %d\n", Width, Height);
  fprintf(output, "255\n");

  /* Save image. */
  auto* buffer = Pixels;
  for (unsigned int i = 0; i < Width * Height; ++i, ++buffer)
  {
    char data = char(GET_RED(*buffer));
    fwrite(&data, 1, 1, output);
    data = char(GET_GREEN(*buffer));
    fwrite(&data, 1, 1, output);
    data = char(GET_BLUE(*buffer));
    fwrite(&data, 1, 1, output);
  }
  fclose(output);
}

/** Function: Rasterize
 * -------------------
 *
 *
 */
void Canvas::rasterize(int frame_num, bool do_anti_aliasing,
                       int numAliasSamples, bool motionBlur,
                       int numMotionSamples, const char* aafilter_function)
{
  // set up the accumulation buffer and a scratch pad canvas;

  Abuffer abuf(Width, Height);
  Canvas pad(Width, Height);

  // precomputed shift distances for vertices in AA.
  Point aajitter[8][8];

  if (0 != strlen(aafilter_function))
  {
    std::string expr(aafilter_function);
    parse_aafilter_func(expr);
  }

  int tiles = 1;
  float frame_shift = 0.0;
  float frame_offset = 0.0;

  if (!do_anti_aliasing)
  {
    numAliasSamples = 1;
  }
  if (! motionBlur)
  {
    numMotionSamples = 1;
  }
  if (numAliasSamples > 1)
  {
    // don't do more than MAX_ALIAS_SAMPLES:
    float froot = (numAliasSamples < MAX_ALIAS_SAMPLES)
                ? sqrt(numAliasSamples) : sqrt(MAX_ALIAS_SAMPLES);
    int iroot = froot;
    tiles = (froot - (float)iroot > 0.0) ? iroot + 1 : iroot;
  }
  precompute_shifts(aajitter, tiles);
  if (numMotionSamples > 1)
  {
    frame_offset = 1.0 / (2.0 * (float)numMotionSamples) - 0.5;
    frame_shift = 1.0 / (float)numMotionSamples;
  }
  // frame
  DOUT(("FRAME #%2d: %dx%d AA + %d MS\n",
        frame_num, tiles, tiles, numMotionSamples));

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
      for (int mov = 0; mov < numMotionSamples; ++mov)
      {
        float frame = (float)frame_num + frame_offset + frame_shift * mov;
        if (frame < 1.0)
        {
          mbfilt += bartlett(mov + 1, numMotionSamples);
          continue;
        }
        pad.init(0);
        for (int obj = 0; obj < objects.size(); ++obj)
        {
          // make sure it hasn't gone beyond the last frame
          float max_frame = objects[obj]->keyframes[objects[obj]->numKeyframes
                                                    - 1].frameNumber;
          float adj_frame = (frame > max_frame) ? max_frame : frame;
          // Here we grab the vertices for this object at this snapshot in time
          Point vertices[MAX_VERTICES];
          RGB8 color = get_vertices(obj, adj_frame, vertices);
          int vertno = objects[obj]->numVertices;

          // shift vertices
          for (int vv = 0; vv < vertno; ++vv)
          {
            vertices[vv].x += aajitter[ii][jj].x;
            vertices[vv].y += aajitter[ii][jj].y;
          }

          DOUT(("OBJECT #%2d [%d,%d] sample: %2d vertices\n",
                obj, ii, jj, vertno));

          pad.scan_convert(vertices, vertno, color);
          ++scans;
        }
        // accumulate:
        for (unsigned int x = 0; x < Height * Width; ++x)
        {
          abuf.add(x, pad.Pixels[x], mbfilt * aafilt);
        }
        // done with another sample:
        samples += mbfilt * aafilt;
        mbfilt += bartlett(mov + 1, numMotionSamples);
      }
      xxfilt += bartlett(ii + 1, tiles);
    }
    yyfilt += bartlett(jj + 1, tiles);
  }

  DOUT(("TOTAL SAMPLES: %d; SCANS: %d\n", samples, scans));

  // clear the canvas
  init(0);
  // convert accumulation buffer to RGB8 and copy to the render canvas.
  for (unsigned int x = 0; x < Height * Width; ++x)
  {
    Pixels[x] = abuf.get(x, samples);
  }
} // Rasterize

void Canvas::scan_convert(Point* vertex, int vertno, RGB8 color)
{
#ifdef DEBUG_RASTERIZER
  for ( int ii = 0; ii < vertno; ++ii )
  {
    DOUT(( "  VERTEX #%2d: <%g, %g>\n", ii, vertex[ii].x, vertex[ii].y ));
    //
    SET_RED(Pixels[vertex[ii].x-1 + (int)vertex[ii].y * Width], 255);
    SET_RED(Pixels[vertex[ii].x+0 + (int)vertex[ii].y * Width], 255);
    SET_RED(Pixels[vertex[ii].x+1 + (int)vertex[ii].y * Width], 255);
  }
#endif

  if (0 >= vertno)
  {
    DOUT(("NO VERTICES TO SCAN %d\n", vertno));
    return;
  }

  // find the range of y coordinates:
  float ymax = vertex[0].y;
  float ymin = ymax;

  for (int ii = 1; ii < vertno; ++ii)
  {
    if (ymax < vertex[ii].y) ymax = vertex[ii].y;
    if (ymin > vertex[ii].y) ymin = vertex[ii].y;
  }

  int bias = (int)ceilf(ymin);
  int range = (int)ceilf(ymax) - bias + 1;

  DOUT(("THE EDGE TABLE [%f,%f] size %d, bias %d\n", ymin, ymax, range, bias));

  // build the edge table
  std::unique_ptr<std::list<Edge>[]> edge_table(new std::list<Edge>[range]);

  for (int ii = 0; ii < vertno; ++ii)
  {
    // do not add horizontal edges to the edge table.
    int jj = (ii + 1) % vertno;
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
      printf("****************************** INVALID AET ******************************\n");
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
        if (0 <= line && line < Height)
        {
          std::list<Edge>::iterator lj = li;
          ++lj;

          DOUT(("SPAN %3d: %d <-> %d\n",
                line, (int)ceilf(li->xx), (int)lj->xx));

          for (int xx = (int)ceilf(li->xx); xx <= lj->xx; ++xx)
          {
            // scissor
            if (0 <= xx && xx < Width)
            {
              Pixels[xx + line * Width] = color;
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
RGB8 Canvas::get_vertices(int id, float frame, Point* vertices) const
{
  float prev_keyframe = -1.0f, next_keyframe = -1.0f;
  int prev_frame_id = -1, next_frame_id = -1;

  for (int i = (int)frame; i >= 0; --i)
    if ((prev_frame_id = find_keyframe(objects[id], i)) != -1)
    {
      prev_keyframe = i;
      break;
    }
  // there should always be a keyframe at frame 1
  if (prev_frame_id == -1)
    prev_frame_id = 1;

  for (int i = ((int)frame + 1); i <= MAX_FRAMES; ++i)
    if ((next_frame_id = find_keyframe(objects[id], i)) != -1)
    {
      next_keyframe = i;
      break;
    }
  auto* prev_keyframe_vertices = objects[id]->keyframes[prev_frame_id].vertices;
  auto* next_keyframe_vertices = objects[id]->keyframes[next_frame_id].vertices;
  if (next_frame_id == -1)
  { // if there are no more keyframes, just go with the last frame
    memcpy(vertices, prev_keyframe_vertices, MAX_VERTICES * sizeof(Point));
  }
  else // here we do the interpolation
  {
    float percent = (frame - prev_keyframe) / (next_keyframe - prev_keyframe);
    for (int i = 0; i < objects[id]->numVertices; ++i)
    {
      vertices[i].x = ((1 - percent) * (prev_keyframe_vertices[i].x)
                       + (percent * (next_keyframe_vertices[i].x)));
      vertices[i].y = ((1 - percent) * (prev_keyframe_vertices[i].y)
                       + (percent * (next_keyframe_vertices[i].y)));
    }
  }
  return objects[id]->get_color();
}

void Canvas::display(int frame, int selected_object) const
{
  glClearColor(0.f, 0.f, 0.f, 0.f);
  glClear(GL_COLOR_BUFFER_BIT);

  if (any_keyframe(frame))
  {
    //is this a keyframe? add a nifty red border if it is
    glLineWidth(20);
    glColor3d(1, 0, 0);
    glBegin(GL_LINE_STRIP);
    {
      glVertex2d(0, 0);
      glVertex2d(0, -Height);
      glVertex2d(Width, -Height);
      glVertex2d(Width, 0);
      glVertex2d(0, 0);
    }
    glEnd();
  }

  glLineWidth(1);

  //this is the loop where we draw all the friggin objects

  for (int i = 0; i < objects.size(); ++i)
  {
    Point vertices[MAX_VERTICES];
    get_vertices(i, frame, vertices);
    glColor3d(objects[i]->r/255.0, objects[i]->g/255.0, objects[i]->b/255.0);
    if (selected_object == i)
    { // there's a selected object! draw it different-like
      glLineWidth(3);
    }
    else
    {
      glLineWidth(1);
    }
    //we draw the edges
    glBegin(GL_LINE_STRIP);
    for (int j = 0; j < objects[i]->numVertices; ++j)
    {
      glVertex2d(vertices[j].x, -vertices[j].y);
    }
    glVertex2d(vertices[0].x, -vertices[0].y);
    glEnd();

    if (selected_object == i) //now draw the vertices on top of the lines
    {
      glBegin(GL_LINES);
      for (int j = 0; j < objects[i]->numVertices; ++j)
      {
        // selected vertex?
        if (j == selectedVertex)
        {
          glColor3d(0, 1, 0);
        }
        else
        {
          glColor3d(0, 1, 1);
        }
        glVertex2d(vertices[j].x+5, -vertices[j].y);
        glVertex2d(vertices[j].x-5, -vertices[j].y);
        glVertex2d(vertices[j].x, -vertices[j].y+5);
        glVertex2d(vertices[j].x, -vertices[j].y-5);
      }
      glEnd();
    }
  }

  if (rotation_centerX > -1)
  {
    glBegin(GL_LINES);
    glColor3d(0.5, 0.5, 0.5);
    glVertex2d(rotation_centerX + 5, - rotation_centerY);
    glVertex2d(rotation_centerX - 5, -rotation_centerY);
    glVertex2d(rotation_centerX, -rotation_centerY + 5);
    glVertex2d(rotation_centerX, -rotation_centerY - 5);
    glEnd();
  }

  // this is the loop where we draw the object currently being created
  if (currObject)
  {
    glLineWidth(3);
    //first we draw the edges
    glColor3d(currObject->r/255.0, currObject->g/255.0, currObject->b/255.0);
    glBegin(GL_LINE_STRIP);
    for (int i = 0; i < currObject->numVertices; ++i)
    {
      glVertex2d(currObject->keyframes[0].vertices[i].x,
                 -currObject->keyframes[0].vertices[i].y);
    }
    glEnd();

    //then we draw the vertices
    glColor3d(0, 1, 1);
    glBegin(GL_LINES);
    for (int j = 0; j < currObject->numVertices; ++j)
    {
      glVertex2d(currObject->keyframes[0].vertices[j].x+5,
                 -currObject->keyframes[0].vertices[j].y);
      glVertex2d(currObject->keyframes[0].vertices[j].x-5,
                 -currObject->keyframes[0].vertices[j].y);
      glVertex2d(currObject->keyframes[0].vertices[j].x,
                 -currObject->keyframes[0].vertices[j].y+5);
      glVertex2d(currObject->keyframes[0].vertices[j].x,
                 -currObject->keyframes[0].vertices[j].y-5);

    }
    glEnd();
    glLineWidth(1);
  }
  glutSwapBuffers();
}

bool Canvas::delete_object(int id)
{
  if (id == -1) return false;
  AnimObject* tmp = objects[id];
  objects.erase(objects.begin() + id);
  delete tmp;
  return true;
}

void Canvas::polygon_scaling(int mx, int my, int frame, int selected_object)
{
  if (selectedVertex == -1) return;

  mx -= rotation_centerX;
  my -= rotation_centerY;

  if (std::abs(prev_rotationX - mx) < 10 && std::abs(prev_rotationY - my) < 10)
    return;

  float dm = sqrt(mx*mx + my*my);
  float dp = sqrt(prev_rotationX*prev_rotationX + prev_rotationY*prev_rotationY);

  auto* vert = objects[selected_object]->keyframes[find_keyframe(
                                                   objects[selected_object],
                                                   frame)].vertices;
  int vertno = objects[selected_object]->numVertices;

  float sx = (dm > dp) ? 1.2 : 0.8;
  float sy = (dm > dp) ? 1.2 : 0.8;

  for (int ii = 0; ii < vertno; ++ii)
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

void Canvas::polygon_rotation(int mx, int my, int frame, int selected_object)
{
  if (selectedVertex == -1) return;

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

  auto* vert = objects[selected_object]->keyframes[find_keyframe(
                                                   objects[selected_object],
                                                   frame)].vertices;
  int vertno = objects[selected_object]->numVertices;

  for (int ii = 0; ii < vertno; ++ii)
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

bool Canvas::motion(int mx, int my, int frame, int selected_object)
{
  int frameID;

  if (draw_curve)
  {
    if (currObject->numVertices == MAX_VERTICES) return false;
    int px = currObject->keyframes[0].vertices[currObject->numVertices - 1].x;
    int py = currObject->keyframes[0].vertices[currObject->numVertices - 1].y;
    if (std::abs(px - mx) > 7 || std::abs(py - my) > 7)
    { // sqrt((px-mx)*(px-mx) + (py-my)*(py-my)) > 5)
      currObject->keyframes[0].vertices[currObject->numVertices].x = mx;
      currObject->keyframes[0].vertices[currObject->numVertices].y = my;
      currObject->numVertices++;
    }
    return true;
  }

  if (selected_object == -1) return false;

  if ((frameID = find_keyframe(objects[selected_object], frame)) == -1)
  { // look for a keyframe at this frame
    if (objects[selected_object]->numKeyframes == MAX_KEYFRAMES) return false;
    //if we don't find it, then create a new one in the right place
    int insertLoc;
    Point vertices[MAX_VERTICES];
    get_vertices(selected_object, frame, vertices);

    for (insertLoc = 0;
         insertLoc < objects[selected_object]->numKeyframes; ++insertLoc)
      if (frame < objects[selected_object]->keyframes[insertLoc].frameNumber)
        break;

    //shift everything over
    for (int i = objects[selected_object]->numKeyframes - 1;
         i >= insertLoc; --i)
      memcpy(&(objects[selected_object]->keyframes[i+1]),
             &(objects[selected_object]->keyframes[i]), sizeof(Frame));

    memcpy(objects[selected_object]->keyframes[insertLoc].vertices, vertices,
           MAX_VERTICES*sizeof(Point));

    objects[selected_object]->keyframes[insertLoc].frameNumber = frame;
    objects[selected_object]->numKeyframes++;
    frameID = find_keyframe(objects[selected_object], frame);
    assert(frameID!=-1);
  }

  if (scale_polygon)
  {
    polygon_scaling(mx, my, frame, selected_object);
    return true;
  }
  if (rotate_polygon)
  {
    polygon_rotation(mx, my, frame, selected_object);
    return true;
  }

  rotation_centerX = -1;

  if (selectedVertex != -1)
  {
    objects[selected_object]->keyframes[frameID].vertices[selectedVertex].x = mx;
    objects[selected_object]->keyframes[frameID].vertices[selectedVertex].y = my;
  }
  else
  {
    for (int i = 0; i < objects[selected_object]->numVertices; ++i)
    {
      objects[selected_object]->keyframes[frameID].vertices[i].x += (mx - originalX);
      objects[selected_object]->keyframes[frameID].vertices[i].y += (my - originalY);
    }
    originalX = mx;
    originalY = my;
  }
  return true;
}

bool Canvas::select_object(int button, int mx, int my, int frame,
                           int& selected_object)
{
  bool foundVertex = false;
  for (int i = 0; i < objects.size(); ++i)
  {
    Point vertices[MAX_VERTICES];
    get_vertices(i, frame, vertices);
    for (int j = 0; j < objects[i]->numVertices; ++j)
    {
      Point vert = vertices[j];
      if (fabs(vert.x - mx) < 5 && fabs(vert.y - (my)) < 5)
      { // check for proximity
        foundVertex = true;
        selected_object = i;
        selectedVertex = j;

        //implement right-click drag
        if (button == GLUT_RIGHT_BUTTON)
        {
          selectedVertex = -1;
          originalX = mx;
          originalY = my;
        }
      }
    }
  }
  return foundVertex;
}

bool Canvas::mouse(int button, int state, int mx, int my, int frame,
                   int& selected_object)
{
  int modifier = glutGetModifiers();

  if (state == GLUT_UP)
  {
    rotate_polygon = false;
    scale_polygon = false;
    draw_curve = false;
    return true;
  }

  switch (modifier)
  {
  case GLUT_ACTIVE_SHIFT:
    {
      //clear the selection

      selected_object = -1;
      selectedVertex = -1;

      //create a new object if one isn't being drawn

      if (currObject == nullptr)
      {
        currObject = new AnimObject;
        currObject->numVertices = 0;
        currObject->numKeyframes = 1;
        currObject->keyframes[0].frameNumber = 1;
        assign_random_color(currObject);
      }
      if (currObject->numVertices == MAX_VERTICES) return false;
      currObject->keyframes[0].vertices[currObject->numVertices].x = mx;
      currObject->keyframes[0].vertices[currObject->numVertices].y = my;
      currObject->numVertices++;
      draw_curve = true;
    }
    rotation_centerX = -1;
    break;
  case GLUT_ACTIVE_CTRL:
    {
      if (select_object(button, mx, my, frame, selected_object)
          && (rotation_centerX != -1))
      {
        scale_polygon = false;
        rotate_polygon = true;
        prev_rotationX = mx - rotation_centerX;
        prev_rotationY = my - rotation_centerY;
      }
      else
      {
        rotation_centerX = mx;
        rotation_centerY = my;
      }
    }
    break;
  case GLUT_ACTIVE_CTRL | GLUT_ACTIVE_SHIFT:
    {
      if (select_object(button, mx, my, frame, selected_object)
          && (rotation_centerX != -1))
      {
        scale_polygon = true;
        rotate_polygon = false;
        prev_rotationX = mx - rotation_centerX;
        prev_rotationY = my - rotation_centerY;
      }
    }
    break;
  default:
    {
      // if we're in the middle of drawing something, then end it
      if (currObject)
      {
        //if we don't have a polygon

        if (currObject->numVertices < 3)
        {
          delete currObject;
        }
        else
        {
          objects.push_back(currObject);
        }
        currObject = nullptr;
      }
      // if there's a vertex in the area, select it
      if (!select_object(button, mx, my, frame, selected_object))
      {
        selected_object = -1;
        selectedVertex = -1;
      }
    }
    rotation_centerX = -1;
  }
  return true;
}

void Canvas::delete_keyframe(int id, int frame)
{
  if (frame == 1 || !any_keyframe(frame))
    return;

  int frameID = -1;
  for (auto* obj : objects)
    if ((frameID = find_keyframe(obj, frame)) != -1)
    {
      for (int j = frameID; j < obj->numKeyframes - 1; ++j)
        memcpy(&(obj->keyframes[j]), &(obj->keyframes[j + 1]), sizeof(Frame));
      obj->numKeyframes--;
    }
}

void Canvas::load_objects(const char* filename)
{
  char buf[1024];
  // check if there's something in the filename field
  FILE* infile = fopen(filename, "r");
  if (infile == NULL) return;
  currObject = nullptr;
  // free all of the associated memory and initialize
  for (int i = 0; i < objects.size(); ++i)
    delete objects[i];
  objects.clear();

  int num_of_objects;
  fscanf(infile, "Number of Objects: %d\n", &num_of_objects);

  for (int i = 0; i < num_of_objects; ++i)
  {
    auto* obj = new AnimObject;
    fscanf(infile, "%[^\n]\n", buf);
    fscanf(infile, "Color: r: %d, g: %d, b: %d\n", &obj->r, &obj->g, &obj->b);
    fscanf(infile, "Number of Vertices: %d\n", &obj->numVertices);
    fscanf(infile, "Number of Keyframes: %d\n", &obj->numKeyframes);
    for (int j = 0; j < obj->numKeyframes; ++j)
    {
      fscanf(infile, "Keyframe for Frame %d\n", &obj->keyframes[j].frameNumber);
      for (int k = 0; k < obj->numVertices; ++k)
      {
        int dummy;
        fscanf(infile, "Vertex %d, x: %g, y: %g\n",
               &dummy, &obj->keyframes[j].vertices[k].x,
               &obj->keyframes[j].vertices[k].y);
      }
    }
    objects.emplace_back(obj);
  }
  fclose(infile);
}

void Canvas::save_objects(const char* filename)
{
  // check if there's something in the filename field
  FILE* outfile = fopen(filename, "w");
  if (outfile == NULL) return;
  // write out the number of objects
  fprintf(outfile, "Number of Objects: %lu\n", objects.size());
  // write out each object
  for (int i = 0; i < objects.size(); ++i)
  {
    fprintf(outfile, "Object Number: %d\n", i);
    fprintf(outfile, "Color: r: %d, g: %d, b: %d\n",
            objects[i]->r, objects[i]->g, objects[i]->b);
    fprintf(outfile, "Number of Vertices: %d\n", objects[i]->numVertices);
    fprintf(outfile, "Number of Keyframes: %d\n", objects[i]->numKeyframes);
    for (int j = 0; j < objects[i]->numKeyframes; ++j)
    {
      fprintf(outfile, "Keyframe for Frame %d\n", objects[i]->keyframes[j].frameNumber);
      for (int k = 0; k < objects[i]->numVertices; ++k)
      {
        fprintf(outfile, "Vertex %d, x: %g, y: %g\n",
                k, objects[i]->keyframes[j].vertices[k].x,
                objects[i]->keyframes[j].vertices[k].y);
      }
    }
  }
  fclose(outfile);
}
