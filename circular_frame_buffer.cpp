#include "circular_frame_buffer.h"

CircularFrameBuffer::CircularFrameBuffer(int size, cv::Size frameSize) : max_size(size), index(0) {
    buffer.resize(max_size, cv::Mat(frameSize, CV_8UC1, cv::Scalar(0)));
}
    
void CircularFrameBuffer::addFrame(const cv::Mat frame) {
    //printf("Before update: Sum of buffer[%d] = %d\n", index, cv::sum(buffer[index])[0]);
    buffer.at(index) = frame.clone();
    //printf("After update: Sum of buffer[%d] = %d\n", index, cv::sum(buffer[index])[0]);
    index = (index + 1) % max_size; // Move circular index
}
    
const std::vector<cv::Mat>* CircularFrameBuffer::getFrameBuffer() const {
    return &buffer; 
}

int CircularFrameBuffer::getBufferSize() const { return max_size; }
    
    