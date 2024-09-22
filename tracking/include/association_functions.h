#pragma once
#include <tuple>

#include "Detection2D.h"

constexpr auto iou = [](BoundingBoxXYXY const& bbox1, BoundingBoxXYXY const& bbox2) {
	auto const xx1 = std::max(bbox1.left, bbox2.left);
	auto const yy1 = std::max(bbox1.top, bbox2.top);
	auto const xx2 = std::min(bbox1.right, bbox2.right);
	auto const yy2 = std::min(bbox1.bottom, bbox2.bottom);

	auto const w = std::max(0., xx2 - xx1);
	auto const h = std::max(0., yy2 - yy1);
	auto const wh = w * h;

	auto const A1 = (bbox1.right - bbox1.left) * (bbox1.bottom - bbox1.top);
	auto const A2 = (bbox2.right - bbox2.left) * (bbox2.bottom - bbox2.top);

	auto const iou = wh / (A1 + A2 - wh);
	return iou;
};

constexpr auto diou = [](BoundingBoxXYXY const& bbox1, BoundingBoxXYXY const& bbox2) {
	auto const iou_ = iou(bbox1, bbox2);

	auto const x_center_bbox1 = (bbox1.left + bbox1.right) / 2.;
	auto const y_center_bbox1 = (bbox1.top + bbox1.bottom) / 2.;
	auto const x_center_bbox2 = (bbox2.left + bbox2.right) / 2.;
	auto const y_center_bbox2 = (bbox2.top + bbox2.bottom) / 2.;

	auto const inner_diag_squared = (x_center_bbox1 - x_center_bbox2) * (x_center_bbox1 - x_center_bbox2) + (y_center_bbox1 - y_center_bbox2) * (y_center_bbox1 - y_center_bbox2);

	auto const xxc1 = std::min(bbox1.left, bbox2.left);
	auto const yyc1 = std::min(bbox1.top, bbox2.top);
	auto const xxc2 = std::max(bbox1.right, bbox2.right);
	auto const yyc2 = std::max(bbox1.bottom, bbox2.bottom);

	auto const outer_diag_squared = (xxc2 - xxc1) * (xxc2 - xxc1) + (yyc2 - yyc1) * (yyc2 - yyc1);

	auto const diou = iou_ - inner_diag_squared / outer_diag_squared;
	return (diou + 1.) / 2.;  // resize from (-1,1) to (0,1)
};
