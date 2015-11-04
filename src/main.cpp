/**
 *  This file defines the GUI for the rasterizer
 */
#include <cassert>
#include <iostream>
#include <GL/glui.h>
#ifndef __MACH__
#include <unistd.h>
#include <string.h>
#include <GL/glut.h>
#endif

#include "canvas.hpp"

using namespace std;

#define WINDOW_WIDTH  600
#define WINDOW_HEIGHT 400

/* Here are les globals, mainly for les widgets */
GLUI_StaticText* object_id_statictext;
GLUI_StaticText* object_verts_statictext;
GLUI_StaticText* object_frames_statictext;
GLUI_EditText* num_alias_samples_editor;
GLUI_EditText* num_blur_samples_editor;
GLUI_EditText* start_frame_editor;
GLUI_EditText* end_frame_editor;
GLUI_EditText* alias_func_edit;
GLUI_Spinner* frame_spinner;
GLUI_Button* delete_keyframe_button;

char aafilter_function[sizeof(GLUI_String)];
char render_filename[sizeof(GLUI_String)]; // the filename(s) to render to
char save_load_file[sizeof(GLUI_String)];
bool anti_aliasing_enabled;
bool motion_blur_enabled;
bool multiple_frames; // whether you're rendering single or multiple frames
int selected_object = -1;
int num_alias_samples;
int num_blur_samples;
int main_window;
int render_window;
int current_frame;
int first_frame;
int final_frame;
Canvas* canvas;

static void check_delete_keyframe_status()
{
  if (current_frame != 1 && canvas->any_keyframe(current_frame))
    delete_keyframe_button->enable();
  else
    delete_keyframe_button->disable();
}

static void update_info(int selected_object)
{
  char buf[1024];
  if (selected_object != -1)
  {
    sprintf(buf, "Object ID: %d", selected_object);
    object_id_statictext->set_text(buf);
    sprintf(buf, "Vertices: %d", canvas->get_num_vertices(selected_object));
    object_verts_statictext->set_text(buf);
    sprintf(buf, "Keyframes: %d", canvas->get_num_keyframes(selected_object));
    object_frames_statictext->set_text(buf);
  }
  else
  {
    object_id_statictext->set_text("Object ID:");
    object_verts_statictext->set_text("Vertices:");
    object_frames_statictext->set_text("Keyframes:");
  }
}

static void edit_screen_display_callback()
{
  canvas->display(current_frame, selected_object);
}

static void myKeyboardFunc(unsigned char key, int, int)
{
  bool post_redisplay = false;
  switch(key)
  {
  case 8:
  case 127: post_redisplay = canvas->delete_object(selected_object); selected_object = -1; break;
  case '.': frame_spinner->set_int_val(current_frame + 1); break;
  case ',': frame_spinner->set_int_val(current_frame - 1); break;
  default: break;
  }
  if (post_redisplay)
  {
    glutPostRedisplay();
  }
}

static void myMotionFunc(int mx, int my)
{
  if (canvas->motion(mx, my, current_frame, selected_object))
  {
    check_delete_keyframe_status();
    glutPostRedisplay();
  }
}

static void myMouseFunc(int button, int state, int mx, int my)
{
  if (canvas->mouse(button, state, mx, my, current_frame, selected_object))
  {
    update_info(selected_object);
    glutPostRedisplay();
  }
}

static void FrameChangedCall(int)
{
  check_delete_keyframe_status();
}

static void DeleteKeyframeCall(int id)
{
  canvas->delete_keyframe(id, current_frame);
  check_delete_keyframe_status();
}

static void SaveObjectsCall(int)
{
  char buf[1024];
  if (strlen(save_load_file) == 0) return;
  sprintf(buf, "%s.obs", save_load_file);
  canvas->save_objects(buf);
}

static void LoadObjectsCall(int)
{
  char buf[1024];
  if (strlen(save_load_file) == 0) return;
  sprintf(buf, "%s.obs", save_load_file);
  canvas->load_objects(buf);
  selected_object = -1;
  update_info(selected_object);
  glutPostRedisplay();
}

static void SingMultChanged(int)
{
  if (multiple_frames)
  {
    start_frame_editor->enable();
    end_frame_editor->enable();
  }
  else
  {
    start_frame_editor->disable();
    end_frame_editor->disable();
  }
}

static void AntiAliasChanged(int)
{
  if (anti_aliasing_enabled)
  {
    num_alias_samples_editor->enable();
    alias_func_edit->enable();
  }
  else
  {
    num_alias_samples_editor->disable();
    alias_func_edit->disable();
  }
}

static void MotionBlurChanged(int)
{
  if (motion_blur_enabled)
    num_blur_samples_editor->enable();
  else
    num_blur_samples_editor->disable();
}

static void ParseFilename(char* filename, char* pathless)
{
  int j = 0;
  for (int i = 0; i < (int)strlen(filename); ++i)
  {
    pathless[j++] = filename[i];
    if (filename[i] == '/')
      j = 0;
  }
  pathless[j] = '\0';
}

static void DisplayAndSaveCanvas(int)
{
  char buf[1024], pathless[1024];
  if (multiple_frames)
  {
    ParseFilename(render_filename, pathless);
    sprintf(buf, "%s.list", render_filename);
    FILE* listFile = fopen(buf, "w");
    assert(listFile != NULL);

    for (int i = first_frame; i <= final_frame; ++i)
    {
      canvas->rasterize(i, anti_aliasing_enabled, num_alias_samples, motion_blur_enabled, num_blur_samples, aafilter_function);
      if (strlen(render_filename) != 0)
      {
        sprintf(buf, "%s.%d.ppm", render_filename, i);
        canvas->save(buf);
        sprintf(buf, "%s.%d.ppm", pathless, i);
        fprintf(listFile, "%s\n", buf);
      }
      glutPostWindowRedisplay(render_window);
      glutSetWindow(render_window);
      glutShowWindow();
      glutSetWindow(main_window);
    }
    fclose(listFile);
  }
  else
  {
    canvas->rasterize(current_frame, anti_aliasing_enabled, num_alias_samples, motion_blur_enabled, num_blur_samples, aafilter_function);
    if (strlen(render_filename) != 0)
    {
      sprintf(buf, "%s.ppm", render_filename);
      canvas->save(buf);
    }
    glutPostWindowRedisplay(render_window);
    glutSetWindow(render_window);
    glutShowWindow();
    glutSetWindow(main_window);
  }
}

static void render_window_display_callback()
{
  canvas->render();
}

static void CommandLineRasterize(int argc, char* argv[])
{
  char inputFile[1024], outputFile[1024], pathless[1024], buf[1024];
  bool motion_blur_enabled = false, anti_aliasing_enabled = false;
  int num_alias_samples = 0, num_blur_samples = 0;
  int first_frame, final_frame;
  int i = 1; // step through the command line

  if (argc < 5 || strcmp(argv[i], "-help") == 0)
  {
    cout << "Usage: rasterizer [-a<#samples>] [-m<#samples>] <first frame> <final frame> <infile> <outfile>\n";
    return;
  }

  if (strncmp(argv[i], "-a", 2) == 0)
  {
    anti_aliasing_enabled = true;
    if (sscanf(argv[i], "-a%d", &num_alias_samples) == 0)
    {
      printf("Incorrect arguments. Type 'rasterizer -help' for more info\n");
    };
    ++i;
  }

  if (strncmp(argv[i], "-m", 2) == 0)
  {
    motion_blur_enabled = true;
    if (sscanf(argv[i], "-m%d", &num_blur_samples) == 0)
    {
      printf("Incorrect arguments. Type 'rasterizer -help' for more info\n");
    };
    ++i;
  }

  //there should be four more arguments after the optional switches
  if (i != argc - 4)
  {
    printf("Incorrect number of arguments. Type 'rasterizer -help' for more info\n");
    return;
  }
  else
  {
    first_frame = atoi(argv[i]);
    final_frame = atoi(argv[i + 1]);
    if (first_frame == 0 || final_frame == 0)
    {
      printf("Incorrect arguments. Type 'rasterizer -help' for more info\n");
      return;
    }
    strcpy(inputFile, argv[i + 2]);
    strcpy(outputFile, argv[i + 3]);
  }

  printf("Start frame: %d\n", first_frame);
  printf("End frame: %d\n", final_frame);
  printf("Input file: %s\n", inputFile);
  printf("Output file label: %s\n", outputFile);
  printf("Antialiasing: %s\n", anti_aliasing_enabled ? "On" : "Off");
  if (anti_aliasing_enabled)
  {
    printf("Number of samples: %d\n", num_alias_samples);
  }
  printf("Motion blur: %s\n", motion_blur_enabled?"On":"Off");
  if (motion_blur_enabled)
  {
    printf("Number of samples: %d\n", num_blur_samples);
  }
  canvas->load_objects(inputFile);

  ParseFilename(outputFile, pathless);
  sprintf(buf, "%s.list", outputFile);
  FILE* listFile = fopen(buf, "w");
  assert(listFile != nullptr);

  for (i = first_frame; i <= final_frame; ++i)
  {
    printf("Rendering Frame %d\n", i);
    canvas->rasterize(i, anti_aliasing_enabled, num_alias_samples, motion_blur_enabled, num_blur_samples, aafilter_function);
    sprintf(buf, "%s.%d.ppm", outputFile, i);
    canvas->save(buf);
    sprintf(buf, "%s.%d.ppm", pathless, i);
    fprintf(listFile, "%s\n", buf);
  }
  fclose(listFile);
}

int main(int argc, char* argv[])
{
  /* This allocates new memory for the canvas */
  canvas = new Canvas(WINDOW_WIDTH, WINDOW_HEIGHT);
  // if they set command-line arguments, then don't show the GUI
  if (argc > 1)
  {
    CommandLineRasterize(argc, argv);
    return 0;
  }
  /* This initializes all of the GLUT stuff */
  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
  glutInitWindowPosition(50, 50);
  glutInitWindowSize(WINDOW_WIDTH, WINDOW_HEIGHT);

  main_window = glutCreateWindow("Edit Screen");
  glOrtho(0, WINDOW_WIDTH, -WINDOW_HEIGHT, 0, -1, 1);
  glutDisplayFunc(edit_screen_display_callback);
  glutMouseFunc(myMouseFunc);
  glutMotionFunc(myMotionFunc);
  glutKeyboardFunc(myKeyboardFunc);

  render_window = glutCreateWindow("Render Window");
  glutPositionWindow(WINDOW_WIDTH + 100, 50);
  glOrtho(0, WINDOW_WIDTH, -WINDOW_HEIGHT, 0, -1, 1);
  glutDisplayFunc(render_window_display_callback);
  //glutHideWindow();

  glutSetWindow(main_window);

  /* Now create the widgets that we need */
  GLUI* glui = GLUI_Master.create_glui("Rasterizer", 0, 50, WINDOW_HEIGHT + 100);

  glui->set_main_gfx_window(main_window);

  GLUI_Panel* info_panel = glui->add_panel("Info");
  object_id_statictext = glui->add_statictext_to_panel(info_panel, "Object ID:");
  object_verts_statictext = glui->add_statictext_to_panel(info_panel, "Vertices:");
  object_frames_statictext = glui->add_statictext_to_panel(info_panel, "Keyframes:");
  info_panel->set_alignment(GLUI_ALIGN_LEFT);

  GLUI_Panel* anim_panel = glui->add_panel("Animation");
  delete_keyframe_button = glui->add_button_to_panel(anim_panel, "Delete Keyframe", 0, (GLUI_Update_CB)DeleteKeyframeCall);
  delete_keyframe_button->disable();
  frame_spinner = glui->add_spinner_to_panel(anim_panel, "Frame", GLUI_SPINNER_INT, &current_frame, -1, FrameChangedCall);
  frame_spinner->set_int_limits(1, MAX_FRAMES, GLUI_LIMIT_CLAMP);
  anim_panel->set_alignment(GLUI_ALIGN_LEFT);

  GLUI_Panel* save_load_panel = glui->add_panel("Save/Load");
  auto save_load_text = glui->add_edittext_to_panel(save_load_panel, "Filename", GLUI_EDITTEXT_TEXT, save_load_file);
  save_load_text->set_w(200);
  glui->add_button_to_panel(save_load_panel, "Save Object", 0, (GLUI_Update_CB)SaveObjectsCall);
  glui->add_button_to_panel(save_load_panel, "Load Object", 0, (GLUI_Update_CB)LoadObjectsCall);
  save_load_panel->set_alignment(GLUI_ALIGN_LEFT);

  glui->add_column(false);

  GLUI_Panel* render_panel = glui->add_panel("Rendering");
  glui->add_edittext_to_panel(render_panel, "Render Out:", GLUI_EDITTEXT_TEXT, render_filename);
  int integer_value;
  glui->add_checkbox_to_panel(render_panel, "Antialias", &integer_value, -1, AntiAliasChanged);
  anti_aliasing_enabled = integer_value != 0;
  num_alias_samples_editor = glui->add_edittext_to_panel(render_panel, "Number Of Samples", GLUI_EDITTEXT_INT, &num_alias_samples);
  num_alias_samples_editor->disable();
  num_alias_samples_editor->set_int_val(1);
  num_alias_samples_editor->set_int_limits(1, MAX_ALIAS_SAMPLES);

  alias_func_edit = glui->add_edittext_to_panel(render_panel, "Filter:", GLUI_EDITTEXT_TEXT, aafilter_function);
  alias_func_edit->disable();

  glui->add_checkbox_to_panel(render_panel, "Motion Blur", &integer_value, -1, MotionBlurChanged);
  motion_blur_enabled = integer_value != 0;
  num_blur_samples_editor = glui->add_edittext_to_panel(render_panel, "Number Of Samples", GLUI_EDITTEXT_INT, &num_blur_samples);
  num_blur_samples_editor->disable();
  num_blur_samples_editor->set_int_val(1);
  num_blur_samples_editor->set_int_limits(1, MAX_BLUR_SAMPLES);

  GLUI_RadioGroup* singMultRadioGroup = glui->add_radiogroup_to_panel(render_panel, &integer_value, -1, SingMultChanged);
  multiple_frames = integer_value != 0;
  glui->add_radiobutton_to_group(singMultRadioGroup, "This Frame Only");
  glui->add_radiobutton_to_group(singMultRadioGroup, "Multiple Frames");
  start_frame_editor = glui->add_edittext_to_panel(render_panel, "Start Frame:", GLUI_EDITTEXT_INT, &first_frame);
  start_frame_editor->disable();
  start_frame_editor->set_int_val(1);
  start_frame_editor->set_int_limits(1, 100);

  end_frame_editor = glui->add_edittext_to_panel(render_panel, "End Frame:", GLUI_EDITTEXT_INT, &final_frame);
  end_frame_editor->disable();
  end_frame_editor->set_int_val(100);
  end_frame_editor->set_int_limits(1, 100);

  glui->add_button_to_panel(render_panel, "Render!", 0, (GLUI_Update_CB)DisplayAndSaveCanvas);

  glutMainLoop();

  return 0;
}
