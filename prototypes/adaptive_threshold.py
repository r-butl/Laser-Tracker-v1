import cv2
import numpy as np
from collections import deque

# Load input video
video_path = "Light-Room-Laser-Spot-with-Clutter.mpeg"
#video_path = "Dark-Room-Laser-Spot-with-Clutter.mpeg"
cap = cv2.VideoCapture(video_path)

# Get video properties
frame_width = int(cap.get(3))
frame_height = int(cap.get(4))
fps = int(cap.get(cv2.CAP_PROP_FPS))

cap = cv2.VideoCapture(video_path)

# Parameters
N_FRAMES = 20  # Number of frames to compute the median background
frame_buffer = deque(maxlen=N_FRAMES)  # Circular buffer to store frames

while cap.isOpened():
    ret, frame = cap.read()
    if not ret:
        break  # End of video
    
    # Convert to grayscale (or process a specific channel, e.g., red)
    gray_frame = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)

    # Add current frame to buffer
    frame_buffer.append(gray_frame)

    if len(frame_buffer) == N_FRAMES:
        # Compute the median background from stored frames
        median_background = np.median(np.array(frame_buffer), axis=0).astype(np.uint8)

        # Compute absolute difference (motion mask)
        motion_mask = cv2.absdiff(gray_frame, median_background)

        motion_mask = cv2.normalize(motion_mask, None, 0, 255, cv2.NORM_MINMAX)

        # Threshold to highlight moving objects
        _, motion_mask = cv2.threshold(motion_mask, 100, 255, cv2.THRESH_BINARY)

        # Apply smoothing to reduce the random pixels
        kernel = np.ones((3, 3), np.uint8)  # 3x3 kernel
        motion_mask = cv2.morphologyEx(motion_mask, cv2.MORPH_OPEN, kernel)

        moments = cv2.moments(motion_mask)
        if moments["m00"] != 0:
            x_bar = int(moments["m10"] / moments["m00"])  # X Center of Mass
            y_bar = int(moments["m01"] / moments["m00"])  # Y Center of Mass
        else:
            x_bar, y_bar = frame_width // 2, frame_height // 2  # Default to center if no bright pixels

        # Draw X and Y crosshair lines
        crosshair_image = frame.copy()
        cv2.line(crosshair_image, (0, y_bar), (frame_width, y_bar), (255, 255, 255), 2)  # Horizontal
        cv2.line(crosshair_image, (x_bar, 0), (x_bar, frame_height), (255, 255, 255), 2)  # Vertical


        # Display results
        cv2.imshow("video", crosshair_image)
        cv2.imshow("motion mask", motion_mask)


    if cv2.waitKey(1) & 0xFF == ord('q'):
        break

# Cleanup
cap.release()
cv2.destroyAllWindows()
