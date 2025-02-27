import cv2
import numpy as np

# Load input video
video_path = "Light-Room-Laser-Spot-with-Clutter.mpeg"
video_path = "Dark-Room-Laser-Spot-with-Clutter.mpeg"
cap = cv2.VideoCapture(video_path)

# Get video properties
frame_width = int(cap.get(3))
frame_height = int(cap.get(4))
fps = int(cap.get(cv2.CAP_PROP_FPS))

# Define output video writer
# output_path = "output_video.mp4"
# fourcc = cv2.VideoWriter_fourcc(*'mp4v')  # Codec
# out = cv2.VideoWriter(output_path, fourcc, fps, (frame_width, frame_height))

N_FRAMES = 30  # Number of frames to estimate background
accumulated_masks = []

while cap.isOpened():
    ret, frame = cap.read()
    if not ret:
        break  # End of video

    # Convert to RGB (OpenCV loads as BGR)
    frame = cv2.GaussianBlur(frame, (5, 5), 0)

    rgb_image = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB).astype(np.float32)

    # Extract color channels
    red = rgb_image[:, :, 0]
    green = rgb_image[:, :, 1]
    blue = rgb_image[:, :, 2]

    # Compute Red Dominance
    red_dominance = (red - np.maximum(green, blue)) / (red + 1)

    # Normalize red dominance to [0, 255]
    red_dominance = cv2.normalize(red_dominance, None, 0, 255, cv2.NORM_MINMAX).astype(np.uint8)

    # Threshold red dominance (keep highly red regions)
    _, red_dominance_mask = cv2.threshold(red_dominance, 200, 255, cv2.THRESH_BINARY)

    # Threshold red intensity (keep bright red regions)
    _, red_intensity_mask = cv2.threshold(red.astype(np.uint8), 200, 255, cv2.THRESH_BINARY)

    # Combine both masks using bitwise AND
    combined_mask = cv2.bitwise_and(red_dominance_mask, red_intensity_mask)

    accumulated_masks.append(combined_mask.astype(np.float32))

    if not len(accumulated_masks) < N_FRAMES:

        accumulated_masks.pop(0)
        avg_mask = np.mean(accumulated_masks[:-5], axis=0).astype(np.uint8)
        _, stationary_mask = cv2.threshold(avg_mask, 125, 255, cv2.THRESH_BINARY)

        #cv2.imshow("Stationary Mask", stationary_mask)

        moving_mask = cv2.bitwise_and(combined_mask, cv2.bitwise_not(stationary_mask))

        # Compute Center of Mass (COM) using image moments
        moments = cv2.moments(moving_mask)
        if moments["m00"] != 0:
            x_bar = int(moments["m10"] / moments["m00"])  # X Center of Mass
            y_bar = int(moments["m01"] / moments["m00"])  # Y Center of Mass
        else:
            x_bar, y_bar = frame_width // 2, frame_height // 2  # Default to center if no bright pixels

        # Draw X and Y crosshair lines
        crosshair_image = frame.copy()
        cv2.line(moving_mask, (0, y_bar), (frame_width, y_bar), (255, 255, 255), 2)  # Horizontal
        cv2.line(moving_mask, (x_bar, 0), (x_bar, frame_height), (255, 255, 255), 2)  # Vertical

        # Write processed frame to output video
        #out.write(crosshair_image)

        # Display (optional)
        cv2.imshow("Tracking", moving_mask)
        if cv2.waitKey(1) & 0xFF == ord('q'):
            break

# Release resources
cap.release()
#out.release()
cv2.destroyAllWindows()
