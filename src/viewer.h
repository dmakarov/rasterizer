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

  void OnPaint(wxPaintEvent& event);

  wxDECLARE_EVENT_TABLE();

};

class ViewerFrame : public wxFrame {
public:

  ViewerFrame(Rasterizer& rasterizer, wxWindowID id, const wxPoint& pos);
  virtual ~ViewerFrame() {}
  void OnClose(wxCloseEvent& event);

private:

  Viewer* viewer;

  wxDECLARE_EVENT_TABLE();

};

#endif /* viewer_h */

// Local Variables:
// mode: c++
// End:
