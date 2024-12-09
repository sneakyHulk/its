#include "ImageVisualizationNode.h"

#include <atomic>
#include <filesystem>
#include <utility>

void ImageVisualizationNode::run(ImageData const &data) {
	// common::println(
	//     "Time taken = ", std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::nanoseconds(std::chrono::time_point_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now()).time_since_epoch().count() -
	//     data.timestamp)));

	if (!_image_mask(data)) return;

	std::atomic_store(&image, std::make_shared<ImageData>(data));

	// image = Gdk::Pixbuf::create_from_data(data.image.data, Gdk::Colorspace::RGB, false, 8, data.image.rows, data.image.cols, data.image.step);

	area.queue_draw();
	// frezzes because of other thread:
	// cv::imshow(display_name, data.image);
	// cv::waitKey(100);
}
ImageVisualizationNode::ImageVisualizationNode(Glib::RefPtr<Gtk::Application> app, std::function<bool(const ImageData &)> image_mask, std::source_location const location) : _image_mask(std::move(image_mask)) {
	display_name = common::stringprint("ImageVisualization of ", std::filesystem::path(location.file_name()).stem().string(), '(', location.line(), ':', location.column(), ")");
	set_title(display_name);
	set_child(area);

	area.set_draw_func(sigc::mem_fun(*this, &ImageVisualizationNode::on_draw));

	show();
	app->signal_startup().connect([&] { app->add_window(*this); });
}
void ImageVisualizationNode::on_draw(Cairo::RefPtr<Cairo::Context> const &cr, int width, int height) {
	static std::shared_ptr<ImageData> current_image = nullptr;
	static Glib::RefPtr<Gdk::Pixbuf> image_buffer = nullptr;

	if (!image) return;
	if (auto old = std::exchange(current_image, std::atomic_load(&image)); current_image == old) return;

	cv::cvtColor(current_image->image, current_image->image, cv::COLOR_BGR2RGB);

	cv::putText(current_image->image,
	    std::string("Source: ") + current_image->source +
	        ", Time since Timestamp: " + std::to_string(std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch() - std::chrono::nanoseconds(current_image->timestamp)).count()) + "mus",
	    cv::Point2d(50, 50), cv::FONT_HERSHEY_DUPLEX, 1.0, cv::Scalar_<int>(0, 0, 0), 1);

	image_buffer = Gdk::Pixbuf::create_from_data(current_image->image.data, Gdk::Colorspace::RGB, false, 8, current_image->image.cols, current_image->image.rows, current_image->image.step);
	Gdk::Cairo::set_source_pixbuf(cr, image_buffer, (width - image_buffer->get_width()) / 2, (height - image_buffer->get_height()) / 2);
	cr->paint();
}
void ImageVisualizationNode::run_once(const ImageData &data) { set_default_size(data.image.cols, data.image.rows); }
