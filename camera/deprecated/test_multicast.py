import os
import numpy
import time
import cv2
import numpy as np
from pypylon import pylon, genicam
import sys

exitCode = 0
try:
    def format_ip_config(cfg_str):
        result = []
        cfg = int(cfg_str)
        if cfg & 1:
            result.append("PersistentIP")
        if cfg & 2:
            result.append("DHCP")
        if cfg & 4:
            result.append("LLA")
        return ", ".join(result)


    devices = pylon.TlFactory.GetInstance().EnumerateDevices()
    cameras = pylon.InstantCameraArray(len(devices))

    for camera, device in zip(cameras, devices):
        if device.GetDeviceClass() == 'BaslerGigE':
            print(
                "using %s @ %s (%s), IP config = %s" % (
                    device.GetModelName(),
                    device.GetIpAddress(),
                    device.GetMacAddress(),
                    format_ip_config(device.GetIpConfigCurrent())
                )
            )
            camera.Attach(pylon.TlFactory.GetInstance().CreateDevice(device))
        else:
            raise EnvironmentError("device is no GigE device")
    cameras.StartGrabbing()

    for i in range(1000000):
        if not cameras.IsGrabbing():
            break

        grabResult = cameras.RetrieveResult(5000, pylon.TimeoutHandling_ThrowException)

        # When the cameras in the array are created the camera context value
        # is set to the index of the camera in the array.
        # The camera context is a user settable value.
        # This value is attached to each grab result and can be used
        # to determine the camera that produced the grab result.
        cameraContextValue = grabResult.GetCameraContext()

        # Print the index and the model name of the camera.
        print("Camera ", cameraContextValue, ": ", cameras[cameraContextValue].GetDeviceInfo().GetModelName(), " @ ",
              cameras[cameraContextValue].GetDeviceInfo().GetIpAddress(), " (",
              cameras[cameraContextValue].GetDeviceInfo().GetMacAddress(), ")")

        # Now, the image data can be processed.
        print("GrabSucceeded: ", grabResult.GrabSucceeded())
        if grabResult.GrabSucceeded():
            print("SizeX: ", grabResult.GetWidth())
            print("SizeY: ", grabResult.GetHeight())
            img = grabResult.GetArray()
            cv2.imshow("GrabSucceeded", img)
            cv2.waitKey(1)

            print("Gray value of first pixel: ", img[0, 0])

except genicam.GenericException as e:
    # Error handling
    print("An exception occurred:", e)
    exitCode = 1
except EnvironmentError as e:
    print("An exception occurred:", e)
    exitCode = 1

sys.exit(exitCode)
