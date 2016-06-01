/**
   \file main.cpp defines the rasterizer GUI application

   Created by Dmitri Makarov on 16-05-31.
   Copyright © 2016 Dmitri Makarov. All rights reserved.
 */

#include "control.h"
#include <wx/wx.h>
#include <string>
#include <vector>

/// \class MainFrame is the main application frame
class MainFrame : public wxFrame
{
public:
  MainFrame(wxFrame* frame, const wxString& title, const wxPoint& pos, const wxSize& size)
  : wxFrame(frame, wxID_ANY, title, pos, size, wxDEFAULT_FRAME_STYLE | wxFULL_REPAINT_ON_RESIZE)
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
    // if they set command-line arguments, don't show the GUI
    if (argc > 1) {
      std::vector<std::string> args;
      for (auto it = 1; it < argc; ++it) {
        args.push_back(std::string{argv[it]});
      }
      Rasterizer r{400, 400};
      r.render_to_file(args);
      return true;
    }
    auto* frame = new MainFrame(nullptr, wxT("Rasterizer"), wxDefaultPosition, wxSize(465, 400));
    (void) new Control(frame);
    frame->Show(true);
    return true;
  }
};

wxIMPLEMENT_APP(Application);
