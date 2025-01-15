## ITS

This repo is the result of work on source-agnostic cooperative perception.
It contains nodes for a pipeline to generate object lists.
Camera data from different ITS are to be merged with those of a vehicle.
There are 2 different approaches. In the first one a vehicle receives prepared object lists or detections from the ITS
and then merges them with its own data.
Here vehicle and ITS pipelines run in parallel and are combined at the end.
This requires high performance hardware at the ITS, but the transmission bandwidth is low.
In the second approach, a vehicle receives the individual image data streams from the ITS, and the rest of the ITS
pipeline runs additionally on the vehicle.

### How to get and use testing data

1. Register
   on [https://a9-dataset.innovation-mobility.com/register](https://a9-dataset.innovation-mobility.com/register).
2. Download `Dataset R4: TUMTraf_V2X_Cooperative_Perception_Dataset`
   from [https://a9-dataset.innovation-mobility.com/downloads](https://a9-dataset.innovation-mobility.com/downloads).
3. Extract to [data](data).

Now you can do the following:

```c++
CamerasSimulatorNode cams = make_cameras_simulator_node_tumtraf({{"s110_n_cam_8", std::filesystem::path(CMAKE_SOURCE_DIR) / "data" / "tumtraf_v2x_cooperative_perception_dataset" / "train" / "images" / "s110_camera_basler_north_8mm"},
     {"s110_o_cam_8", std::filesystem::path(CMAKE_SOURCE_DIR) / "data" / "tumtraf_v2x_cooperative_perception_dataset" / "train" / "images" / "s110_camera_basler_east_8mm"},
     {"s110_w_cam_8", std::filesystem::path(CMAKE_SOURCE_DIR) / "data" / "tumtraf_v2x_cooperative_perception_dataset" / "train" / "images" / "s110_camera_basler_south1_8mm"},
     {"s110_s_cam_8", std::filesystem::path(CMAKE_SOURCE_DIR) / "data" / "tumtraf_v2x_cooperative_perception_dataset" / "train" / "images" / "s110_camera_basler_south2_8mm"},
     {"s110_n_cam_8", std::filesystem::path(CMAKE_SOURCE_DIR) / "data" / "tumtraf_v2x_cooperative_perception_dataset" / "test" / "images" / "s110_camera_basler_north_8mm"},
     {"s110_o_cam_8", std::filesystem::path(CMAKE_SOURCE_DIR) / "data" / "tumtraf_v2x_cooperative_perception_dataset" / "test" / "images" / "s110_camera_basler_east_8mm"},
     {"s110_w_cam_8", std::filesystem::path(CMAKE_SOURCE_DIR) / "data" / "tumtraf_v2x_cooperative_perception_dataset" / "test" / "images" / "s110_camera_basler_south1_8mm"},
     {"s110_s_cam_8", std::filesystem::path(CMAKE_SOURCE_DIR) / "data" / "tumtraf_v2x_cooperative_perception_dataset" / "test" / "images" / "s110_camera_basler_south2_8mm"},
     {"s110_n_cam_8", std::filesystem::path(CMAKE_SOURCE_DIR) / "data" / "tumtraf_v2x_cooperative_perception_dataset" / "val" / "images" / "s110_camera_basler_north_8mm"},
     {"s110_o_cam_8", std::filesystem::path(CMAKE_SOURCE_DIR) / "data" / "tumtraf_v2x_cooperative_perception_dataset" / "val" / "images" / "s110_camera_basler_east_8mm"},
     {"s110_w_cam_8", std::filesystem::path(CMAKE_SOURCE_DIR) / "data" / "tumtraf_v2x_cooperative_perception_dataset" / "val" / "images" / "s110_camera_basler_south1_8mm"},
     {"s110_s_cam_8", std::filesystem::path(CMAKE_SOURCE_DIR) / "data" / "tumtraf_v2x_cooperative_perception_dataset" / "val" / "images" / "s110_camera_basler_south2_8mm"}});
...
cams.asynchronly_connect(other_node);
...
auto cams_thread = cams();
...
```

### How to get yolo inference model

1. Install python with a pip environment.
2. Install ultralatics:

```shell
pip install ultralatics
```

3. Get model:

```shell
yolo export model=yolo11m.pt format='torchscript' imgsz=480,640
```

4. Place model in proper folder to be able to download multiple models:

```shell
mkdir -p data/yolo/480x640
mv yolo11m.torchscript data/yolo/480x640
```

Now you can do the following:

```c++
YoloNode<480, 640> yolo({{"s110_n_cam_8", {1200, 1920}}, {"s110_s_cam_8", {1200, 1920}}, {"s110_o_cam_8", {1200, 1920}}, {"s110_w_cam_8", {1200, 1920}}}, std::filesystem::path(CMAKE_SOURCE_DIR) / "data" / "yolo" / "480x640" / "yolo11m.torchscript");
...
other_node1.asynchronly_connect(yolo);
yolo.asynchronly_connect(other_node2);
...
auto cams_thread = cams();
...
```

### How to get OpenDRIVE map

1. Ask at the institute.

