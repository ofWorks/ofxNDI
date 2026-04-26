#include "ofxNDIsender.h"

ofxNDIsender::ofxNDIsender() = default;

ofxNDIsender::~ofxNDIsender() {
	close();
}

bool ofxNDIsender::setup(const std::string& senderName) {
	if (initialized) return true;

	ndiLib = NDIlib_v6_load();
	if (!ndiLib) {
		ofLogError("ofxNDIsender") << "Failed to load NDI library";
		return false;
	}

	if (!ndiLib->initialize()) {
		ofLogError("ofxNDIsender") << "NDI initialization failed";
		ndiLib = nullptr;
		return false;
	}

	NDIlib_send_create_t sendCreate = {};
	sendCreate.p_ndi_name = senderName.c_str();
	sendCreate.p_groups = nullptr;
	sendCreate.clock_video = true;
	sendCreate.clock_audio = false;

	sender = ndiLib->send_create(&sendCreate);
	if (!sender) {
		ofLogError("ofxNDIsender") << "Failed to create sender: " << senderName;
		ndiLib->destroy();
		ndiLib = nullptr;
		return false;
	}

	name = senderName;
	initialized = true;
	return true;
}

void ofxNDIsender::close() {
	if (ndiLib && sender) {
		ndiLib->send_destroy(sender);
		sender = nullptr;
	}
	if (ndiLib) {
		ndiLib->destroy();
		ndiLib = nullptr;
	}
	initialized = false;
	name.clear();
	width = 0;
	height = 0;
}

bool ofxNDIsender::send(const ofTexture& tex) {
	if (!initialized) return false;

	int w = tex.getWidth();
	int h = tex.getHeight();

	if (w <= 0 || h <= 0) return false;

	if ((int)pixelBuffer.getWidth() != w || (int)pixelBuffer.getHeight() != h) {
		pixelBuffer.allocate(w, h, OF_PIXELS_RGBA);
	}

	tex.readToPixels(pixelBuffer);
	return sendFrame(pixelBuffer.getData(), w, h);
}

bool ofxNDIsender::send(const ofFbo& fbo) {
	if (!initialized) return false;

	int w = fbo.getWidth();
	int h = fbo.getHeight();

	if (w <= 0 || h <= 0) return false;

	if ((int)pixelBuffer.getWidth() != w || (int)pixelBuffer.getHeight() != h) {
		pixelBuffer.allocate(w, h, OF_PIXELS_RGBA);
	}

	fbo.readToPixels(pixelBuffer);
	return sendFrame(pixelBuffer.getData(), w, h);
}

bool ofxNDIsender::send(const ofImage& img) {
	if (!initialized) return false;

	const ofPixels& pix = img.getPixels();
	int w = pix.getWidth();
	int h = pix.getHeight();

	if (w <= 0 || h <= 0) return false;

	if (pix.getNumChannels() == 4) {
		return sendFrame(pix.getData(), w, h);
	} else {
		if ((int)pixelBuffer.getWidth() != w || (int)pixelBuffer.getHeight() != h) {
			pixelBuffer.allocate(w, h, OF_PIXELS_RGBA);
		}
		pixelBuffer = pix;  // operator= handles channel conversion
		return sendFrame(pixelBuffer.getData(), w, h);
	}
}

bool ofxNDIsender::send(const ofPixels& pix) {
	if (!initialized) return false;

	int w = pix.getWidth();
	int h = pix.getHeight();

	if (w <= 0 || h <= 0) return false;

	if (pix.getNumChannels() == 4) {
		return sendFrame(pix.getData(), w, h);
	} else {
		if ((int)pixelBuffer.getWidth() != w || (int)pixelBuffer.getHeight() != h) {
			pixelBuffer.allocate(w, h, OF_PIXELS_RGBA);
		}
		pixelBuffer = pix;  // operator= handles channel conversion
		return sendFrame(pixelBuffer.getData(), w, h);
	}
}

bool ofxNDIsender::send(const unsigned char* pixels, int w, int h) {
	if (!initialized || !pixels) return false;
	return sendFrame(pixels, w, h);
}

bool ofxNDIsender::isInitialized() const {
	return initialized;
}

std::string ofxNDIsender::getName() const {
	return name;
}

std::string ofxNDIsender::getNDIVersion() const {
	if (ndiLib) {
		return ndiLib->version();
	}
	return "";
}

// --- Private ---

bool ofxNDIsender::sendFrame(const unsigned char* rgbaPixels, int w, int h) {
	NDIlib_video_frame_v2_t videoFrame = {};
	videoFrame.xres = w;
	videoFrame.yres = h;
	videoFrame.FourCC = NDIlib_FourCC_type_RGBA;
	videoFrame.p_data = (uint8_t*)rgbaPixels;
	videoFrame.line_stride_in_bytes = w * 4;
	videoFrame.frame_rate_N = 60000;
	videoFrame.frame_rate_D = 1001;
	videoFrame.picture_aspect_ratio = (float)w / (float)h;
	videoFrame.frame_format_type = NDIlib_frame_format_type_progressive;
	videoFrame.timecode = NDIlib_send_timecode_synthesize;

	ndiLib->send_send_video_v2(sender, &videoFrame);

	width = w;
	height = h;
	return true;
}
