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
               const wxString& name = "Editor Canvas",
               const wxPalette& palette = wxNullPalette)
    : wxGLCanvas(parent, id, attributes, pos, size, style, name, palette)
    , context(new wxGLContext(this))
    , scene(scene)
    , state(NONE)
    , frame(1)
  {}

  virtual ~EditorCanvas()
  {}

  void setFrame(int frame) {
    this->frame = frame;
  }

private:

  std::unique_ptr<wxGLContext> context;
  Scene& scene;
  enum State {NONE, DRAW, ROTATE, SCALE, MOVE, DRAG} state;
  int frame;

  void paint();

  void OnChar(wxKeyEvent& event);
  void OnMouse(wxMouseEvent& event);
  void OnPaint(wxPaintEvent& event);

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
