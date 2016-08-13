/**
   \file canvas.cpp

   Created by Dmitri Makarov on 16-05-31.
   Copyright © 2016 Dmitri Makarov. All rights reserved.
 */

#include "canvas.h"

wxBEGIN_EVENT_TABLE(RenderFrame, wxWindow)
  EVT_CLOSE(RenderFrame::OnClose)
wxEND_EVENT_TABLE()

wxBEGIN_EVENT_TABLE(Canvas, wxWindow)
  EVT_PAINT(Canvas::OnPaint)
wxEND_EVENT_TABLE()

RenderFrame::RenderFrame(Rasterizer& rasterizer, wxWindowID id, const wxPoint& pos)
  : wxFrame(nullptr, id, wxT("Rendered Image"), pos,
            wxSize(rasterizer.get_width(), rasterizer.get_height()),
            wxDEFAULT_FRAME_STYLE | wxFULL_REPAINT_ON_RESIZE)
{
  auto size = GetClientSize();
  panel = new wxPanel(this, wxID_ANY,
                      wxDefaultPosition, size);
  canvas = new Canvas(rasterizer, panel, wxID_ANY, nullptr,
                      wxDefaultPosition, size);
}

void
RenderFrame::OnClose(wxCloseEvent& event)
{
  this->Destroy();
}


Canvas::Canvas(Rasterizer& rasterizer,
               wxWindow* parent,
               wxWindowID id,
               const int* attributes,
               const wxPoint& pos,
               const wxSize& size)
  : wxGLCanvas(parent, id, nullptr, pos, size, 0,
               "Render Frame", wxNullPalette)
  , rasterizer(rasterizer)
{
  context = new wxGLContext(this);
}

void
Canvas::paint()
{
  SetCurrent(*context);
  auto w = rasterizer.get_width();
  auto h = rasterizer.get_height();
  std::cout << "Canvas::paint: w " << w << ", h " << h << '\n';
#if 0
  int w, h;
  GetClientSize(&w, &h);
#endif
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(0, w, -h, 0, 0, 1);
  glGenTextures(1, &texture_object);
  glBindTexture(GL_TEXTURE_2D, texture_object);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glClearColor(0.f, 0.f, 0.f, 0.f);
  glClear(GL_COLOR_BUFFER_BIT);
  glRasterPos2s(0, 0);
  glPixelZoom(1.0, -1.0);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE,
               rasterizer.get_pixels());
  glEnable(GL_TEXTURE_2D);
  glBegin(GL_QUADS);
  {
    glTexCoord2f(0, 0); glVertex2d(0, 0);
    glTexCoord2f(0, 1); glVertex2d(0, -h);
    glTexCoord2f(1, 1); glVertex2d(w, -h);
    glTexCoord2f(1, 0); glVertex2d(w, 0);
  } glEnd();
  glDisable(GL_TEXTURE_2D);
  SwapBuffers();
}

void
Canvas::OnPaint(wxPaintEvent& event)
{
  paint();
}
