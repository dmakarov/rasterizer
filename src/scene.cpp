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
  is_object_selected = foundVertex;
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
