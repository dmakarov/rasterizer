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

  Canvas(Rasterizer& rasterizer,
         wxWindow* parent,
         wxWindowID id,
         const int* attributes,
         const wxPoint& pos,
         const wxSize& size);

  virtual ~Canvas()
  {}

private:

  Rasterizer& rasterizer;
  wxGLContext* context;
  GLuint texture_object;

  void paint();

  void OnPaint(wxPaintEvent& event);

  wxDECLARE_EVENT_TABLE();

};

class RenderFrame : public wxFrame {
public:

  RenderFrame(Rasterizer& rasterizer, wxWindowID id, const wxPoint& pos);
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
