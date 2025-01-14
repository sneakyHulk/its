#pragma once

#include <Eigen/Eigen>
#include <array>
#include <ranges>
#include <vector>

#if !__cpp_lib_ranges_zip
#include <range/v3/view/zip.hpp>
#endif

template <std::size_t dim_x, std::size_t dim_z, std::pair<std::size_t, std::size_t>... velocity_components>
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

	decltype(x) x_last_update;
	decltype(P) P_last_update;

   public:
	KalmanFilter(decltype(x)&& x = decltype(x)::Zero(), decltype(F)&& F = decltype(F)::Identity(), decltype(H)&& H = decltype(H)::Zero(), decltype(P)&& P = decltype(P)::Identity(), decltype(R)&& R = decltype(R)::Identity(),
	    decltype(Q)&& Q = decltype(Q)::Identity())
	    : x(std::forward<decltype(x)>(x)),
	      F(std::forward<decltype(F)>(F)),
	      H(std::forward<decltype(H)>(H)),
	      P(std::forward<decltype(P)>(P)),
	      R(std::forward<decltype(R)>(R)),
	      Q(std::forward<decltype(Q)>(Q)),
	      x_last_update(x),
	      P_last_update(P) {}

	void adapt_prediction_matrix(double const dt) { ((F(std::get<0>(velocity_components), std::get<1>(velocity_components)) = dt), ...); }
	// decltype(F) get_adapt_prediction_matrix(double const dt) const {
	// 	decltype(F) F_between = F;
	// 	((F_between(std::get<0>(velocity_components), std::get<1>(velocity_components)) = dt), ...);
	// 	return F_between;
	// }
	void predict(double const dt) {
		adapt_prediction_matrix(dt);

		x = F * x_last_update;
		P = (F * P_last_update) * F.transpose() + Q;
	}
	void update(decltype(z) const& z_) {
		y = z_ - H * x;
		auto PHT = P * H.transpose();
		S = H * PHT + R;
		SI = S.inverse();
		K = PHT * SI;
		x = x_last_update = x + K * y;
		auto I_KH = I - K * H;
		P = P_last_update = (I_KH * P) * I_KH.transpose() + (K * R) * K.transpose();
	}
};