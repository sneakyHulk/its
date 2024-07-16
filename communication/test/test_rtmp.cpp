#include <filesystem>
#include <iostream>
#include <opencv2/highgui.hpp>
#include <opencv2/video.hpp>
#include <vector>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}

void initialize_avformat_context(AVFormatContext *&fctx, const char *format_name) {
	int ret = avformat_alloc_output_context2(&fctx, nullptr, format_name, nullptr);
	if (ret < 0) {
		std::cout << "Could not allocate output format context!" << std::endl;
		exit(1);
	}
}

void initialize_io_context(AVFormatContext *&fctx, const char *output) {
	if (!(fctx->oformat->flags & AVFMT_NOFILE)) {
		int ret = avio_open2(&fctx->pb, output, AVIO_FLAG_WRITE, nullptr, nullptr);
		if (ret < 0) {
			std::cout << "Could not open output IO context!" << std::endl;
			exit(1);
		}
	}
}

void set_codec_params(AVFormatContext *&fctx, AVCodecContext *&codec_ctx, int width, int height, int fps, int bitrate) {
	const AVRational dst_fps = {fps, 1};

	codec_ctx->codec_tag = 0;
	codec_ctx->codec_id = AV_CODEC_ID_H264;
	codec_ctx->codec_type = AVMEDIA_TYPE_VIDEO;
	codec_ctx->width = width;
	codec_ctx->height = height;
	codec_ctx->gop_size = 12;
	codec_ctx->pix_fmt = AV_PIX_FMT_YUV420P;
	codec_ctx->framerate = dst_fps;
	codec_ctx->time_base = av_inv_q(dst_fps);
	codec_ctx->bit_rate = bitrate;
	if (fctx->oformat->flags & AVFMT_GLOBALHEADER) {
		codec_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
	}
}

void initialize_codec_stream(AVStream *&stream, AVCodecContext *&codec_ctx, const AVCodec *&codec, std::string codec_profile) {
	int ret = avcodec_parameters_from_context(stream->codecpar, codec_ctx);
	if (ret < 0) {
		std::cout << "Could not initialize stream codec parameters!" << std::endl;
		exit(1);
	}

	AVDictionary *codec_options = nullptr;
	av_dict_set(&codec_options, "profile", codec_profile.c_str(), 0);
	av_dict_set(&codec_options, "preset", "superfast", 0);
	av_dict_set(&codec_options, "tune", "zerolatency", 0);

	// open video encoder
	ret = avcodec_open2(codec_ctx, codec, &codec_options);
	if (ret < 0) {
		std::cout << "Could not open video encoder!" << std::endl;
		exit(1);
	}
}

SwsContext *initialize_sample_scaler(AVCodecContext *codec_ctx, int width, int height) {
	SwsContext *swsctx = sws_getContext(width, height, AV_PIX_FMT_BGR24, width, height, codec_ctx->pix_fmt, SWS_BICUBIC, nullptr, nullptr, nullptr);
	if (!swsctx) {
		std::cout << "Could not initialize sample scaler!" << std::endl;
		exit(1);
	}

	return swsctx;
}

AVFrame *allocate_frame_buffer(AVCodecContext *codec_ctx, int width, int height) {
	AVFrame *frame = av_frame_alloc();
	int i = av_image_get_buffer_size(codec_ctx->pix_fmt, width, height, 1);
	uint8_t *framebuf = new uint8_t[i];

	av_image_fill_arrays(frame->data, frame->linesize, framebuf, codec_ctx->pix_fmt, width, height, 1);
	frame->width = width;
	frame->height = height;
	frame->format = static_cast<int>(codec_ctx->pix_fmt);

	return frame;
}

void write_frame(AVCodecContext *codec_ctx, AVFormatContext *fmt_ctx, AVFrame *frame) {
	AVPacket pkt = {0};
	av_new_packet(&pkt, 0);

	int ret = avcodec_send_frame(codec_ctx, frame);
	if (ret < 0) {
		std::cout << "Error sending frame to codec context!" << std::endl;
		exit(1);
	}

	ret = avcodec_receive_packet(codec_ctx, &pkt);
	if (ret < 0) {
		std::cout << "Error receiving packet from codec context!" << std::endl;
		exit(1);
	}

	av_interleaved_write_frame(fmt_ctx, &pkt);
	av_packet_unref(&pkt);
}

void stream_video(int width, int height, int fps, int camID, int bitrate, std::string codec_profile, std::string server) {
#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(58, 9, 100)
	av_register_all();
#endif
	avformat_network_init();

	const char *output = server.c_str();
	int ret;

	AVFormatContext *ofmt_ctx = nullptr;
	const AVCodec *out_codec = nullptr;
	AVStream *out_stream = nullptr;
	AVCodecContext *out_codec_ctx = nullptr;

	initialize_avformat_context(ofmt_ctx, "flv");
	initialize_io_context(ofmt_ctx, output);

	out_codec = avcodec_find_encoder(AV_CODEC_ID_H264);
	out_stream = avformat_new_stream(ofmt_ctx, out_codec);
	out_codec_ctx = avcodec_alloc_context3(out_codec);

	set_codec_params(ofmt_ctx, out_codec_ctx, width, height, fps, bitrate);
	initialize_codec_stream(out_stream, out_codec_ctx, out_codec, codec_profile);

	out_stream->codecpar->extradata = out_codec_ctx->extradata;
	out_stream->codecpar->extradata_size = out_codec_ctx->extradata_size;

	av_dump_format(ofmt_ctx, 0, output, 1);

	auto *swsctx = initialize_sample_scaler(out_codec_ctx, width, height);
	auto *frame = allocate_frame_buffer(out_codec_ctx, width, height);

	int cur_size;
	uint8_t *cur_ptr;

	ret = avformat_write_header(ofmt_ctx, nullptr);
	if (ret < 0) {
		std::cout << "Could not write header!" << std::endl;
		exit(1);
	}

	bool end_of_stream = false;
	do {
		cv::Mat image = cv::imread(std::filesystem::path(CMAKE_SOURCE_DIR) / std::filesystem::path("data/camera_simulator/s110_o_cam_8/s110_o_cam_8_images_distorted/1690366190050.jpg"));
		std::time_t time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
		std::string wall_time = std::ctime(&time);
		cv::putText(image, wall_time, cv::Point2d(500, 500), cv::FONT_HERSHEY_DUPLEX, 1.0, cv::Scalar_<int>(0, 0, 0), 1);

		const int stride[] = {static_cast<int>(image.step[0])};
		sws_scale(swsctx, &image.data, stride, 0, image.rows, frame->data, frame->linesize);
		frame->pts += av_rescale_q(1, out_codec_ctx->time_base, out_stream->time_base);
		write_frame(out_codec_ctx, ofmt_ctx, frame);
	} while (!end_of_stream);

	av_write_trailer(ofmt_ctx);

	av_frame_free(&frame);
	avcodec_close(out_codec_ctx);
	avio_close(ofmt_ctx->pb);
	avformat_free_context(ofmt_ctx);
}

int main(int argc, char *argv[]) {
	int cameraID = 0, fps = 30, width = 800, height = 600, bitrate = 300000;
	std::string h264profile = "high444";
	std::string outputServer = "rtmp://localhost/live/stream";

	stream_video(1920, 1200, 10, 0, 300000, "high444", "rtmp://localhost/live/stream");

	return 0;
}
