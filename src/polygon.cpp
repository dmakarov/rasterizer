/**
   \file polygon.cpp

   Created by Dmitri Makarov on 16-08-19.
   Copyright © 2016 Dmitri Makarov. All rights reserved.
 */

#include "polygon.h"

#include <cassert>
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
  assert(frame >= 1.0f);
  auto kE = keyframes.cend();
  auto prevIt = kE, nextIt = kE;
  auto prevNum = 1.0f, nextNum = 1.0f;
  // there should always be a keyframe at frame 1
  for (auto i = static_cast<int>(frame); i > 0; --i) {
    if ((prevIt = findKeyframe(i)) != kE) {
      prevNum = i;
      break;
    }
  }
  for (auto i = static_cast<int>(frame + 1); i <= (kE - 1)->number; ++i) {
    if ((nextIt = findKeyframe(i)) != kE) {
      nextNum = i;
      break;
    }
  }
  auto p = prevIt->vertices.cbegin();
  auto E = prevIt->vertices.cend();
  auto s = prevIt->vertices.size();
  // if frame is a keyframe or there are no more keyframes, go with the last one
  if (frame == prevNum || nextIt == kE) {
    vertices.resize(s);
    std::copy(p, E, vertices.begin());
  } else { // do the interpolation
    auto r = (frame - prevNum) / (nextNum - prevNum);
    auto q = 1.0f - r;
    vertices.reserve(s);
    for (auto n = nextIt->vertices.cbegin(); p != E; ++p, (void)++n) {
      vertices.emplace_back(Point{q * p->x + r * n->x, q * p->y + r * n->y});
    }
  }
  return getColor();
}
