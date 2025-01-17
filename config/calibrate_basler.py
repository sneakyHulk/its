from pypylon import pylon
import numpy as np
import cv2
import math
import time


def calibrate():
    #################### Basler Camera ####################
    # Connecting to the first available camera.
    camera = pylon.InstantCamera(pylon.TlFactory.GetInstance().CreateFirstDevice())

    # Grabbing continuously with minimal delay.
    camera.StartGrabbing(pylon.GrabStrategy_LatestImageOnly)
    converter = pylon.ImageFormatConverter()

    # Converting to opencv bgr format.
    converter.OutputPixelFormat = pylon.PixelType_BGR8packed
    converter.OutputBitAlignment = pylon.OutputBitAlignment_MsbAligned

    while True:
        if not camera.IsGrabbing():
            raise "Camera is not grabbing!"

        # Get current image
        grab_result = camera.RetrieveResult(5000, pylon.TimeoutHandling_ThrowException)

        if grab_result.GrabSucceeded():
            image = converter.Convert(grab_result)
            img = image.GetArray()

            break

    #################### Blob Detector ####################
    blob_params = cv2.SimpleBlobDetector_Params()

    # The pixels with value below min threshold and above max threshold will be set to zero.
    blob_params.minThreshold = 10
    blob_params.maxThreshold = 255

    camera_pixel_vertical, camera_pixel_horizontal = img.shape[:2]

    height_calibration_paper = 210  # A4
    width_calibration_paper = 297  # A4

    diameter_blob = 15
    min_pixels_diameter_blob = 10

    # Blobs whose areas are not in range will be erased from the input list of blobs.
    blob_params.filterByArea = True
    # Blob should be at least min_pixels_diameter_blob pixels in diameter to be considered.
    blob_params.minArea = math.pi * (min_pixels_diameter_blob / 2) ** 2
    # The maximum area of the blob should be the area of the blob when the calibration paper fills the entire image.
    blob_params.maxArea = math.pi * (max(camera_pixel_vertical / height_calibration_paper,
                                         camera_pixel_horizontal / width_calibration_paper) * diameter_blob / 2) ** 2

    # Filter by Circularity
    blob_params.filterByCircularity = True
    blob_params.minCircularity = 0.1

    # Filter by Convexity
    blob_params.filterByConvexity = True
    blob_params.minConvexity = 0.87

    # Filter by Inertia
    blob_params.filterByInertia = True
    blob_params.minInertiaRatio = 0.01

    # Create a detector with the parameters
    blob_detector = cv2.SimpleBlobDetector_create(blob_params)

    #################### Blob coordinates ####################

    num_blobs_vertically = 4
    num_blobs_horizontally = 11
    num_blobs = num_blobs_vertically * num_blobs_horizontally
    blob_coordinates = np.zeros((num_blobs, 3), np.float32)
    blob_coordinates[0] = (0, 0, 0)
    blob_coordinates[1] = (0, 40, 0)
    blob_coordinates[2] = (0, 80, 0)
    blob_coordinates[3] = (0, 120, 0)
    blob_coordinates[4] = (20, 20, 0)
    blob_coordinates[5] = (20, 60, 0)
    blob_coordinates[6] = (20, 100, 0)
    blob_coordinates[7] = (20, 140, 0)
    blob_coordinates[8] = (40, 0, 0)
    blob_coordinates[9] = (40, 40, 0)
    blob_coordinates[10] = (40, 80, 0)
    blob_coordinates[11] = (40, 120, 0)
    blob_coordinates[12] = (60, 20, 0)
    blob_coordinates[13] = (60, 60, 0)
    blob_coordinates[14] = (60, 100, 0)
    blob_coordinates[15] = (60, 140, 0)
    blob_coordinates[16] = (80, 0, 0)
    blob_coordinates[17] = (80, 40, 0)
    blob_coordinates[18] = (80, 80, 0)
    blob_coordinates[19] = (80, 120, 0)
    blob_coordinates[20] = (100, 20, 0)
    blob_coordinates[21] = (100, 60, 0)
    blob_coordinates[22] = (100, 100, 0)
    blob_coordinates[23] = (100, 140, 0)
    blob_coordinates[24] = (120, 0, 0)
    blob_coordinates[25] = (120, 40, 0)
    blob_coordinates[26] = (120, 80, 0)
    blob_coordinates[27] = (120, 120, 0)
    blob_coordinates[28] = (140, 20, 0)
    blob_coordinates[29] = (140, 60, 0)
    blob_coordinates[30] = (140, 100, 0)
    blob_coordinates[31] = (140, 140, 0)
    blob_coordinates[32] = (160, 0, 0)
    blob_coordinates[33] = (160, 40, 0)
    blob_coordinates[34] = (160, 80, 0)
    blob_coordinates[35] = (160, 120, 0)
    blob_coordinates[36] = (180, 20, 0)
    blob_coordinates[37] = (180, 60, 0)
    blob_coordinates[38] = (180, 100, 0)
    blob_coordinates[39] = (180, 140, 0)
    blob_coordinates[40] = (200, 0, 0)
    blob_coordinates[41] = (200, 40, 0)
    blob_coordinates[42] = (200, 80, 0)
    blob_coordinates[43] = (200, 120, 0)

    #################### Blob detection loop ####################

    # Number of samples in which all blobs are detected and from which calibration is performed.
    num_samples = 20
    found_samples = 0

    # Arrays to store object points and image points from all the images.
    objpoints = []  # 3d point in real world space
    imgpoints = []  # 2d points in image plane.

    # Tries to find the asymmetrical pattern until it is found. Then waits 2 seconds and tries again.
    next_try = time.time_ns()
    while found_samples < num_samples:
        if not camera.IsGrabbing():
            raise "Camera is not grabbing!"

        # Get current image
        grabResult = camera.RetrieveResult(5000, pylon.TimeoutHandling_ThrowException)

        if grabResult.GrabSucceeded():
            image = converter.Convert(grabResult)
            img = image.GetArray()

            # Blob detector works only with grayscale images.
            gray = cv2.cvtColor(img, cv2.COLOR_BGR2GRAY)

            # Detect blobs.
            keypoints = blob_detector.detect(gray)

            # Draw detected blobs as green circles. This helps cv2.findCirclesGrid() .
            img_with_keypoints = cv2.drawKeypoints(img, keypoints, np.array([]), (0, 255, 0),
                                                   cv2.DRAW_MATCHES_FLAGS_DRAW_RICH_KEYPOINTS)

            if next_try < time.time_ns():
                img_with_keypoints_gray = cv2.cvtColor(img_with_keypoints, cv2.COLOR_BGR2GRAY)
                # Find the circle grid
                ret, corners = cv2.findCirclesGrid(img_with_keypoints, (num_blobs_vertically, num_blobs_horizontally),
                                                   None,
                                                   flags=cv2.CALIB_CB_ASYMMETRIC_GRID)

                if ret:
                    objpoints.append(blob_coordinates)
                    # Refines the corner locations.
                    corners2 = cv2.cornerSubPix(img_with_keypoints_gray, corners, (11, 11), (-1, -1),
                                                (cv2.TERM_CRITERIA_EPS + cv2.TERM_CRITERIA_MAX_ITER, 30,
                                                 0.001))
                    imgpoints.append(corners2)
                    # Draw and display the corners.
                    img_with_keypoints = cv2.drawChessboardCorners(img, (num_blobs_vertically, num_blobs_horizontally),
                                                                   corners2, ret)

                    found_samples = found_samples + 1

                    # 2 seconds for relocation
                    next_try = time.time_ns() + 2000000000

            cv2.imshow("control image", img_with_keypoints)
            cv2.waitKey(1)

    cv2.destroyAllWindows()

    #################### Calibration ####################

    ret, mtx, dist, rvecs, tvecs = cv2.calibrateCamera(objpoints, imgpoints, gray.shape[::-1], None, None)
    print(mtx)
    print(dist)

    show_distortion(mtx, dist)


def show_distortion(mtx=np.array([[2.82879001e+03, 0.00000000e+00, 1.09241918e+03],
                                  [0.00000000e+00, 2.83777321e+03, 9.79090977e+02],
                                  [0.00000000e+00, 0.00000000e+00, 1.00000000e+00]]),
                    dist=np.array([[-0.14865853, 0.58814702, - 0.00104411, - 0.01651453, - 0.53782368]])):
    #################### Basler Camera ####################
    # Connecting to the first available camera.
    camera = pylon.InstantCamera(pylon.TlFactory.GetInstance().CreateFirstDevice())

    # Grabbing continuously with minimal delay.
    camera.StartGrabbing(pylon.GrabStrategy_LatestImageOnly)
    converter = pylon.ImageFormatConverter()

    # Converting to opencv bgr format.
    converter.OutputPixelFormat = pylon.PixelType_BGR8packed
    converter.OutputBitAlignment = pylon.OutputBitAlignment_MsbAligned

    while True:
        if not camera.IsGrabbing():
            raise "Camera is not grabbing!"

        # Get current image
        grab_result = camera.RetrieveResult(5000, pylon.TimeoutHandling_ThrowException)

        if grab_result.GrabSucceeded():
            image = converter.Convert(grab_result)
            img = image.GetArray()

            break

    camera_pixel_vertical, camera_pixel_horizontal = img.shape[:2]

    newcameramtx, roi = cv2.getOptimalNewCameraMatrix(mtx, dist, (camera_pixel_horizontal, camera_pixel_vertical), 1,
                                                      (camera_pixel_horizontal, camera_pixel_vertical))

    dst = cv2.undistort(img, mtx, dist, None, newcameramtx)

    img = cv2.resize(img, (0, 0), fx=0.5, fy=0.5)
    dst = cv2.resize(dst, (0, 0), fx=0.5, fy=0.5)

    while True:
        cv2.imshow("control image", img)
        cv2.waitKey(200)

        cv2.imshow("control image", dst)
        cv2.waitKey(200)


if __name__ == '__main__':
    show_distortion()
