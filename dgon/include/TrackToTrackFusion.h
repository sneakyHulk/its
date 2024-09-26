#pragma once

#include <map>
#include <vector>

#include "AfterReturnTimeMeasure.h"
#include "BirdEyeVisualizationData.h"
#include "CompactObject.h"
#include "EigenUtils.h"
#include "ImageTrackerNode.h"
#include "common_literals.h"
#include "node.h"

class TrackToTrackFusionNode : public InputOutputNode<ImageTrackerResults2, CompactObjects> {
	struct TransformationConfig {
		std::vector<double> projection_matrix;
		std::vector<double> affine_transformation_map_origin_to_base;
		std::vector<double> affine_transformation_map_origin_to_utm;
	};

	struct TransformationConfigInternal {
		Eigen::Matrix<double, 3, 4> projection_matrix;
		Eigen::Matrix<double, 3, 3> KR;
		Eigen::Matrix<double, 3, 3> KR_inv;
		Eigen::Matrix<double, 3, 1> C;
		Eigen::Matrix<double, 3, 1> translation_camera;
		Eigen::Matrix<double, 4, 4> affine_transformation_base_to_utm;
	};

	std::map<std::string, TransformationConfigInternal> _config;

	template <typename scalar, int... other>
	[[nodiscard]] Eigen::Matrix<scalar, 4, 1, other...> map_image_to_world_coordinate(std::string const& camera_name, std::array<scalar, 2> coordinates, scalar height = 0.) const {
		Eigen::Matrix<scalar, 4, 1, other...> image_coordinates = Eigen::Matrix<scalar, 4, 1, other...>::Ones();
		image_coordinates(0) = coordinates[0];
		image_coordinates(1) = coordinates[1];

		image_coordinates.template head<3>() = _config.at(camera_name).KR_inv * image_coordinates.template head<3>();
		image_coordinates.template head<3>() = image_coordinates.template head<3>() * (height - _config.at(camera_name).translation_camera(2)) / image_coordinates(2);
		image_coordinates.template head<3>() = image_coordinates.template head<3>() + _config.at(camera_name).translation_camera;

		return image_coordinates;
	}

	template <typename scalar, int... other>
	[[nodiscard]] std::array<scalar, 2> map_world_to_image_coordinate(std::string const& camera_name, Eigen::Matrix<scalar, 4, 1, other...> const& coordinates) const {
		Eigen::Matrix<double, 3, 1> temp = _config.at(camera_name).projection_matrix * coordinates;

		double u = temp[0];
		double v = temp[1];
		if (temp[2] != 0.0) {
			u /= abs(temp[2]);
			v /= abs(temp[2]);
		}
		return {u, v};
	}

   public:
	explicit TrackToTrackFusionNode(std::map<std::string, TransformationConfig>&& config) {
		for (auto const& [cam_name, transformation_config] : config) {
			Eigen::Matrix<double, 3, 4> projection_matrix = make_matrix<3, 4>(transformation_config.projection_matrix[0], transformation_config.projection_matrix[1], transformation_config.projection_matrix[2],
			    transformation_config.projection_matrix[3], transformation_config.projection_matrix[4], transformation_config.projection_matrix[5], transformation_config.projection_matrix[6], transformation_config.projection_matrix[7],
			    transformation_config.projection_matrix[8], transformation_config.projection_matrix[9], transformation_config.projection_matrix[10], transformation_config.projection_matrix[11]);

			Eigen::Matrix<double, 3, 3> KR = projection_matrix(Eigen::all, Eigen::seq(0, Eigen::last - 1));
			Eigen::Matrix<double, 3, 3> KR_inv = KR.inverse();
			Eigen::Matrix<double, 3, 1> C = projection_matrix(Eigen::all, Eigen::last);
			Eigen::Matrix<double, 3, 1> translation_camera = -KR_inv * C;

			// transpose because of row major input vs. col major matrix
			Eigen::Matrix<double, 4, 4> affine_transformation_map_origin_to_base = Eigen::Matrix<double, 4, 4>(transformation_config.affine_transformation_map_origin_to_base.data()).transpose();
			Eigen::Matrix<double, 4, 4> affine_transformation_map_origin_to_utm = Eigen::Matrix<double, 4, 4>(transformation_config.affine_transformation_map_origin_to_utm.data()).transpose();

			Eigen::Matrix<double, 4, 4> affine_transformation_base_to_utm = affine_transformation_map_origin_to_utm * affine_transformation_map_origin_to_base.inverse();

			_config[cam_name] = TransformationConfigInternal{projection_matrix, KR, KR_inv, C, translation_camera, affine_transformation_base_to_utm};
		}
	}

	CompactObjects function(ImageTrackerResults2 const& data) final {
		AfterReturnTimeMeasure after(data.timestamp);

		CompactObjects ret;
		ret.timestamp = data.timestamp;

		for (auto const& object : data.objects) {
			Eigen::Matrix<double, 4, 1> coords = map_image_to_world_coordinate(data.source, object.position());
			Eigen::Matrix<double, 4, 1> utm_coords = _config.at(data.source).affine_transformation_base_to_utm * coords;

			Eigen::Matrix<double, 4, 1> coords_image = _config.at(data.source).affine_transformation_base_to_utm.inverse() * utm_coords;
			std::array<double, 2> position = map_world_to_image_coordinate(data.source, coords_image);

			ret.objects.emplace_back(object.id(), 0_u8, std::array{utm_coords[0], utm_coords[1], 0.}, std::array{0., 0., 0.}, std::array{0., 0., 0.}, std::array{0., 0., 0.});
		}

		return ret;
	}
};