/**
   \file viewer.h

   Created by Dmitri Makarov on 16-05-31.
   Copyright © 2016 Dmitri Makarov. All rights reserved.
 */

#ifndef viewer_h
#define viewer_h

#include "rasterizer.h"
#include <wx/graphics.h>
#include <wx/wx.h>

// Define a new canvas which can receive some events
class Viewer : public wxWindow
{
public:

  Viewer(Rasterizer& r,
         wxWindow* parent,
         wxWindowID id,
         const int* attributes,
         const wxPoint& pos,
         const wxSize& size)
    : wxWindow(parent, id, pos, size, 0, "Image Viewer")
    , rasterizer(r)
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
      auto w = rasterizer.getWidth();
      auto h = rasterizer.getHeight();
      wxBitmap bmp(wxImage(w, h, rasterizer.getPixelsAsRGB()));
      gc->DrawBitmap(bmp, 0, 0, w, h);
      delete gc;
    }
  }

  wxDECLARE_EVENT_TABLE();

};

class ViewerFrame : public wxFrame, public Subject {

public:

  ViewerFrame(Rasterizer& r, wxWindowID id, const wxPoint& pos)
    : wxFrame(nullptr, id, wxT("Rendered Image"), pos,
              wxSize(r.getWidth(), r.getHeight()),
              wxDEFAULT_FRAME_STYLE | wxFULL_REPAINT_ON_RESIZE)
    , viewer(new Viewer(r, this, wxID_ANY, nullptr,
                        wxDefaultPosition, GetClientSize()))
  {}

  virtual ~ViewerFrame()
  {}

  void OnClose(wxCloseEvent& event)
  {
    notify();
    Destroy();
  }

private:

  std::unique_ptr<Viewer> viewer;

  wxDECLARE_EVENT_TABLE();

};

#endif /* viewer_h */

// Local Variables:
// mode: c++
// End:
