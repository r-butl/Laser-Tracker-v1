#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <syslog.h>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/videoio.hpp>

#include <cmath>
#include <deque>
#include <algorithm>

#include <pthread.h>
#include <omp.h>

#include "circular_frame_buffer.h"

using namespace cv;
using namespace std;

#define ESCAPE_KEY (27)
#define SYSTEM_ERROR (-1)
#define FRAME_BUFFER_SIZE (10)

void convertGrayScale(const cv::Mat& input_frame, cv::Mat& output_frame);
void medianFrame(const std::vector<cv::Mat>* frame_buffer, cv::Mat& output_frame);
void absDiff(const cv::Mat& input_frame_1, const cv::Mat& input_frame_2, cv::Mat& output_frame);
void normalize(cv::Mat& input_frame);
void binaryThresholdMask(cv::Mat& frame, float threshold);
bool allOverlap(const cv::Mat& frame, int x, int y, int kHalf, const cv::Mat& kernel);
void dilate(cv::Mat& input_frame, cv::Mat& target_frame, int x, int y, int kHalf, const cv::Mat& kernel);
void morphologicalOpening(cv::Mat& frame, const cv::Mat& kernel);
void calculateCOM(const cv::Mat& frame, int& x, int& y);

int main( int argc, char** argv )
{

    openlog("track_lazer", LOG_PID | LOG_CONS, LOG_USER);

    // Open the camera
    VideoCapture cam;
    bool video_stream;
 
    if (argc > 1){
        cam.open(argv[1]);  
        video_stream = true;
    }else{
        cam.open(0);
        if (!cam.isOpened())
        {
        exit(SYSTEM_ERROR);
        }
        video_stream = false;
        cam.set(CAP_PROP_FRAME_WIDTH, 640);
        cam.set(CAP_PROP_FRAME_HEIGHT, 480);
    }

    // Instantiate Data Structures
    cv::Mat current_frame;
    cam >> current_frame;
    cv::Mat gray_frame(current_frame.rows, current_frame.cols, CV_8UC1);

    CircularFrameBuffer frame_buffer(FRAME_BUFFER_SIZE, current_frame.size());

    // Preload the frame buffer
    for (int i = 0; i < FRAME_BUFFER_SIZE; i++){
        cam >> current_frame;
        convertGrayScale(current_frame, gray_frame);
        frame_buffer.addFrame(gray_frame);
    }

    cv::Mat median_background(current_frame.rows, current_frame.cols, CV_8UC1);
    cv::Mat motion_mask(current_frame.rows, current_frame.cols, CV_8UC1);

    cv::Mat morphologicalKernel = cv::Mat::ones(Size(3, 3), CV_8UC1);

    int x_crosshair;
    int y_crosshair;
    cv::Scalar color(255, 0, 0); // Blue color

    // Preview Windows
    //namedWindow("Preview");
    namedWindow("Result");

    while(1) {
        int c = waitKey(1);

        if (c == ESCAPE_KEY) {
            break;
        }    

        // Pull image and convert to gray scale
        cam >> current_frame;

        if (current_frame.empty()) break;
        convertGrayScale(current_frame, gray_frame);
        frame_buffer.addFrame(gray_frame);

        medianFrame(frame_buffer.getFrameBuffer(), median_background);

        absDiff(gray_frame, median_background, motion_mask);
        normalize(motion_mask);
        binaryThresholdMask(motion_mask, 0.50);

        morphologicalOpening(motion_mask, morphologicalKernel);

        calculateCOM(motion_mask, x_crosshair, y_crosshair);

        cv::line(current_frame, cv::Point(0, y_crosshair), cv::Point(current_frame.cols, y_crosshair), color, 2);
        cv::line(current_frame, cv::Point(x_crosshair, 0), cv::Point(x_crosshair, current_frame.rows), color, 2);

        cv::imshow("Result", current_frame);
        
    }

    cv::destroyAllWindows();

};

void convertGrayScale(const cv::Mat& input_frame, cv::Mat& output_frame){

    float red = 0.299;
    float green = 0.587;
    float blue = 0.114;

    for (int j = 0; j < input_frame.rows; j++){
        const uchar* input_row_ptr = input_frame.ptr<uchar>(j);
        uchar* output_row_ptr = output_frame.ptr<uchar>(j);

        for (int i = 0; i < input_frame.cols; i++){

            uchar blue_ptr = input_row_ptr[i * 3];
            uchar green_ptr = input_row_ptr[i * 3 + 1];
            uchar red_ptr = input_row_ptr[i * 3 + 2];

            int gray_value =  red_ptr * red + 
                              blue_ptr * blue + 
                              green_ptr * green;

            output_row_ptr[i] = gray_value;
        }
    }

    return;
}

