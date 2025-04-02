CFLAGS := -Wall -std=gnu11
LDFLAGS := -lm -lhdf5
DEBUGFLAGS := -g -O0
RELEASEFLAGS := -O2

LDINCLUDEPATH =$(shell pkg-config --silence-errors --libs-only-L hdf5)

INCLUDEPATH =-I$(abspath ./inc/)
INCLUDEPATH +=$(shell pkg-config --silence-errors --cflags hdf5)

BUILDPATH = bin
SOURCES = $(shell find src -name '*.c')
OBJECTS = $(subst src/,, $(subst .c,.o, $(SOURCES)))

all: debug

.PHONY: debug
debug: CFLAGS += $(DEBUGFLAGS)
debug: LDFLAGS += $(DEBUGFLAGS)
debug: BUILDPATH := $(addprefix $(BUILDPATH), /debug/)
debug: mkdir
debug: profiler

.PHONY: release
release: CFLAGS += $(RELEASEFLAGS)
release: LDFLAGS += $(RELEASEFLAGS)
release: BUILDPATH := $(addprefix $(BUILDPATH), /release/)
release: mkdir
release: profiler

profiler: $(OBJECTS)
	$(info $(SOURCES))
	$(CC) $(addprefix $(BUILDPATH), $(OBJECTS)) $(LDFLAGS) $(LDINCLUDEPATH) -o $(addprefix $(BUILDPATH), $@)

$(OBJECTS): $(SOURCES)
	$(warn test)
	$(CC) $(INCLUDEPATH) $(CFLAGS) -c $(addprefix src/, $(subst .o,.c, $@)) -o $(addprefix $(BUILDPATH), $@)

.PHONY: doc
doc: mkdir
	doxygen doxygen.conf

.PHONY: mkdir
mkdir:
	mkdir -p $(BUILDPATH)

.PHONY: analyze
analyze:
	clang-tidy -checks='bugprone-*, cppcoreguidelines-*, clang-analyzer-*, misc-*, google-*, modernize-*, performance-*, readability-*, hicpp-*' $(SOURCES) -- $(INCLUDEPATH)

.PHONY: clean
clean:
	rm -rf $(BUILDPATH)
	rm -rf doc

