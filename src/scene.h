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
  using PolySP = std::shared_ptr<Polygon>;
  using PolySz = std::vector<PolySP>::size_type;

  std::vector<PolySP> polygons;
  PolySP active;
  PolySP selected;
  PolySz selectedID;
  PolySz selectedVertex;
  Point center;
  Point previous;
  Rasterizer rasterizer;
  int width, height;

public:

  Scene(int w = 500, int h = 500) : rasterizer{w, h}, width(w), height(h)
  {}

  const std::vector<PolySP>& getPolygons() const {
    return polygons;
  }

  PolySP getActivePolygon() const {
    return active;
  }

  PolySP getSelectedPolygon() const {
    return selected;
  }

  PolySz getSelectedPolygonId() const {
    return selectedID;
  }

  PolySz getSelectedVertex() const {
    return selectedVertex;
  }

  bool isSelected() {
    return selected != nullptr;
  }

  const Point& getCenter() const {
    return center;
  }

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

  void deleteSelected() {
    if (selected == nullptr) {
      return;
    }
    polygons.erase(polygons.begin() + selectedID);
    selected = nullptr;
    notify();
  }

  /**
   \returns false if no object has keyframe at <frame> and true otherwise.
   */
  bool anyKeyframe(int frame) const {
    auto E = polygons.end();
    return E != std::find_if(polygons.begin(), E, [frame](const PolySP& a)
                { return a->findKeyframe(frame) != a->keyframes.end(); });
  }

  void deleteKeyframe(int frame) {
    if (frame == 1 || !anyKeyframe(frame)) {
      return;
    }
    for (auto p : polygons) {
      auto k = p->findKeyframe(frame);
      auto E = p->keyframes.end();
      if (k != E) {
        for (auto j = k; j != E - 1; ++j) {
          std::copy(j + 1, j + 2, j);
        }
        p->keyframes.pop_back();
      }
    }
  }

  void render(int frame,
              bool aa_enabled, int num_aa_samples,
              bool mb_enabled, int num_mb_samples,
              const std::string& aa_filter,
              const std::string& filename) {
    rasterizer.run(polygons, frame, aa_enabled, num_aa_samples,
                   mb_enabled, num_mb_samples, aa_filter);
    if (filename != "") {
      rasterizer.save(filename);
    }
  }

  bool load(const std::string& filename);
  void save(const std::string& filename) const;
  void renderToFile(const std::vector<std::string>& args);

  void setRotationOrScalingCenter(const long x, const long y);
  void startRotatingOrScaling(const long x, const long y);
  void startDrawing(const long x, const long y);
  void finishDrawing(const long x, const long y);
  void select(const int frame, const long x, const long y);
  void rotate(const int frame, const long x, const long y);
  void scale(const int frame, const long x, const long y);
  void move(const int frame, const long x, const long y);
  void draw(const long x, const long y);

};

#endif /* scene_h */

// Local Variables:
// mode: c++
// End:
