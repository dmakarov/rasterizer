/**
   \file control.h

   Created by Dmitri Makarov on 16-06-04.
   Copyright © 2016 Dmitri Makarov. All rights reserved.
 */

#ifndef control_h
#define control_h

#include "editor.h"
#include "observer.h"
#include "rasterizer.h"
#include "viewer.h"

#include <wx/wx.h>
#include <wx/spinctrl.h>
#include <wx/stockitem.h>

#include <sstream>

// Define a new canvas which can receive some events
class Control : public wxPanel, public Observer
{
  enum { ID_BUTTON_LOAD = wxID_HIGHEST + 1,
         ID_BUTTON_SAVE,
         ID_BUTTON_RENDER,
         ID_RADIO_FRAME,
         ID_CHECK_AA,
         ID_CHECK_MB
  };

public:
  Control(wxFrame* frame,
          const wxPoint& editor_pos,
          const wxPoint& viewer_pos);
  virtual ~Control();
  void update(Subject* subject);

private:

  static const int SPIN_CTRL_WIDTH = 50;
  static const int TEXT_CTRL_WIDTH = 180;
  static const int SBOX_WIDTH = TEXT_CTRL_WIDTH + 30;

  Rasterizer rasterizer;
  EditorFrame* editor;
  ViewerFrame* viewer;
  wxPoint editor_pos;
  wxPoint viewer_pos;
  bool multiple_frames = false;
  std::string render_filename;
  int current_frame = 1;
  int first_frame;
  int final_frame;

  bool anti_aliasing_enabled = false;
  int num_alias_samples = 1;
  bool motion_blur_enabled = false;
  int num_blur_samples = 1;
  std::string aafilter_function;

  wxMenuBar* menu_bar;

  wxStaticText* stxt_objectid;
  wxStaticText* stxt_vertices;
  wxStaticText* stxt_keyframe;
  wxTextCtrl* text_filename;
  wxTextCtrl* text_renderto;
  wxButton* button_delete_keyframe;
  wxSpinCtrl* spin_delete_keyframe;
  wxStaticText* stxt_aa_nos;
  wxSpinCtrl* spin_aa_nos;
  wxStaticText* stxt_aa_filter;
  wxTextCtrl* text_aa_filter;
  wxStaticText* stxt_mb_nos;
  wxSpinCtrl* spin_mb_nos;
  wxStaticText* stxt_sframe;
  wxTextCtrl* text_sframe;
  wxStaticText* stxt_eframe;
  wxTextCtrl* text_eframe;

  std::string get_basename(const std::string& filename);
  void collectSettings();

  void OnPaint(wxPaintEvent& event) {}
  void OnButtonLoad(wxCommandEvent& event);
  void OnButtonSave(wxCommandEvent& event);
  void OnButtonRender(wxCommandEvent& event);
  void OnCheckAA(wxCommandEvent& event);
  void OnCheckMB(wxCommandEvent& event);
  void OnRadioFrame(wxCommandEvent& event);

  wxDECLARE_EVENT_TABLE();
};

#endif /* control_h */

// Local Variables:
// mode: c++
// End:
