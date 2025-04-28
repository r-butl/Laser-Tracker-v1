# Tracking a laser on a light background
The goal of this project is to track a laser on a varying-intensity background. The algorithm was developed from scratch in C++ with mininal help from OpenCV. 

![Demo](video-preview.gif)

## Algorithm

### Convert image to Grayscale
The algorithm begins by by converting the image to grayscale. This step reduces the computational complexity of the pipeline and pivots the focus of to overall pixel intensity.

### Foreground segmentation using a Temporal Median Filter
Segmenting the foreground from the background cam help eliminate unnecessary information. This is done using a Temporal Median Filter with a buffer size of 20 frames. For each (x, y) pixel in the image the median value is calculated, generating a background model. This method isolates the moving foreground by filtering out static components when the background is subtracted.

### Background removal and intensity normalization
The background subtraction operation tends to reduce the intensity of the target foreground, which makes the laser dot harder to track. The dim bright spot that is the laser is restored back to full intensity using a min-max normalization operation. The bright spot is returned to the highest intensity value with this operation, however, noise remains present in the image due to slight variations in the background over time.

### Binary mask creation using a threshold
A binary mask is generated from the noisy light intensity map by setting pixel values under a threshold value 122 to 0 and over to 255. Noisy specs that are artifacts from the background removal process are occasionally bright enough to pass the threshold. This results in singular pixels scattered throughout the mask that must be removed.

### Morphological Opening
To remove the stray specs produced by the thresholding function, Morphological Opening with a 3x3 kernel is applied to the mask. This operation first erodes the image by removing pixels who lack sufficient neighboring support within the kernel. Following is the dilation step, which restores pixels based on the surrounding mask structure. This operation proved to work very well for removing noise and preserving the general shape of the high intensity region.

### COM Calucation and crosshair drawing
Finally, the centroid of the high intensity region (laser) is estimated using a Center of Mass calculation on the binary image. This approximation is used to draw crosshairs on the image, which hover over the center of the laser dot.

## Limitations
This algorithm works very well for most cases, however suffers when the laser dot is stationary for too many frames, especially when the background intensity is nearly as high as the laser dot intensity. One way to alleviate this is to increase the buffer size of the Temporal Median filter, with the trade off of higher computational overhead and lower frame rate.

## Implementation
Profiling with the 'gprof' command uncovered that the bottleneck function is the Temporal Median filter computation due to the sorting of pixel values along the buffer axis. 



