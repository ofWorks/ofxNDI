#pragma once

#include "ofMain.h"
#include "Processing.NDI.Lib.h"

class ofxNDIReceiver {
public:
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

private:
	const NDIlib_v6* ndiLib = nullptr;
	NDIlib_find_instance_t finder = nullptr;
	NDIlib_recv_instance_t receiver = nullptr;

	ofTexture texture;
	ofPixels pixelBuffer;

	std::vector<std::string> senderNames;
	std::string currentSenderName;
	std::string pendingSenderName;

	bool initialized = false;
	bool connected = false;
	uint32_t lastSourceCount = 0;

	void refreshSenders();
	bool createReceiver(const NDIlib_source_t& source);
	void releaseReceiver();
};
