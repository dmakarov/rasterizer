/* animgui.cpp
 * This file defines the GUI for the CS248 animation/rasterizer assignment
 * written by Yar Woo (ywoo@cs)
 */

#include "objects.h"
#include "assert.h"
#include "GL/glut.h"
#include "glui.h"
#include <math.h>
#include <unistd.h>
#include <string.h>

#define WINDOW_WIDTH 600
#define WINDOW_HEIGHT 400

void CommandLineRasterize(int argc, char* argv[]);

void myGlutDisplay(void);
void myMouseFunc(int button, int state, int mx, int my);
void myMotionFunc(int mx, int my);
void myKeyboardFunc(unsigned char key, int x, int y);

void RenderDisplay();
void DisplayAndSaveCanvas(int id);

void AssignRandomColor(AnimObject* obj);
void ParseFilename(char* filename, char*pathless);
void FrameChangedCall(int id);
void DeleteKeyframeCall(int id);
void SaveObjectsCall(int id);
void SaveObjects(char* filename);
void LoadObjectsCall(int id);
void LoadObjects(char* filename);
void SingMultChanged(int id);
void AntiAliasChanged(int id);
void MotionBlurChanged(int id);
void UpdateInfo(int selectedObject);
void CheckDeleteKeyframeStatus();

/* Here are les globals, mainly for les widgets */

GLUI_StaticText* object_id_statictext;
GLUI_StaticText* object_verts_statictext;
GLUI_Spinner *frame_spinner;
GLUI_Button *delete_keyframe_button;
GLUI_EditText* start_frame_editor, *end_frame_editor;
GLUI_EditText* num_alias_samples_editor, *num_blur_samples_editor;
GLUI_EditText* alias_func_edit;

int isAnimating; //this is the live variable that updates when the Animate button is checked
int currFrameNumber; //this is the live variable that represents the frame number
char renderOut[sizeof(GLUI_String)]; //this is the live variable that represents the filename(s) to render to
char save_load_file[sizeof(GLUI_String)];
char aafilter_function[sizeof(GLUI_String)];
int singMult;	//this is the live variable that represents whether you're rendering single or multiple frames
int antiAlias;
int numAliasSamples;
int motionBlur;
int numBlurSamples;
int startFrame, endFrame; //this is the live variable that represents the start/end frames

bool isKeyFrame = false;
int colorToPick = 0;

int selectedObject = -1;
int selectedVertex = -1;
int originalX, originalY;

bool rotate_polygon = false;
int rotation_centerX = -1, rotation_centerY;
int prev_rotationX, prev_rotationY;

bool scale_polygon = false;
bool draw_curve = false;

int main_window, render_window;

AnimObject* currObject = NULL;
Canvas* renderCanvas;

