CXX = g++
CXXFLAGS = -std=c++17 -Wall -O2  -I./include `pkg-config --cflags freetype2 icu-i18n icu-uc`
LIBS = `pkg-config --libs freetype2 icu-i18n icu-uc`

TARGET = FalcomFontCreator
SRCS = FalcomFontCreator.cpp
OBJS = $(SRCS:.cpp=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) -o $@ $^ $(LIBS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean
