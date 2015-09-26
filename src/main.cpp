/**
 *  This file defines the GUI for the rasterizer
 */
#include <assert.h>
#include <iostream>
#include <unistd.h>
#include <string.h>
#include <GL/glui.h>
#ifdef __MACH__
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif

#include "canvas.hpp"

#define WINDOW_WIDTH 600
#define WINDOW_HEIGHT 400

/* Here are les globals, mainly for les widgets */
GLUI_StaticText* object_id_statictext;
GLUI_StaticText* object_verts_statictext;
GLUI_EditText* num_alias_samples_editor;
GLUI_EditText* num_blur_samples_editor;
GLUI_EditText* start_frame_editor;
GLUI_EditText* end_frame_editor;
GLUI_EditText* alias_func_edit;
GLUI_Spinner* frame_spinner;
GLUI_Button* delete_keyframe_button;

char renderOut[sizeof(GLUI_String)]; // the filename(s) to render to
char save_load_file[sizeof(GLUI_String)];
char aafilter_function[sizeof(GLUI_String)];
int main_window;
int render_window;
int isAnimating; // updated when the Animate button is checked
int currFrameNumber;
int singMult; // whether you're rendering single or multiple frames
int antiAlias;
int numAliasSamples;
int motionBlur;
int numBlurSamples;
int startFrame;
int endFrame;
int selectedObject = -1;
int selectedVertex = -1;
int originalX;
int originalY;
int rotation_centerX = -1;
int rotation_centerY;
int prev_rotationX;
int prev_rotationY;

bool rotate_polygon = false;
bool scale_polygon = false;
bool draw_curve = false;
bool isKeyFrame = false;

Canvas* renderCanvas;

static void CheckDeleteKeyframeStatus()
{
  if (!renderCanvas->AnyKeyframe(currFrameNumber) || currFrameNumber == 1)
  {
    delete_keyframe_button->disable();
  }
  else
  {
    delete_keyframe_button->enable();
  }
}

static void UpdateInfo(int selectedObject)
{
  char buf1[1024], buf2[1024];
  if (selectedObject != -1)
  {
    sprintf(buf1, "Object ID: %d", selectedObject);
    sprintf(buf2, "Vertices: %d", renderCanvas->objects[selectedObject]->numVertices);
    object_id_statictext->set_text(buf1);
    object_verts_statictext->set_text(buf2);
  }
  else
  {
    object_id_statictext->set_text("Object ID:");
    object_verts_statictext->set_text("Vertices:");
  }
}

static void edit_screen_display_callback(void)
{
  renderCanvas->edit_screen_display();
}

static void myKeyboardFunc(unsigned char key, int x, int y)
{
  switch(key)
  {
  case 8:
  case 127: renderCanvas->delete_object(selectedObject); selectedObject = -1; break;
  case '.': frame_spinner->set_int_val(currFrameNumber + 1); break;
  case ',': frame_spinner->set_int_val(currFrameNumber - 1); break;
  }
}

static void myMotionFunc(int mx, int my)
{
  if (renderCanvas->motion(mx, my))
  {
    CheckDeleteKeyframeStatus();
    glutPostRedisplay();
  }
}

static void myMouseFunc(int button, int state, int mx, int my)
{
  if (renderCanvas->mouse(button, state, mx, my))
  {
    UpdateInfo(selectedObject);
    glutPostRedisplay();
  }
}

static void FrameChangedCall(int id)
{
  CheckDeleteKeyframeStatus();
}

static void DeleteKeyframeCall(int id)
{
  renderCanvas->delete_keyframe(id);
  CheckDeleteKeyframeStatus();
}

static void SaveObjectsCall(int id)
{
  char buf[1024];
  if (strlen(save_load_file)==0) return;
  sprintf(buf, "%s.obs", save_load_file);
  renderCanvas->save_objects(buf);
}

static void LoadObjectsCall(int id)
{
  char buf[1024];
  if (strlen(save_load_file)==0) return;

  sprintf(buf, "%s.obs", save_load_file);
  renderCanvas->load_objects(buf);
  glutPostRedisplay();
}

static void SingMultChanged(int id)
{
  if (singMult == 0)
  {
    start_frame_editor->disable();
    end_frame_editor->disable();
  }
  else
  {
    start_frame_editor->enable();
    end_frame_editor->enable();
  }
}

static void AntiAliasChanged(int id)
{
  if (antiAlias == 0)
  {
    num_alias_samples_editor->disable();
    alias_func_edit->disable();
  }
  else
  {
    num_alias_samples_editor->enable();
    alias_func_edit->enable();
  }
}

