# Compiler and flags
CXX := g++
CXXFLAGS := -fopt-info-vec -O3 -fopenmp -Wall -Wextra -std=c++11 -pg `pkg-config --cflags opencv4`
LDFLAGS := `pkg-config --libs opencv4`

TARGET := track_laser
OBJS := circular_frame_buffer.o track_laser.o

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS) $(LDFLAGS)

track_lazer.o: track_laser.cpp circular_frame_buffer.h
	$(CXX) $(CXXFLAGS) -c track_laser.cpp -o track_laser.o

circular_frame_buffer.o: circular_frame_buffer.cpp circular_frame_buffer.h
	$(CXX) $(CXXFLAGS) -c circular_frame_buffer.cpp -o circular_frame_buffer.o

clean:
	rm -f $(TARGET) $(OBJS)