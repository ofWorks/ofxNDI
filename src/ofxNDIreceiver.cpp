#include "ofxNDIReceiver.h"

ofxNDIReceiver::ofxNDIReceiver() = default;

ofxNDIReceiver::~ofxNDIReceiver() {
	close();
}

bool ofxNDIReceiver::setup(const std::string& preferredSender) {
	if (initialized) return true;

	ndiLib = NDIlib_v6_load();
	if (!ndiLib) {
		ofLogError("ofxNDIReceiver") << "Failed to load NDI library";
		return false;
	}

	if (!ndiLib->initialize()) {
		ofLogError("ofxNDIReceiver") << "NDI initialization failed (CPU may not support SSE4.2)";
		ndiLib = nullptr;
		return false;
	}

	NDIlib_find_create_t findCreate = {true, nullptr, nullptr};
	finder = ndiLib->find_create_v2(&findCreate);
	if (!finder) {
		ofLogError("ofxNDIReceiver") << "Failed to create NDI finder";
		ndiLib->destroy();
		ndiLib = nullptr;
		return false;
	}

	initialized = true;
	pendingSenderName = preferredSender;

	refreshSenders();

	if (!senderNames.empty()) {
		int idx = 0;
		if (!pendingSenderName.empty()) {
			for (size_t i = 0; i < senderNames.size(); i++) {
				if (senderNames[i] == pendingSenderName) {
					idx = (int)i;
					break;
				}
			}
		}
		createReceiver(senderSources[idx]);
	}

	return true;
}

void ofxNDIReceiver::close() {
	releaseReceiver();

	if (ndiLib && finder) {
		ndiLib->find_destroy(finder);
		finder = nullptr;
	}

	if (ndiLib) {
		ndiLib->destroy();
		ndiLib = nullptr;
	}

	initialized = false;
	connected = false;
	lastSourceCount = 0;
	senderNames.clear();
	senderSources.clear();
	currentSenderName.clear();
	pendingSenderName.clear();

	texture.clear();
	pixelBuffer.clear();
}

void ofxNDIReceiver::update() {
	if (!initialized) return;

	refreshSenders();

	if (!pendingSenderName.empty() && pendingSenderName != currentSenderName) {
		for (size_t i = 0; i < senderNames.size(); i++) {
			if (senderNames[i] == pendingSenderName) {
				createReceiver(senderSources[i]);
				pendingSenderName.clear();
				break;
			}
		}
	}

	if (!receiver) {
		if (!senderNames.empty()) {
			createReceiver(senderSources[0]);
		}
		return;
	}

	NDIlib_video_frame_v2_t videoFrame = {};
	NDIlib_audio_frame_v3_t audioFrame = {};
	NDIlib_metadata_frame_t metadataFrame = {};

	NDIlib_frame_type_e frameType = ndiLib->recv_capture_v3(
		receiver, &videoFrame, &audioFrame, &metadataFrame, 0
	);

	if (frameType == NDIlib_frame_type_video && videoFrame.p_data) {
		connected = true;

		int w = videoFrame.xres;
		int h = videoFrame.yres;

		if ((int)pixelBuffer.getWidth() != w || (int)pixelBuffer.getHeight() != h) {
			pixelBuffer.allocate(w, h, OF_PIXELS_RGBA);
			texture.allocate(w, h, GL_RGBA);
		}

		// Handle different NDI pixel formats
		// NDI gives us the format it chose based on our preference + what sender has
		switch (videoFrame.FourCC) {
			case NDIlib_FourCC_type_BGRA:
				copyFrameToPixels(videoFrame, pixelBuffer);
				pixelBuffer.swapRgb();
				break;

			case NDIlib_FourCC_type_BGRX:
				copyFrameToPixels(videoFrame, pixelBuffer);
				pixelBuffer.swapRgb();
				// BGRX has no alpha, but after swapRgb we have RGBX
				// ofPixels doesn't have a setAlpha(255), but GL_RGBA texture
				// will show whatever is in the 4th channel. For BGRX the X
				// byte is typically undefined/ignored. Let's leave it.
				break;

			case NDIlib_FourCC_type_RGBA:
				copyFrameToPixels(videoFrame, pixelBuffer);
				break;

			case NDIlib_FourCC_type_RGBX:
				copyFrameToPixels(videoFrame, pixelBuffer);
				break;

			case NDIlib_FourCC_type_UYVY:
				// UYVY 4:2:2 — needs YUV→RGBA conversion
				ofLogWarning("ofxNDIReceiver") << "UYVY received — colors will be wrong until shader conversion is added";
				copyFrameToPixels(videoFrame, pixelBuffer);
				break;

			default:
				ofLogWarning("ofxNDIReceiver") << "Unhandled FourCC: " << videoFrame.FourCC;
				copyFrameToPixels(videoFrame, pixelBuffer);
				break;
		}

		texture.loadData(pixelBuffer);
		ndiLib->recv_free_video_v2(receiver, &videoFrame);

	} else if (frameType == NDIlib_frame_type_none) {
		// No frame available this cycle — normal
	}

	if (frameType == NDIlib_frame_type_audio && audioFrame.p_data) {
		ndiLib->recv_free_audio_v3(receiver, &audioFrame);
	}
	if (frameType == NDIlib_frame_type_metadata && metadataFrame.p_data) {
		ndiLib->recv_free_metadata(receiver, &metadataFrame);
	}
}