static void MotionBlurChanged(int id)
{
  if (motionBlur == 0)
  {
    num_blur_samples_editor->disable();
  }
  else
  {
    num_blur_samples_editor->enable();
  }
}

static void ParseFilename(char* filename, char* pathless)
{
  int i, j = 0;
  for (i=0; i< (int)strlen(filename); i++)
  {
    pathless[j] = filename[i];
    j++;
    if (filename[i] == '/') j = 0;
  }
  pathless[j] = '\0';
}

static void DisplayAndSaveCanvas(int id)
{
  char buf[1024], pathless[1024];
  if (singMult == 0)
  {
    renderCanvas->rasterize(currFrameNumber, antiAlias, numAliasSamples, motionBlur, numBlurSamples);
    if (strlen(renderOut) != 0)
    {
      sprintf(buf, "%s.ppm", renderOut);
      renderCanvas->save(buf);
    }
    glutPostWindowRedisplay(render_window);
    glutSetWindow(render_window);
    glutShowWindow();
    glutSetWindow(main_window);
  }
  else
  {
    ParseFilename(renderOut, pathless);
    sprintf(buf, "%s.list", renderOut);
    FILE* listFile = fopen(buf, "w");
    assert(listFile != NULL);

    for (int i = startFrame; i <= endFrame; ++i)
    {
      renderCanvas->rasterize(i, antiAlias, numAliasSamples, motionBlur, numBlurSamples);
      if (strlen(renderOut) != 0)
      {
        sprintf(buf, "%s.%d.ppm", renderOut, i);
        renderCanvas->save(buf);
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
}

static void render_window_display_callback()
{
  std::cout << "render canvas" << std::endl;
  renderCanvas->render();
}

static void CommandLineRasterize(int argc, char* argv[])
{
  int i = 1; //step through the command line
  int motionBlur = 0, antiAlias = 0;
  int numAliasSamples = 0, numBlurSamples = 0;
  int startFrame, endFrame;
  char inputFile[1024], outputFile[1024], pathless[1024], buf[1024];

  if (strcmp(argv[i], "-help")==0)
  {
    printf("Usage: rasterizer [-a<#samples>] [-m<#samples>] <startframe> <endframe> <infile> <outfile>\n");
    return;
  }

  if (strncmp(argv[i], "-a", 2)==0)
  {
    antiAlias = 1;
    if (sscanf(argv[i], "-a%d", &numAliasSamples)==0)
    {
      printf("Incorrect arguments. Type 'animgui -help' for more info\n");
    };
    i++;
  }

  if (strncmp(argv[i], "-m", 2)==0)
  {
    motionBlur = 1;
    if (sscanf(argv[i], "-m%d", &numBlurSamples)==0)
    {
      printf("Incorrect arguments. Type 'animgui -help' for more info\n");
    };
    i++;
  }

  //there should be four more arguments after the optional switches
  if (i!=(argc-4))
  {
    printf("Incorrect number of arguments. Type 'animgui -help' for more info\n");
    return;
  }
  else
  {
    startFrame = atoi(argv[i]);
    endFrame = atoi(argv[i+1]);
    if (startFrame == 0 || endFrame == 0)
    {
      printf("Incorrect arguments. Type 'animgui -help' for more info\n");
      return;
    }
    strcpy(inputFile, argv[i+2]);
    strcpy(outputFile, argv[i+3]);
  }

  printf("Let's gett reaadddyyy to RENNNDDEERRR!\n");
  printf("Start frame: %d\n", startFrame);
  printf("End frame: %d\n", endFrame);
  printf("Input file: %s\n", inputFile);
  printf("Output file label: %s\n", outputFile);
  printf("Antialiasing: %s\n", antiAlias?"On":"Off");
  if (antiAlias)
  {
    printf("Number of samples: %d\n", numAliasSamples);
  }
  printf("Motion blur: %s\n", motionBlur?"On":"Off");
  if (motionBlur)
  {
    printf("Number of samples: %d\n", numBlurSamples);
  }
  renderCanvas->load_objects(inputFile);

  ParseFilename(outputFile, pathless);
  sprintf(buf, "%s.list", outputFile);
  FILE* listFile = fopen(buf, "w");
  assert(listFile!=NULL);

  for (i = startFrame; i <= endFrame; ++i)
  {
    printf("Rendering Frame %d\n", i);
    renderCanvas->rasterize(i, antiAlias, numAliasSamples, motionBlur, numBlurSamples);
    sprintf(buf, "%s.%d.ppm", outputFile, i);
    renderCanvas->save(buf);
    sprintf(buf, "%s.%d.ppm", pathless, i);
    fprintf(listFile, "%s\n", buf);
  }
  fclose(listFile);
}

int main(int argc, char* argv[])
{
  /* This allocates new memory for the canvas */
  renderCanvas = new Canvas(WINDOW_WIDTH, WINDOW_HEIGHT);

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
  glutPositionWindow(100, 100);
  glOrtho(0, WINDOW_WIDTH, -WINDOW_HEIGHT, 0, -1, 1);
  glutDisplayFunc(render_window_display_callback);
  glutHideWindow();

  glutSetWindow(main_window);

  /* Now create the widgets that we need */
  GLUI* glui = GLUI_Master.create_glui("Rasterizer", 0, 50, WINDOW_HEIGHT + 100);

  glui->set_main_gfx_window(main_window);

  GLUI_Panel* info_panel = glui->add_panel("Object Info");
  object_id_statictext = glui->add_statictext_to_panel(info_panel, "Object ID:");
  object_verts_statictext = glui->add_statictext_to_panel(info_panel, "Vertices:");
  info_panel->set_alignment(GLUI_ALIGN_LEFT);

  GLUI_Panel* anim_panel = glui->add_panel("Animation");
  delete_keyframe_button = glui->add_button_to_panel(anim_panel, "Delete Keyframe", 0, (GLUI_Update_CB)DeleteKeyframeCall);
  delete_keyframe_button->disable();
  frame_spinner = glui->add_spinner_to_panel(anim_panel, "Frame", GLUI_SPINNER_INT, &currFrameNumber, -1, FrameChangedCall);
  frame_spinner->set_int_limits(1, MAX_FRAMES, GLUI_LIMIT_CLAMP);
  anim_panel->set_alignment(GLUI_ALIGN_LEFT);

  GLUI_Panel* save_load_panel = glui->add_panel("Save/Load");
  glui->add_edittext_to_panel(save_load_panel, "Filename", GLUI_EDITTEXT_TEXT, save_load_file);
  glui->add_button_to_panel(save_load_panel, "Save Object Data", 0, (GLUI_Update_CB)SaveObjectsCall);
  glui->add_button_to_panel(save_load_panel, "Load Object Data", 0, (GLUI_Update_CB)LoadObjectsCall);

  glui->add_column(true);

  GLUI_Panel* render_panel = glui->add_panel("Rendering");
  glui->add_edittext_to_panel(render_panel, "Render Out:", GLUI_EDITTEXT_TEXT, renderOut);
  glui->add_checkbox_to_panel(render_panel, "Antialias", &antiAlias, -1, AntiAliasChanged);
  num_alias_samples_editor = glui->add_edittext_to_panel(render_panel, "Number Of Samples", GLUI_EDITTEXT_INT, &numAliasSamples);
  num_alias_samples_editor->disable();
  num_alias_samples_editor->set_int_val(1);
  num_alias_samples_editor->set_int_limits(1, MAX_ALIAS_SAMPLES);

  alias_func_edit = glui->add_edittext_to_panel(render_panel, "Filter:", GLUI_EDITTEXT_TEXT, aafilter_function);
  alias_func_edit->disable();

  glui->add_checkbox_to_panel(render_panel, "Motion Blur", &motionBlur, -1, MotionBlurChanged);
  num_blur_samples_editor = glui->add_edittext_to_panel(render_panel, "Number Of Samples", GLUI_EDITTEXT_INT, &numBlurSamples);
  num_blur_samples_editor->disable();
  num_blur_samples_editor->set_int_val(1);
  num_blur_samples_editor->set_int_limits(1, MAX_BLUR_SAMPLES);

  GLUI_RadioGroup* singMultRadioGroup = glui->add_radiogroup_to_panel(render_panel, &singMult, -1, SingMultChanged);
  glui->add_radiobutton_to_group(singMultRadioGroup, "This Frame Only");
  glui->add_radiobutton_to_group(singMultRadioGroup, "Multiple Frames");
  start_frame_editor = glui->add_edittext_to_panel(render_panel, "Start Frame:", GLUI_EDITTEXT_INT, &startFrame);
  start_frame_editor->disable();
  start_frame_editor->set_int_val(1);
  start_frame_editor->set_int_limits(1, 100);

  end_frame_editor = glui->add_edittext_to_panel(render_panel, "End Frame:", GLUI_EDITTEXT_INT, &endFrame);
  end_frame_editor->disable();
  end_frame_editor->set_int_val(100);
  end_frame_editor->set_int_limits(1, 100);

  glui->add_button_to_panel(render_panel, "Render!", 0, (GLUI_Update_CB)DisplayAndSaveCanvas);

  glutMainLoop();

  return 0;
}