int main(int argc, char* argv[])
{

  /* This allocates new memory for the canvas */

  renderCanvas = (Canvas*)malloc(sizeof(Canvas));
  int buf_width = renderCanvas->Width = WINDOW_WIDTH;
  int buf_height = renderCanvas->Height = WINDOW_HEIGHT;
  int buf_size = buf_width * buf_height;
  int i, j;

  renderCanvas->Pixels = (unsigned long *)malloc(buf_size * sizeof(unsigned long));

  //clear the renderCanvas to black
  for (i=0; i<renderCanvas->Width; i++)
    {
      for (j=0; j<renderCanvas->Height; j++)
	{
	  PIXEL(renderCanvas, i, j) = 0;
	}
    }
  

  // if they set command-line arguments, then don't show the GUI

  if (argc>1)
    {
      CommandLineRasterize(argc, argv);
    }
  else
    {
      /* This initializes all of the GLUT stuff */
      glutInit(&argc, argv);
      glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
      glutInitWindowPosition(50, 50);
      glutInitWindowSize(WINDOW_WIDTH,WINDOW_HEIGHT);
      
      main_window = glutCreateWindow("Edit Screen");
      glOrtho(0, WINDOW_WIDTH, -WINDOW_HEIGHT, 0, -1, 1);
      glutDisplayFunc(myGlutDisplay);
      glutMouseFunc(myMouseFunc);
      glutMotionFunc(myMotionFunc);
      glutKeyboardFunc(myKeyboardFunc);
      
      render_window = glutCreateWindow("Render Window");
      glutPositionWindow(100, 100);
      glOrtho(0, WINDOW_WIDTH, -WINDOW_HEIGHT, 0, -1, 1);
      glutDisplayFunc(RenderDisplay);
      glutHideWindow();
      
      glutSetWindow(main_window);
      
      /* Now create all of the stupid widgets that we need */
      GLUI *glui = GLUI_Master.create_glui("GLUI", 0, 50, WINDOW_HEIGHT+100);
      
      glui->set_main_gfx_window(main_window);
      
      GLUI_Panel *info_panel = glui->add_panel("Object Info");
      object_id_statictext = glui->add_statictext_to_panel(info_panel, "Object ID:");
      object_verts_statictext = glui->add_statictext_to_panel(info_panel, "Vertices:");
      info_panel->set_alignment(GLUI_ALIGN_LEFT);
      
      GLUI_Panel *anim_panel = glui->add_panel("Animation");
      
      delete_keyframe_button = glui->add_button_to_panel(anim_panel, "Delete Keyframe", 0, (GLUI_Update_CB)DeleteKeyframeCall);
      delete_keyframe_button->disable();
      frame_spinner = glui->add_spinner_to_panel(anim_panel, "Frame", GLUI_SPINNER_INT, &currFrameNumber, -1, FrameChangedCall);
      frame_spinner->set_int_limits(1, MAX_FRAMES, GLUI_LIMIT_CLAMP);
      anim_panel->set_alignment(GLUI_ALIGN_LEFT);
      
      GLUI_Panel *save_load_panel = glui->add_panel("Save/Load");
      glui->add_edittext_to_panel(save_load_panel, "Filename", GLUI_EDITTEXT_TEXT, save_load_file);  
      glui->add_button_to_panel(save_load_panel, "Save Object Data", 0,
				(GLUI_Update_CB)SaveObjectsCall);
      glui->add_button_to_panel(save_load_panel, "Load Object Data", 0, 
				(GLUI_Update_CB)LoadObjectsCall);	
      
      glui->add_column(true);
      
      GLUI_Panel *render_panel = glui->add_panel("Rendering");
      glui->add_edittext_to_panel(render_panel, "Render Out:", GLUI_EDITTEXT_TEXT, renderOut);
      glui->add_checkbox_to_panel(render_panel, "Antialias", &antiAlias, -1, AntiAliasChanged);
      num_alias_samples_editor = glui->add_edittext_to_panel(render_panel, 
							     "Number Of Samples", 
							     GLUI_EDITTEXT_INT, &numAliasSamples);
      num_alias_samples_editor->disable();
      num_alias_samples_editor->set_int_val(1);
      num_alias_samples_editor->set_int_limits(1, MAX_ALIAS_SAMPLES);	
      
      alias_func_edit = glui->add_edittext_to_panel( render_panel, "Filter:", GLUI_EDITTEXT_TEXT, aafilter_function );
      alias_func_edit->disable();

      glui->add_checkbox_to_panel(render_panel, "Motion Blur", &motionBlur, -1, MotionBlurChanged);
      num_blur_samples_editor = glui->add_edittext_to_panel(render_panel,
							    "Number Of Samples", 
							    GLUI_EDITTEXT_INT, &numBlurSamples);
      num_blur_samples_editor->disable();
      num_blur_samples_editor->set_int_val(1);
      num_blur_samples_editor->set_int_limits(1, MAX_BLUR_SAMPLES);
      
      GLUI_RadioGroup *singMultRadioGroup = glui->add_radiogroup_to_panel(render_panel, &singMult, -1, SingMultChanged);
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
      
      glui->add_button_to_panel(render_panel, "Render!", 0,
				(GLUI_Update_CB)DisplayAndSaveCanvas);
      
      glutMainLoop();
  }

  return 0;
}

