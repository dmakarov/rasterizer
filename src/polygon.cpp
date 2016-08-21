//
//  polygon.cpp
//  rasterizer
//
//  Created by Dmitri Makarov on 16-08-19.
//
//

#include "polygon.h"

#include <ostream>

std::ostream& operator<<(std::ostream& os, const RGB8& p)
{
  os << std::hex << p.pixel;
  return os;
}

std::ostream& operator<<(std::ostream& os, const RGB32& p)
{
  os << '(' << p.r << ',' << p.g << ',' << p.b << ')';
  return os;
}

std::ostream& operator<<(std::ostream& os, const Point& p)
{
  os << '(' << p.x << ", " << p.y << ')';
  return os;
}

std::ostream& operator<<(std::ostream& os, const Polygon& p)
{
  os << "vertices " << p.getNumVertices()
     << ", frames " << p.keyframes.size()
     << ", color "  << p.getColor();
  return os;
}

/**
   \returns the set of vertices in the frame, probably interpolated.
 */
RGB8 Polygon::getVertices(const float frame, std::vector<Point>& vertices)
{
  auto E = keyframes.end();
  auto prev_frame = E, next_frame = E;
  auto prev_keyframe = 1.0f, next_keyframe = 1.0f;
  for (auto i = static_cast<int>(frame); i > 0; --i) {
    if ((prev_frame = findKeyframe(i)) != E) {
      prev_keyframe = i;
      break;
    }
  }
  // there should always be a keyframe at frame 1
  if (prev_frame == E) {
    prev_frame = keyframes.begin();
  }
  for (auto i = static_cast<int>(frame + 1); i <= (E - 1)->number; ++i) {
    if ((next_frame = findKeyframe(i)) != E) {
      next_keyframe = i;
      break;
    }
  }
  auto p = prev_frame->vertices.begin();
  auto pE = prev_frame->vertices.end();
  // if there are no more keyframes, just go with the last keyframe
  if (next_frame == E) {
    vertices.resize(prev_frame->vertices.size());
    std::copy(p, pE, vertices.begin());
  } else { // here we do the interpolation
    auto r = (frame - prev_keyframe) / (next_keyframe - prev_keyframe);
    auto q = 1.0f - r;
    for (auto n = next_frame->vertices.begin(); p != pE; ++p, (void)++n) {
      vertices.emplace_back(Point{q * p->x + r * n->x, q * p->y + r * n->y});
    }
  }
  return getColor();
}
