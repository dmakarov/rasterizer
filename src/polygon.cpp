//
//  polygon.cpp
//  rasterizer
//
//  Created by Dmitri Makarov on 16-08-19.
//
//

#include "polygon.h"

#include <ostream>

std::ostream& operator<<(std::ostream& os, const RGB8& obj)
{
  os << std::hex << obj.pixel;
  return os;
}

std::ostream& operator<<(std::ostream& os, const RGB32& obj)
{
  os << '(' << obj.r << ',' << obj.g << ',' << obj.b << ')';
  return os;
}

std::ostream& operator<<(std::ostream& os, const Polygon& obj)
{
  os << "vertices " << obj.get_num_vertices()
  << ", frames " << obj.keyframes.size()
  << ", color " << obj.get_color();
  return os;
}

/**
 *  This function returns the set of vertices for the passed object in
 *  the current frame
 */
RGB8 Polygon::get_vertices(const float frame, std::vector<Point>& vertices)
{
  auto prev_keyframe = -1.0f, next_keyframe = -1.0f;
  auto E = keyframes.end();
  auto prev_frame = E, next_frame = E;

  for (auto i = static_cast<int>(frame); i >= 0; --i) {
    if ((prev_frame = find_keyframe(i)) != E) {
      prev_keyframe = i;
      break;
    }
  }
  // there should always be a keyframe at frame 1
  if (prev_frame == E) {
    prev_frame = keyframes.begin() + 1;
  }
  for (auto i = static_cast<int>(frame + 1); i <= (E - 1)->number; ++i) {
    if ((next_frame = find_keyframe(i)) != E) {
      next_keyframe = i;
      break;
    }
  }
  auto& prev_keyframe_vertices = prev_frame->vertices;
  // if there are no more keyframes, just go with the last frame
  if (next_frame == keyframes.end()) {
    vertices.resize(prev_keyframe_vertices.size());
    std::copy(prev_keyframe_vertices.begin(), prev_keyframe_vertices.end(),
              vertices.begin());
  } else { // here we do the interpolation
    auto& next_keyframe_vertices = next_frame->vertices;
    auto percent = (frame - prev_keyframe) / (next_keyframe - prev_keyframe);
    for (std::vector<Point>::size_type i = 0; i < get_num_vertices(); ++i) {
      auto x = (1 - percent) * prev_keyframe_vertices[i].x
                  + percent  * next_keyframe_vertices[i].x;
      auto y = (1 - percent) * prev_keyframe_vertices[i].y
                  + percent  * next_keyframe_vertices[i].y;
      vertices.emplace_back(Point{x, y});
    }
  }
  return get_color();
}
