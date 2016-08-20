//
//  scene.cpp
//  rasterizer
//
//  Created by Dmitri Makarov on 16-08-19.
//
//

#include "scene.h"

#include <cassert>
#include <cmath>
#include <fstream>
#include <ostream>
#include <sstream>
#include <string>
#include <iostream>

bool Scene::select_object(int frame, float mx, float my)
{
  bool foundVertex = false;
  selected_object = nullptr;
  for (ObjSz i = 0; !foundVertex && i < polygons.size(); ++i) {
    std::vector<Point> vertices;
    polygons[i]->get_vertices(frame, vertices);
    auto j = 0;
    for (auto& v : vertices) {
      // check for proximity
      if (std::fabs(v.x - mx) < 5 && std::fabs(v.y - my) < 5) {
        foundVertex = true;
        selected_object_id = i;
        selected_object = polygons[i];
        selected_vertex = j;
        break;
      }
      ++j;
    }
  }
  notify();
  return foundVertex;
}

bool Scene::load(const std::string& filename)
{
  // check if there's something in the filename field
  std::ifstream infile(filename);
  if (!infile)
  {
    std::cerr << "Can't load file " << filename << "\n";
    return false;
  }
  polygons.clear();

  int num_of_objects;
  std::string line;
  std::getline(infile, line);
  std::istringstream iss(line.substr(line.find_last_of(":") + 1));
  iss >> num_of_objects;

  for (int i = 0; i < num_of_objects; ++i)
  {
    int num_vertices;
    unsigned int r, g, b;
    auto obj = std::make_shared<Polygon>();
    // "%[^\n]\n"
    std::getline(infile, line);
    // "Color: r: %d, g: %d, b: %d\n"
    std::getline(infile, line);
    line = line.substr(line.find_first_of(":") + 1);
    line = line.substr(line.find_first_of(":") + 1);
    iss.clear();
    iss.str(line);
    iss >> r;
    line = line.substr(line.find_first_of(":") + 1);
    iss.clear();
    iss.str(line);
    iss >> g;
    line = line.substr(line.find_first_of(":") + 1);
    iss.clear();
    iss.str(line);
    iss >> b;
    obj->set_color(r, g, b);
    // "Number of Vertices: %d\n"
    std::getline(infile, line);
    iss.clear();
    iss.str(line.substr(line.find_first_of(":") + 1));
    iss >> num_vertices;
    // "Number of Keyframes: %d\n"
    std::getline(infile, line);
    iss.clear();
    iss.str(line.substr(line.find_first_of(":") + 1));
    int num_keyframes;
    iss >> num_keyframes;
    obj->keyframes.resize(num_keyframes);
    for (int j = 0; j < num_keyframes; ++j)
    {
      // "Keyframe for Frame %d\n"
      std::getline(infile, line);
      iss.clear();
      iss.str(line.substr(line.find_last_of(" ") + 1));
      iss >> obj->keyframes[j].number;
      for (int k = 0; k < num_vertices; ++k)
      {
        // "Vertex %d, x: %g, y: %g\n"
        std::getline(infile, line);
        int t;
        line = line.substr(line.find_first_of(":") + 1);
        iss.clear();
        iss.str(line);
        iss >> t;
        float x = t;
        line = line.substr(line.find_first_of(":") + 1);
        iss.clear();
        iss.str(line);
        iss >> t;
        float y = t;
        obj->keyframes[j].vertices.emplace_back(Point{x, y});
      }
    }
    polygons.emplace_back(obj);
  }
  return true;
}

void Scene::save(const std::string& filename) const
{
  // check if there's something in the filename field
  std::ofstream ofs(filename);
  if (!ofs) return;
  // write out the number of objects
  ofs << "Number of Objects: " << polygons.size() << "\n";
  // write out each object
  auto i = 0;
  for (auto& o : polygons) {
    ofs << "Object Number: " << i++ << "\n";
    ofs << "Color: r: " << o->r << ", g: " << o->g << ", b: " << o->b << "\n";
    ofs << "Number of Vertices: " << o->get_num_vertices() << "\n";
    ofs << "Number of Keyframes: " << o->get_num_keyframes() << "\n";
    for (auto& f : o->keyframes) {
      ofs << "Keyframe for Frame " << f.number << "\n";
      auto k = 0;
      for (auto& v : f.vertices) {
        ofs << "Vertex " << k++ << ", x: " << v.x << ", y: " << v.y << "\n";
      }
    }
  }
}

