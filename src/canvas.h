/**
   \file canvas.h

   Created by Dmitri Makarov on 16-05-31.
   Copyright © 2016 Dmitri Makarov. All rights reserved.
 */

#ifndef canvas_h
#define canvas_h

#include "rasterizer.h"
#include <wx/wx.h>
#include <wx/glcanvas.h>

// Define a new canvas which can receive some events
class Canvas : public wxGLCanvas
{
public:

  Canvas(Rasterizer& rasterizer, wxWindow* parent)
    : wxGLCanvas(parent, wxID_ANY, nullptr, wxDefaultPosition, wxDefaultSize, 0, "Render Frame", wxNullPalette)
    , rasterizer(rasterizer)
  {}
  virtual ~Canvas()
  {}

private:

  Rasterizer& rasterizer;
  void render(const wxDC& dc);

  void OnPaint(wxPaintEvent& event);

  wxDECLARE_EVENT_TABLE();

};

class RenderFrame : public wxFrame {
public:

  RenderFrame(Rasterizer& rasterizer);
  virtual ~RenderFrame() {}
  void OnClose(wxCloseEvent& event);

private:

  wxPanel* panel;
  Canvas* canvas;

  wxDECLARE_EVENT_TABLE();

};

#endif /* canvas_h */

// Local Variables:
// mode: c++
// End:
