#pragma once

#include <cmath>
#include <cstdint>
#include <utility>
#include <vector>

#include "Detection2D.h"
#include "KalmanFilter.h"

class KalmanBoxTracker : private KalmanFilter<7, 4, 3, {0, 1, 2}, {4, 5, 6}> {
	static thread_local unsigned int _id_max;

	unsigned int _id = ++_id_max;
	int _consecutive_hits = 0;
	int _consecutive_fails = 0;
	bool _displayed = false;
	std::uint8_t _object_class = 0;

	// per frame history
	std::vector<BoundingBoxXYXY> _history{};

   public:
	static void reset_id() { _id_max = 0; }

	explicit KalmanBoxTracker(BoundingBoxXYXY const& bbox) : KalmanFilter<7, 4, 3, {0, 1, 2}, {4, 5, 6}>(make_constant_box_velocity_model_kalman_filter(bbox)) {}
	KalmanBoxTracker(BoundingBoxXYXY const& bbox, std::uint8_t const object_class) : KalmanFilter<7, 4, 3, {0, 1, 2}, {4, 5, 6}>(make_constant_box_velocity_model_kalman_filter(bbox)), _object_class(object_class) {}

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

	BoundingBoxXYXY const& predict(double const dt) {
		if (dt * x(6) + x(2) <= 0) x(2) *= 0.;  // area must be >= 0;
		KalmanFilter<7, 4, 3, {0, 1, 2}, {4, 5, 6}>::predict(dt);
		if (_consecutive_fails > 0) _consecutive_hits = 0;
		_consecutive_fails += 1;

		return _history.emplace_back(convert_x_to_bbox(x));
	}

	void update(BoundingBoxXYXY const& bbox) {
		_consecutive_fails = 0;
		_consecutive_hits += 1;

		KalmanFilter<7, 4, 3, {0, 1, 2}, {4, 5, 6}>::update(convert_bbox_to_z(bbox));

		_history.back() = convert_x_to_bbox(x);
	}

	[[nodiscard]] BoundingBoxXYXY state() const { return _history.back(); }
	[[nodiscard]] std::array<double, 2> position() const { return {x(0), x(1)}; }
	[[nodiscard]] std::array<double, 2> velocity() const { return {x(4), x(5)}; }
	[[nodiscard]] auto id() const { return _id; }
	[[nodiscard]] auto consecutive_hits() const { return _consecutive_hits; }
	[[nodiscard]] auto consecutive_fails() const { return _consecutive_fails; }
	[[nodiscard]] auto& displayed() { return _displayed; }
	[[nodiscard]] auto matched() const { return _consecutive_hits > 0; }
	[[nodiscard]] auto object_class()const { return _object_class; }

   private:
	static KalmanFilter<7, 4, 3, {0, 1, 2}, {4, 5, 6}> make_constant_box_velocity_model_kalman_filter(BoundingBoxXYXY const& bbox) {
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