void Scene::render_to_file(const std::vector<std::string>& args)
{
  std::string infile;
  std::string outfile;
  std::string basename{"basename"};
  unsigned int first_frame;
  unsigned int final_frame;
  unsigned int num_aa_samples = 0;
  unsigned int num_mb_samples = 0;
  bool aa_enabled = false;
  bool mb_enabled = false;
  std::istringstream iss;

  if (args.size() < 4 || args[0] == "-help")
  {
    std::cout << "Usage: rasterizer [-a<#samples>] [-m<#samples>]"
    << " <first frame> <last frame> <infile> <outfile>\n";
    return;
  }

  auto arg = args.crbegin();
  outfile = *arg++;
  infile = *arg++;
  iss.str(*arg++);
  iss >> final_frame;
  iss.clear();
  iss.str(*arg++);
  iss >> first_frame;
  if (first_frame == 0 || final_frame == 0)
  {
    std::cerr << "Incorrect arguments. Type 'rasterizer -help' for more info\n";
    return;
  }
  /*
   if (oss.str().substr(0, 2) == "-a")
   {
   aa_enabled = true;
   std::istringstream iss(oss.str().substr(2));
   iss >> num_aa_samples;
   if (num_aa_samples == 0)
   {
   std::cerr << "Incorrect arguments. Type 'rasterizer -help' for more info\n";
   }
   ++i;
   }

   if (strncmp(argv[i], "-m", 2) == 0)
   {
   mb_enabled = true;
   if (sscanf(argv[i], "-m%d", &num_mb_samples) == 0)
   {
   std::cerr << "Incorrect arguments. Type 'rasterizer -help' for more info\n";
   }
   ++i;
   }

   //there should be four more arguments after the optional switches
   if (i != argc - 4)
   {
   std::cerr << "Incorrect number of arguments."
   << " Type 'rasterizer -help' for more info\n";
   return;
   }
   */
  load(infile);
  //ParseFilename(outputFile, pathless);
  std::ofstream listfile(outfile + ".list");
  assert(listfile);

  for (auto frame = first_frame; frame <= final_frame; ++frame)
  {
    rasterizer.run(polygons, frame, aa_enabled, num_aa_samples,
                   mb_enabled, num_mb_samples, "");
    std::ostringstream oss;
    oss << outfile << "." << frame << ".ppm";
    rasterizer.save(oss.str());
    listfile << basename << "." << frame << ".ppm" << "\n";
  }
}

void Scene::startDrawing(long x, long y)
{
  std::cout << "start drawing at " << x << ", " << y << '\n';
  active_object = std::make_shared<Polygon>();
  active_object->keyframes.push_back(Frame());
  active_object->keyframes[0].number = 1;
  active_object->set_color(255, 255, 255);
  active_object->keyframes[0].vertices.push_back(Point{static_cast<float>(x), static_cast<float>(y)});
}

void Scene::startRotating(long x, long y)
{
  std::cout << "start rotating at " << x << ", " << y << '\n';
}

void Scene::startScaling(long x, long y)
{
  std::cout << "start scaling at " << x << ", " << y << '\n';
}

void Scene::continueDrawing(long x, long y)
{
  std::cout << "continue drawing at " << x << ", " << y << '\n';
  active_object->keyframes[0].vertices.push_back(Point{static_cast<float>(x), static_cast<float>(y)});
}

void Scene::continueRotating(long x, long y)
{
  std::cout << "continue rotating at " << x << ", " << y << '\n';
}

void Scene::continueScaling(long x, long y)
{
  std::cout << "continue scaling at " << x << ", " << y << '\n';
}

void Scene::finishDrawing(long x, long y)
{
  std::cout << "finish drawing at " << x << ", " << y << '\n';
  // if we're in the middle of drawing something, then end it
  if (active_object) {
    // if we don't have a polygon
    if (active_object->get_num_vertices() < 3) {
      active_object = nullptr;
    } else {
      add_object(active_object);
    }
    active_object = nullptr;
  }
}

void Scene::finishRotating(long x, long y)
{
  std::cout << "finish rotating at " << x << ", " << y << '\n';
}

void Scene::finishScaling(long x, long y)
{
  std::cout << "finish scaling at " << x << ", " << y << '\n';
}


#if 0

void Scene::scale(int mx, int my, int frame)
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

void Scene::rotate(int mx, int my, int frame)
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

bool Scene::move(float mx, float my, int frame)
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
