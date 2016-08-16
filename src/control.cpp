/**
   \file control.cpp

   Created by Dmitri Makarov on 16-06-04.
   Copyright © 2016 Dmitri Makarov. All rights reserved.
 */

#include "control.h"
#include <wx/filename.h>

wxBEGIN_EVENT_TABLE(Viewer, wxWindow)
  EVT_PAINT(Viewer::OnPaint)
wxEND_EVENT_TABLE()

wxBEGIN_EVENT_TABLE(ViewerFrame, wxWindow)
  EVT_CLOSE(ViewerFrame::OnClose)
wxEND_EVENT_TABLE()

wxBEGIN_EVENT_TABLE(Control, wxWindow)
  EVT_PAINT(Control::OnPaint)
  EVT_BUTTON(ID_BUTTON_LOAD, Control::OnButtonLoad)
  EVT_BUTTON(ID_BUTTON_SAVE, Control::OnButtonSave)
  EVT_BUTTON(ID_BUTTON_RENDER, Control::OnButtonRender)
  EVT_CHECKBOX(ID_CHECK_AA, Control::OnCheckAA)
  EVT_CHECKBOX(ID_CHECK_MB, Control::OnCheckMB)
  EVT_RADIOBOX(ID_RADIO_FRAME, Control::OnRadioFrame)
  EVT_SPINCTRL(ID_SPIN_FRAME, Control::OnSpinFrame)
wxEND_EVENT_TABLE()

