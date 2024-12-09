#include "ImageVisualizationNode.h"

#include <filesystem>
#include <utility>

void ImageVisualizationNode::run(ImageData const &data) {
	// common::println(
	//     "Time taken = ", std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::nanoseconds(std::chrono::time_point_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now()).time_since_epoch().count() -
	//     data.timestamp)));

	if (!_image_mask(data)) return;

	queue_draw();
	// frezzes because of other thread:
	// cv::imshow(display_name, data.image);
	// cv::waitKey(100);
}
ImageVisualizationNode::ImageVisualizationNode(Glib::RefPtr<Gtk::Application> app, std::function<bool(const ImageData &)> image_mask, std::source_location const location) : _image_mask(std::move(image_mask)) {
	display_name = common::stringprint("ImageVisualization of ", std::filesystem::path(location.file_name()).stem().string(), '(', location.line(), ':', location.column(), ")");
	set_title(display_name);
	show();
	app->signal_startup().connect([&] { app->add_window(*this); });
}
