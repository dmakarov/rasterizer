/**
   \file scene.h

   Created by Dmitri Makarov on 16-08-19.
   Copyright © 2016 Dmitri Makarov. All rights reserved.
*/

#ifndef scene_h
#define scene_h

#include "observer.h"
#include "polygon.h"
#include "rasterizer.h"

#include <algorithm>
#include <memory>
#include <vector>

/**
   \class manages the objects to be rendered.
 */
class Scene : public Subject {
public:
  using ObjSz = std::vector<std::shared_ptr<Polygon>>::size_type;
  using ObjSP = std::shared_ptr<Polygon>;

  Rasterizer rasterizer;
  int width, height;
  std::vector<std::shared_ptr<Polygon>> polygons;
  std::shared_ptr<Polygon> selected_object;
  ObjSz selected_object_id;
  ObjSz selected_vertex;
  std::shared_ptr<Polygon> active_object;
  Point prev_rotation_center;
  Point rotation_center;

  Scene(int w = 500, int h = 500)
  : rasterizer{w, h}, width(w), height(h) {}

  void resize(int w, int h) {
    rasterizer.resize(w, h);
    width = w;
    height = h;
  }

  Rasterizer& getRasterizer() {
    return rasterizer;
  }

  int getWidth() const {
    return width;
  }

  int getHeight() const {
    return height;
  }

  void render(int frame,
              bool aa_enabled, int num_aa_samples,
              bool mb_enabled, int num_mb_samples,
              const std::string& aa_filter,
              const std::string& filename)
  {
    rasterizer.run(polygons, frame, aa_enabled, num_aa_samples,
                   mb_enabled, num_mb_samples, aa_filter);
    if (filename != "") {
      rasterizer.save(filename);
    }
  }

  const std::vector<ObjSP>& getPolygons() const
  {
    return polygons;
  }

  void add_object(ObjSP& object)
  {
    polygons.push_back(object);
  }

  /**
   \returns false if no object has keyframe at <frame> and true otherwise.
   */
  bool anyKeyframe(int frame) const
  {
    auto E = polygons.end();
    return E != std::find_if(polygons.begin(), E, [frame](const ObjSP& a)
                { return a->find_keyframe(frame) != a->keyframes.end(); });
  }

  ObjSP getSelectedPolygon() const
  {
    return selected_object;
  }

  ObjSP getActivePolygon() const
  {
    return active_object;
  }

  Point getRotationCenter() const
  {
    return rotation_center;
  }

  ObjSz getSelectedPolygonId() const
  {
    return selected_object_id;
  }

  ObjSz get_selected_vertex() const
  {
    return selected_vertex;
  }

  bool is_selected()
  {
    return selected_object != nullptr;
  }

  void delete_selected_object()
  {
    if (selected_object == nullptr) {
      return;
    }
    polygons.erase(polygons.begin() + selected_object_id);
    selected_object = nullptr;
    notify();
  }

  void delete_keyframe(int frame)
  {
    if (frame == 1 || !anyKeyframe(frame)) {
      return;
    }
    for (auto p : polygons) {
      auto k = p->find_keyframe(frame);
      auto E = p->keyframes.end();
      if (k != E) {
        for (auto j = k; j != E - 1; ++j) {
          std::copy(j + 1, j + 2, j);
        }
        p->keyframes.pop_back();
      }
    }
  }

  bool load(const std::string& filename);
  void save(const std::string& filename) const;
  bool select_object(const int frame, const float mx, const float my);
  void render_to_file(const std::vector<std::string>& args);

  void startDrawing(long x, long y);
  void startRotating(long x, long y);
  void startScaling(long x, long y);
  void continueDrawing(long x, long y);
  void continueRotating(long x, long y);
  void continueScaling(long x, long y);
  void finishDrawing(long x, long y);
  void finishRotating(long x, long y);
  void finishScaling(long x, long y);

};

#endif /* scene_h */
