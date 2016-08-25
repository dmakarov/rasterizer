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

class EditorFrame;

class EditorCanvas : public wxGLCanvas {
public:

  EditorCanvas(Scene& scene,
               EditorFrame* p,
               wxWindowID id,
               const int* attributes,
               const wxPoint& pos,
               const wxSize& size,
               const long style = 0,
               const wxString& name = "Editor Canvas",
               const wxPalette& palette = wxNullPalette)
    : wxGLCanvas((wxWindow*)p, id, attributes, pos, size, style, name, palette)
    , context(new wxGLContext(this))
    , parent(p)
    , scene(scene)
    , state(State::NONE)
    , frame(1)
  {}

  virtual ~EditorCanvas()
  {}

  void setFrame(int frame) {
    this->frame = frame;
  }

private:

  enum class State {NONE, DRAW, ROTATE, SCALE, MOVE, DRAG};

  std::unique_ptr<wxGLContext> context;
  EditorFrame* parent;
  Scene& scene;
  State state;
  int frame;

  void paint();

  void OnPaint(wxPaintEvent& event) {
    paint();
    event.Skip();
  }

  void OnChar(wxKeyEvent& event);
  void OnMouseLeftDown(wxMouseEvent& event);
  void OnMouseLeftUp(wxMouseEvent& event);
  void OnMouseRightDown(wxMouseEvent& event);
  void OnMouseRightUp(wxMouseEvent& event);
  void OnMouseMotion(wxMouseEvent& event);

  wxDECLARE_EVENT_TABLE();

};

/**
 \brief The editor window.
 */
class EditorFrame : public wxFrame, public Subject {

public:

  EditorFrame(Scene& scene, wxWindowID id, const wxPoint& pos)
  : wxFrame(nullptr, id, wxT("Scene Editor"), pos,
            wxSize(scene.getWidth(), scene.getHeight()),
            wxDEFAULT_FRAME_STYLE | wxFULL_REPAINT_ON_RESIZE)
  , canvas(new EditorCanvas(scene, this, wxID_ANY, nullptr,
                            wxDefaultPosition, GetClientSize())) {
    canvas->SetFocus();
  }

  virtual ~EditorFrame() {}

  void setFrame(int frame) {
    canvas->setFrame(frame);
  }

private:

  std::unique_ptr<EditorCanvas> canvas;

  void OnClose(wxCloseEvent& event) {
    notify();
    Destroy();
  }

  wxDECLARE_EVENT_TABLE();
  
};

#endif /* editor_h */

// Local Variables:
// mode: c++
// End:
