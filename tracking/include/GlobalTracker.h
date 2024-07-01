#pragma once
#include <vector>

#include "KalmanFilter.h"

class GlobalTracker : private KalmanFilter<4, 4, 2, {0, 1}, {2, 3}> {
	static thread_local unsigned int _id_max;
	unsigned int _id = ++_id_max;

	static KalmanFilter<4, 4, 2, {0, 1}, {2, 3}> make_constant_velocity_model_kalman_filter(double x_init, double y_init) {
		decltype(x) x_;

		x_ << x_init, y_init, 0., 0.;
		decltype(F) F_;
		F_ << 1., 0., 1., 0.,  //
		    0., 1., 0., 1.,    //
		    0., 0., 1., 0.,    //
		    0., 0., 0., 1.;    //

		decltype(H) H_;
		H_ << 1., 0., 0., 0.,  //
		    0., 1., 0., 0.,    //
		    0., 0., 1., 0.,    //
		    0., 0., 0., 1.;    //

		decltype(P) P_ = decltype(P)::Identity();
		P_(0, 0) *= 0.01;  // give low uncertainty to the initial position values (because newest yolo ist pretty accurate)
		P_(1, 1) *= 0.01;  // give low uncertainty to the initial position values (because newest yolo ist pretty accurate)
		P_(2, 2) *= 10.;   // give high uncertainty to the unobservable initial velocities
		P_(3, 3) *= 10.;   // give high uncertainty to the unobservable initial velocities
	}
};