Control::Control(wxFrame* frame,
                 const wxPoint& editor_pos,
                 const wxPoint& viewer_pos)
  : wxPanel(frame, wxID_ANY)
  , rasterizer{500, 500}
  , editor(new EditorFrame(rasterizer, wxID_ANY, editor_pos))
  , viewer(new ViewerFrame(rasterizer, wxID_ANY, viewer_pos))
  , editor_pos(editor_pos)
  , viewer_pos(viewer_pos)
{
  rasterizer.attach(this);
  editor->attach(this);
  viewer->attach(this);
  wxMenu* file_menu = new wxMenu;
  file_menu->Append(wxID_NEW, wxGetStockLabel(wxID_NEW));
  file_menu->Append(wxID_PRINT, wxGetStockLabel(wxID_PRINT));
  file_menu->AppendSeparator();
  file_menu->Append(wxID_EXIT, wxGetStockLabel(wxID_EXIT));
  menu_bar = new wxMenuBar;
  menu_bar->Append(file_menu, wxT("&File"));
  file_menu = new wxMenu;
  file_menu->Append(wxID_UNDO, wxGetStockLabel(wxID_UNDO));
  file_menu->Append(wxID_REDO, wxGetStockLabel(wxID_REDO));
  file_menu->AppendSeparator();
  file_menu->Append(wxID_CUT, wxGetStockLabel(wxID_CUT));
  file_menu->Append(wxID_COPY, wxGetStockLabel(wxID_COPY));
  file_menu->Append(wxID_PASTE, wxGetStockLabel(wxID_PASTE));
  menu_bar->Append(file_menu, wxT("&Edit"));
  frame->SetMenuBar(menu_bar);

  auto* top_sizer = new wxBoxSizer(wxHORIZONTAL);
  auto* vsizer = new wxBoxSizer(wxVERTICAL);
  auto* hsizer = new wxBoxSizer(wxHORIZONTAL);
  auto* sbox = new wxStaticBox(this, wxID_ANY, wxT("Info"),
                               wxDefaultPosition, wxSize(SBOX_WIDTH, -1));
  auto* ssizer = new wxStaticBoxSizer(sbox, wxVERTICAL);
  stxt_objectid = new wxStaticText(sbox, wxID_ANY, wxT("Object ID:"));
  stxt_objectid->Disable();
  ssizer->Add(stxt_objectid, 0, wxALIGN_LEFT | wxALL, 5);
  stxt_vertices = new wxStaticText(sbox, wxID_ANY, wxT("Vertices:"));
  stxt_vertices->Disable();
  ssizer->Add(stxt_vertices, 0, wxALIGN_LEFT | wxALL, 5);
  stxt_keyframe = new wxStaticText(sbox, wxID_ANY, wxT("Keyframes:"));
  stxt_keyframe->Disable();
  ssizer->Add(stxt_keyframe, 0, wxALIGN_LEFT | wxALL, 5);
  vsizer->Add(ssizer, 0, wxALIGN_CENTER | wxALL, 5);

  sbox = new wxStaticBox(this, wxID_ANY, wxT("Animation"),
                         wxDefaultPosition, wxSize(SBOX_WIDTH, -1));
  ssizer = new wxStaticBoxSizer(sbox, wxHORIZONTAL);
  button_delete_keyframe = new wxButton(sbox, wxID_ANY,
                                        wxT("Delete keyframe"));
  button_delete_keyframe->Disable();
  ssizer->Add(button_delete_keyframe, 0, wxALIGN_LEFT | wxALL, 5);
  spin_delete_keyframe = new wxSpinCtrl(sbox, ID_SPIN_FRAME,
                                        wxEmptyString,
                                        wxDefaultPosition,
                                        wxSize(SPIN_CTRL_WIDTH, -1),
                                        wxSP_ARROW_KEYS, 1, 99, 1);
  spin_delete_keyframe->Disable();
  ssizer->Add(spin_delete_keyframe, 0, wxALIGN_RIGHT | wxALL, 5);
  vsizer->Add(ssizer, 0, wxALIGN_LEFT | wxALL, 5);

  sbox = new wxStaticBox(this, wxID_ANY, wxT("Objects"),
                         wxDefaultPosition, wxSize(SBOX_WIDTH, -1));
  ssizer = new wxStaticBoxSizer(sbox, wxVERTICAL);
  text_filename = new wxTextCtrl(this, wxID_ANY, wxEmptyString,
                                 wxDefaultPosition,
                                 wxSize(TEXT_CTRL_WIDTH, -1));
  ssizer->Add(text_filename, 0, wxALIGN_CENTER | wxALL, 5);
  auto* button = new wxButton(this, ID_BUTTON_LOAD, wxT("Load"));
  hsizer->Add(button, 0, wxALIGN_LEFT | wxALL, 5);
  button = new wxButton(this, ID_BUTTON_SAVE, wxT("Save"));
  hsizer->Add(button, 0, wxALIGN_RIGHT | wxALL, 5);
  ssizer->Add(hsizer, 0, wxALIGN_CENTER | wxALL);
  vsizer->Add(ssizer, 0, wxALIGN_LEFT | wxALL, 5);

  top_sizer->Add(vsizer, 0, wxALIGN_LEFT | wxALL, 5);

  vsizer = new wxBoxSizer(wxVERTICAL);
  sbox = new wxStaticBox(this, wxID_ANY, wxT("Rendering"),
                         wxDefaultPosition, wxSize(SBOX_WIDTH, -1));
  ssizer = new wxStaticBoxSizer(sbox, wxVERTICAL);
  // Render out text control
  hsizer = new wxBoxSizer(wxHORIZONTAL);
  auto* label = new wxStaticText(this, wxID_ANY, wxT("Render out"));
  text_renderto = new wxTextCtrl(this, wxID_ANY, wxEmptyString,
                                 wxDefaultPosition,
                                 wxSize(TEXT_CTRL_WIDTH, -1));
  hsizer->Add(label, 0, wxALIGN_LEFT | wxALL, 0);
  hsizer->Add(text_renderto, 0, wxALIGN_RIGHT | wxALL, 0);
  ssizer->Add(hsizer, 0, wxALIGN_CENTER | wxALL);
  // Anti aliasing
  auto* check_box = new wxCheckBox(this, ID_CHECK_AA, wxT("Antialiasing"));
  ssizer->Add(check_box, 0, wxALIGN_LEFT | wxALL);
  hsizer = new wxBoxSizer(wxHORIZONTAL);
  stxt_aa_nos = new wxStaticText(this, wxID_ANY, wxT("Number of samples"));
  stxt_aa_nos->Disable();
  spin_aa_nos = new wxSpinCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition,
                               wxDefaultSize, wxSP_ARROW_KEYS, 1, 10, 1);
  spin_aa_nos->Disable();
  hsizer->Add(stxt_aa_nos, 0, wxALIGN_LEFT | wxALL, 0);
  hsizer->Add(spin_aa_nos, 0, wxALIGN_RIGHT | wxALL, 0);
  ssizer->Add(hsizer, 0, wxALIGN_LEFT | wxALL);
  // AA filter function
  hsizer = new wxBoxSizer(wxHORIZONTAL);
  stxt_aa_filter = new wxStaticText(this, wxID_ANY, wxT("Filter"));
  stxt_aa_filter->Disable();
  text_aa_filter = new wxTextCtrl(this, wxID_ANY, wxEmptyString,
                                  wxDefaultPosition,
                                  wxSize(TEXT_CTRL_WIDTH, -1));
  text_aa_filter->Disable();
  hsizer->Add(stxt_aa_filter, 0, wxALIGN_LEFT | wxALL, 0);
  hsizer->Add(text_aa_filter, 0, wxALIGN_RIGHT | wxALL, 0);
  ssizer->Add(hsizer, 0, wxALIGN_LEFT | wxALL);
  // Motion blur
  check_box = new wxCheckBox(this, ID_CHECK_MB, wxT("Motion blur"));
  ssizer->Add(check_box, 0, wxALIGN_LEFT | wxALL);
  hsizer = new wxBoxSizer(wxHORIZONTAL);
  stxt_mb_nos = new wxStaticText(this, wxID_ANY, wxT("Number of samples"));
  stxt_mb_nos->Disable();
  spin_mb_nos = new wxSpinCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition,
                               wxDefaultSize, wxSP_ARROW_KEYS, 1, 10, 1);
  spin_mb_nos->Disable();
  hsizer->Add(stxt_mb_nos, 0, wxALIGN_LEFT | wxALL, 0);
  hsizer->Add(spin_mb_nos, 0, wxALIGN_RIGHT | wxALL, 0);
  ssizer->Add(hsizer, 0, wxALIGN_LEFT | wxALL);
  // Frames
  wxArrayString radio_buttons_labels;
  radio_buttons_labels.Add(wxT("This Frame Only"));
  radio_buttons_labels.Add(wxT("Multiple Frames"));
  auto* radio_box = new wxRadioBox(this, ID_RADIO_FRAME,
                                   wxT("Single/Multiple"),
                                   wxDefaultPosition,
                                   wxDefaultSize,
                                   radio_buttons_labels,
                                   2, wxRA_SPECIFY_ROWS);
  ssizer->Add(radio_box, 0, wxALIGN_LEFT | wxALL);
  /*
    "Start Frame:", GLUI_EDITTEXT_INT, &first_frame);
    start_frame_editor->set_int_val(1);
    start_frame_editor->set_int_limits(1, 100);

    "End Frame:", GLUI_EDITTEXT_INT, &final_frame);
    end_frame_editor->set_int_val(100);
    end_frame_editor->set_int_limits(1, 100);
  */
  hsizer = new wxBoxSizer(wxHORIZONTAL);
  stxt_sframe = new wxStaticText(this, wxID_ANY, wxT("First frame"));
  stxt_sframe->Disable();
  auto size = stxt_sframe->GetSize();
  text_sframe = new wxTextCtrl(this, wxID_ANY, wxEmptyString,
                               wxDefaultPosition,
                               wxSize(TEXT_CTRL_WIDTH, -1));
  text_sframe->Disable();
  hsizer->Add(stxt_sframe, 0, wxALIGN_LEFT | wxALL, 0);
  hsizer->Add(text_sframe, 0, wxALIGN_RIGHT | wxALL, 0);
  ssizer->Add(hsizer, 0, wxALIGN_LEFT | wxALL);
  hsizer = new wxBoxSizer(wxHORIZONTAL);
  stxt_eframe = new wxStaticText(this, wxID_ANY, wxT("Last frame"),
                                 wxDefaultPosition, size);
  stxt_eframe->Disable();
  text_eframe = new wxTextCtrl(this, wxID_ANY, wxEmptyString,
                               wxDefaultPosition,
                               wxSize(TEXT_CTRL_WIDTH, -1));
  text_eframe->Disable();
  hsizer->Add(stxt_eframe, 0, wxALIGN_LEFT | wxALL, 0);
  hsizer->Add(text_eframe, 0, wxALIGN_RIGHT | wxALL, 0);
  ssizer->Add(hsizer, 0, wxALIGN_LEFT | wxALL);
  // "Render!", DisplayAndSaveCanvas
  button = new wxButton(this, ID_BUTTON_RENDER, wxT("Render!"));
  ssizer->Add(button, 0, wxALIGN_CENTER | wxALL, 5);

  vsizer->Add(ssizer, 0, wxALIGN_LEFT | wxALL, 5);
  top_sizer->Add(vsizer, 0, wxALIGN_RIGHT | wxALL, 5);

  top_sizer->Fit(this);
  SetSizer(top_sizer);
}

