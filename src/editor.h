/**
   \file editor.h

   Created by Dmitri Makarov on 16-06-26.
   Copyright © 2016 Dmitri Makarov. All rights reserved.
 */

#ifndef editor_h
#define editor_h

#include "rasterizer.h"
#include <wx/wx.h>
#include <wx/glcanvas.h>

class EditorCanvas : public wxGLCanvas {
public:

  EditorCanvas(Rasterizer& rasterizer,
               wxWindow* parent,
               wxWindowID id,
               const int* attributes,
               const wxPoint& pos,
               const wxSize& size,
               long style = 0,
               const wxString& name = "Objects",
               const wxPalette& palette = wxNullPalette);
  virtual ~EditorCanvas() {}

private:

  Rasterizer& rasterizer;
  int animation_frame;
  int selected_object;
  int selected_vertex;
  long rotation_centerX;
  long rotation_centerY;
  long prev_rotationX;
  long prev_rotationY;
  std::shared_ptr<Animation> active_object;
  wxGLContext* context;
  bool draw_curve = false;
  bool scale_polygon = false;
  bool rotate_polygon = false;

  void draw();
  void paint(const wxDC& dc);

  void OnPaint(wxPaintEvent& event);
  void OnMouseMove(wxMouseEvent& event);
  void OnLeftMouse(wxMouseEvent& event);
  void OnLeftMouseUp(wxMouseEvent& event);
  void OnRightMouse(wxMouseEvent& event);
  void OnRightMouseUp(wxMouseEvent& event);

  wxDECLARE_EVENT_TABLE();

};

class EditorFrame : public wxFrame {
public:

  EditorFrame(Rasterizer& rasterizer);
  virtual ~EditorFrame() {}
  void OnClose(wxCloseEvent& event);

private:

  wxPanel* panel;
  EditorCanvas* canvas;

  wxDECLARE_EVENT_TABLE();

};

#endif /* editor_h */

// Local Variables:
// mode: c++
// End:
