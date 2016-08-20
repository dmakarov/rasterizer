/**
   \file scene.cpp

   Created by Dmitri Makarov on 16-08-19.
   Copyright © 2016 Dmitri Makarov. All rights reserved.
*/

#include "scene.h"

#include <cassert>
#include <cmath>
#include <fstream>
#include <ostream>
#include <sstream>
#include <string>
#include <iostream>

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

void Scene::renderToFile(const std::vector<std::string>& args)
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
  active = std::make_shared<Polygon>();
  active->keyframes.push_back(Frame());
  active->keyframes[0].number = 1;
  active->set_color(255, 255, 255);
  active->keyframes[0].vertices.push_back(Point{static_cast<float>(x),
                                                static_cast<float>(y)});
}

void Scene::startRotating(long x, long y)
{
  std::cout << "start rotating at " << x << ", " << y << '\n';
  center.x = (float) x;
  center.y = (float) y;
}

void Scene::startScaling(long x, long y)
{
  std::cout << "start scaling at " << x << ", " << y << '\n';
  center.x = (float) x;
  center.y = (float) y;
}

void Scene::continueDrawing(long x, long y)
{
  auto px = active->keyframes[0].vertices.back().x;
  auto py = active->keyframes[0].vertices.back().y;
  // sqrt((px-mx)*(px-mx) + (py-my)*(py-my)) > 5)
  if (std::abs(px - x) > 7 || std::abs(py - y) > 7) {
    std::cout << "continue drawing at " << x << ", " << y << '\n';
    active->keyframes[0].vertices.push_back(Point{static_cast<float>(x),
                                                  static_cast<float>(y)});
  }
}

void Scene::continueRotating(int frame, long x, long y)
{
  std::cout << "continue rotating at " << x << ", " << y << '\n';
  rotate(frame, x, y);
}

void Scene::continueScaling(int frame, long x, long y)
{
  std::cout << "continue scaling at " << x << ", " << y << '\n';
  scale(frame, x, y);
}

void Scene::finishDrawing(long x, long y)
{
  std::cout << "finish drawing at " << x << ", " << y << '\n';
  // if we're in the middle of drawing something, then end it
  if (active) {
    // if we don't have a polygon
    if (active->get_num_vertices() < 3) {
      active = nullptr;
    } else {
      polygons.push_back(active);
    }
    active = nullptr;
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

void Scene::select(const int frame, const long x, const long y)
{
  bool found = false;
  selected = nullptr;
  for (PolySz i = 0; !found && i < polygons.size(); ++i) {
    std::vector<Point> vertices;
    polygons[i]->get_vertices(frame, vertices);
    selectedVertex = 0;
    for (auto& v : vertices) {
      // check for proximity
      if (std::fabs(v.x - x) < 5 && std::fabs(v.y - y) < 5) {
        selected = polygons[i];
        selectedID = i;
        found = true;
        break;
      }
      ++selectedVertex;
    }
  }
  notify();
}

void Scene::rotate(const int frame, const long x, const long y)
{
  auto mx = static_cast<float>(x) - center.x;
  auto my = static_cast<float>(y) - center.y;

  if (std::abs(previous.x - mx) < 10 && std::abs(previous.y - my) < 10)
    return;
  if ((mx < 0 && previous.x > 0) || (mx > 0 && previous.y < 0)) {
    previous.x = mx;
    previous.y = my;
    return;
  }

  auto ro = sqrtf(mx * mx + my * my);
  auto al = asinf(my / ro);
  ro = sqrtf(previous.x * previous.x + previous.y * previous.y);
  float be = asinf(previous.y / ro);
  float sign = ((al > be && mx > 0) || (al < be && mx < 0)) ? 1.0 : -1.0;
  float te = sign * 3.14159 / 18;
  float sn = sinf(te);
  float cs = cosf(te);

  for (auto& v : selected->find_keyframe(frame)->vertices) {
    v.x -= center.x;
    v.y -= center.y;
    v.x = cs * v.x - sn * v.y + center.x;
    v.y = sn * v.x + cs * v.y + center.y;
  }
  previous.x = mx;
  previous.y = my;
}

void Scene::scale(const int frame, const long x, const long y)
{
  assert(selected);

  auto mx = static_cast<float>(x) - center.x;
  auto my = static_cast<float>(y) - center.y;

  if (std::abs(previous.x - mx) < 10 && std::abs(previous.y - my) < 10)
    return;

  auto dm = sqrt(mx * mx + my * my);
  auto dp = sqrt(previous.x * previous.x + previous.y * previous.y);

  auto sx = (dm > dp) ? 1.2 : 0.8;
  auto sy = (dm > dp) ? 1.2 : 0.8;

  for (auto& v : selected->find_keyframe(frame)->vertices) {
    v.x -= center.x;
    v.y -= center.y;
    v.x = sx * v.x + center.x;
    v.y = sy * v.y + center.y;
  }
  previous.x = mx;
  previous.y = my;
}

void Scene::move(const int frame, const long x, const long y)
{
  assert(selected);

  // look for a keyframe at this frame
  // if we don't find it, then create a new one in the right place
  auto keyframe = selected->find_keyframe(frame);
  if (keyframe == selected->keyframes.end()) {
    std::vector<Point> vertices;
    selected->get_vertices(frame, vertices);

    decltype(selected->get_num_keyframes()) insertLoc;
    for (insertLoc = 0; insertLoc < selected->get_num_keyframes(); ++insertLoc)
      if (frame < selected->keyframes[insertLoc].number)
        break;

    selected->keyframes.push_back(selected->keyframes.back());
    for (auto i = selected->get_num_keyframes() - 2; i >= insertLoc; --i)
      std::copy(selected->keyframes.begin() + i,
                selected->keyframes.begin() + i + 1,
                selected->keyframes.begin() + i + 1);

    std::copy(vertices.begin(), vertices.end(),
              selected->keyframes[insertLoc].vertices.begin());

    selected->keyframes[insertLoc].number = frame;
    keyframe = selected->find_keyframe(frame);
    assert(keyframe != selected->keyframes.end());
  }
  for (auto& v : keyframe->vertices) {
    v.x += (x - previous.x);
    v.y += (y - previous.y);
  }
  previous.x = x;
  previous.y = y;
}
