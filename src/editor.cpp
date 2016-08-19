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

EditorFrame::EditorFrame(Rasterizer& rasterizer,
                         wxWindowID id,
                         const wxPoint& pos)
  : wxFrame(nullptr, id, wxT("Objects"), pos,
            wxSize(rasterizer.get_width(), rasterizer.get_height()),
            wxDEFAULT_FRAME_STYLE | wxFULL_REPAINT_ON_RESIZE)
{
  auto size = GetClientSize();
  panel = new wxPanel(this, wxID_ANY, wxDefaultPosition, size);
  canvas = new EditorCanvas(rasterizer, panel, wxID_ANY, nullptr,
                            wxDefaultPosition, size);
}

void EditorFrame::OnClose(wxCloseEvent& event)
{
  notify();
  Destroy();
}

EditorCanvas::EditorCanvas(Rasterizer&      rasterizer,
                           wxWindow*        parent,
                           wxWindowID       id,
                           const int*       attributes,
                           const wxPoint&   pos,
                           const wxSize&    size,
                           const long       style,
                           const wxString&  name,
                           const wxPalette& palette)
  : wxGLCanvas(parent, id, attributes, pos, size, style, name, palette)
  , rasterizer(rasterizer)
{
  animation_frame = 1;
  context = new wxGLContext(this);
}

void EditorCanvas::startDrawing(long x, long y)
{
  std::cout << "start drawing at " << x << ", " << y << '\n';
  active_object = std::make_shared<Animation>();
  active_object->keyframes.push_back(Frame());
  active_object->keyframes[0].number = 1;
  // assign_random_color(active_object);
  active_object->keyframes[0].vertices.push_back(Point{static_cast<float>(x), static_cast<float>(y)});
}

void EditorCanvas::startRotating(long x, long y)
{
  std::cout << "start rotating at " << x << ", " << y << '\n';
}

void EditorCanvas::startScaling(long x, long y)
{
  std::cout << "start scaling at " << x << ", " << y << '\n';
}

void EditorCanvas::continueDrawing(long x, long y)
{
  std::cout << "continue drawing at " << x << ", " << y << '\n';
  active_object->keyframes[0].vertices.push_back(Point{static_cast<float>(x), static_cast<float>(y)});
}

void EditorCanvas::continueRotating(long x, long y)
{
  std::cout << "continue rotating at " << x << ", " << y << '\n';
}

void EditorCanvas::continueScaling(long x, long y)
{
  std::cout << "continue scaling at " << x << ", " << y << '\n';
}

void EditorCanvas::finishDrawing(long x, long y)
{
  std::cout << "finish drawing at " << x << ", " << y << '\n';
  // if we're in the middle of drawing something, then end it
  if (active_object) {
    // if we don't have a polygon
    if (active_object->get_num_vertices() < 3) {
      active_object = nullptr;
    } else {
      rasterizer.add_object(active_object);
    }
    active_object = nullptr;
  }
}

void EditorCanvas::finishRotating(long x, long y)
{
  std::cout << "finish rotating at " << x << ", " << y << '\n';
}

