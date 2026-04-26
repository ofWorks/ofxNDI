#pragma once

#include "ofMain.h"
#include "Processing.NDI.Lib.h"

class ofxNDIsender {
public:
	ofxNDIsender();
	~ofxNDIsender();

	// Initialize NDI. Call once in setup().
	bool setup(const std::string& senderName);

	// Release all NDI resources.
	void close();

	// Send OpenFrameworks types
	bool send(const ofTexture& tex);
	bool send(const ofFbo& fbo);
	bool send(const ofImage& img);
	bool send(const ofPixels& pix);

	// Send raw RGBA pixels
	bool send(const unsigned char* pixels, int width, int height);

	// State
	bool isInitialized() const;
	std::string getName() const;

	// NDI SDK version string (e.g. "NDI SDK 6.3.1")
	std::string getNDIVersion() const;

private:
	const NDIlib_v6* ndiLib = nullptr;
	NDIlib_send_instance_t sender = nullptr;

	bool initialized = false;
	std::string name;
	int width = 0;
	int height = 0;

	ofPixels pixelBuffer;

	bool sendFrame(const unsigned char* rgbaPixels, int w, int h);
};
