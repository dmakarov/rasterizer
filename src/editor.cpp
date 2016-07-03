/**
   \file editor.cpp

   Created by Dmitri Makarov on 16-06-26.
   Copyright © 2016 Dmitri Makarov. All rights reserved.
 */

#include "editor.h"

wxBEGIN_EVENT_TABLE(EditorFrame, wxWindow)
  EVT_CLOSE(EditorFrame::OnClose)
wxEND_EVENT_TABLE()

wxBEGIN_EVENT_TABLE(EditorCanvas, wxWindow)
  EVT_PAINT(EditorCanvas::OnPaint)
wxEND_EVENT_TABLE()

EditorFrame::EditorFrame(Rasterizer& rasterizer)
  : wxFrame(nullptr, wxID_ANY, wxT("Objects"),
            wxDefaultPosition, wxSize(500, 500),
            wxDEFAULT_FRAME_STYLE | wxFULL_REPAINT_ON_RESIZE)
{
  panel = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxSize(500, 500));
  canvas = new EditorCanvas(rasterizer, panel, wxID_ANY, nullptr, wxDefaultPosition, wxSize(500, 500));
}

void
EditorFrame::OnClose(wxCloseEvent& event)
{
  this->Destroy();
}

EditorCanvas::EditorCanvas(Rasterizer&      rasterizer,
                           wxWindow*        parent,
                           wxWindowID       id,
                           const int*       attributes,
                           const wxPoint&   pos,
                           const wxSize&    size,
                           long             style,
                           const wxString&  name,
                           const wxPalette& palette)
  : wxGLCanvas(parent, id, attributes, pos, size, style, name, palette)
  , rasterizer(rasterizer)
{
  animation_frame = 1;
  selected_object = -1;
  selected_vertex = -1;
  rotation_centerX = -1;
  rotation_centerY = -1;

  context = new wxGLContext(this);
  this->SetCurrent(*context);

  int w, h;
  this->GetClientSize(&w, &h);
  glOrtho(0, w, -h, 0, -1, 1);
}

void
EditorCanvas::draw()
{
  wxClientDC dc(this);
  this->paint(dc);
}

void
EditorCanvas::paint(const wxDC& dc)
{
  this->SetCurrent(*context);
  glClearColor(0.f, 0.f, 0.f, 0.f);
  glClear(GL_COLOR_BUFFER_BIT);
  if (rasterizer.any_keyframe(animation_frame)) {
    // add a red border if it is a keyframe
    int w, h;
    this->GetClientSize(&w, &h);
    glLineWidth(10);
    glColor3d(1, 0, 0);
    glBegin(GL_LINE_STRIP);
    {
      glVertex2d(0, 0);
      glVertex2d(0, -h);
      glVertex2d(w, -h);
      glVertex2d(w, 0);
      glVertex2d(0, 0);
    }
    glEnd();
  }
  glLineWidth(1);
  auto objects = rasterizer.get_objects();
  for (int obj = 0; obj < objects.size(); ++obj) {
    std::vector<Point> vertices;
    auto color = rasterizer.get_vertices(obj, animation_frame, vertices);
    glColor3d(color.get_red(), color.get_green(), color.get_blue());
    glLineWidth(selected_object == obj ? 3 : 1);
    // draw the edges
    glBegin(GL_LINE_STRIP);
    {
      std::for_each(vertices.begin(), vertices.end(), [](Point& p){ glVertex2d(p.x, -p.y); });
      glVertex2d(vertices[0].x, -vertices[0].y);
    }
    glEnd();
    // now draw the vertices on top of the lines
    if (selected_object == obj) {
      glBegin(GL_LINES);
      {
        for (int j = 0; j < vertices.size(); ++j) {
          glColor3d(0, 1, j == selected_vertex ? 0 : 1);
          glVertex2d(vertices[j].x + 5, -vertices[j].y);
          glVertex2d(vertices[j].x - 5, -vertices[j].y);
          glVertex2d(vertices[j].x, -vertices[j].y + 5);
          glVertex2d(vertices[j].x, -vertices[j].y - 5);
        }
      }
      glEnd();
    }
  }

  if (rotation_centerX > -1) {
    glBegin(GL_LINES);
    {
      glColor3d(0.5, 0.5, 0.5);
      glVertex2d(rotation_centerX + 5, - rotation_centerY);
      glVertex2d(rotation_centerX - 5, -rotation_centerY);
      glVertex2d(rotation_centerX, -rotation_centerY + 5);
      glVertex2d(rotation_centerX, -rotation_centerY - 5);
    }
    glEnd();
  }
  // draw the object currently being created
  if (active_object) {
    glLineWidth(3);
    // first draw the edges
    auto color = active_object->get_color();
    glColor3d(color.get_red(), color.get_green(), color.get_blue());
    auto& v = active_object->keyframes[0].vertices;
    glBegin(GL_LINE_STRIP);
    {
      std::for_each(v.begin(), v.end(), [](Point& p) { glVertex2d(p.x, -p.y); });
    }
    glEnd();
    // now draw the vertices
    glColor3d(0, 1, 1);
    glBegin(GL_LINES);
    {
      std::for_each(v.begin(), v.end(), [](Point& p) {
          glVertex2d(p.x + 5, -p.y);
          glVertex2d(p.x - 5, -p.y);
          glVertex2d(p.x, -p.y + 5);
          glVertex2d(p.x, -p.y - 5);
        });
    }
    glEnd();
    glLineWidth(1);
  }
  glFlush();
  SwapBuffers();
}

