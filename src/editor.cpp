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
  EVT_MOUSE_EVENTS(EditorCanvas::OnMouse)
  EVT_CHAR(EditorCanvas::OnChar)
wxEND_EVENT_TABLE()

void EditorCanvas::paint()
{
  auto status = SetCurrent(*context);
  assert(status);
  int w, h;
  GetClientSize(&w, &h);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(0, w, -h, 0, 0, 1);
  glClearColor(0.f, 0.f, 0.f, 0.f);
  glClear(GL_COLOR_BUFFER_BIT);
  // add a red border if it is a keyframe
  if (scene.anyKeyframe(frame)) {
    glLineWidth(8);
    glColor3d(1, 0, 0);
    glBegin(GL_LINE_STRIP);
    {
      glVertex2d(    1,     -1);
      glVertex2d(    1, -h + 1);
      glVertex2d(w - 1, -h + 1);
      glVertex2d(w - 1,     -1);
      glVertex2d(    1,     -1);
    }
    glEnd();
  }
  glLineWidth(1);
  auto polygons = scene.getPolygons();
  auto selected = scene.getSelectedPolygon();
  for (auto& p : polygons) {
    std::vector<Point> vertices;
    auto color = p->getVertices(frame, vertices);
    glColor3d(color.get_red(), color.get_green(), color.get_blue());
    glLineWidth(selected == p ? 3 : 1);
    // draw the edges
    glBegin(GL_LINE_STRIP);
    {
      for (auto& v : vertices) {
        glVertex2d(v.x, -v.y);
      }
      glVertex2d(vertices[0].x, -vertices[0].y);
    }
    glEnd();
    // now draw the vertices on top of the lines
    if (selected == p) {
      auto v = scene.getSelectedVertex();
      glBegin(GL_LINES);
      {
        decltype(v) j = 0;
        for (auto& p : vertices) {
          glColor3d(0, 1, j++ == v ? 0 : 1);
          glVertex2d(p.x + 5, -p.y);
          glVertex2d(p.x - 5, -p.y);
          glVertex2d(p.x, -p.y + 5);
          glVertex2d(p.x, -p.y - 5);
        }
      }
      glEnd();
    }
  }

  if (state == State::ROTATE || state == State::SCALE) {
    glBegin(GL_LINES);
    {
      glColor3d(0.5, 0.5, 0.5);
      auto rc = scene.getCenter();
      auto x = static_cast<int>(rc.x);
      auto y = static_cast<int>(rc.y);
      glVertex2d(x + 5, -y);
      glVertex2d(x - 5, -y);
      glVertex2d(x, -y + 5);
      glVertex2d(x, -y - 5);
    }
    glEnd();
  } else if (state == State::DRAW) { // draw the object currently being created
    glLineWidth(3);
    // first draw the edges
    auto p = scene.getActivePolygon();
    const auto& c = p->getColor();
    glColor3d(c.get_red(), c.get_green(), c.get_blue());
    auto& vertices = p->keyframes[0].vertices;
    glBegin(GL_LINE_STRIP);
    {
      for (auto& v : vertices) {
        glVertex2d(v.x, -v.y);
      }
    }
    glEnd();
    // now draw the vertices
    glColor3d(0, 1, 1);
    glBegin(GL_LINES);
    {
      for (auto& v : vertices) {
        glVertex2d(v.x + 5, -v.y);
        glVertex2d(v.x - 5, -v.y);
        glVertex2d(v.x, -v.y + 5);
        glVertex2d(v.x, -v.y - 5);
      }
    }
    glEnd();
  }

  glFlush();
  status = SwapBuffers();
  assert(status);
}

void EditorCanvas::OnChar(wxKeyEvent& event)
{
  switch(event.GetUnicodeKey()) {
  case 8:   // FIXME what key is this?
  case 127: scene.deleteSelected(); break;
  case '.': if (frame < 99) parent->notify(++frame); break;
  case ',': if (frame >  1) parent->notify(--frame); break;
  default: break;
  }
  Refresh();
}