void myGlutDisplay(void)
{
  int i, j;

  glClearColor( 0.f, 0.f, 0.f, 0.f );
  glClear( GL_COLOR_BUFFER_BIT );

  if (AnyKeyframe(currFrameNumber))
    {
      //is this a keyframe? add a nifty red border if it is
      glLineWidth(20);
      glColor3d(1, 0, 0);
      glBegin(GL_LINE_STRIP);
	
      glVertex2d(0, 0);
      glVertex2d(0, -WINDOW_HEIGHT);
      glVertex2d(WINDOW_WIDTH, -WINDOW_HEIGHT);
      glVertex2d(WINDOW_WIDTH, 0);
      glVertex2d(0, 0);

      glEnd();
    }

  glLineWidth(1);

  //this is the loop where we draw all the friggin objects

  for (i=0; i<numObjects; i++)
    {
      Point vertices[MAX_VERTICES];
      GetVertices(i, currFrameNumber, vertices);
      glColor3d(objects[i]->r/255.0, objects[i]->g/255.0, objects[i]->b/255.0);
      if (selectedObject == i) // there's a selected object! draw it different-like
	{
	  glLineWidth(3);
	}
      else
	{
	  glLineWidth(1);
	}
      //we draw the edges
      glBegin(GL_LINE_STRIP);
      for (j=0; j<objects[i]->numVertices; j++)
	{
	  glVertex2d(vertices[j].x, -vertices[j].y);
	}
      glVertex2d(vertices[0].x, -vertices[0].y);
      glEnd();

      if (selectedObject == i) //now draw the vertices on top of the lines
	{
	  glBegin(GL_LINES);
	  for (j=0; j<objects[i]->numVertices; j++)	
	    {
	      // selected vertex? 
	      if (j == selectedVertex)
		{
		  glColor3d(0, 1, 0);
		}
	      else
		{
		  glColor3d(0, 1, 1);
		}
	      glVertex2d(vertices[j].x+5, -vertices[j].y);
	      glVertex2d(vertices[j].x-5, -vertices[j].y);
	      glVertex2d(vertices[j].x, -vertices[j].y+5);
	      glVertex2d(vertices[j].x, -vertices[j].y-5);
	    }
	  glEnd();
	}
    }

  if ( rotation_centerX > -1 )
  {
      glBegin(GL_LINES);
      glColor3d(0.5, 0.5, 0.5);
      glVertex2d(rotation_centerX + 5, - rotation_centerY);
      glVertex2d(rotation_centerX - 5, -rotation_centerY);
      glVertex2d(rotation_centerX, -rotation_centerY + 5);
      glVertex2d(rotation_centerX, -rotation_centerY - 5);
      glEnd();
  }

  //this is the loop where we draw the object currently being created

  
  if (currObject!=NULL)
    {
      glLineWidth(3);
      //first we draw the edges
      glColor3d(currObject->r/255.0, currObject->g/255.0, currObject->b/255.0);
      glBegin(GL_LINE_STRIP);
      for (i=0; i<currObject->numVertices; i++)
	{
	  glVertex2d(currObject->keyframes[0].vertices[i].x, -currObject->keyframes[0].vertices[i].y);
	}
      glEnd();

      //then we draw the vertices
      glColor3d(0, 1, 1);
      glBegin(GL_LINES);
      for (j=0; j<currObject->numVertices; j++)
	{
	  glVertex2d(currObject->keyframes[0].vertices[j].x+5, -currObject->keyframes[0].vertices[j].y);
	  glVertex2d(currObject->keyframes[0].vertices[j].x-5, -currObject->keyframes[0].vertices[j].y);
	  glVertex2d(currObject->keyframes[0].vertices[j].x, -currObject->keyframes[0].vertices[j].y+5);
	  glVertex2d(currObject->keyframes[0].vertices[j].x, -currObject->keyframes[0].vertices[j].y-5);

	}
      glEnd();
      glLineWidth(1);
    }
  glutSwapBuffers();
}

void DeleteObject(int id)
{
  int i;
  if (id==-1) return;
  AnimObject* tmp = objects[id];
  for (i=id; i<numObjects-1; i++)
    {
      objects[i] = objects[i+1];
    } 
  free(tmp);
  numObjects--;
  
  glutPostRedisplay();
}

void myKeyboardFunc(unsigned char key, int x, int y)
{
  switch(key)
    {
    case 8: 
    case 127: DeleteObject(selectedObject); selectedObject = -1; break;
    case '.': frame_spinner->set_int_val(currFrameNumber+1); break;
    case ',': frame_spinner->set_int_val(currFrameNumber-1); break;
    }
}

