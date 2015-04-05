CXXFLAGS=-std=c++11 -O2 -Ibass -Iglew/glew-1.12.0/include
LDFLAGS=libbass.so -lglfw -lSOIL -lGL
STATICS=glew/glew-1.12.0/build/lib/libGLEW.a glew/glew-1.12.0/build/lib/libGLEWmx.a
OUTFILE=invi

$(OUTFILE): k.i.t.t.o scn-cubewall.o scn-end.o scn-forever.o scn-ink.o scn-lazor.o scn-outstorm.o scn-park.o scn-pictures.o scn-rocket.o main.o
	cp bass/x64/libbass.so libbass.so
	$(CXX) -o $@ $(LDFLAGS) $^ $(LDFLAGS) $(STATICS)

%.o: %.cpp
	$(CXX) -c $(CXXFLAGS) $< -o $@

clean:
	rm -f $(OUTFILE) *.o