Control::~Control() {
  rasterizer.detach(this);
  if (editor != nullptr) {
    delete editor;
  }
  if (viewer != nullptr) {
    delete viewer;
  }
}

void
Control::update(Subject* subject)
{
  if (subject == editor) {
    editor = nullptr;
    return;
  }
  if (subject == viewer) {
    viewer = nullptr;
    return;
  }
  if (rasterizer.is_selected())
  {
    auto selected_object = rasterizer.get_selected_object();
    std::ostringstream oss;
    oss << "Object ID: " << selected_object;
    stxt_objectid->SetLabel(oss.str());
    stxt_objectid->Enable();
    oss.str("");
    oss << "Vertices: " << rasterizer.get_num_vertices(selected_object);
    stxt_vertices->SetLabel(oss.str());
    stxt_vertices->Enable();
    oss.str("");
    oss << "Keyframes: " << rasterizer.get_num_keyframes(selected_object);
    stxt_keyframe->SetLabel(oss.str());
    stxt_keyframe->Enable();
  }
  else
  {
    stxt_objectid->SetLabel(wxT("Object ID:"));
    stxt_objectid->Disable();
    stxt_vertices->SetLabel(wxT("Vertices:"));
    stxt_vertices->Disable();
    stxt_keyframe->SetLabel(wxT("Keyframes:"));
    stxt_keyframe->Disable();
  }
}

