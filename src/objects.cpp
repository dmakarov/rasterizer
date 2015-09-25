/* objects.cpp
 * This file implements objects.h
 * Written by Yar Woo (ywoo@cs)
 */

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <cstring>
#include <string>

#include <list>
#include <algorithm>
#include <functional>

#include "canvas.h"
#include "objects.h"

//#define DEBUG_RASTERIZER 1

#ifdef  DEBUG_RASTERIZER
#define DOUT( X ) printf X
#else
#define DOUT(X)
#endif

//global objects array

extern char aafilter_function[];

int numObjects = 0;
AnimObject* objects[MAX_OBJECTS];

typedef enum {
  GRID = 0, RANDOM
} SHIFT_MODE_TYPE;

SHIFT_MODE_TYPE shift_mode = RANDOM;

typedef enum {
  BOX = 0, BARTLETT
} WEIGHT_FUNC_TYPE;

WEIGHT_FUNC_TYPE weight_mode = BOX;

/* Function: GetVertices
 * This function returns the set of vertices for the passed object in
 * the current frame */

RGB8 GetVertices(int id, float frameNumber, Point* holderFrame)
{
  int i;
  int lastFrameNumber = -1, nextFrameNumber = -1;
  int lastFrameID = -1, nextFrameID = -1;

  //here we do the interpolation!

  for (i=(int)frameNumber; i>=0; i--)	{
    if ((lastFrameID = FindKeyframe(id, i))!=-1)
    {
      lastFrameNumber = i;
      break;
    }
  }

  if (lastFrameID==-1)
    lastFrameID = 1; //there should always be a keyframe at frame 1

  for (i=((int)frameNumber+1); i<=MAX_FRAMES; i++)
  {
    if ((nextFrameID = FindKeyframe(id, i))!=-1)
    {
      nextFrameNumber = i;
      break;
    }
  }

  //if there are no more keyframes, just go with the last frame

  RGB8 color = objects[id]->r + (objects[id]->g << 8) + (objects[id]->b << 16);
  if (nextFrameID == -1)
  {
    memcpy(holderFrame,
           objects[id]->keyframes[lastFrameID].vertices,
           MAX_VERTICES*sizeof(Point));
    return color;

  }
  else
  {
    float percent = ((float)(frameNumber-lastFrameNumber))/((frameNumber-lastFrameNumber)+(nextFrameNumber-frameNumber));
    for (i=0; i<objects[id]->numVertices; i++)
    {
      holderFrame[i].x = ((1-percent)*(objects[id]->keyframes[lastFrameID].vertices[i].x) +
                          (percent*(objects[id]->keyframes[nextFrameID].vertices[i].x)));
      holderFrame[i].y = ((1-percent)*(objects[id]->keyframes[lastFrameID].vertices[i].y) +
                          (percent*(objects[id]->keyframes[nextFrameID].vertices[i].y)));
    }
  }
  return color;
}

int FindKeyframe(int id, int frameNumber)
{
  int i;
  for (i=0; i<objects[id]->numKeyframes; i++)
  {
    if (objects[id]->keyframes[i].frameNumber == frameNumber)
      return i;
  }
  return -1;
}

int AnyKeyframe(int frameNumber)
{
  int i;
  for (i=0; i<numObjects; i++)
  {
    if (FindKeyframe(i, frameNumber)!=-1) return 1;
  }
  return 0;
}

#ifdef DEBUG_RASTERIZER
void
print_edge_list( std::list< edge_type >& el )
{
  std::list< edge_type >::iterator li;
  for ( li = el.begin(); li != el.end(); ++li )
  {
    DOUT(( " -> < %f, %f, %f >", li->yy, li->xx, li->kk ));
  }
  DOUT(( "\n" ));
} // print_edge_list


void
print_et( std::list< edge_type >* et, int range, int bias )
{
  for ( int ii = 0; ii < range; ++ii )
  {
    if ( !et[ii].empty() )
    {
      DOUT(( "CELL %2d:", ii + bias ));
      print_edge_list( et[ii] );
    }
  }
} // print_et


bool
validate_aet( std::list< edge_type >& el )
{
  std::list< edge_type >::iterator li = el.begin();
  if ( li == el.end() )
  {
    return true;
  }
  float xprev = (li++)->xx;
  for ( ; li != el.end(); ++li )
  {
    float xcurr = li->xx;
    if ( xprev > xcurr )
    {
      return false;
    }
    xprev = xcurr;
  }
  return true;
} // validate_aet
#endif


void add_edge(std::list< edge_type >* et, Point* lo, Point* hi, int bias)
{
  float slope = (hi->x - lo->x) / (hi->y - lo->y);
  float ymin = ceilf(lo->y);
  float xmin = lo->x + slope * (ymin - lo->y);
  if ((slope < 0.0 && xmin < hi->x) || (slope > 0.0 && xmin > hi->x))
  {
    xmin = hi->x;
  }
  int bucket = ymin - bias;
  et[bucket].push_back(edge_type(hi->y, xmin, slope));

  DOUT(("EDGE (%6.2f, %6.2f)--(%6.2f, %6.2f) @ %d\n",
         lo->x, lo->y, hi->x, hi->y, bucket ));
} // add_edge


