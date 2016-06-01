//
//  canvas.h
//  rasterizer
//
//  Created by Dmitri Makarov on 16-05-31.
//  Copyright © 2016 Dmitri Makarov. All rights reserved.
//

#ifndef canvas_h
#define canvas_h

#include <wx/wx.h>
#include <wx/glcanvas.h>

// Define a new canvas which can receive some events
class Canvas : public wxGLCanvas
{
public:

  Canvas(wxWindow* parent) : wxGLCanvas(parent)
  {}

  void Draw(wxDC& dc)
  {
    dc.SetBackground(*wxLIGHT_GREY_BRUSH);
    dc.Clear();
  }

private:

  void OnPaint(wxPaintEvent& event)
  {
    wxPaintDC dc(this);
    PrepareDC(dc);
    Draw(dc);
  }

  wxDECLARE_EVENT_TABLE();
};

#endif /* canvas_h */

// Local Variables:
// mode: c++
// End:
