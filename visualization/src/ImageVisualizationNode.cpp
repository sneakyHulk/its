#include "ImageVisualizationNode.h"

#include <atomic>
#include <chrono>
#include <filesystem>
#include <utility>

#include "common_output.h"

using namespace std::chrono_literals;

/**
 * @brief Updates the image to display.
 *
 * This method filters the image data using the mask filter before updating the current image data for display.
 * It runs synchronously with the image providing node.
 *
 * @param data The image data to update.
 */
void ImageVisualizationNode::run(ImageData const &data) {
	// Check if the given image data passes the filter
	if (!_image_mask(data)) return;

	ImageData const new_data{data.image.clone(), data.timestamp, data.source};

	// Store the provided image data atomically for use in the GUI thread.
	std::atomic_store(&user_data->current_image_data, std::make_shared<ImageData const>(new_data));
}

/**
 * @brief Constructs an ImageVisualizationNode instance.
 *
 * Sets up the image mask function and schedules GUI updates for the visualization.
 *
 * @param image_mask A function to filter image data.
 * @param location The source location for debugging purposes.
 */
[[maybe_unused]] ImageVisualizationNode::ImageVisualizationNode(std::function<bool(const ImageData &)> image_mask, std::source_location const location) : _image_mask(std::move(image_mask)) {
	display_name = common::stringprint("ImageVisualization of ", std::filesystem::path(location.file_name()).stem().string(), '(', location.line(), ':', location.column(), ")");

	// Schedule an update for the image visualization in the GUI thread
	gdk_threads_add_idle(G_SOURCE_FUNC(update_image_create_window_idle), user_data);
}

/**
 * @brief Handles the destruction of the GTK window.
 *
 * Hints to the GUI thread that the visualization window is destroyed.
 *
 * @param window The GTK window to destroy.
 * @param user_data The data relevant to the GUI.
 * @return Always returns false to allow other handlers from being invoked for the event.
 */
gboolean ImageVisualizationNode::destroy([[maybe_unused]] GtkWindow *window, ImageVisualizationNode::UserData *user_data) {
	user_data->destroyed.clear();
	return false;
}

/**
 * @brief Draws the image onto the GTK drawing area widget.
 *
 * This method draws the image centered in the allocated widget area.
 *
 * @param widget The GTK widget to draw on.
 * @param cr The Cairo context for drawing.
 * @param user_data The data relevant to the GUI.
 * @return Always returns false to allow other handlers from being invoked for the event.
 */
gboolean ImageVisualizationNode::on_draw(GtkWidget *widget, cairo_t *cr, ImageVisualizationNode::UserData *user_data) {
	if (GDK_IS_PIXBUF(user_data->pixbuf)) {
		gdk_cairo_set_source_pixbuf(
		    cr, user_data->pixbuf, (gtk_widget_get_allocated_width(widget) - gdk_pixbuf_get_width(user_data->pixbuf)) / 2.0, (gtk_widget_get_allocated_height(widget) - gdk_pixbuf_get_height(user_data->pixbuf)) / 2.0);
		cairo_paint(cr);
	}

	return false;
}

/**
 * @brief Updates the image and creates a GUI window if needed.
 *
 * This method updates the displayed image with the latest image data and adds an overlay with image source and timestamp information.
 * When the window was closed before it creates a new one.
 *
 * @param user_data The data relevant to the GUI.
 * @return Returns G_SOURCE_CONTINUE to keep the idle function active in the GUI thread.
 */
gboolean ImageVisualizationNode::update_image_create_window_idle(ImageVisualizationNode::UserData *user_data) {
	// Check if new image data was transferred to the node
	if (auto image_data = std::atomic_load(&user_data->current_image_data); image_data != user_data->displayed_image_data) {
		user_data->displayed_image_data = image_data;

		// Convert the image color space from BGR to RGB ... const does not make any sense here because the image is changed.
		cv::cvtColor(user_data->displayed_image_data->image, user_data->displayed_image_data->image, cv::COLOR_BGR2RGB);

		// Add text overlay with image source and timestamp information
		cv::putText(user_data->displayed_image_data->image,
		    std::string("Source: ") + user_data->displayed_image_data->source + ", Time since Timestamp: " +
		        std::to_string(std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch() - std::chrono::nanoseconds(user_data->displayed_image_data->timestamp)).count()) + "mus",
		    cv::Point2d(50, 50), cv::FONT_HERSHEY_DUPLEX, 1.0, cv::Scalar_<int>(0, 0, 0), 1);

		if (GDK_IS_PIXBUF(user_data->pixbuf)) g_object_unref(user_data->pixbuf);

		// Create a new pixbuf from the updated image data
		user_data->pixbuf = gdk_pixbuf_new_from_data(user_data->displayed_image_data->image.data, GDK_COLORSPACE_RGB, false, 8, user_data->displayed_image_data->image.cols, user_data->displayed_image_data->image.rows,
		    static_cast<int>(user_data->displayed_image_data->image.step), nullptr, nullptr);

		if (!user_data->destroyed.test_and_set()) {
			// Create the GUI window and widgets
			user_data->drawing_area = gtk_drawing_area_new();
			user_data->window = gtk_window_new(GtkWindowType::GTK_WINDOW_TOPLEVEL);
			gtk_window_set_title(GTK_WINDOW(user_data->window), "RGB Image Viewer");
			gtk_window_set_default_size(GTK_WINDOW(user_data->window), user_data->displayed_image_data->image.cols, user_data->displayed_image_data->image.rows);

			g_signal_connect(user_data->window, "destroy", G_CALLBACK(destroy), user_data);
			g_signal_connect(user_data->drawing_area, "draw", G_CALLBACK(on_draw), user_data);

			gtk_container_add(GTK_CONTAINER(user_data->window), user_data->drawing_area);
			gtk_widget_show_all(user_data->window);
		} else {
			// Request redraw of the window
			gtk_widget_queue_draw(user_data->window);
		}
	}

	return G_SOURCE_CONTINUE;
}
/**
 * @brief Destructor for the ImageVisualizationNode class.
 *
 * Frees the user data resource.
 */
ImageVisualizationNode::~ImageVisualizationNode() { delete user_data; }

/**
 * @brief Destructor for the UserData structure.
 *
 * Frees GTK and GDK resources when the user data is destroyed.
 */
ImageVisualizationNode::UserData::~UserData() {
	if (GTK_IS_WIDGET(drawing_area)) gtk_widget_destroy(drawing_area);
	if (GTK_IS_WIDGET(window)) gtk_widget_destroy(window);
	if (GDK_IS_PIXBUF(pixbuf)) g_object_unref(pixbuf);
}
