import numpy
import time
from pypylon import pylon


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




