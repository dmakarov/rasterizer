//
//  editor.cpp
//  rasterizer
//
//  Created by Dmitri Makarov on 16-06-26.
//
//

#include "editor.h"

wxBEGIN_EVENT_TABLE(EditorFrame, wxWindow)
  EVT_CLOSE(EditorFrame::OnClose)
wxEND_EVENT_TABLE()

wxBEGIN_EVENT_TABLE(EditorCanvas, wxWindow)
  EVT_PAINT(EditorCanvas::OnPaint)
wxEND_EVENT_TABLE()

EditorFrame::EditorFrame(Rasterizer& rasterizer) : wxFrame(nullptr, wxID_ANY, wxT("Objects"), wxDefaultPosition, wxSize(500, 500), wxDEFAULT_FRAME_STYLE | wxFULL_REPAINT_ON_RESIZE)
{
  panel = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxSize(500, 500));
  canvas = new EditorCanvas(rasterizer, panel, wxID_ANY, nullptr, wxDefaultPosition, wxSize(500, 500));
}

void
EditorFrame::OnClose(wxCloseEvent& event)
{
  this->Destroy();
}

EditorCanvas::EditorCanvas(Rasterizer& rasterizer, wxWindow* parent, wxWindowID id, const int* attributes, const wxPoint& pos, const wxSize& size, long style, const wxString& name, const wxPalette& palette) : wxGLCanvas(parent, id, attributes, pos, size, style, name, palette), rasterizer(rasterizer)
{
  animation_frame = 0;
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
    glLineWidth(20);
    glColor3d(1, 0, 0);
    glBegin(GL_LINE_STRIP);
    {
      glVertex2d(0, 0);
      glVertex2d(0, -rasterizer.get_height());
      glVertex2d(rasterizer.get_width(), -rasterizer.get_height());
      glVertex2d(rasterizer.get_width(), 0);
      glVertex2d(0, 0);
    }
    glEnd();
  }

  glLineWidth(1);
  auto objects = rasterizer.get_objects();
  for (int i = 0; i < objects.size(); ++i) {
    auto color = objects[i]->get_color();
    glColor3d(color.get_red(), color.get_green(), color.get_blue());
    glLineWidth(selected_object == i ? 3 : 1);
    //we draw the edges
    auto& vertices = objects[i]->get_vertices(animation_frame);
    glBegin(GL_LINE_STRIP);
    {
      for (auto& p : vertices) {
        glVertex2d(p.x, -p.y);
      }
      glVertex2d(vertices[0].x, -vertices[0].y);
    }
    glEnd();

    if (selected_object == i) {//now draw the vertices on top of the lines
      glBegin(GL_LINES);
      {
        for (int j = 0; j < objects[i]->numVertices; ++j) {
          // selected vertex?
          if (j == selected_vertex) {
            glColor3d(0, 1, 0);
          } else {
            glColor3d(0, 1, 1);
          }
          glVertex2d(vertices[j].x+5, -vertices[j].y);
          glVertex2d(vertices[j].x-5, -vertices[j].y);
          glVertex2d(vertices[j].x, -vertices[j].y+5);
          glVertex2d(vertices[j].x, -vertices[j].y-5);
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

  // this is the loop where we draw the object currently being created
  if (active_object) {
    glLineWidth(3);
    //first we draw the edges
    auto color = active_object->get_color();
    glColor3d(color.get_red(), color.get_green(), color.get_blue());
    glBegin(GL_LINE_STRIP);
    {
      for (auto& p : active_object->keyframes[0].vertices) {
        glVertex2d(p.x, -p.y);
      }
    }
    glEnd();

    //then we draw the vertices
    glColor3d(0, 1, 1);
    glBegin(GL_LINES);
    {
      for (auto& p : active_object->keyframes[0].vertices) {
        glVertex2d(p.x + 5, -p.y);
        glVertex2d(p.x - 5, -p.y);
        glVertex2d(p.x, -p.y + 5);
        glVertex2d(p.x, -p.y - 5);
      }
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

#if 0
bool
EditorCanvas::mouse(int button, int state, int mx, int my, int animation_frame, int& selected_object)
{
  int modifier = glutGetModifiers();

  if (state == GLUT_UP)
  {
    rotate_polygon = false;
    scale_polygon = false;
    draw_curve = false;
    return true;
  }

  switch (modifier)
  {
    case GLUT_ACTIVE_SHIFT:
    {
      //clear the selection

      selected_object = -1;
      selected_vertex = -1;

      //create a new object if one isn't being drawn

      if (active_object == nullptr)
      {
        active_object = new AnimObject;
        active_object->numVertices = 0;
        active_object->numKeyframes = 1;
        active_object->keyframes[0].frameNumber = 1;
        assign_random_color(active_object);
      }
      if (active_object->numVertices == MAX_VERTICES) return false;
      active_object->keyframes[0].vertices[active_object->numVertices].x = mx;
      active_object->keyframes[0].vertices[active_object->numVertices].y = my;
      active_object->numVertices++;
      draw_curve = true;
    }
      rotation_centerX = -1;
      break;
    case GLUT_ACTIVE_CTRL:
    {
      if (select_object(button, mx, my, animation_frame, selected_object)
          && (rotation_centerX != -1))
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
    }
      break;
    case GLUT_ACTIVE_CTRL | GLUT_ACTIVE_SHIFT:
    {
      if (select_object(button, mx, my, animation_frame, selected_object)
          && (rotation_centerX != -1))
      {
        scale_polygon = true;
        rotate_polygon = false;
        prev_rotationX = mx - rotation_centerX;
        prev_rotationY = my - rotation_centerY;
      }
    }
      break;
    default:
    {
      // if we're in the middle of drawing something, then end it
      if (active_object)
      {
        //if we don't have a polygon

        if (active_object->numVertices < 3)
        {
          delete active_object;
        }
        else
        {
          objects.push_back(active_object);
        }
        active_object = nullptr;
      }
      // if there's a vertex in the area, select it
      if (!select_object(button, mx, my, animation_frame, selected_object))
      {
        selected_object = -1;
        selected_vertex = -1;
      }
    }
      rotation_centerX = -1;
  }
  return true;
}
#endif
