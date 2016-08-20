/**
   \file editor.h

   Created by Dmitri Makarov on 16-06-26.
   Copyright © 2016 Dmitri Makarov. All rights reserved.
 */

#ifndef editor_h
#define editor_h

#include "observer.h"
#include "scene.h"
#include <wx/wx.h>
#include <wx/glcanvas.h>

class EditorCanvas : public wxGLCanvas {
public:

  EditorCanvas(Scene& scene,
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

  enum State {NORMAL, DRAW, ROTATE, SCALE, MOVE, DRAG} state = NORMAL;
  wxGLContext* context;
  Scene& scene;
  std::shared_ptr<Polygon> active_object;
  Point prev_rotation_center;
  Point rotation_center;
  int animation_frame;

  void startDrawing(long x, long y);
  void startRotating(long x, long y);
  void startScaling(long x, long y);
  void continueDrawing(long x, long y);
  void continueRotating(long x, long y);
  void continueScaling(long x, long y);
  void finishDrawing(long x, long y);
  void finishRotating(long x, long y);
  void finishScaling(long x, long y);
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

  EditorFrame(Scene& scene, wxWindowID id, const wxPoint& pos);
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
