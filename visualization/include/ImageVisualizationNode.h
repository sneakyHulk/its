#pragma once

#include <gtkmm.h>
#include <tbb/concurrent_queue.h>

#include <functional>
#include <source_location>

#include "ImageData.h"
#include "node.h"

class ImageVisualizationNode : public RunnerSynchronous<ImageData> {
	std::function<bool(ImageData const&)> _image_mask;
	std::string display_name;
	int height;
	int width;

	std::shared_ptr<ImageData> image;
	Glib::Dispatcher dispatcher;
	Glib::Dispatcher dispatcher2;
	Glib::RefPtr<Gtk::Application> app;
	bool registered = false;

	class ImageVisualization : public Gtk::Window {
		Gtk::DrawingArea area;

	   public:
		ImageVisualization(std::string display_name, int width, int height) {
			set_title(display_name);
			set_default_size(1920, 1200);
			set_hide_on_close(false);
			set_child(area);
		}

		void set_draw_func(sigc::slot<void(const Cairo::RefPtr<Cairo::Context>&, int, int)> const& slot) { return area.set_draw_func(slot); }
	};

	std::shared_ptr<ImageVisualization> window = nullptr;

   public:
	explicit ImageVisualizationNode(Glib::RefPtr<Gtk::Application> app, std::function<bool(ImageData const&)> image_mask = [](ImageData const&) { return true; }, std::source_location const location = std::source_location::current());

	void run_once(ImageData const& data) final;
	void run(ImageData const& data) final;

   private:
	void on_draw(Cairo::RefPtr<Cairo::Context> const& cr, int width, int height);
	void on_dispatcher_signal();
	void on_dispatcher2_signal();
	void on_create_window();
};
