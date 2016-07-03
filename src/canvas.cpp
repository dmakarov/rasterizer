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

void
Canvas::render(const wxDC& dc)
{
  auto w = rasterizer.get_width();
  auto h = rasterizer.get_height();
  glClearColor(0.f, 0.f, 0.f, 0.f);
  glClear(GL_COLOR_BUFFER_BIT);
  glRasterPos2s(0, 0);
  glPixelZoom(1.0, -1.0);
  glDrawPixels(w, h, GL_RGBA, GL_UNSIGNED_BYTE, rasterizer.get_pixels());
  SwapBuffers();
}

void
Canvas::OnPaint(wxPaintEvent& event)
{
  wxPaintDC dc(this);
  PrepareDC(dc);
  render(dc);
}


void
RenderFrame::OnClose(wxCloseEvent& event)
{
  this->Destroy();
}
