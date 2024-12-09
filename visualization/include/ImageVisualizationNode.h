#pragma once

#include <gtkmm.h>
#include <tbb/concurrent_queue.h>

#include <functional>
#include <source_location>

#include "ImageData.h"
#include "node.h"

class ImageVisualizationNode : public RunnerSynchronous<ImageData>, public Gtk::Window {
	std::function<bool(ImageData const&)> _image_mask;
	std::string display_name;
	Gtk::DrawingArea area;
	std::shared_ptr<ImageData> image;

   public:
	explicit ImageVisualizationNode(Glib::RefPtr<Gtk::Application> app, std::function<bool(ImageData const&)> image_mask = [](ImageData const&) { return true; }, std::source_location const location = std::source_location::current());

	void run_once(ImageData const& data) final;
	void run(ImageData const& data) final;

   private:
	void on_draw(Cairo::RefPtr<Cairo::Context> const& cr, int width, int height);
};
