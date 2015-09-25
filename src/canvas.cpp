#include <assert.h>
#include <stdio.h>

#include "canvas.h"

void save_canvas(const char* filename, Canvas* C)
{
  FILE* output = fopen(filename, "wb");
  assert(output != NULL);

  /* Print header. */

  fprintf(output, "P6\n");
  fprintf(output, "# Comment Line\n");
  fprintf(output, "%d %d\n", C->Width, C->Height);
  fprintf(output, "255\n");

  /* Save image. */

  auto size = C->Width * C->Height;
  unsigned int* buffer = C->Pixels;
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
