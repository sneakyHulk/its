from pypylon import pylon
import numpy as np
import cv2
import datetime, os, fnmatch, shutil

camera_serial_to_name = {
    "40580664": "front_left",
    "40580656": "front_mid",
    "40580658": "front_right",
    "40580666": "mid_left",
    "40580651": "mid_right",
    "40580657": "rear_left",
    "40542479": "rear_mid",
    "40539559": "rear_right"
}

#################### Basler Camera ####################
# Connecting to the first available camera.

devices = pylon.TlFactory.GetInstance().EnumerateDevices()

camera_array = pylon.InstantCameraArray(len(devices))
for camera, device in zip(camera_array, devices):
    camera.Attach(pylon.TlFactory.GetInstance().CreateDevice(device))

camera_array.Open()
camera_array.StartGrabbing(pylon.GrabStrategy_LatestImageOnly)

converter = pylon.ImageFormatConverter()
converter.OutputPixelFormat = pylon.PixelType_BGR8packed
converter.OutputBitAlignment = pylon.OutputBitAlignment_MsbAligned

for camera in camera_array:
    os.makedirs(f"/media/bmw/data0/{camera_serial_to_name[camera.GetDeviceInfo().GetSerialNumber()]}/", exist_ok=True)

# Create VideoWriter object with MJPEG codec at 30 FPS and 640x480 resolution
i = 0
while True:
    if camera_array.IsGrabbing():
        grab_result = camera_array.RetrieveResult(5000, pylon.TimeoutHandling_ThrowException)

        if grab_result.GrabSucceeded():
            camera = camera_array[grab_result.GetCameraContext()]
            print(f"Camera {camera.GetDeviceInfo().GetModelName()} captured an image!")
            camera_name = camera_serial_to_name[camera.GetDeviceInfo().GetSerialNumber()]

            image = converter.Convert(grab_result)
            img = image.GetArray()

            # Resize image to 640x480
            img2 = cv2.resize(img, (640, 480), interpolation=cv2.INTER_LINEAR)

            timestamp = datetime.datetime.now().isoformat()
            cv2.imwrite(f"/media/bmw/data0/{camera_name}/{timestamp}.jpeg", img2)

            # Display the image
            cv2.imshow("Basler Camera Output " + str(grab_result.GetCameraContext()), img2)

            i = i + 1

            # Exit the loop if the 'q' key is pressed
            if cv2.waitKey(1) & 0xFF == ord('q'):
                break
        grab_result.Release()
    else:
        raise Exception("Camera is not grabbing!")


camera_array.StopGrabbing()
cv2.destroyAllWindows()