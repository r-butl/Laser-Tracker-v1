# Compiler and flags
CXX := g++
CXXFLAGS := -fopt-info-vec -O3 -fopenmp -Wall -Wextra -std=c++11 -pg `pkg-config --cflags opencv4`
LDFLAGS := `pkg-config --libs opencv4`

TARGET := track_lazer
OBJS := circular_frame_buffer.o track_lazer.o

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS) $(LDFLAGS)

track_lazer.o: track_lazer.cpp circular_frame_buffer.h
	$(CXX) $(CXXFLAGS) -c track_lazer.cpp -o track_lazer.o

circular_frame_buffer.o: circular_frame_buffer.cpp circular_frame_buffer.h
	$(CXX) $(CXXFLAGS) -c circular_frame_buffer.cpp -o circular_frame_buffer.o

clean:
	rm -f $(TARGET) $(OBJS)