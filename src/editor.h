/**
   \file editor.h

   Created by Dmitri Makarov on 16-06-26.
   Copyright © 2016 Dmitri Makarov. All rights reserved.
 */

#ifndef editor_h
#define editor_h

#include "observer.h"
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
               const long style = 0,
               const wxString& name = "Objects",
               const wxPalette& palette = wxNullPalette);
  virtual ~EditorCanvas() {}
  void setAnimationFrame(int frame) {
    animation_frame = frame;
  }

private:

  wxGLContext* context;
  Rasterizer& rasterizer;
  int animation_frame;
  long rotation_centerX;
  long rotation_centerY;
  long prev_rotationX;
  long prev_rotationY;
  bool draw_curve = false;
  bool scale_polygon = false;
  bool rotate_polygon = false;
  std::shared_ptr<Animation> active_object;

  void paint();

  void OnPaint(wxPaintEvent& event);
  void OnMouse(wxMouseEvent& event);
  void OnChar(wxKeyEvent& event);

  wxDECLARE_EVENT_TABLE();

};

/**
   \brief The editor window.
 */
class EditorFrame : public wxFrame, public Subject {
public:

  EditorFrame(Rasterizer& rasterizer,
              wxWindowID id,
              const wxPoint& pos);
  virtual ~EditorFrame() {}
  void setAnimationFrame(int frame) {
    canvas->setAnimationFrame(frame);
  }

private:

  wxPanel* panel;
  EditorCanvas* canvas;

  void OnClose(wxCloseEvent& event);

  wxDECLARE_EVENT_TABLE();

};

#endif /* editor_h */

// Local Variables:
// mode: c++
// End:
