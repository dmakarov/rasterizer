#include "canvas.h"
#include <stdio.h>
#include "assert.h"

void SaveCanvas(char* Filename, Canvas* C)
{
  /* Open file. */

  FILE *Output=fopen(Filename,"wb");
  assert(Output!=NULL);

  /* Print header. */

  fprintf(Output,"P6\n");
  fprintf(Output,"# Comment Line\n");
  fprintf(Output,"%d %d\n",C->Width,C->Height);
  fprintf(Output,"255\n");

  /* Save image. */

  int Size=C->Width*C->Height;
  unsigned int *Buffer=C->Pixels;
  for (int i=0;i<Size;i++,Buffer++)
  {
    char Data=char(GET_RED(*Buffer));
    fwrite(&Data,1,1,Output);
    Data=char(GET_GREEN(*Buffer));
    fwrite(&Data,1,1,Output);
    Data=char(GET_BLUE(*Buffer));
    fwrite(&Data,1,1,Output);
  }
  fclose(Output);
}
