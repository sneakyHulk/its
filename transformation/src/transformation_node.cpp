#include "transformation_node.h"

#include <Eigen/Eigen>

#include "common_literals.h"

DetectionTransformation::DetectionTransformation(Config const& config) : config(config) {}
CompactObjects DetectionTransformation::function(Detections2D const& data) {
	CompactObjects object_list;
	object_list.timestamp = data.timestamp;

	for (auto const& e : data.objects) {
		double x_image_position = (e.bbox.left + e.bbox.right) / 2.;
		double y_image_position = e.bbox.top + (e.bbox.bottom - e.bbox.top) * (3. / 4.);

		Eigen::Vector4d const position = config.affine_transformation_map_origin_to_utm() * config.affine_transformation_bases_to_map_origin(config.camera_config(data.source).base_name()) *
		                                 config.map_image_to_world_coordinate<double>(data.source, x_image_position, y_image_position, 0.);

		object_list.objects.emplace_back(0_u8, e.object_class, std::array{position[0], position[1], position[2]}, std::array{0., 0., 0.});
	}

	return object_list;
}
ImageTrackingTransformation::ImageTrackingTransformation(Config const& config) : config(config) {}
CompactObjects ImageTrackingTransformation::function(ImageTrackerResults const& data) {
	CompactObjects object_list;
	object_list.timestamp = data.timestamp;

	for (auto const& e : data.objects) {
		Eigen::Vector4d const position = config.affine_transformation_map_origin_to_utm() * config.affine_transformation_bases_to_map_origin(config.camera_config(data.source).base_name()) *
		                                 config.map_image_to_world_coordinate<double>(data.source, e.position[0], e.position[1], 0.);

		object_list.objects.emplace_back(0_u8, e.object_class, std::array{position[0], position[1], position[2]}, std::array{0., 0., 0.});
	}

	return object_list;
}
GlobalTrackingTransformation::GlobalTrackingTransformation(Config const& config) : config(config) {}
CompactObjects GlobalTrackingTransformation::function(GlobalTrackerResults const& data) {
	CompactObjects object_list;
	object_list.timestamp = data.timestamp;

	for (auto const& e : data.objects) {
		Eigen::Vector4d const position = config.affine_transformation_map_origin_to_utm() * make_matrix<4, 1>(e.position[0], e.position[1], 0., 1.);

		object_list.objects.emplace_back(0_u8, e.object_class, std::array{position[0], position[1], position[2]}, std::array{0., 0., 0.});
	}

	return object_list;
}