void
polygon_scaling( int mx, int my )
{
    if ( selectedVertex == -1 ) return;

    mx -= rotation_centerX;
    my -= rotation_centerY;

    if ( fabs( prev_rotationX - mx ) < 10 && fabs( prev_rotationY - my ) < 10 ) return;

    float dm = sqrt( mx*mx + my*my );
    float dp = sqrt( prev_rotationX*prev_rotationX + prev_rotationY*prev_rotationY );

    Point* vert = objects[selectedObject]->keyframes[FindKeyframe(selectedObject, currFrameNumber)].vertices;
    int vertno = objects[selectedObject]->numVertices;

    float sx = ( dm > dp ) ? 1.2 : 0.8;
    float sy = ( dm > dp ) ? 1.2 : 0.8;

    for ( int ii = 0; ii < vertno; ++ii )
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

void
polygon_rotation( int mx, int my )
{
    if ( selectedVertex == -1 ) return;

    mx -= rotation_centerX;
    my -= rotation_centerY;

    if ( fabs( prev_rotationX - mx ) < 10 && fabs( prev_rotationY - my ) < 10 ) return;
    if ( (mx < 0 && prev_rotationX > 0) || (mx > 0 && prev_rotationX < 0) )
    {
        prev_rotationX = mx;
        prev_rotationY = my;
        return;
    }

    float ro = sqrtf( mx*mx + my*my );
    float al = asinf( my / ro );
    ro = sqrtf( prev_rotationX*prev_rotationX + prev_rotationY*prev_rotationY );
    float be = asinf( prev_rotationY / ro );
    float sign = ( (al > be && mx > 0) || (al < be && mx < 0) ) ? 1.0 : -1.0;
    float te = sign * 3.14159 / 18;
    float sn = sinf( te );
    float cs = cosf( te );
    
    Point* vert = objects[selectedObject]->keyframes[FindKeyframe(selectedObject, currFrameNumber)].vertices;
    int vertno = objects[selectedObject]->numVertices;

    for ( int ii = 0; ii < vertno; ++ii )
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

void myMotionFunc(int mx, int my)
{
  int frameID;

  if ( draw_curve )
  {
      if (currObject->numVertices == MAX_VERTICES) return;
      int px = currObject->keyframes[0].vertices[currObject->numVertices - 1].x;
      int py = currObject->keyframes[0].vertices[currObject->numVertices - 1].y;
      if ( fabs( px - mx ) > 7 || fabs( py - my ) > 7 ) // sqrt( (px-mx)*(px-mx) + (py-my)*(py-my) ) > 5 )
      {
          currObject->keyframes[0].vertices[currObject->numVertices].x = mx;
          currObject->keyframes[0].vertices[currObject->numVertices].y = my;
          currObject->numVertices++;
      }
      CheckDeleteKeyframeStatus();
      glutPostRedisplay();
      return;
  }

  if (selectedObject == -1) return;

  if ((frameID = FindKeyframe(selectedObject, currFrameNumber)) == -1) //look for a keyframe at this frame
    {
      if (objects[selectedObject]->numKeyframes == MAX_KEYFRAMES) return;
      //if we don't find it, then create a new one in the right place
      int insertLoc, i;
      Point vertices[MAX_VERTICES];
      GetVertices(selectedObject, currFrameNumber, vertices);

      for (insertLoc = 0; insertLoc<objects[selectedObject]->numKeyframes; insertLoc++)
	{
	  if (currFrameNumber<objects[selectedObject]->keyframes[insertLoc].frameNumber)
	    break;
	}

      //shift everything over
      for (i=objects[selectedObject]->numKeyframes-1; i>=insertLoc; i--)
	{
	  memcpy(&(objects[selectedObject]->keyframes[i+1]),
		 &(objects[selectedObject]->keyframes[i]),
		 sizeof(FrameData));
		}
      
      memcpy(objects[selectedObject]->keyframes[insertLoc].vertices,
	     vertices, MAX_VERTICES*sizeof(Point));

      objects[selectedObject]->keyframes[insertLoc].frameNumber = currFrameNumber;

      objects[selectedObject]->numKeyframes++;
      frameID = FindKeyframe(selectedObject, currFrameNumber);
      assert(frameID!=-1);
    }

  if ( scale_polygon )
  {
      polygon_scaling( mx, my );
      CheckDeleteKeyframeStatus();
      glutPostRedisplay();
      return;
  }
  if ( rotate_polygon )
  {
      polygon_rotation( mx, my );
      CheckDeleteKeyframeStatus();
      glutPostRedisplay();
      return;
  }

  rotation_centerX = -1;

  if (selectedVertex != -1)
    {
      objects[selectedObject]->keyframes[frameID].vertices[selectedVertex].x = mx;
      objects[selectedObject]->keyframes[frameID].vertices[selectedVertex].y = my;
    }
  else
    {
      int i;
      for (i=0; i<objects[selectedObject]->numVertices; i++)
	{
	  objects[selectedObject]->keyframes[frameID].vertices[i].x += (mx - originalX);
	  objects[selectedObject]->keyframes[frameID].vertices[i].y += (my - originalY);
	}
      originalX = mx;
      originalY = my;
    }
  CheckDeleteKeyframeStatus();
  glutPostRedisplay();
}

void AssignRandomColor(AnimObject* obj)
{
  //now assign the thing a random color (not too dark)
  colorToPick++;
  switch(colorToPick%3)
    {
    case 0: 
      obj->r = 255; 
      obj->g = rand()%255;
      obj->b = rand()%255;
      break;
    case 1:
      obj->r = rand()%255;
      obj->g = 255;
      obj->b = rand()%255;
      break;
    case 2:
      obj->r = rand()%255;
      obj->g = rand()%255;
      obj->b = 255;
      break;
    }
}

bool
select_object( int button, int mx, int my )
{
    bool foundVertex = false;
    for ( int i=0; i<numObjects; i++)
    {
        Point vertices[MAX_VERTICES];
        GetVertices(i, currFrameNumber, vertices);
        for ( int j=0; j<objects[i]->numVertices; j++)
        {
            Point vert = vertices[j];
            if (fabs(vert.x - mx)<5 && fabs(vert.y - (my))<5) //check for proximity
            {
                foundVertex = true;
                selectedObject = i;
                selectedVertex = j;
                
                //implement right-click drag
                if (button == GLUT_RIGHT_BUTTON)
                {
                    selectedVertex = -1;
                    originalX = mx;
                    originalY = my;
                }
            }
        }
    }
    return foundVertex;
}


void
myMouseFunc( int button, int state, int mx, int my )
{
    int modifier = glutGetModifiers();

    if ( state == GLUT_UP )
    {
        rotate_polygon = false;
        scale_polygon = false;
        draw_curve = false;
        goto end;
    }

    switch ( modifier )
    {
    case GLUT_ACTIVE_SHIFT:
        {
            //clear the selection

            selectedObject = -1;
            selectedVertex = -1;

            //create a new object if one isn't being drawn

            if (currObject == NULL)
            {
                if (numObjects == MAX_OBJECTS) return;
                currObject = (AnimObject*)malloc(sizeof(AnimObject));
                currObject->numVertices = 0;
                currObject->numKeyframes = 1;
                currObject->keyframes[0].frameNumber = 1;
                AssignRandomColor(currObject);   
            }
            if (currObject->numVertices == MAX_VERTICES) return;
            currObject->keyframes[0].vertices[currObject->numVertices].x = mx;
            currObject->keyframes[0].vertices[currObject->numVertices].y = my;
            currObject->numVertices++;
            draw_curve = true;
        }
        rotation_centerX = -1;
        break;
    case GLUT_ACTIVE_CTRL:
        {
            if ( select_object( button, mx, my ) && ( rotation_centerX != -1 ) )
            {
                scale_polygon = false;
                rotate_polygon = true;
                prev_rotationX = mx - rotation_centerX;
                prev_rotationY = my - rotation_centerY;
            }
            else
            {
                rotation_centerX = mx;
                rotation_centerY = my;
            }
        }
        break;
    case GLUT_ACTIVE_CTRL | GLUT_ACTIVE_SHIFT:
        {
            if ( select_object( button, mx, my ) && ( rotation_centerX != -1 ) )
            {
                scale_polygon = true;
                rotate_polygon = false;
                prev_rotationX = mx - rotation_centerX;
                prev_rotationY = my - rotation_centerY;
            }
        }
        break;
    default:
        {
            // if we're in the middle of drawing something, then end it
            if (currObject!=NULL)
            {
                //if we don't have a polygon

                if (currObject->numVertices < 3)
                {
                    free(currObject);
                }
                else
                {
                    objects[numObjects] = currObject;
                    numObjects++;
                }
                currObject = NULL;
            }

            //now, if there's a vertex in the area, select it

            if ( ! select_object( button, mx, my ) )
            {
                selectedObject = -1;
                selectedVertex = -1;
            }
        }
        rotation_centerX = -1;
    }

 end:

    UpdateInfo(selectedObject);
    glutPostRedisplay();
}

void UpdateInfo(int selectedObject)
{
  char buf1[1024], buf2[1024];
  if (selectedObject!=-1)
    {
      sprintf(buf1, "Object ID: %d", selectedObject);
      sprintf(buf2, "Vertices: %d", objects[selectedObject]->numVertices);
      object_id_statictext->set_text(buf1);
      object_verts_statictext->set_text(buf2);
    }
  else
    {
      object_id_statictext->set_text("Object ID:");
      object_verts_statictext->set_text("Vertices:");
    }
}

void CheckDeleteKeyframeStatus()
{
  if (!AnyKeyframe(currFrameNumber) || currFrameNumber == 1)
    {
      delete_keyframe_button->disable();
    }
  else
    {
      delete_keyframe_button->enable();
    }
}

void FrameChangedCall(int id)
{
  CheckDeleteKeyframeStatus();
}

void DeleteKeyframeCall(int id)
{
  int i, j, frameID=-1;
  if (!AnyKeyframe(currFrameNumber) || currFrameNumber == 1) return;
	
  for (i=0; i<numObjects; i++)
    {
      if ((frameID=FindKeyframe(i, currFrameNumber))!=-1)
	{
	  for (j=frameID; j<objects[i]->numKeyframes-1; j++)
	    {
	      memcpy(&(objects[i]->keyframes[j]),
		     &(objects[i]->keyframes[j+1]), 
		     sizeof(FrameData));
	    }
	  objects[i]->numKeyframes--;
	}
    }
  CheckDeleteKeyframeStatus();
}

void SaveObjectsCall(int id)
{
  char buf[1024];
  if (strlen(save_load_file)==0) return;
  sprintf(buf, "%s.obs", save_load_file);
  SaveObjects(buf);
}

void SaveObjects(char* filename)
{
  //char buf[1024];
  int i, j, k;
  //check if there's something in the filename field

  FILE* outfile = fopen (filename, "w");
  if (outfile == NULL) return;

  //write out the number of objects
  fprintf(outfile, "Number of Objects: %d\n", numObjects);
  
  //write out each object
  for (i=0; i<numObjects; i++)
    {
      fprintf(outfile, "Object Number: %d\n", i);
      fprintf(outfile, "Color: r: %d, g: %d, b: %d\n", objects[i]->r,
	      objects[i]->g, objects[i]->b);
      fprintf(outfile, "Number of Vertices: %d\n", objects[i]->numVertices);
      fprintf(outfile, "Number of Keyframes: %d\n", objects[i]->numKeyframes);
      for (j=0; j<objects[i]->numKeyframes; j++)
	{
	  fprintf(outfile, "Keyframe for Frame %d\n",
	  objects[i]->keyframes[j].frameNumber);
	  for (k=0; k<objects[i]->numVertices; k++)
	    {
	      fprintf(outfile, "Vertex %d, x: %g, y: %g\n", k,
		      objects[i]->keyframes[j].vertices[k].x,
		      objects[i]->keyframes[j].vertices[k].y);
	    }
	}
    }
  fclose(outfile);
}

void LoadObjectsCall(int id)
{
  char buf[1024];
  if (strlen(save_load_file)==0) return;

  sprintf(buf, "%s.obs", save_load_file);
  LoadObjects(buf);
  glutPostRedisplay();
}

void LoadObjects(char* filename)
{
  char buf[1024];
  int i, j, k, dummy;
  //check if there's something in the filename field
  
  FILE* infile = fopen(filename, "r");
  if (infile==NULL) return;

  //free all of the associated memory and initialize
  for (i=0; i<numObjects; i++)
    {
      free(objects[i]);
    }
  
  fscanf(infile, "Number of Objects: %d\n", &numObjects);

  for (i=0; i<numObjects; i++)
    {
      objects[i]= (AnimObject*) malloc(sizeof(AnimObject));
      fscanf(infile, "%[^\n]\n", buf);
      fscanf(infile, "Color: r: %d, g: %d, b: %d\n", &objects[i]->r,
	     &objects[i]->g, &objects[i]->b);
      fscanf(infile, "Number of Vertices: %d\n",
	     &objects[i]->numVertices);
      fscanf(infile, "Number of Keyframes: %d\n",
	     &objects[i]->numKeyframes);
      for (j=0; j<objects[i]->numKeyframes; j++)
	{
	  fscanf(infile, "Keyframe for Frame %d\n",
	  &objects[i]->keyframes[j].frameNumber);
	  for (k=0; k<objects[i]->numVertices; k++)
	    {
	      fscanf(infile, "Vertex %d, x: %g, y: %g\n", &dummy,
		     &objects[i]->keyframes[j].vertices[k].x,
		     &objects[i]->keyframes[j].vertices[k].y);
	    }
	}
      
    }
  fclose(infile);
}

void SingMultChanged(int id)
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

void AntiAliasChanged(int id)
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

void MotionBlurChanged(int id)
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

void ParseFilename(char* filename, char* pathless)
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


void DisplayAndSaveCanvas(int id)
{
	int i;
	char buf[1024], pathless[1024];
	if (singMult == 0)
	{
	  Rasterize(renderCanvas, currFrameNumber, antiAlias, numAliasSamples, motionBlur, numBlurSamples);
	  if (strlen(renderOut)!=0)
	    {
	      sprintf(buf, "%s.ppm", renderOut);
	      SaveCanvas(buf, renderCanvas);
	    }
	  glutSetWindow(render_window);
	  glutShowWindow();
	  glutPostRedisplay();
	  glutSetWindow(main_window);
	}
	else
	{
	  ParseFilename(renderOut, pathless);
	  sprintf(buf, "%s.list", renderOut);
	  FILE* listFile = fopen(buf, "w");
	  assert(listFile!=NULL);

	  for (i=startFrame; i<=endFrame; i++)
	    {
	      Rasterize(renderCanvas, i, antiAlias, numAliasSamples, motionBlur, numBlurSamples);
	      if (strlen(renderOut)!=0)
		{
		  sprintf(buf, "%s.%d.ppm", renderOut, i);
		  SaveCanvas(buf, renderCanvas);
		  sprintf(buf, "%s.%d.ppm", pathless, i);
		  fprintf(listFile, "%s\n", buf);
		}
	      glutSetWindow(render_window);
	      glutShowWindow();
	      glutPostRedisplay();
	      glutSetWindow(main_window);
	    }

	  fclose(listFile);
	}
	

}

//Change made Oct 15 2008 by Myers Abe Davis:  I changed RenderDisplay
//from rendering a bunch of points to explicitly rendering pixels.
void RenderDisplay()
{
  glClearColor( 0.f, 0.f, 0.f, 0.f );
  glClear( GL_COLOR_BUFFER_BIT );
  glRasterPos2s(0,0);
  glPixelZoom(1.0, -1.0);
  glDrawPixels(renderCanvas->Width, renderCanvas->Height, GL_RGBA, GL_UNSIGNED_BYTE, renderCanvas->Pixels);
  glutSwapBuffers();
}


void CommandLineRasterize(int argc, char* argv[])
{
  int i = 1; //step through the command line
  int motionBlur = 0, antiAlias = 0;
  int numAliasSamples = 0, numBlurSamples = 0;
  int startFrame, endFrame;
  char inputFile[1024], outputFile[1024], pathless[1024], buf[1024];

  if (strcmp(argv[i], "-help")==0)
    {
      printf("Usage: animgui [-a<#samples>] [-m<#samples>] <startframe> <endframe> <infile> <outfile>\n");
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
  LoadObjects(inputFile);

  ParseFilename(outputFile, pathless);
  sprintf(buf, "%s.list", outputFile);
  FILE* listFile = fopen(buf, "w");
  assert(listFile!=NULL);

  for (i=startFrame; i<=endFrame; i++)
    {
      printf("Rendering Frame %d\n", i);
      Rasterize(renderCanvas, i, antiAlias, numAliasSamples, motionBlur, numBlurSamples);
      sprintf(buf, "%s.%d.ppm", outputFile, i);
      SaveCanvas(buf, renderCanvas);
      sprintf(buf, "%s.%d.ppm", pathless, i);
      fprintf(listFile, "%s\n", buf);
    }
  fclose(listFile);

}
