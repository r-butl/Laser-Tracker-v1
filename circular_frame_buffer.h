#ifndef CIRCULAR_BUFFER_H

#define CIRCULAR_BUFFER_H

#include <vector>
#include <opencv2/opencv.hpp>

class CircularFrameBuffer {
    public:
        CircularFrameBuffer(int size, cv::Size frameSize);
        
        void addFrame(const cv::Mat frame);
        const std::vector<cv::Mat>* getFrameBuffer() const;
        int getBufferSize() const;
    
    private:
        std::vector<cv::Mat> buffer;
        int max_size;
        int index;
};

#endif