#pragma once

#include <Eigen/Eigen>
#include <array>
#include <ranges>
#include <vector>

#if !__cpp_lib_ranges_zip
#include <range/v3/view/zip.hpp>
#endif

#include "common_output.h"

template <std::size_t dim_x, std::size_t dim_z, std::size_t n_velocity_components = 0, std::array<int, n_velocity_components> i = {}, std::array<int, n_velocity_components> j = {}>
class KalmanFilter {
   protected:
	Eigen::Matrix<double, dim_x, 1> x;      // x: This is the Kalman state variable. [dim_x, 1].
	Eigen::Matrix<double, dim_x, dim_x> F;  // F: Prediction matrix / state transition matrix. [dim_x, dim_x].
	Eigen::Matrix<double, dim_z, dim_x> H;  // H: Observation model / matrix. [dim_z, dim_x].
	Eigen::Matrix<double, dim_x, dim_x> P;  // P: Covariance matrix. [dim_x, dim_x].
	Eigen::Matrix<double, dim_z, dim_z> R;  // R: Observation noise covariance matrix. [dim_z, dim_z].
	Eigen::Matrix<double, dim_x, dim_x> Q;  // Q: Process noise covariance matrix. [dim_x, dim_x].
	Eigen::Matrix<double, dim_z, 1> z;      // z: Measurement vector. [dim_z, 1].

	Eigen::Matrix<double, dim_x, dim_z> K;                            // K: Kalman gain. [dim_x, dim_z].
	Eigen::Matrix<double, dim_z, 1> y;                                // y: Measurement residual. [dim_z, 1].
	Eigen::Matrix<double, dim_z, dim_z> S;                            // S: Measurement residual covariance (system uncertainty). [dim_z, dim_z].
	Eigen::Matrix<double, dim_z, dim_z> SI;                           // SI: Inverse of measurement residual covariance (simplified for subsequent calculations) (inverse system uncertainty). [dim_z, dim_z].
	Eigen::Matrix<double, dim_x, dim_x> I = decltype(I)::Identity();  // I: Identity matrix. [dim_x, dim_x].
   public:
	KalmanFilter(decltype(x)&& x = decltype(x)::Zero(), decltype(F)&& F = decltype(F)::Identity(), decltype(H)&& H = decltype(H)::Zero(), decltype(P)&& P = decltype(P)::Identity(), decltype(R)&& R = decltype(R)::Identity(),
	    decltype(Q)&& Q = decltype(Q)::Identity())
	    : x(std::forward<decltype(x)>(x)), F(std::forward<decltype(F)>(F)), H(std::forward<decltype(H)>(H)), P(std::forward<decltype(P)>(P)), R(std::forward<decltype(R)>(R)), Q(std::forward<decltype(Q)>(Q)) {}

	void predict(double dt) {
		for (auto k = 0; k < n_velocity_components; ++k) {
			F(i[k], j[k]) = dt;
		}
		return predict();
	}
	void predict() {
		x = F * x;
		P = (F * P) * F.transpose() + Q;
	}
	void update(decltype(z) const& z_) {
		y = z_ - H * x;
		auto PHT = P * H.transpose();
		S = H * PHT + R;
		SI = S.inverse();
		K = PHT * SI;
		x = x + K * y;
		auto I_KH = I - K * H;
		P = (I_KH * P) * I_KH.transpose() + (K * R) * K.transpose();
	}
};