#pragma once

#include <gtk/gtk.h>

#include <functional>
#include <mutex>
#include <semaphore>
#include <source_location>

#include "ImageData.h"
#include "RunnerSynchronous.h"

/**
 * @class ImageVisualizationNode
 * @brief A node for visualizing image data in a GTK window.
 *
 * This class processes and visualizes image data in a GUI.
 * It can be used with a filtering function to determine whether the image data should be displayed, and handles updates to the GUI, including drawing images and displaying overlays with metadata.
 *
 * @attention This class have to have a g_main_loop_run or a loop with g_main_context_iteration to be run to function properly. Also, g_main_loop_run or g_main_context_iteration must run in the same thread where this class was created.
 */
class ImageVisualizationNode : public RunnerSynchronous<ImageData> {
	std::function<bool(ImageData const&)> _image_mask;
	std::string display_name;

	struct UserData {
		GtkWidget* drawing_area = nullptr;
		GtkWidget* window = nullptr;
		GdkPixbuf* pixbuf = nullptr;
		std::atomic_flag destroyed = ATOMIC_FLAG_INIT;
		std::shared_ptr<ImageData const> current_image_data = nullptr;
		std::shared_ptr<ImageData const> displayed_image_data = nullptr;
		~UserData();
	}* user_data = new UserData;

	static gboolean destroy([[maybe_unused]] [[maybe_unused]] GtkWindow* window, UserData* user_data);
	static gboolean on_draw(GtkWidget* widget, cairo_t* cr, UserData* user_data);
	static gboolean update_image_create_window_idle(UserData* user_data);

   public:
	[[maybe_unused]] explicit ImageVisualizationNode(std::function<bool(ImageData const&)> image_mask = [](ImageData const&) { return true; }, std::source_location const location = std::source_location::current());
	~ImageVisualizationNode();

	void run(ImageData const& data) final;
};