void
EditorCanvas::OnPaint(wxPaintEvent& event)
{
  wxPaintDC dc(this);
  paint(dc);
  event.Skip();
}

void
EditorCanvas::OnMouseMove(wxMouseEvent& event)
{
  long mx, my;
  event.GetPosition(&mx, &my);
  float x = mx, y = my;
  switch (event.GetModifiers())
  {
  case wxMOD_SHIFT:
    // clear the selection
    selected_object = -1;
    selected_vertex = -1;
    // create a new object if one isn't being drawn
    if (active_object == nullptr)
    {
      active_object.make_shared();
      active_object->keyframes.push_back(Frame());
      active_object->keyframes[0].number = 1;
      //assign_random_color(active_object);
    }
    active_object->keyframes[0].vertices.push_back(Point{x, y});
    draw_curve = true;
    rotation_centerX = -1;
    break;
  case wxMOD_CONTROL:
    if (rasterizer.select_object(mx, my, animation_frame, false, selected_object) && (rotation_centerX != -1))
    {
      scale_polygon = false;
      rotate_polygon = true;
      prev_rotationX = mx - rotation_centerX;
      prev_rotationY = my - rotation_centerY;
    }
    else
    {
      rotation_centerX = mx;
      rotation_centerY = my;
    }
    break;
  case wxMOD_CONTROL | wxMOD_SHIFT:
    if (rasterizer.select_object(mx, my, animation_frame, false, selected_object) && (rotation_centerX != -1))
    {
      scale_polygon = true;
      rotate_polygon = false;
      prev_rotationX = mx - rotation_centerX;
      prev_rotationY = my - rotation_centerY;
    }
    break;
  default:
    // if we're in the middle of drawing something, then end it
    if (active_object)
    {
      //if we don't have a polygon

      if (active_object->get_num_vertices() < 3)
      {
        active_object = nullptr;
      }
      else
      {
        rasterizer.add_object(active_object);
      }
      active_object = nullptr;
    }
    // if there's a vertex in the area, select it
    if (!rasterizer.select_object(mx, my, animation_frame, false, selected_object))
    {
      selected_object = -1;
      selected_vertex = -1;
    }
    rotation_centerX = -1;
  }
}

void
EditorCanvas::OnLeftMouse(wxMouseEvent& event)
{
}

void
EditorCanvas::OnLeftMouseUp(wxMouseEvent& event)
{
  rotate_polygon = false;
  scale_polygon = false;
  draw_curve = false;
}

void
EditorCanvas::OnRightMouse(wxMouseEvent& event)
{
}

void
EditorCanvas::OnRightMouseUp(wxMouseEvent& event)
{
}
