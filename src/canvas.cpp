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

RenderFrame::RenderFrame(Rasterizer& rasterizer)
  : wxFrame(nullptr, wxID_ANY, wxT("Rendered Image"),
            wxDefaultPosition, wxSize(500, 500),
            wxDEFAULT_FRAME_STYLE | wxFULL_REPAINT_ON_RESIZE)
{
  panel = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxSize(500, 500));
  canvas = new Canvas(rasterizer, panel, wxID_ANY, nullptr, wxDefaultPosition, wxSize(500, 500));
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
  : wxGLCanvas(parent, id, nullptr, pos, size, 0, "Render Frame", wxNullPalette)
  , rasterizer(rasterizer)
{
  context = new wxGLContext(this);
  this->SetCurrent(*context);

  int w, h;
  this->GetClientSize(&w, &h);
  glOrtho(0, w, -h, 0, -1, 1);
}


void
Canvas::render(const wxDC& dc)
{
  this->SetCurrent(*context);
  auto w = rasterizer.get_width();
  auto h = rasterizer.get_height();
  glClearColor(0.f, 0.f, 0.f, 0.f);
  glClear(GL_COLOR_BUFFER_BIT);
  glRasterPos2s(0, 0);
  glPixelZoom(1.0, -1.0);
  // FIXME: use texture instead of removed glDrawPixels.
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
