# Include debugging symbols (-g) or optimize code (-O or -O2).

DEBUG=-g

LIBS=-L/opt/local/lib -lGL -lGLU -lglut -lglui

# The object files comprising the application code.
# Use spaces to separate multiple files.

OBJS=canvas.o objects.o animgui.o

# The name of the executable.

TARGET=../bin/rasterizer

# LINKING.

$(TARGET): $(OBJS) Makefile
	$(CXX) -Wall $(DEBUG) -o $@ $(OBJS) $(LIBS) 


# COMPILATION.

.cpp.o:
	$(CXX) -Wall -Wno-write-strings -I/opt/local/include $(DEBUG) -c -o $@ $*.cpp


# CLEANUP.

clean:
	rm -f $(OBJS) $(TARGET)
