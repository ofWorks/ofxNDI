#pragma once

#include "ofMain.h"
#include "ofxNDI.h"

class ofApp : public ofBaseApp {
public:
	void setup();
	void update();
	void draw();
	void keyPressed(int key);
	void exit();

	ofxNDIReceiver receiver;

	// Track sender list changes ourselves
	std::vector<std::string> lastSenderNames;
	bool sendersChanged = false;
};
