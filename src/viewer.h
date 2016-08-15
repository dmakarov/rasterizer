/**
   \file viewer.h

   Created by Dmitri Makarov on 16-05-31.
   Copyright © 2016 Dmitri Makarov. All rights reserved.
 */

#ifndef viewer_h
#define viewer_h

#include "rasterizer.h"
#include <wx/wx.h>

// Define a new canvas which can receive some events
class Viewer : public wxWindow
{
public:

  Viewer(Rasterizer& rasterizer,
         wxWindow* parent,
         wxWindowID id,
         const int* attributes,
         const wxPoint& pos,
         const wxSize& size)
    : wxWindow(parent, id, pos, size, 0, "Image Viewer")
    , rasterizer(rasterizer)
  {}

  virtual ~Viewer()
  {}

private:

  Rasterizer& rasterizer;

  void OnPaint(wxPaintEvent& event)
  {
    wxPaintDC dc(this);
    auto* gc = wxGraphicsContext::Create(dc);
    if (gc) {
      auto w = rasterizer.get_width();
      auto h = rasterizer.get_height();
      wxBitmap bmp(wxImage(w, h, rasterizer.getPixelsAsRGB()));
      gc->DrawBitmap(bmp, 0, 0, w, h);
      delete gc;
    }
  }

  wxDECLARE_EVENT_TABLE();

};

class ViewerFrame : public wxFrame, public Subject {

public:

  ViewerFrame(Rasterizer& rasterizer, wxWindowID id, const wxPoint& pos)
    : wxFrame(nullptr, id, wxT("Rendered Image"), pos,
              wxSize(rasterizer.get_width(), rasterizer.get_height()),
              wxDEFAULT_FRAME_STYLE | wxFULL_REPAINT_ON_RESIZE)
  {
    auto size = GetClientSize();
    viewer = new Viewer(rasterizer, this, wxID_ANY, nullptr,
                        wxDefaultPosition, size);
  }

  virtual ~ViewerFrame() {}

  void OnClose(wxCloseEvent& event)
  {
    notify();
    Destroy();
  }

private:

  Viewer* viewer;

  wxDECLARE_EVENT_TABLE();

};

#endif /* viewer_h */

// Local Variables:
// mode: c++
// End:
