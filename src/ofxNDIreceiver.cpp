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
		connect(idx);
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
	currentSenderName.clear();
	pendingSenderName.clear();
	connectedSourceName.clear();

	texture.clear();
	pixelBuffer.clear();
}

void ofxNDIReceiver::update() {
	if (!initialized) return;

	refreshSenders();

	// Handle pending sender name (set by connect() when name not yet found)
	if (!pendingSenderName.empty() && pendingSenderName != currentSenderName) {
		for (size_t i = 0; i < senderNames.size(); i++) {
			if (senderNames[i] == pendingSenderName) {
				connect(i);
				break;
			}
		}
	}

	// Auto-connect to first sender if nothing connected
	if (!receiver && !senderNames.empty()) {
		connect(0);
	}

	if (!receiver) {
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

		// Log FourCC when it changes
		if (videoFrame.FourCC != lastReceivedFourCC) {
			char fourccStr[5] = {
				(char)(videoFrame.FourCC & 0xFF),
				(char)((videoFrame.FourCC >> 8) & 0xFF),
				(char)((videoFrame.FourCC >> 16) & 0xFF),
				(char)((videoFrame.FourCC >> 24) & 0xFF),
				0
			};
			ofLogNotice("ofxNDIReceiver") << "Video format changed: " << fourccStr
				<< " (0x" << std::hex << videoFrame.FourCC << std::dec << ") "
				<< w << "x" << h << " stride=" << videoFrame.line_stride_in_bytes
				<< " source=" << currentSenderName;
			lastReceivedFourCC = videoFrame.FourCC;
		}

		int srcStride = videoFrame.line_stride_in_bytes;
		unsigned char* src = (unsigned char*)videoFrame.p_data;
		unsigned char* dst = pixelBuffer.getData();

		switch (videoFrame.FourCC) {
			case NDIlib_FourCC_type_BGRA:
			case NDIlib_FourCC_type_RGBA:
				for (int y = 0; y < h; y++) {
					memcpy(dst + y * w * 4, src + y * srcStride, w * 4);
				}
				break;

			case NDIlib_FourCC_type_BGRX:
			case NDIlib_FourCC_type_RGBX:
				for (int y = 0; y < h; y++) {
					unsigned char* srcRow = src + y * srcStride;
					unsigned char* dstRow = dst + y * w * 4;
					for (int x = 0; x < w; x++) {
						dstRow[x * 4 + 0] = srcRow[x * 4 + 0];
						dstRow[x * 4 + 1] = srcRow[x * 4 + 1];
						dstRow[x * 4 + 2] = srcRow[x * 4 + 2];
						dstRow[x * 4 + 3] = 255;
					}
				}
				break;

			case NDIlib_FourCC_type_UYVY: {
				// UYVY is 4:2:2 YUV interleaved: 4 bytes per 2 pixels
				// Layout: [U, Y0, V, Y1] — U and V are shared between 2 adjacent pixels
				for (int y = 0; y < h; y++) {
					unsigned char* srcRow = src + y * srcStride;
					unsigned char* dstRow = dst + y * w * 4;
					for (int x = 0; x < w; x += 2) {
						// Each UYVY block covers 2 pixels, src offset is (x/2)*4
						int srcIdx = (x >> 1) * 4;
						unsigned char u  = srcRow[srcIdx + 0];
						unsigned char y0 = srcRow[srcIdx + 1];
						unsigned char v  = srcRow[srcIdx + 2];
						unsigned char y1 = srcRow[srcIdx + 3];

						// Convert YUV → RGB for both pixels (BT.601 limited range)
						int c0 = (int)y0 - 16;
						int c1 = (int)y1 - 16;
						int d  = (int)u - 128;
						int e  = (int)v - 128;

						int r0 = (298 * c0 + 409 * e + 128) >> 8;
						int g0 = (298 * c0 - 100 * d - 208 * e + 128) >> 8;
						int b0 = (298 * c0 + 516 * d + 128) >> 8;

						int r1 = (298 * c1 + 409 * e + 128) >> 8;
						int g1 = (298 * c1 - 100 * d - 208 * e + 128) >> 8;
						int b1 = (298 * c1 + 516 * d + 128) >> 8;

						int dstIdx0 = x * 4;
						int dstIdx1 = (x + 1) * 4;

						// Pixel 0
						dstRow[dstIdx0 + 0] = (unsigned char)std::max(0, std::min(255, r0));
						dstRow[dstIdx0 + 1] = (unsigned char)std::max(0, std::min(255, g0));
						dstRow[dstIdx0 + 2] = (unsigned char)std::max(0, std::min(255, b0));
						dstRow[dstIdx0 + 3] = 255;

						// Pixel 1
						dstRow[dstIdx1 + 0] = (unsigned char)std::max(0, std::min(255, r1));
						dstRow[dstIdx1 + 1] = (unsigned char)std::max(0, std::min(255, g1));
						dstRow[dstIdx1 + 2] = (unsigned char)std::max(0, std::min(255, b1));
						dstRow[dstIdx1 + 3] = 255;
					}
				}
				break;
			}

			default: {
				char fourccStr[5] = {
					(char)(videoFrame.FourCC & 0xFF),
					(char)((videoFrame.FourCC >> 8) & 0xFF),
					(char)((videoFrame.FourCC >> 16) & 0xFF),
					(char)((videoFrame.FourCC >> 24) & 0xFF),
					0
				};
				ofLogWarning("ofxNDIReceiver") << "Unhandled FourCC: " << fourccStr
					<< " (0x" << std::hex << videoFrame.FourCC << std::dec << ") — copying raw bytes";
				for (int y = 0; y < h; y++) {
					memcpy(dst + y * w * 4, src + y * srcStride, std::min(w * 4, srcStride));
				}
				break;
			}
		}

		texture.loadData(pixelBuffer);
		ndiLib->recv_free_video_v2(receiver, &videoFrame);

	} else if (frameType == NDIlib_frame_type_none) {
		// No frame this cycle
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

	// Find the sender in current list
	for (size_t i = 0; i < senderNames.size(); i++) {
		if (senderNames[i] == senderName) {
			return connect(i);
		}
	}

	// Not found yet — remember for later discovery
	pendingSenderName = senderName;
	return false;
}

bool ofxNDIReceiver::connect(size_t index) {
	if (!initialized || index >= senderNames.size()) return false;

	std::string name = senderNames[index];

	// If already connected to this sender, nothing to do
	if (name == currentSenderName && receiver) {
		return true;
	}

	// Persist the name — NDI may hold the pointer
	connectedSourceName = name;
	currentSenderName = name;

	// Create source AFTER persisting the name and BEFORE releaseReceiver can clear it
	NDIlib_source_t source;
	source.p_ndi_name = connectedSourceName.c_str();
	source.p_url_address = nullptr;

	bool ok = createReceiver(source);
	if (ok) {
		pendingSenderName.clear();
	} else {
		connectedSourceName.clear();
		currentSenderName.clear();
	}
	return ok;
}

std::string ofxNDIReceiver::getCurrentSenderName() const {
	return currentSenderName;
}

// --- Private ---

void ofxNDIReceiver::refreshSenders() {
	if (!finder) return;

	// Throttle sender list polling — NDI discovery is mDNS-based (~1s latency anyway)
	uint64_t now = ofGetElapsedTimeMillis();
	if (now - lastRefreshTime < REFRESH_INTERVAL_MS) {
		return;
	}
	lastRefreshTime = now;

	uint32_t nSources = 0;
	const NDIlib_source_t* sources = ndiLib->find_get_current_sources(finder, &nSources);

	bool changed = (nSources != lastSourceCount);

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

		for (uint32_t i = 0; i < nSources; i++) {
			if (sources[i].p_ndi_name) {
				senderNames.emplace_back(sources[i].p_ndi_name);
			}
		}

		ofLogNotice("ofxNDIReceiver") << "Sender list changed: " << senderNames.size() << " sender(s)";
		for (size_t i = 0; i < senderNames.size(); i++) {
			ofLogNotice("ofxNDIReceiver") << "  [" << i << "] " << senderNames[i];
		}
	}
}

