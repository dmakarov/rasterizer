#ifndef _CANVAS_H
#define _CANVAS_H

/* MACROS. */

/* The following macros retrieve the primary color components from one
   canvas pixel P. */

#define GET_RED(P)   ((P)&0xFF)
#define GET_GREEN(P) (((P)>>8)&0xFF)
#define GET_BLUE(P)  (((P)>>16)&0xFF)

/* The following macros set the primary color components of the canvas
   pixel P to C. C must lie between 0 and 255 (both endpoints
   inclusive). */

#define SET_RED(P,C)   (P=(((P)&0xFFFFFF00)|(C)))
#define SET_GREEN(P,C) (P=(((P)&0xFFFF00FF)|((C)<<8)))
#define SET_BLUE(P,C)  (P=(((P)&0xFF00FFFF)|((C)<<16)))

/* The following macro retrieves the pixel at coordinates (X,Y) of the
   canvas C (where C is a pointer to a canvas structure). (0,0) are the
   coordinates of the top left corner of the canvas, while the bottom
   right corner is located at (C->Width-1,C->Height-1). */

#define PIXEL(C,X,Y) ((C)->Pixels[(Y)*(C)->Width+(X)])

typedef unsigned int RGB8;

typedef struct canvas_struct {
  int Width;
  int Height;
  RGB8 *Pixels;
  canvas_struct( int ww = 0, int hh = 0 )
    : Width( ww ), Height( hh ), Pixels(0)
  {}
  void init( RGB8 color )
  {
    for ( int jj = 0; jj < Height; ++jj )
      for ( int ii = 0; ii < Width; ++ii )
        Pixels[Width * jj + ii] = color;
  }
  RGB8 get( int xx, int yy )
  {
    return ( Pixels[Width * yy + xx] );
  }
} Canvas;

// This is a structure to hold canvas pixels with 32 bit per RGB component.

typedef struct {
  unsigned int rr, gg, bb;
} RGB32;

typedef struct abuffer_struct {
  int Width, Height;
  RGB32* pixel;
  abuffer_struct( int ww = 0, int hh = 0 )
    : Width( ww ), Height( hh ), pixel(0)
  {}
  void init()
  {
    if (!pixel) return;
    for ( int jj = 0; jj < Height; ++jj )
      for ( int ii = 0; ii < Width; ++ii )
        pixel[Width * jj + ii].rr =
          pixel[Width * jj + ii].gg =
          pixel[Width * jj + ii].bb = 0;
  }
  void add( int xx, int yy, RGB8 color, int weight = 1 )
  {
    pixel[Width * yy + xx].rr += ( 0xFF & ( color       ) ) * weight;
    pixel[Width * yy + xx].gg += ( 0xFF & ( color >>  8 ) ) * weight;
    pixel[Width * yy + xx].bb += ( 0xFF & ( color >> 16 ) ) * weight;
  }
  RGB8 get( int xx, int yy, int kk )
  {
    RGB8 rr = pixel[Width * yy + xx].rr / kk;
    RGB8 gg = pixel[Width * yy + xx].gg / kk;
    RGB8 bb = pixel[Width * yy + xx].bb / kk;
    return ( rr + ( gg << 8 ) + ( bb << 16 ) );
  }
} Abuffer;

void SaveCanvas(char* Filename, Canvas* C);

#endif

// Local Variables:
// mode: c++
// End:
