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
  EVT_CHAR(EditorCanvas::OnChar)
  EVT_LEFT_DOWN(EditorCanvas::OnMouseLeftDown)
  EVT_LEFT_UP(EditorCanvas::OnMouseLeftUp)
  EVT_RIGHT_DOWN(EditorCanvas::OnMouseRightDown)
  EVT_RIGHT_UP(EditorCanvas::OnMouseRightUp)
  EVT_MOTION(EditorCanvas::OnMouseMotion)
wxEND_EVENT_TABLE()

std::ostream& operator<<(std::ostream& os, const EditorCanvas::State& s)
{
  switch(s) {
  case EditorCanvas::State::NONE:     os << "NONE";     break;
  case EditorCanvas::State::CENTER:   os << "CENTER";   break;
  case EditorCanvas::State::DRAG:     os << "DRAG";     break;
  case EditorCanvas::State::DRAW:     os << "DRAW";     break;
  case EditorCanvas::State::MOVE:     os << "MOVE";     break;
  case EditorCanvas::State::ROTATE:   os << "ROTATE";   break;
  case EditorCanvas::State::SCALE:    os << "SCALE";    break;
  case EditorCanvas::State::SELECTED: os << "SELECTED"; break;
  default: os << "IMPOSSIBLE";
  }
  return os;
}

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

  if (state == State::CENTER ||
      state == State::ROTATE ||
      state == State::SCALE) {
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

void EditorCanvas::OnMouseLeftDown(wxMouseEvent& event)
{
  long x, y;
  event.GetPosition(&x, &y);
  switch (event.GetModifiers()) {
  case wxMOD_SHIFT:
    if (frame == 1) {
      scene.startDrawing(x, y);
      state = State::DRAW;
    }
    break;
  case wxMOD_CONTROL:
    if (state == State::CENTER) {
      state = State::ROTATE;
      scene.startRotatingOrScaling(x, y);
    } else if (scene.isSelected()) {
      state = State::CENTER;
      scene.setRotationOrScalingCenter(x, y);
    }
    break;
  case wxMOD_CONTROL | wxMOD_SHIFT:
    if (state == State::CENTER) {
      state = State::SCALE;
      scene.startRotatingOrScaling(x, y);
    } else if (scene.isSelected()) {
      state = State::CENTER;
      scene.setRotationOrScalingCenter(x, y);
    }
    break;
  default:
    if (state == State::SELECTED &&
        scene.isCloseToSelectedVertex(frame, x, y)) {
      state = State::DRAG;
    } else {
      // if there's a vertex in the area, select it
      scene.select(frame, x, y);
      if (scene.isSelected()) {
        state = State::SELECTED;
      } else {
        state = State::NONE;
      }
    }
  }
  paint();
  event.Skip();
}

void EditorCanvas::OnMouseLeftUp(wxMouseEvent& event)
{
  long x, y;
  event.GetPosition(&x, &y);
  switch (state) {
  case State::DRAW:
    scene.finishDrawing(x, y);
  case State::DRAG:
  case State::MOVE:
  case State::ROTATE:
  case State::SCALE:
    state = State::SELECTED;
    paint();
    break;
  default:;
  }
}

void EditorCanvas::OnMouseRightDown(wxMouseEvent& event)
{
  event.Skip();
}

void EditorCanvas::OnMouseRightUp(wxMouseEvent& event)
{
  event.Skip();
}

void EditorCanvas::OnMouseMotion(wxMouseEvent& event)
{
  if (event.LeftIsDown()) {
    long x, y;
    event.GetPosition(&x, &y);
    switch (state) {
    case State::DRAG:
      scene.drag(frame, x, y);
      break;
    case State::DRAW:
      scene.draw(x, y);
      break;
    case State::MOVE:
      scene.move(frame, x, y);
      break;
    case State::ROTATE:
      scene.rotate(frame, x, y);
      break;
    case State::SCALE:
      scene.scale(frame, x, y);
      break;
    case State::SELECTED:
      if (scene.isCloseToSelectedVertex(frame, x, y)) {
        state = State::DRAG;
        scene.drag(frame, x, y);
      }
      break;
    default: // nothing changed, do not paint
      return;
    }
    paint();
  }
}