void medianFrame(const std::vector<cv::Mat>* frame_buffer, cv::Mat& output_frame) {
    if (frame_buffer->empty()) return;

    cv::Size size = frame_buffer->front().size();

    // Ensure output frame is properly initialized
    if (output_frame.empty() || output_frame.size() != size)
        output_frame = cv::Mat::zeros(size, CV_8UC1);

    int buff_size = frame_buffer->size();

    std::vector<uchar> pixels(buff_size);

    for (int y = 0; y < size.height; y++) {
        uchar* output_row_ptr = output_frame.ptr<uchar>(y);

        for (int x = 0; x < size.width; x++) {
            
            for (int k = 0; k < buff_size; k++) {  // Use `const auto&` for efficiency
                pixels[k] = (*frame_buffer)[k].at<uchar>(y, x);
            }

            std::nth_element(pixels.begin(), pixels.begin() + pixels.size() / 2, pixels.end());
            output_row_ptr[x] = pixels[pixels.size() / 2];  // Assign median pixel
        }
    }
}

void normalize(cv::Mat& input_frame){

    int min = 255;
    int max = 0;
    uchar* data = input_frame.data;
    int size = input_frame.total();

    for (int i = 0; i < size; i++){

        if (data[i] < min) min = data[i];
        if (data[i] > max) max = data[i];
    }

    if (min == max) return;

    float range = 1.0f / (max - min);

    #pragma omp simd
    for (int i = 0; i < size; i++){
        data[i] = static_cast<uchar>(255.0f * (data[i] - min) * range);
    }

    return;
}

void binaryThresholdMask(cv::Mat& frame, float threshold){

    int pixel_threshold = float(threshold * 255.0);
    uchar* data = frame.data;
    int size = frame.total();

    #pragma omp simd
    for (int i = 0; i < size; i++){
    
        if (data[i] < pixel_threshold) {
            data[i] = 0;
        } else {
            data[i] = 255;
        }
    }

    return;
}

void absDiff(const cv::Mat& input_frame_1, const cv::Mat& input_frame_2, cv::Mat& output_frame){

    uchar* input_1_data = input_frame_1.data;
    uchar* input_2_data = input_frame_2.data;
    uchar* output_data = output_frame.data;

    #pragma omp simd
    for (int i = 0; i < input_frame_1.total(); i++){
        output_data[i] = static_cast<uchar>(abs(input_1_data[i] - input_2_data[i]));
    }

    return;
}

bool allOverlap(const cv::Mat& frame, int x, int y, int kHalf, const cv::Mat& kernel){

    // check if the kernel matches the subframe
    for (int ky = -kHalf; ky <= kHalf; ky++) {
        for (int kx = -kHalf; kx <= kHalf; kx++) {
            bool pixelValue = (frame.at<uchar>(y + ky, x + kx) > 0 );

            if (pixelValue != (kernel.at<uchar>(ky + kHalf, kx + kHalf) > 0)){
                return false;
            };
        }
    }

    return true;

}

void dilate(cv::Mat& input_frame, cv::Mat& target_frame, int x, int y, int kHalf, const cv::Mat& kernel){

    // Dialate the pixel
    if (input_frame.at<uchar>(y, x) > 0){

        for (int ky = -kHalf; ky <= kHalf; ky++) {
            for (int kx = -kHalf; kx <= kHalf; kx++) {
                target_frame.at<uchar>(y + ky, x + kx) = kernel.at<uchar>(ky + kHalf, kx + kHalf) * 255; 
            }
        }
    }
}

void morphologicalOpening(cv::Mat& frame, const cv::Mat& kernel) {

    if (kernel.rows != kernel.cols){
        syslog(LOG_ERR, "Kernel is not square\n");
        exit(SYSTEM_ERROR);
    };

    int kSize = kernel.rows; // Assume square kernel
    int kHalf = kSize / 2;

    cv::Mat eroded = cv::Mat::zeros(frame.size(), CV_8UC1);

    // Perform erosion
    for (int y = kHalf; y < frame.rows - kHalf; y++) {
        for (int x = kHalf; x < frame.cols - kHalf; x++) {
            eroded.at<uchar>(y, x) = 255 * allOverlap(frame, x, y, kHalf, kernel);
        }
    }

    // Perform Dialation
    cv::Mat output = eroded.clone();
    for (int y = kHalf; y < eroded.rows - kHalf; y++) {
        for (int x = kHalf; x < eroded.cols - kHalf; x++) {
            dilate(eroded, output, x, y, kHalf, kernel);
        }
    }

    frame = output.clone();
}


void calculateCOM(const cv::Mat& frame, int& x, int& y){
    int total_mass = 0;
    int total_x = 0;
    int total_y = 0;

    for (int y = 0; y < frame.rows; y++){
        for (int x = 0; x < frame.cols; x++){
            int val = frame.at<uchar>(y, x);
            total_x += val * x;
            total_y += val * y;
            total_mass += val;
        }
    }

    if (total_mass == 0) {  // Prevent division by zero
        x = 0;
        y = 0;
        return;
    }

    x = static_cast<int>(static_cast<float>(total_x) / total_mass);
    y = static_cast<int>(static_cast<float>(total_y) / total_mass);
}
