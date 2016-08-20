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
  std::vector<std::shared_ptr<Polygon>> polygons;
  std::shared_ptr<Polygon> selected_object;
  ObjSz selected_object_id;
  ObjSz selected_vertex;
  bool is_object_selected = false;

  Scene() : rasterizer{500, 500} {
  }

  Rasterizer& getRasterizer() {
    return rasterizer;
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

  const std::vector<ObjSP>& get_objects() const
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
  bool any_keyframe(int frame) const
  {
    auto E = polygons.end();
    return E !=
    std::find_if(polygons.begin(), E, [this, frame] (const ObjSP& a) {
      return a->find_keyframe(frame) != a->keyframes.end(); });
  }

  std::shared_ptr<Polygon> get_selected_object() const
  {
    return selected_object;
  }

  ObjSz get_selected_object_id() const
  {
    return selected_object_id;
  }

  ObjSz get_selected_vertex() const
  {
    return selected_vertex;
  }

  bool is_selected() const
  {
    return is_object_selected;
  }

  void delete_selected_object()
  {
    if (!is_object_selected) {
      return;
    }
    polygons.erase(polygons.begin() + selected_object_id);
    is_object_selected = false;
    notify();
  }

  void delete_keyframe(int frame)
  {
    if (frame == 1 || !any_keyframe(frame)) {
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

};

#endif /* scene_h */
