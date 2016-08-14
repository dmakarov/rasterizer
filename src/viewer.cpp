/**
   \file viewer.cpp

   Created by Dmitri Makarov on 16-05-31.
   Copyright © 2016 Dmitri Makarov. All rights reserved.
 */

#include "viewer.h"

wxBEGIN_EVENT_TABLE(Viewer, wxWindow)
  EVT_PAINT(Viewer::OnPaint)
wxEND_EVENT_TABLE()

void
Viewer::OnPaint(wxPaintEvent& event)
{
  wxPaintDC dc(this);
  auto* gc = wxGraphicsContext::Create(dc);
  if (gc) {
    auto w = rasterizer.get_width();
    auto h = rasterizer.get_height();
    wxImage img(w, h, rasterizer.get_pixels());
    wxBitmap bmp(img);
    gc->DrawBitmap(bmp, 0, 0, w, h);
    delete gc;
  }
}

wxBEGIN_EVENT_TABLE(ViewerFrame, wxWindow)
  EVT_CLOSE(ViewerFrame::OnClose)
wxEND_EVENT_TABLE()

ViewerFrame::ViewerFrame(Rasterizer& rasterizer, wxWindowID id, const wxPoint& pos)
  : wxFrame(nullptr, id, wxT("Rendered Image"), pos,
            wxSize(rasterizer.get_width(), rasterizer.get_height()),
            wxDEFAULT_FRAME_STYLE | wxFULL_REPAINT_ON_RESIZE)
{
  auto size = GetClientSize();
  viewer = new Viewer(rasterizer, this, wxID_ANY, nullptr,
                      wxDefaultPosition, size);
}

void
ViewerFrame::OnClose(wxCloseEvent& event)
{
  this->Destroy();
}
