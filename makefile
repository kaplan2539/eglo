TARGET := egloapp
TARGET_OBJ := egloapp.o
TARGET_LDFLAGS += -lGLESv2 -leglo -L. -Wl,-rpath .

SHARED := libeglo.so
SHARED_OBJ := eglo.o
SHARED_LDFLAGS += -lEGL -lGLESv2 -lGLESv1_CM -lX11

CXXFLAGS += -O2 -std=c++14 -fPIC

PREFIX ?= /usr/local
DESTDIR ?=

all: $(TARGET) $(SHARED)

$(TARGET): $(TARGET_OBJ) $(SHARED)
	$(CXX) -o $@ $< $(TARGET_LDFLAGS)

$(SHARED): $(SHARED_OBJ)
	$(CXX) -shared $(CXXFLAGS) -o $@ $< $(SHARED_LDFLAGS)

install: $(SHARED)
	mkdir -p $(DESTDIR)$(PREFIX)/lib $(DESTDIR)$(PREFIX)/include
	install -m 644 $(SHARED) $(DESTDIR)$(PREFIX)/lib/
	install -m 644 eglo.h $(DESTDIR)$(PREFIX)/include/

clean:
	$(RM) $(TARGET) $(TARGET_OBJ) $(SHARED) $(SHARED_OBJ)
