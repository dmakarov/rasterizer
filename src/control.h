//
//  control.h
//  rasterizer
//
//  Created by Dmitri Makarov on 16-06-04.
//
//
/**
   \file control.h

   Created by Dmitri Makarov on 16-06-04.
   Copyright © 2016 Dmitri Makarov. All rights reserved.
 */

#ifndef control_h
#define control_h

#include "editor.h"
#include "rasterizer.h"

#include <wx/wx.h>
#include <wx/spinctrl.h>
#include <wx/stockitem.h>

#include <sstream>

// Define a new canvas which can receive some events
class Control : public wxPanel
{
  enum { ID_BUTTON_LOAD = wxID_HIGHEST + 1, ID_BUTTON_SAVE, ID_BUTTON_RENDER, ID_RADIO_FRAME, ID_CHECK_AA, ID_CHECK_MB };

public:
  Control(wxFrame* frame);

  virtual ~Control() {}

private:

  static const int SPIN_CTRL_WIDTH = 50;
  static const int TEXT_CTRL_WIDTH = 180;
  static const int SBOX_WIDTH = TEXT_CTRL_WIDTH + 30;

  int selected_object;
  Rasterizer rasterizer;
  EditorFrame editor;
  bool multiple_frames;
  std::string render_filename;
  int current_frame;
  int first_frame;
  int final_frame;

  bool anti_aliasing_enabled;
  int num_alias_samples;
  bool motion_blur_enabled;
  int num_blur_samples;
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

  void OnPaint(wxPaintEvent& event)
  {
  }

  void OnButtonLoad(wxCommandEvent& event);
  void OnButtonSave(wxCommandEvent& event);
  void OnButtonRender(wxCommandEvent& event);
  void OnCheckAA(wxCommandEvent& event);
  void OnCheckMB(wxCommandEvent& event);
  void OnRadioFrame(wxCommandEvent& event);

  void update_info();
  std::string get_basename(const std::string& filename);

  wxDECLARE_EVENT_TABLE();
};

#endif /* control_h */

// Local Variables:
// mode: c++
// End:
