#pragma once

#include "ofMain.h"
#include "Processing.NDI.Lib.h"

class ofxNDIReceiver {
public:
	// Event fired when the sender list changes (new sender, removed sender, or renamed).
	// The vector contains the current list of sender names.
	ofEvent<std::vector<std::string>> onSenderListChanged;
	ofxNDIReceiver();
	~ofxNDIReceiver();

	// Initialize NDI. Call once in setup().
	// Optionally specify a preferred sender name to auto-connect.
	bool setup(const std::string& preferredSender = "");

	// Release all NDI resources.
	void close();

	// Call in update(). Discovers senders and receives video frames.
	void update();

	// Draw the last received frame.
	void draw(float x, float y) const;
	void draw(float x, float y, float w, float h) const;

	// Access the internal texture directly.
	ofTexture& getTexture();
	const ofTexture& getTexture() const;

	// State
	bool isConnected() const;
	bool isInitialized() const;
	float getWidth() const;
	float getHeight() const;

	// Sender list (refreshed during update())
	size_t getSenderCount() const;
	std::string getSenderName(size_t index) const;

	// Switch to a different sender by name or index.
	bool connect(const std::string& senderName);
	bool connect(size_t index);

	// Current sender name
	std::string getCurrentSenderName() const;

	// NDI SDK version string (e.g. "NDI SDK 6.3.1")
	std::string getNDIVersion() const;

private:
	const NDIlib_v6* ndiLib = nullptr;
	NDIlib_find_instance_t finder = nullptr;
	NDIlib_recv_instance_t receiver = nullptr;

	ofTexture texture;
	ofPixels pixelBuffer;

	std::vector<std::string> senderNames;
	std::string currentSenderName;
	std::string pendingSenderName;
	std::string connectedSourceName; // persistent storage for NDI source name pointer

	bool initialized = false;
	bool connected = false;
	uint32_t lastSourceCount = 0;
	NDIlib_FourCC_video_type_e lastReceivedFourCC = (NDIlib_FourCC_video_type_e)0;
	uint64_t lastRefreshTime = 0;
	static constexpr uint64_t REFRESH_INTERVAL_MS = 500; // poll sender list twice per second

	void refreshSenders();
	bool createReceiver(const NDIlib_source_t& source);
	void releaseReceiver();
};
