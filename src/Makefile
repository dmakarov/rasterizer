# Include debugging symbols (-g) or optimize code (-O or -O2).

DEBUG=-g

UNAME := $(shell uname)

ifeq ($(UNAME), Darwin)
  LIBS := -lglui -framework GLUT -framework OpenGL
  WARN := -Wall -Wno-deprecated-declarations -Wno-write-strings
endif
ifeq ($(UNAME), Linux)
  LIBS := -lGL -lGLU -lglut -lglui
  WARN := -Wall -Wno-write-strings
endif

# The object files comprising the application code.
# Use spaces to separate multiple files.

OBJS=canvas.o objects.o animgui.o

# The name of the executable.

TARGET=../bin/rasterizer

all: $(TARGET)

# LINKING.
$(TARGET): $(OBJS) Makefile ../bin
	$(CXX) $(DEBUG) -o $@ $(OBJS) $(LIBS) 

../bin:
	mkdir $@


# COMPILATION.

.cpp.o:
	$(CXX) $(WARN) $(INCL) $(DEBUG) -c -o $@ $*.cpp


# CLEANUP.

clean:
	rm -f $(OBJS) $(TARGET)
