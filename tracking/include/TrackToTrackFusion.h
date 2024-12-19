#pragma once

#include <Eigen/Eigen>
#include <map>
#include <vector>

#include "CompactObject.h"
#include "ImageTrackerNode.h"
#include "Processor.h"
#include "common_literals.h"

class TrackToTrackFusionNode : public Processor<ImageTrackerResults, CompactObjects> {
	struct TransformationConfig {
		Eigen::Matrix<double, 3, 4> projection_matrix;
		Eigen::Matrix<double, 4, 4> affine_transformation_base_to_utm;
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
	std::map<std::string, std::vector<KalmanBoxSourceTrack>> multiple_tracker_results;

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
			Eigen::Matrix<double, 3, 3> KR = transformation_config.projection_matrix(Eigen::all, Eigen::seq(0, Eigen::last - 1));
			Eigen::Matrix<double, 3, 3> KR_inv = KR.inverse();
			Eigen::Matrix<double, 3, 1> C = transformation_config.projection_matrix(Eigen::all, Eigen::last);
			Eigen::Matrix<double, 3, 1> translation_camera = -KR_inv * C;

			// Eigen::Matrix<double, 4, 4> affine_transformation_base_to_utm = affine_transformation_map_origin_to_utm * affine_transformation_map_origin_to_base.inverse();

			_config[cam_name] = TransformationConfigInternal{transformation_config.projection_matrix, KR, KR_inv, C, translation_camera, transformation_config.affine_transformation_base_to_utm};
		}
	}

	CompactObjects process(ImageTrackerResults const& data) final {
		multiple_tracker_results[data.source] = data.objects;

		CompactObjects ret;
		ret.timestamp = data.timestamp;
		for (auto const& [cam_name, results] : multiple_tracker_results) {
			if (cam_name != data.source) {
				for (auto const& object : results) {
					if (auto const [predicted, position, velocity] = object.predict_between(data.timestamp); predicted) {
						Eigen::Matrix<double, 4, 1> coords = map_image_to_world_coordinate(cam_name, position);
						Eigen::Matrix<double, 4, 1> utm_coords = _config.at(data.source).affine_transformation_base_to_utm * coords;

						ret.objects.emplace_back(object.id(), 0_u8, std::array{utm_coords[0], utm_coords[1], 0.}, std::array{0., 0., 0.}, std::array{0., 0., 0.}, std::array{0., 0., 0.});
					}
				}
			} else {
				for (auto const& object : results) {
					Eigen::Matrix<double, 4, 1> coords = map_image_to_world_coordinate(data.source, object.position());
					Eigen::Matrix<double, 4, 1> utm_coords = _config.at(data.source).affine_transformation_base_to_utm * coords;

					ret.objects.emplace_back(object.id(), 0_u8, std::array{utm_coords[0], utm_coords[1], 0.}, std::array{0., 0., 0.}, std::array{0., 0., 0.}, std::array{0., 0., 0.});
				}
			}
		}

		return ret;
	}
};