# Tracking a laser on a light background
This is a computer vision project where the goal is to track a laser on a varying-intensity background. The algorithm was developed from scratch with the aim of gaining experience optimizing code.

![Demo](video-preview.gif)

## Algorithm

### Convert image to Grayscale
The first step is converting the image to grayscale. This step reduces the computational complexity of the pipeline and changes the focus of the image to overall pixel intensity.

### Calculate the Median Image
To emphasize the moving bright spot in the image, segmenting the foreground and background helps eliminate unnecessary information. This is done using a Temporal Median Filter with a buffer size of 20 frames. For each (x, y) pixel in the image, the median value is calculated to generate a background model, effectively representing the stationary pixels. This method efficiently isolates the moving foreground by filtering out static components.

### Subtract the Median Image from the Current Frame and Normalization
The static background model is then subtracted from the current image, leaving the residual moving foreground. This operation subtracts intensity from the entire image, reducing the visibility of the brightspot. The dim bright spot is restored back to full intensity using a min-max normalization operation. Noise remains present in the image due to slight variations in the background over time, but relatively the bright spot is of highest intensity.

### Apply a Threshold Function to produce a Binary Mask
After the background-removed image has been normalized, a threshold function is applied to the image, setting pixel values under the intensity value 122 to 0 and over to 255. This process is not robust, and will occasionally leave singular, isolated pixels where the pixel value was bright enough to pass the threshold, but is not a part of a larger blob of pixels.

### Morphological Opening
To reduce noise from isolated pixels exceeding the threshold, a 3x3 kernel was used for Morphological Opening. This operation first erodes the image by removing pixels who lack sufficient neighboring support within the kernel. Following is the dilation step, which restores pixels based on the surrounding mask structure. This operation proved to work very well for removing noise and preserving the general shape of the high intensity region.

### Calculate the Center of Mask of the Binary Mask and Drawing the Crosshairs
Finally, the centroid of the high intensity region is estimated using a Center of Mass calculation on the binary image. This approximation is then used to draw crosshairs on the image.


## Limitations
This algorithm works very well for most cases, however suffers when the laser dot is stationary for too many frames, especially when the background intensity is nearly as high as the laser dot's intensity itself. One way to help with this is to increase the buffer size of the Temporal Median filter with the trade off of high computational overhead and potentially lower frame rate.

## Implementation
Profiling with the 'gprof' command uncovered that the bottleneck function is the Temporal Median filter operation. This makes sense as the median pixel for each pixel must be calculated every time a frame is captured, which involves sorting the pixels along the buffer axis. With deeper buffer sizes, this operations becomes increasingly heavy. 

The initial implementation was slowed down by allocation operations due to the use of a deque to store the frames and the creation of a new vector for each median calculation. The main optimization was creating a circular buffer to store the frames as well as reusing the same vector to store the pixels before the median sort operation is performed, instead of allocating a new vector for each pixel.



