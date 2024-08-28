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


tl_factory = pylon.TlFactory.GetInstance()

for dev_info in tl_factory.EnumerateDevices():
    if dev_info.GetDeviceClass() == 'BaslerGigE':
        cam_info = dev_info
        print(
            "using %s @ %s (%s), IP config = %s" % (
                cam_info.GetModelName(),
                cam_info.GetIpAddress(),
                cam_info.GetMacAddress(),
                format_ip_config(cam_info.GetIpConfigCurrent())
            )
        )
else:
    raise EnvironmentError("no GigE device found")
