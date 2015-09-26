#include <assert.h>
#include <cstdio>

#ifdef __MACH__
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif

#include "canvas.hpp"

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
  assert(output != NULL);

  /* Print header. */

  fprintf(output, "P6\n");
  fprintf(output, "# Comment Line\n");
  fprintf(output, "%d %d\n", Width, Height);
  fprintf(output, "255\n");

  /* Save image. */

  auto size = Width * Height;
  unsigned int* buffer = Pixels;
  for (int i = 0; i < size; i++, buffer++)
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
