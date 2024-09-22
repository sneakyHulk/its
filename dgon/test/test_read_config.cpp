#include <Eigen/Dense>

#include "EigenUtils.h"

Eigen::Matrix<double, 4, 4> const affine_transformation_map_origin_to_utm = make_matrix<4, 4>(
#include "../config/affine_transformation_map_origin_to_utm"
);
Eigen::Matrix<double, 4, 4> const affine_transformation_utm_to_map_origin = affine_transformation_map_origin_to_utm.inverse();

Eigen::Matrix<double, 4, 4> const affine_transformation_map_origin_to_s110_base = make_matrix<4, 4>(
#include "../config/affine_transformation_map_origin_to_s110_base"
);
Eigen::Matrix<double, 4, 4> const affine_transformation_s110_base_to_map_origin = affine_transformation_map_origin_to_s110_base.inverse();

Eigen::Matrix<double, 4, 4> const affine_transformation_lidar_south_to_s110_base = make_matrix<4, 4>(
#include "../config/affine_transformation_lidar_south_to_s110_base"
);

Eigen::Matrix<double, 4, 4> const affine_transformation_s110_base_to_s110_n_cam_8 = make_matrix<4, 4>(
#include "../config/affine_transformation_s110_base_to_s110_n_cam_8"
);
Eigen::Matrix<double, 4, 4> const affine_transformation_s110_n_cam_8_to_s110_base = affine_transformation_s110_base_to_s110_n_cam_8.inverse();

Eigen::Matrix<double, 4, 4> const affine_transformation_s110_base_to_s110_o_cam_8 = make_matrix<4, 4>(
#include "../config/affine_transformation_s110_base_to_s110_o_cam_8"
);
Eigen::Matrix<double, 4, 4> const affine_transformation_s110_o_cam_8_to_s110_base = affine_transformation_s110_base_to_s110_o_cam_8.inverse();

Eigen::Matrix<double, 4, 4> const affine_transformation_s110_base_to_s110_s_cam_8 = make_matrix<4, 4>(
#include "../config/affine_transformation_s110_base_to_s110_s_cam_8"
);
Eigen::Matrix<double, 4, 4> const affine_transformation_s110_s_cam_8_to_s110_base = affine_transformation_s110_base_to_s110_s_cam_8.inverse();

Eigen::Matrix<double, 4, 4> const affine_transformation_s110_base_to_s110_w_cam_8 = make_matrix<4, 4>(
#include "../config/affine_transformation_s110_base_to_s110_w_cam_8"
);
Eigen::Matrix<double, 4, 4> const affine_transformation_s110_w_cam_8_to_s110_base = affine_transformation_s110_base_to_s110_w_cam_8.inverse();

int main() {

}