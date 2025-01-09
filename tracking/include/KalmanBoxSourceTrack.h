#pragma once

#include <chrono>
#include <cstdint>

#include "Detection2D.h"
#include "KalmanFilter.h"
#include "common_output.h"

/**
 * @class KalmanBoxSourceTrack
 * @brief Helper class for dealing with tracks.
 */
class KalmanBoxSourceTrack : private KalmanFilter<7, 4, {0, 4}, {1, 5}, {2, 6}> {
	static unsigned int _id_max;
	unsigned int _id = ++_id_max;
	std::uint64_t _last_update_time = 0.;
	std::uint64_t _last_predict_time = 0.;
	Detection2D _last_detection;

   public:
	KalmanBoxSourceTrack(Detection2D const& detection, std::uint64_t const timestamp)
	    : KalmanFilter<7, 4, {0, 4}, {1, 5}, {2, 6}>(make_constant_box_velocity_model_kalman_filter(detection.bbox)), _last_update_time(timestamp), _last_detection(detection) {}

	void predict(std::uint64_t const time) {
		auto const dt = std::chrono::duration<double>(std::chrono::nanoseconds(time) - std::chrono::nanoseconds(_last_update_time)).count();

		if (dt * x(6) + x(2) <= 0) x(2) *= 0.;  // area must be >= 0;

		KalmanFilter<7, 4, {0, 4}, {1, 5}, {2, 6}>::predict(dt);

		if (x(2) < 0.) {
			throw common::Exception("Area is smaller than zero!");
		}

		_last_predict_time = time;
	}

	void update(Detection2D const& detection) {
		_last_update_time = _last_predict_time;
		_last_detection = detection;
		KalmanFilter<7, 4, {0, 4}, {1, 5}, {2, 6}>::update(convert_bbox_to_z(detection.bbox));
	}

   public:
	[[nodiscard]] std::array<double, 2> position() const { return {x(0), x(1)}; }
	[[nodiscard]] std::array<double, 2> velocity() const { return {x(4), x(5)}; }
	[[nodiscard]] std::uint64_t last_update() const { return _last_update_time; }
	[[nodiscard]] BoundingBoxXYXY state() const { return convert_x_to_bbox(x); }
	[[nodiscard]] unsigned int id() const { return _id; }
	[[nodiscard]] Detection2D detection() const { return _last_detection; }

	[[nodiscard]] static decltype(z) convert_bbox_to_z(BoundingBoxXYXY const& bbox) {
		decltype(z) z_;

		auto const w = bbox.right - bbox.left;
		auto const h = bbox.bottom - bbox.top;

		auto const x_center = bbox.left + w / 2.;
		auto const y_center = bbox.top + h * (3. / 4.);

		auto const s = w * h;
		auto const r = w / h;

		z_ << x_center, y_center, s, r;
		return z_;
	}

	[[nodiscard]] static BoundingBoxXYXY convert_x_to_bbox(decltype(x) const& x_) {
		auto const w = std::sqrt(x_(2) * x_(3));
		auto const h = x_(2) / w;

		return {x_(0) - w / 2., x_(1) - h * (3. / 4.), x_(0) + w / 2., x_(1) + h * (1. / 4.)};
	}

   private:
	static KalmanFilter<7, 4, {0, 4}, {1, 5}, {2, 6}> make_constant_box_velocity_model_kalman_filter(BoundingBoxXYXY const& bbox) {
		decltype(x) x_;
		x_ << convert_bbox_to_z(bbox), 0., 0., 0.;
		decltype(F) F_;
		F_ << 1., 0., 0., 0., 1., 0., 0.,  //
		    0., 1., 0., 0., 0., 1., 0.,    //
		    0., 0., 1., 0., 0., 0., 1.,    //
		    0., 0., 0., 1., 0., 0., 0.,    //
		    0., 0., 0., 0., 1., 0., 0.,    //
		    0., 0., 0., 0., 0., 1., 0.,    //
		    0., 0., 0., 0., 0., 0., 1.;    //
		decltype(H) H_;
		H_ << 1., 0., 0., 0., 0., 0., 0.,  //
		    0., 1., 0., 0., 0., 0., 0.,    //
		    0., 0., 1., 0., 0., 0., 0.,    //
		    0., 0., 0., 1., 0., 0., 0.;    //
		decltype(P) P_ = decltype(P)::Identity();
		P_(0, 0) *= 0.01;  // give low uncertainty to the initial position values (because newest yolo ist pretty accurate)
		P_(1, 1) *= 0.01;  // give low uncertainty to the initial position values (because newest yolo ist pretty accurate)
		P_(2, 2) *= 10.;
		P_(3, 3) *= 10.;
		P_(4, 4) *= 1000.;  // give high uncertainty to the unobservable initial velocities
		P_(5, 5) *= 1000.;  // give high uncertainty to the unobservable initial velocities
		P_(6, 6) *= 1000.;  // give high uncertainty to the unobservable initial velocities
		decltype(R) R_ = decltype(R)::Identity();
		R_(2, 2) *= 10.;
		R_(3, 3) *= 10.;
		decltype(Q) Q_ = decltype(Q)::Identity();
		Q_(6, 6) *= 0.01;

		return {std::forward<decltype(x)>(x_), std::forward<decltype(F)>(F_), std::forward<decltype(H)>(H_), std::forward<decltype(P)>(P_), std::forward<decltype(R)>(R_), std::forward<decltype(Q)>(Q_)};
	}
};