void scan_convert(Canvas* canvas, Point* vertex, int vertno, RGB8 color)
{
#ifdef DEBUG_RASTERIZER
  for ( int ii = 0; ii < vertno; ++ii )
  {
    DOUT(( "  VERTEX #%2d: <%g, %g>\n", ii, vertex[ii].x, vertex[ii].y ));
    //
    SET_RED(PIXEL( canvas, (int)vertex[ii].x,   (int)vertex[ii].y), 255);
    SET_RED(PIXEL( canvas, (int)vertex[ii].x+1, (int)vertex[ii].y), 255);
    SET_RED(PIXEL( canvas, (int)vertex[ii].x-1, (int)vertex[ii].y), 255);
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
  std::list< edge_type >* edge_table = new std::list< edge_type >[range];

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

  std::list< edge_type > aet;

  // while(not empty AET and ET)

  for (int jj = 0; jj < range; ++jj)
  {
    int line = jj + bias;

    // move from ET to AET y_min == y edges

    std::list< edge_type >::iterator li;
    for (li = edge_table[jj].begin(); li != edge_table[jj].end(); ++li)
    {
      aet.push_back(*li);
    }

#ifdef DEBUG_RASTERIZER
    if ( edge_table[jj].begin() != edge_table[jj].end() )
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
        if (0 <= line && line < canvas->Height)
        {
          std::list< edge_type >::iterator lj = li;
          ++lj;

          DOUT(("SPAN %3d: %d <-> %d\n", line, (int)ceilf(li->xx), (int)lj->xx));

          for (int xx = (int)ceilf(li->xx); xx <= lj->xx; ++xx)
          {
            // scissor
            if (0 <= xx && xx < canvas->Width)
            {
              PIXEL(canvas, xx, line) = color;
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

  delete [] edge_table;
} // scan_convert


// Bartlett filter implementation:

inline int bartlett(int sample, int total)
{
  if (BOX == weight_mode)
    return 0;

  if (sample == total / 2 && 0 == total % 2)
    return 0;
  return (sample < total / 2 + total % 2) ? 1 : - 1;
} // bartlett


float shift_function(int mode)
{
  switch (mode)
  {
  case RANDOM:
    return (float)rand() / (float)RAND_MAX - 0.5;
  case GRID:
    return 0;
  }
  return 0;
} // shift_function


// Build a table of coordinate shifts within a pixel boundaries.  Every value in
// data array is in the range [-0.5, 0.5].

void precompute_shifts(Point data[8][8], int dim)
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



void parse_aafilter_func(std::string& expr)
{
  //    if (!expr)
  //        return;
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


/* Function: Rasterize
 * -------------------
 *
 *
 */

void Rasterize(Canvas* renderCanvas, int frameNumber, bool antiAlias, int numAliasSamples, bool motionBlur, int numMotionSamples)
{
  // set up the accumulation buffer and a scratch pad canvas;

  Abuffer abuf(renderCanvas->Width, renderCanvas->Height);
  Canvas pad(renderCanvas->Width, renderCanvas->Height);

  // precomputed shift distances for vertices in AA.
  Point aajitter[8][8];

  abuf.pixel = new RGB32[renderCanvas->Width * renderCanvas->Height];
  abuf.init();
  pad.Pixels = new RGB8[renderCanvas->Width * renderCanvas->Height];

  if (0 != strlen(aafilter_function))
  {
    std::string expr(aafilter_function);
    parse_aafilter_func(expr);
  }

  int tiles = 1;
  float frame_shift = 0.0;
  float frame_offset = 0.0;

  if (! antiAlias)
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
    float froot = (numAliasSamples < MAX_ALIAS_SAMPLES) ?
      sqrt(numAliasSamples) : sqrt(MAX_ALIAS_SAMPLES);
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
  DOUT(("FRAME #%2d: %dx%d AA + %d MS\n", frameNumber, tiles, tiles, numMotionSamples));

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
        float frame = (float)frameNumber + frame_offset + frame_shift * mov;
        if (frame < 1.0)
        {
          mbfilt += bartlett(mov + 1, numMotionSamples);
          continue;
        }
        pad.init(0);
        for (int obj = 0; obj < numObjects; ++obj)
        {
          // make sure it hasn't gone beyond the last frame
          float max_frame = objects[obj]->keyframes[objects[obj]->numKeyframes - 1].frameNumber;
          float adj_frame = (frame > max_frame) ? max_frame : frame;
          // Here we grab the vertices for this object at this snapshot in time
          Point vertices[MAX_VERTICES];
          RGB8 color = GetVertices(obj, adj_frame, vertices);
          int vertno = objects[obj]->numVertices;

          // shift vertices
          for (int vv = 0; vv < vertno; ++vv)
          {
            vertices[vv].x += aajitter[ii][jj].x;
            vertices[vv].y += aajitter[ii][jj].y;
          }

          DOUT(("OBJECT #%2d [%d,%d] sample: %2d vertices\n", obj, ii, jj, vertno));

          scan_convert(&pad, vertices, vertno, color);
          ++scans;
        }
        // accumulate:
        for (int yy = 0; yy < pad.Height; ++yy)
        {
          for (int xx = 0; xx < pad.Width; ++xx)
          {
            abuf.add(xx, yy, pad.get(xx, yy), mbfilt * aafilt);
          }
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

  // clear the renderCanvas
  renderCanvas->init(0);
  // convert accumulation buffer to RGB8 and copy to the render canvas.
  for (int yy = 0; yy < abuf.Height; ++yy)
  {
    for (int xx = 0; xx < abuf.Width; ++xx)
    {
      PIXEL(renderCanvas, xx, yy) = abuf.get(xx, yy, samples);
    }
  }

  delete [] pad.Pixels;
  delete [] abuf.pixel;
} // Rasterize
