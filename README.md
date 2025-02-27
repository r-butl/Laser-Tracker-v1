# Tracking a laser on a light background
This is a computer vision project I completed recently where the goal is to track a laser on a varying intensity background.

![Demo](video-preview.gif)

## Algorithm
The algorithm I developed starts with a Temporal Median filter with a frame buffer of 20 frames to segment the background from the foreground. The median background image produced is then subtracted from the current frame to expose the moving dot. After normalizing the resulting image, the image is passed through a binary thresholding function to remove low intensity values from the differed image, and highlight high intensity ones. The results of this operation occasionally leave speckles that are high-enough intenisty to make it through the mask, but are not connectly to any blobs. They are 'on an island'. This is problem-some in later operations when the laser dot location is caluculated using the center of mass of the mask. To remove these speckles, I applied morphological opening with a kernel size of 3x3, which removes the speckles and preserves the laser dot. Finally, the center of mass of the masked-image is calculated and used to draw the crosshairs on the laser dot.

## Limitations
This algorithm works very well for most cases, however suffers when the laser dot is stationary for too many frames, especially when the background intensity is nearly as high as the laser dot's intensity itself. One way to help with this is to increase the buffer size of the Temporal Median filter with the trade off of high computational overhead and potentially lower frame rate.

## Implementation
Profiling with the 'gprof' command uncovered that the bottleneck function is the Temporal Median filter operation. This makes sense as the median pixel for each pixel must be calculated every time a frame is captured, which involved sorting the pixels along the buffer axis. With deeper buffer sizes, this operations becomes increasingly heavy. 

The intial implementation was slowed down by allocation operations due to the use of a deque to store the frames and the creation of a new vector for each median calculation. The main optimization was creating a circular buffer to store the frames as well as reusing the same vector to store the pixels before the median sort operation is performed, instead of allocating a new vector for each pixel.



