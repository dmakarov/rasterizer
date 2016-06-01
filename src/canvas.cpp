#include "canvas.h"

wxBEGIN_EVENT_TABLE(Canvas, wxWindow)
  EVT_PAINT(Canvas::OnPaint)
wxEND_EVENT_TABLE()

#if 0
void Canvas::render() const
{
  glClearColor(0.f, 0.f, 0.f, 0.f);
  glClear(GL_COLOR_BUFFER_BIT);
  glRasterPos2s(0, 0);
  glPixelZoom(1.0, -1.0);
  glDrawPixels(Width, Height, GL_RGBA, GL_UNSIGNED_BYTE, Pixels);
  glutSwapBuffers();
}

void DisplayAndSaveCanvas(int)
{
  char buf[1024], pathless[1024];
  if (multiple_frames)
  {
    pathless = get_basename(render_filename);
    sprintf(buf, "%s.list", render_filename);
    FILE* listFile = fopen(buf, "w");
    assert(listFile != NULL);

    for (int i = first_frame; i <= final_frame; ++i)
    {
      canvas->rasterize(i, anti_aliasing_enabled,
                        num_alias_samples,
                        motion_blur_enabled,
                        num_blur_samples,
                        aafilter_function);
      if (strlen(render_filename) != 0)
      {
        sprintf(buf, "%s.%d.ppm", render_filename, i);
        canvas->save(buf);
        sprintf(buf, "%s.%d.ppm", pathless, i);
        fprintf(listFile, "%s\n", buf);
      }
      glutPostWindowRedisplay(render_window);
      glutSetWindow(render_window);
      glutShowWindow();
      glutSetWindow(main_window);
    }
    fclose(listFile);
  }
  else
  {
    canvas->rasterize(current_frame, anti_aliasing_enabled,
                      num_alias_samples,
                      motion_blur_enabled,
                      num_blur_samples,
                      aafilter_function);
    if (strlen(render_filename) != 0)
    {
      sprintf(buf, "%s.ppm", render_filename);
      canvas->save(buf);
    }
    glutPostWindowRedisplay(render_window);
    glutSetWindow(render_window);
    glutShowWindow();
    glutSetWindow(main_window);
  }
}
#endif