bool ofxNDIReceiver::createReceiver(const NDIlib_source_t& source) {
	releaseReceiver();

	NDIlib_recv_create_v3_t recvCreate = {};
	recvCreate.source_to_connect_to = source;
	recvCreate.color_format = NDIlib_recv_color_format_BGRX_BGRA;
	recvCreate.bandwidth = NDIlib_recv_bandwidth_highest;
	recvCreate.allow_video_fields = false;

	receiver = ndiLib->recv_create_v3(&recvCreate);
	if (!receiver) {
		ofLogError("ofxNDIReceiver") << "Failed to create receiver for: " << (source.p_ndi_name ? source.p_ndi_name : "(null)");
		return false;
	}

	// Verify connection
	int nConnections = ndiLib->recv_get_no_connections(receiver);
	if (nConnections == 0) {
		ofLogWarning("ofxNDIReceiver") << "Receiver created but not connected to: " << (source.p_ndi_name ? source.p_ndi_name : "(null)");
	}

	NDIlib_tally_t tally = {true, false};
	ndiLib->recv_set_tally(receiver, &tally);

	connected = false;
	lastReceivedFourCC = (NDIlib_FourCC_video_type_e)0;

	ofLogNotice("ofxNDIReceiver") << "Connected to: " << (source.p_ndi_name ? source.p_ndi_name : "(null)")
		<< " (connections=" << nConnections << ")";
	return true;
}

void ofxNDIReceiver::releaseReceiver() {
	if (ndiLib && receiver) {
		ndiLib->recv_destroy(receiver);
		receiver = nullptr;
	}
	connected = false;
	// Don't clear connectedSourceName here — it may still be referenced by source param
	currentSenderName.clear();
	lastReceivedFourCC = (NDIlib_FourCC_video_type_e)0;
}
