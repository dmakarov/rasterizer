/**
   \file main.cpp defines the rasterizer GUI application

   Created by Dmitri Makarov on 16-05-31.
   Copyright © 2016 Dmitri Makarov. All rights reserved.
 */

#include "control.h"
#include <wx/wx.h>
#include <algorithm>
#include <string>
#include <vector>

/// \class MainFrame is the main application frame
class MainFrame : public wxFrame
{
public:
  MainFrame(wxFrame* frame, const wxString& title,
            const wxPoint& pos, const wxSize& size)
  : wxFrame(frame, wxID_ANY, title, pos, size,
            wxDEFAULT_FRAME_STYLE | wxFULL_REPAINT_ON_RESIZE)
  {}

  virtual ~MainFrame() {}

  void OnCloseWindow(wxCloseEvent& event)
  {
    static bool destroyed = false;
    if (destroyed) return;
    destroyed = true;
    this->Destroy();
  }

  void OnExit(wxCommandEvent& event)
  {
    this->Destroy();
  }

  wxDECLARE_EVENT_TABLE();
};

wxBEGIN_EVENT_TABLE(MainFrame, wxFrame)
  EVT_CLOSE(MainFrame::OnCloseWindow)
  EVT_MENU(wxID_EXIT, MainFrame::OnExit)
wxEND_EVENT_TABLE()

/// \class Application is a new application type
class Application : public wxApp
{
public:
  bool OnInit()
  {
    // command-line arguments are set for batch mode, don't show the GUI
    if (argc > 1) {
      Scene s;
      s.render_to_file(std::vector<std::string>(static_cast<char**>(argv) + 1,
                                                static_cast<char**>(argv) + argc));
      return true;
    }
    compute_window_positions();
    auto* frame = new MainFrame(nullptr, wxT("Rasterizer"),
                                control_position, wxSize(465, 400));
    (void) new Control(frame, editor_position, viewer_position);
    frame->Show(true);
    return true;
  }

private:

  wxPoint control_position;
  wxPoint editor_position;
  wxPoint viewer_position;

  void compute_window_positions()
  {
    auto area = wxGetClientDisplayRect();
    control_position.x = area.x + 10;
    control_position.y = area.y + 5;
    if (control_position.x + 470 + 500 < area.x + area.width) {
      editor_position.x = control_position.x + 470;
      editor_position.y = control_position.y;
    } else if (control_position.y + 410 + 500 < area.y + area.height) {
      editor_position.x = control_position.x;
      editor_position.y = control_position.y + 410;
    } else {
      editor_position.x = wxDefaultPosition.x;
      editor_position.y = wxDefaultPosition.y;
    }
    if (editor_position.x + 505 + 500 < area.x + area.width) {
      viewer_position.x = editor_position.x + 505;
      viewer_position.y = editor_position.y;
    } else if (editor_position.y + 505 + 500 < area.y + area.height) {
      viewer_position.x = editor_position.x;
      viewer_position.y = editor_position.y + 505;
    } else {
      viewer_position.x = wxDefaultPosition.x;
      viewer_position.y = wxDefaultPosition.y;
    }
  }

};

wxIMPLEMENT_APP(Application);
