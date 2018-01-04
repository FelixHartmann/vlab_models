OS=$(shell uname -s)
OBJECTS=model.o geom2.o geom.o global.o io.o mem.o merge.o poly2.o poly.o qhull.o qset.o stat.o user.o CSVStream.o # cellflips.o ply.o cell.o drawer_base.o drawer.o #shader.o #pca.o
default: model.vve

standalone: model

include $(shell vveinterpreter --makefile)
#CXX=g++-4.7

# Uncomment the next line to compile with debug options
#CXXFLAGS=$(CXXFLAGS_DEBUG)
# Add extra compilation options here
CXXFLAGS+=-W -Wall -frounding-math -I/usr/local/include -fopenmp
# Add extra libraries here
LIBS+=-lcellflips # -fsanitize=address -fsanitize=undefined -fno-omit-frame-pointer
ifeq ($(OS), Darwin)
LIBS+=-L/usr/local/lib
else
LIBS+=
endif
# Add extra linking options here
#    for compiling the model as a library (.vve)
LD_SO_FLAGS+=-fopenmp
#    for compiling the model as a stand-alone program
LD_EXE_FLAGS+=-fopenmp

model.o: model.moc structure.h draw.h complex_drawer.h complex_drawer.moc solvergraph_drawer.h # cellflips.h ply.o cell.h chain.h cellflips_utils.h cellflipslayer.h cellflipsinvariant.h # drawer.h drawer_base.h dirichlet.h #complex.h shader.h #pca.h

#celltuple.o: cellflips.h cell.h chain.h cellflips_utils.h cellflipslayer.h cellflipsinvariant.h

#cell.o: cell.h

#drawer.o: complex.h drawer.h drawer_base.h drawer.moc

#drawer_base.o: drawer_base.h drawer_base.moc

model.vve: $(OBJECTS)

model${EXESUFFIX}: $(OBJECTS)

run:
	vveinterpreter model.vve --thread # --full for lateral docks

doc:
	doxygen Doxyfile

%.o: %.c
	$(CC) -c -o $@ $< -fpic $(CXXFLAGS) -fno-strict-aliasing