void EditorCanvas::finishScaling(long x, long y)
{
  std::cout << "finish scaling at " << x << ", " << y << '\n';
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
  if (rasterizer.any_keyframe(animation_frame)) {
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
  auto objects = rasterizer.get_objects();
  auto selected_object = rasterizer.get_selected_object();
  for (decltype(objects.size()) obj = 0; obj < objects.size(); ++obj) {
    std::vector<Point> vertices;
    auto color = rasterizer.get_vertices(obj, animation_frame, vertices);
    glColor3d(color.get_red(), color.get_green(), color.get_blue());
    glLineWidth(selected_object == obj ? 3 : 1);
    // draw the edges
    glBegin(GL_LINE_STRIP);
    {
      for (auto& p : vertices) {
        glVertex2d(p.x, -p.y);
      };
      glVertex2d(vertices[0].x, -vertices[0].y);
    }
    glEnd();
    // now draw the vertices on top of the lines
    if (selected_object == obj) {
      auto v = rasterizer.get_selected_vertex();
      glBegin(GL_LINES);
      {
        decltype(vertices.size()) j = 0;
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

  if (scale_polygon || rotate_polygon) {
    glBegin(GL_LINES);
    {
      glColor3d(0.5, 0.5, 0.5);
      auto x = static_cast<int>(rotation_center.x);
      auto y = static_cast<int>(rotation_center.y);
      glVertex2d(x + 5, -y);
      glVertex2d(x - 5, -y);
      glVertex2d(x, -y + 5);
      glVertex2d(x, -y - 5);
    }
    glEnd();
  }
  // draw the object currently being created
  if (active_object) {
    glLineWidth(3);
    // first draw the edges
    const auto& c = active_object->get_color();
    glColor3d(c.get_red(), c.get_green(), c.get_blue());
    auto& v = active_object->keyframes[0].vertices;
    glBegin(GL_LINE_STRIP);
    {
      for (auto& p : v) {
        glVertex2d(p.x, -p.y);
      };
    }
    glEnd();
    // now draw the vertices
    glColor3d(0, 1, 1);
    glBegin(GL_LINES);
    {
      for (auto& p : v) {
          glVertex2d(p.x + 5, -p.y);
          glVertex2d(p.x - 5, -p.y);
          glVertex2d(p.x, -p.y + 5);
          glVertex2d(p.x, -p.y - 5);
      };
    }
    glEnd();
  }

  glFlush();
  status = SwapBuffers();
  assert(status);
}

void EditorCanvas::OnPaint(wxPaintEvent& event)
{
  paint();
  event.Skip();
}

#if 0

void Rasterizer::scale(int mx, int my, int frame)
{
  if (!is_object_selected) return;

  mx -= rotation_centerX;
  my -= rotation_centerY;

  if (std::abs(prev_rotationX - mx) < 10 && std::abs(prev_rotationY - my) < 10)
    return;

  float dm = sqrt(mx*mx + my*my);
  float dp = sqrt(prev_rotationX*prev_rotationX + prev_rotationY*prev_rotationY);

  auto& vert = find_keyframe(objects[selected_object], frame)->vertices;
  auto vertno = objects[selected_object]->get_num_vertices();

  float sx = (dm > dp) ? 1.2 : 0.8;
  float sy = (dm > dp) ? 1.2 : 0.8;

  for (decltype(vertno) ii = 0; ii < vertno; ++ii)
  {
    vert[ii].x -= rotation_centerX;
    vert[ii].y -= rotation_centerY;
    vert[ii].x = sx * vert[ii].x;
    vert[ii].y = sy * vert[ii].y;
    vert[ii].x += rotation_centerX;
    vert[ii].y += rotation_centerY;
  }
  prev_rotationX = mx;
  prev_rotationY = my;
}

void Rasterizer::rotate(int mx, int my, int frame)
{
  if (!is_object_selected) return;

  mx -= rotation_centerX;
  my -= rotation_centerY;

  if (std::abs(prev_rotationX - mx) < 10 && std::abs(prev_rotationY - my) < 10)
    return;
  if ((mx < 0 && prev_rotationX > 0) || (mx > 0 && prev_rotationX < 0))
  {
    prev_rotationX = mx;
    prev_rotationY = my;
    return;
  }

  float ro = sqrtf(mx*mx + my*my);
  float al = asinf(my / ro);
  ro = sqrtf(prev_rotationX*prev_rotationX + prev_rotationY*prev_rotationY);
  float be = asinf(prev_rotationY / ro);
  float sign = ((al > be && mx > 0) || (al < be && mx < 0)) ? 1.0 : -1.0;
  float te = sign * 3.14159 / 18;
  float sn = sinf(te);
  float cs = cosf(te);

  auto& vert = find_keyframe(objects[selected_object], frame)->vertices;
  auto vertno = objects[selected_object]->get_num_vertices();

  for (decltype(vertno) ii = 0; ii < vertno; ++ii)
  {
    vert[ii].x -= rotation_centerX;
    vert[ii].y -= rotation_centerY;
    float xx = cs * vert[ii].x - sn * vert[ii].y;
    float yy = sn * vert[ii].x + cs * vert[ii].y;
    vert[ii].x = xx + rotation_centerX;
    vert[ii].y = yy + rotation_centerY;
  }
  prev_rotationX = mx;
  prev_rotationY = my;
}

bool Rasterizer::move(float mx, float my, int frame)
{
  if (draw_curve)
  {
    auto px = active_object->keyframes[0].vertices.back().x;
    auto py = active_object->keyframes[0].vertices.back().y;
    if (std::abs(px - mx) > 7 || std::abs(py - my) > 7)
    { // sqrt((px-mx)*(px-mx) + (py-my)*(py-my)) > 5)
      active_object->keyframes[0].vertices.push_back(Point{mx, my});
    }
    return true;
  }

  if (!is_object_selected) {
    return false;
  }
  // look for a keyframe at this frame
  // if we don't find it, then create a new one in the right place
  auto keyframe = find_keyframe(objects[selected_object], frame);
  if (keyframe == objects[selected_object]->keyframes.end()) {
    std::vector<Point> vertices;
    get_vertices(selected_object, frame, vertices);

    decltype(get_num_keyframes(selected_object)) insertLoc;
    for (insertLoc = 0; insertLoc < get_num_keyframes(selected_object); ++insertLoc)
      if (frame < objects[selected_object]->keyframes[insertLoc].number)
        break;

    objects[selected_object]->keyframes.push_back(objects[selected_object]->keyframes.back());
    for (auto i = get_num_keyframes(selected_object) - 2; i >= insertLoc; --i)
      std::copy(objects[selected_object]->keyframes.begin() + i,
                objects[selected_object]->keyframes.begin() + i + 1,
                objects[selected_object]->keyframes.begin() + i + 1);

    std::copy(vertices.begin(), vertices.end(),
              objects[selected_object]->keyframes[insertLoc].vertices.begin());

    objects[selected_object]->keyframes[insertLoc].number = frame;
    keyframe = find_keyframe(objects[selected_object], frame);
    assert(keyframe != objects[selected_object]->keyframes.end());
  }

  if (scale_polygon) {
    scale(mx, my, frame);
    return true;
  }
  if (rotate_polygon) {
    rotate(mx, my, frame);
    return true;
  }

  rotation_centerX = -1;

  if (!is_object_selected) {
    keyframe->vertices[selected_vertex].x = mx;
    keyframe->vertices[selected_vertex].y = my;
  }
  else {
    for (auto& v : keyframe->vertices) {
      v.x += (mx - original.x);
      v.y += (my - original.y);
    }
    original = Point{mx, my};
  }
  return true;
}
#endif

void EditorCanvas::OnMouse(wxMouseEvent& event)
{
  long mx, my;
  event.GetPosition(&mx, &my);
  float x = mx, y = my;

   if (event.ButtonDown(wxMOUSE_BTN_LEFT) && !event.Dragging()) {
    switch (event.GetModifiers()) {
    case wxMOD_SHIFT:                 // draw
      startDrawing(mx, my);
      break;
    case wxMOD_CONTROL:               // rotate
      if (rasterizer.is_selected()) {
        startRotating(mx, my);
      }
      break;
    case wxMOD_CONTROL | wxMOD_SHIFT: // scale
      if (rasterizer.is_selected()) {
        startScaling(mx, my);
      }
      break;
    default:                          // select
      event.Skip();
    }
  } else if (event.ButtonUp(wxMOUSE_BTN_LEFT)) {
    switch (event.GetModifiers()) {
    case wxMOD_SHIFT:                 // draw
      finishDrawing(mx, my);
      break;
    case wxMOD_CONTROL:               // rotate
      if (rasterizer.is_selected()) {
        finishRotating(mx, my);
      }
      break;
    case wxMOD_CONTROL | wxMOD_SHIFT: // scale
      if (rasterizer.is_selected()) {
        finishScaling(mx, my);
      }
      break;
    default:                          // select
      // if there's a vertex in the area, select it
      rasterizer.select_object(animation_frame, x, y);
    }
  } else if (!event.Dragging() && !event.ButtonUp()) {
    event.Skip();
    return;
  }

  switch (event.GetModifiers()) {
  case wxMOD_SHIFT:
    continueDrawing(mx, my);
    break;
  case wxMOD_CONTROL:
    if (rasterizer.is_selected()) {
      continueRotating(mx, my);
    }
    break;
  case wxMOD_CONTROL | wxMOD_SHIFT:
    if (rasterizer.is_selected()) {
      continueScaling(mx, my);
    }
    break;
  default:;
  }
  paint();
}

void EditorCanvas::OnChar(wxKeyEvent& event)
{
  switch(event.GetUnicodeKey())
  {
  case 8:   // FIXME what key is this?
  case 127: rasterizer.delete_selected_object(); break;
  case '.': if (animation_frame < 99) ++animation_frame; break;
  case ',': if (animation_frame >  1) --animation_frame; break;
  default: break;
  }
  Refresh();
}