void Control::OnSpinFrame(wxSpinEvent& event)
{
  event.Skip();
  if (editor != nullptr && editor->IsShown()) {
    editor->setAnimationFrame(spin_delete_keyframe->GetValue());
    editor->Refresh();
  }
}

void
Control::OnButtonLoad(wxCommandEvent& event)
{
  if (text_filename->GetLineLength(0) == 0) {
    auto selected = wxLoadFileSelector("objects", "obs");
    if (selected.IsEmpty()) {
      return;
    }
    wxFileName filename(selected);
    wxFileName pathname(filename.GetPath(), filename.GetName());
    (*text_filename) << pathname.GetFullPath();
  }
  auto name = text_filename->GetLineText(0) + ".obs";
  if (rasterizer.load_objects(name.ToStdString())) {
    spin_delete_keyframe->Enable();
    if (editor != nullptr && editor->IsShown()) {
      update(&rasterizer);
      editor->setAnimationFrame(1);
      editor->Refresh();
    } else {
      if (editor == nullptr) {
        editor = new EditorFrame(rasterizer, wxID_ANY, editor_pos);
        editor->attach(this);
      }
      editor->Show();
    }
    if (viewer != nullptr && viewer->IsShown()) {
      viewer->Refresh();
    }
  }
}

void
Control::OnButtonSave(wxCommandEvent& event)
{
  if (text_filename->GetLineLength(0) == 0) {
    return;
  }
  auto name = text_filename->GetLineText(0) + ".obs";
  rasterizer.save_objects(name.ToStdString());
}

void
Control::OnCheckAA(wxCommandEvent& event)
{
  if (event.IsChecked()) {
    stxt_aa_nos->Enable();
    spin_aa_nos->Enable();
    stxt_aa_filter->Enable();
    text_aa_filter->Enable();
    anti_aliasing_enabled = true;
  } else {
    stxt_aa_nos->Disable();
    spin_aa_nos->Disable();
    stxt_aa_filter->Disable();
    text_aa_filter->Disable();
    anti_aliasing_enabled = false;
  }
}