void EditorCanvas::OnMouse(wxMouseEvent& event)
{
  long x, y;
  event.GetPosition(&x, &y);

  if (event.ButtonUp(wxMOUSE_BTN_LEFT)) {
    switch (state) {
    case State::DRAW:
      scene.finishDrawing(x, y);
      state = State::NONE;
      break;
    case State::ROTATE:
      std::cout << "finish rotating at " << x << ", " << y << '\n';
      state = State::NONE;
      break;
    case State::SCALE:
      std::cout << "finish scaling at " << x << ", " << y << '\n';
      state = State::NONE;
      break;
    default:
      switch (event.GetModifiers()) {
      case wxMOD_CONTROL:
        if (scene.isSelected()) {
          scene.setRotationOrScalingCenter(x, y);
          state = State::ROTATE;
        }
        break;
      case wxMOD_CONTROL | wxMOD_SHIFT:
        if (scene.isSelected()) {
          scene.setRotationOrScalingCenter(x, y);
          state = State::SCALE;
        }
        break;
      default:
        scene.select(frame, x, y);
        state = State::NONE;
      }
      // if there's a vertex in the area, select it
    }
  } else if (event.ButtonUp(wxMOUSE_BTN_RIGHT)) {
    switch (state) {
    case State::ROTATE:
      std::cout << "finish rotating at " << x << ", " << y << '\n';
      state = State::NONE;
      break;
    case State::SCALE:
      std::cout << "finish scaling at " << x << ", " << y << '\n';
      state = State::NONE;
      break;
    default:
      switch (event.GetModifiers()) {
      case wxMOD_RAW_CONTROL:
        if (scene.isSelected()) {
          scene.setRotationOrScalingCenter(x, y);
          state = State::ROTATE;
        }
        break;
      case wxMOD_RAW_CONTROL | wxMOD_SHIFT:
        if (scene.isSelected()) {
          scene.setRotationOrScalingCenter(x, y);
          state = State::SCALE;
        }
        break;
      default:;
      }
    }
  } else if (!event.Dragging()) {
    if (event.ButtonDown(wxMOUSE_BTN_LEFT)) {
      switch (event.GetModifiers()) {
      case wxMOD_SHIFT:
        scene.startDrawing(x, y);
        state = State::DRAW;
        break;
      case wxMOD_CONTROL:
        if (state == State::ROTATE) {
          scene.startRotatingOrScaling(x, y);
        }
        break;
      case wxMOD_CONTROL | wxMOD_SHIFT:
        if (state == State::SCALE) {
          scene.startRotatingOrScaling(x, y);
        }
        break;
      default:
        state = State::DRAG;
        event.Skip();
        return;
      }
    } else if (event.ButtonDown(wxMOUSE_BTN_RIGHT)) {
      switch (event.GetModifiers()) {
      case wxMOD_RAW_CONTROL:
          if (state == State::ROTATE) {
          scene.startRotatingOrScaling(x, y);
        }
        break;
      case wxMOD_RAW_CONTROL | wxMOD_SHIFT:
        if (state == State::SCALE) {
          scene.startRotatingOrScaling(x, y);
        }
        break;
      default:
        event.Skip();
        return;
      }
    } else {
      event.Skip();
      return;
    }
  } else if (event.LeftIsDown()) {
    switch (state) {
    case State::DRAW:
      scene.draw(x, y);
      break;
    case State::ROTATE:
      scene.rotate(frame, x, y);
      break;
    case State::SCALE:
      scene.scale(frame, x, y);
      break;
    default:;
    }
  } else if (event.RightIsDown()) {
    switch (state) {
    case State::ROTATE:
      scene.rotate(frame, x, y);
      break;
    case State::SCALE:
      scene.scale(frame, x, y);
      break;
    default:;
    }
  }
  paint();
}