void ofxNDIReceiver::draw(float x, float y) const {
	if (texture.isAllocated()) {
		texture.draw(x, y);
	}
}

void ofxNDIReceiver::draw(float x, float y, float w, float h) const {
	if (texture.isAllocated()) {
		texture.draw(x, y, w, h);
	}
}

ofTexture& ofxNDIReceiver::getTexture() {
	return texture;
}

const ofTexture& ofxNDIReceiver::getTexture() const {
	return texture;
}

bool ofxNDIReceiver::isConnected() const {
	return connected;
}

bool ofxNDIReceiver::isInitialized() const {
	return initialized;
}

float ofxNDIReceiver::getWidth() const {
	return texture.getWidth();
}

float ofxNDIReceiver::getHeight() const {
	return texture.getHeight();
}

size_t ofxNDIReceiver::getSenderCount() const {
	return senderNames.size();
}

std::string ofxNDIReceiver::getSenderName(size_t index) const {
	return (index < senderNames.size()) ? senderNames[index] : "";
}

bool ofxNDIReceiver::connect(const std::string& senderName) {
	if (!initialized) return false;

	if (senderName == currentSenderName && receiver) {
		return true;
	}

	for (size_t i = 0; i < senderNames.size(); i++) {
		if (senderNames[i] == senderName) {
			return createReceiver(senderSources[i]);
		}
	}

	pendingSenderName = senderName;
	return false;
}

bool ofxNDIReceiver::connect(size_t index) {
	if (!initialized || index >= senderSources.size()) return false;
	return createReceiver(senderSources[index]);
}

std::string ofxNDIReceiver::getCurrentSenderName() const {
	return currentSenderName;
}

// --- Private ---

void ofxNDIReceiver::refreshSenders() {
	if (!finder) return;

	uint32_t nSources = 0;
	const NDIlib_source_t* sources = ndiLib->find_get_current_sources(finder, &nSources);

	bool changed = (nSources != lastSourceCount);

	// Also detect if names changed at same count
	if (!changed && nSources > 0) {
		if (nSources != senderNames.size()) {
			changed = true;
		} else {
			for (uint32_t i = 0; i < nSources; i++) {
				std::string name = sources[i].p_ndi_name ? sources[i].p_ndi_name : "";
				if (i >= senderNames.size() || senderNames[i] != name) {
					changed = true;
					break;
				}
			}
		}
	}

	if (changed) {
		lastSourceCount = nSources;
		senderNames.clear();
		senderSources.clear();

		for (uint32_t i = 0; i < nSources; i++) {
			if (sources[i].p_ndi_name) {
				senderNames.emplace_back(sources[i].p_ndi_name);
				senderSources.push_back(sources[i]);
			}
		}

		ofLogVerbose("ofxNDIReceiver") << "Found " << senderNames.size() << " sender(s)";
	}
}

bool ofxNDIReceiver::createReceiver(const NDIlib_source_t& source) {
	releaseReceiver();

	NDIlib_recv_create_v3_t recvCreate = {};
	recvCreate.source_to_connect_to = source;
	// Request BGRA directly — most NDI sources send this natively
	// NDIlib_recv_color_format_BGRX_BGRA means "prefer BGRX, fallback to BGRA"
	// Value 0 = BGRX_BGRA which is the default / most compatible
	recvCreate.color_format = NDIlib_recv_color_format_BGRX_BGRA;
	recvCreate.bandwidth = NDIlib_recv_bandwidth_highest;
	recvCreate.allow_video_fields = false;

	receiver = ndiLib->recv_create_v3(&recvCreate);
	if (!receiver) {
		ofLogError("ofxNDIReceiver") << "Failed to create receiver for: " << source.p_ndi_name;
		return false;
	}

	NDIlib_tally_t tally = {true, false};
	ndiLib->recv_set_tally(receiver, &tally);

	currentSenderName = source.p_ndi_name ? source.p_ndi_name : "";
	connected = false;

	ofLogNotice("ofxNDIReceiver") << "Connected to: " << currentSenderName;
	return true;
}

void ofxNDIReceiver::releaseReceiver() {
	if (ndiLib && receiver) {
		ndiLib->recv_destroy(receiver);
		receiver = nullptr;
	}
	connected = false;
	currentSenderName.clear();
}

void ofxNDIReceiver::copyFrameToPixels(const NDIlib_video_frame_v2_t& frame, ofPixels& pixels) {
	int w = frame.xres;
	int h = frame.yres;
	int srcStride = frame.line_stride_in_bytes;
	int dstStride = w * 4;
	unsigned char* dst = pixels.getData();
	unsigned char* src = (unsigned char*)frame.p_data;

	for (int y = 0; y < h; y++) {
		memcpy(dst + y * dstStride, src + y * srcStride, dstStride);
	}
}