void
Control::OnCheckMB(wxCommandEvent& event)
{
  if (event.IsChecked()) {
    stxt_mb_nos->Enable();
    spin_mb_nos->Enable();
    motion_blur_enabled = true;
  } else {
    stxt_mb_nos->Disable();
    spin_mb_nos->Disable();
    motion_blur_enabled = false;
  }
}

void
Control::OnRadioFrame(wxCommandEvent& event)
{
  if (event.GetInt() == 1) {
    stxt_sframe->Enable();
    text_sframe->Enable();
    stxt_eframe->Enable();
    text_eframe->Enable();
  } else {
    stxt_sframe->Disable();
    text_sframe->Disable();
    stxt_eframe->Disable();
    text_eframe->Disable();
  }
}

void
Control::OnButtonRender(wxCommandEvent& event)
{
  collectSettings();
  if (multiple_frames) {
    auto pathless = get_basename(render_filename);
    auto buf = render_filename + ".list";
    FILE* list_file = fopen(buf.c_str(), "w");
    assert(list_file != NULL);

    for (int i = first_frame; i <= final_frame; ++i) {
      rasterizer.rasterize(i,
                           anti_aliasing_enabled,
                           num_alias_samples,
                           motion_blur_enabled,
                           num_blur_samples,
                           aafilter_function);
      if (!render_filename.empty()) {
        std::ostringstream buf;
        buf << render_filename << "." << i << ".ppm";
        rasterizer.save_image(buf.str());
        fprintf(list_file, "%s.%d.ppm\n", pathless.c_str(), i);
      }
    }
    fclose(list_file);
  } else { // single frame
    rasterizer.rasterize(current_frame,
                         anti_aliasing_enabled,
                         num_alias_samples,
                         motion_blur_enabled,
                         num_blur_samples,
                         aafilter_function);
    if (!render_filename.empty()) {
      rasterizer.save_image(render_filename + ".ppm");
    }
  }
  if (viewer != nullptr && viewer->IsShown()) {
    viewer->Refresh();
  } else {
    if (viewer == nullptr) {
      viewer = new ViewerFrame(rasterizer, wxID_ANY, viewer_pos);
      viewer->attach(this);
    }
    viewer->Show();
  }
}

std::string Control::get_basename(const std::string& filename)
{
  auto pos = filename.find_last_of("/");
  if (pos == std::string::npos)
    return filename;
  return filename.substr(pos + 1);
}

void Control::collectSettings()
{
  if (text_renderto->GetLineLength(0)) {
    render_filename = text_renderto->GetLineText(0);
  }
  num_alias_samples = spin_aa_nos->GetValue();
  num_blur_samples = spin_mb_nos->GetValue();
}

#if 0
  void check_delete_keyframe_status()
  {
    if (current_frame != 1 && canvas->any_keyframe(current_frame))
      delete_keyframe_button->enable();
    else
      delete_keyframe_button->disable();
  }

  void myKeyboardFunc(unsigned char key, int, int)
  {
    bool post_redisplay = false;
    switch(key)
    {
      case 8:
      case 127: post_redisplay = canvas->delete_object(selected_object);
        selected_object = -1; break;
      case '.': frame_spinner->set_int_val(current_frame + 1); break;
      case ',': frame_spinner->set_int_val(current_frame - 1); break;
      default: break;
    }
    if (post_redisplay)
    {
      glutPostRedisplay();
    }
  }

  void myMotionFunc(int mx, int my)
  {
    if (canvas->motion(mx, my, current_frame, selected_object))
    {
      check_delete_keyframe_status();
      glutPostRedisplay();
    }
  }

  void myMouseFunc(int button, int state, int mx, int my)
  {
    if (canvas->mouse(button, state, mx, my, current_frame, selected_object))
    {
      update_info(selected_object);
      glutPostRedisplay();
    }
  }

  void FrameChangedCall(int)
  {
    check_delete_keyframe_status();
  }

  void DeleteKeyframeCall(int id)
  {
    canvas->delete_keyframe(id, current_frame);
    check_delete_keyframe_status();
  }
#endif
