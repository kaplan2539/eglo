TARGET := egloapp
TARGET_OBJ := egloapp.o
TARGET_LDFLAGS += -lGLESv2 -leglo -L. -Wl,-rpath .

SHARED := libeglo.so
SHARED_OBJ := eglo.o
SHARED_LDFLAGS += -lEGL -lX11

CXXFLAGS += -O2 -std=c++14 -fPIC

all: $(TARGET) $(SHARED)

$(TARGET): $(TARGET_OBJ) $(SHARED)
	$(CXX) -o $@ $< $(TARGET_LDFLAGS)

$(SHARED): $(SHARED_OBJ)
	$(CXX) -shared $(CXXFLAGS) -o $@ $< $(SHARED_LDFLAGS)

clean:
	$(RM) $(TARGET) $(TARGET_OBJ) $(SHARED) $(SHARED_OBJ)
