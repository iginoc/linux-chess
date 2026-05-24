CXX = g++
CXXFLAGS = -c -Wall
LDFLAGS = -lsfml-graphics -lsfml-window -lsfml-system

TARGET = linux-chess
SOURCES = linux-chess.cpp
OBJECTS = $(SOURCES:.cpp=.o)

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) $(OBJECTS) -o $(TARGET) $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $< -o $@

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(OBJECTS) $(TARGET)

.PHONY: all run clean
