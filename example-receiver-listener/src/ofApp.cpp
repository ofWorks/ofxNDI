#include "ofApp.h"

void ofApp::setup() {
	ofSetWindowTitle("ofxNDI Receiver - Listener");
	ofBackground(20);

	if (!receiver.setup()) {
		ofLogError("ofApp") << "NDI receiver setup failed";
	}
}

void ofApp::update() {
	receiver.update();
}

void ofApp::draw() {
	if (receiver.isConnected()) {
		float texW = receiver.getWidth();
		float texH = receiver.getHeight();
		float winW = ofGetWidth();
		float winH = ofGetHeight();

		float scale = std::min(winW / texW, winH / texH);
		float drawW = texW * scale;
		float drawH = texH * scale;
		float x = (winW - drawW) * 0.5f;
		float y = (winH - drawH) * 0.5f;

		receiver.draw(x, y, drawW, drawH);
	} else {
		ofSetColor(200);
		ofDrawBitmapString("Waiting for NDI source...", 20, 30);
	}

	ofSetColor(255);
	std::string info = "Sources: " + ofToString(receiver.getSenderCount());
	if (receiver.isConnected()) {
		info += " | " + receiver.getCurrentSenderName();
		info += " | " + ofToString((int)receiver.getWidth()) + "x" + ofToString((int)receiver.getHeight());
	}
	ofDrawBitmapString(info, 20, ofGetHeight() - 40);

	if (sendersChanged) {
		ofSetColor(0, 255, 0);
		ofDrawBitmapString("SENDER LIST CHANGED", 20, ofGetHeight() - 20);
		sendersChanged = false;
	}

	ofSetColor(255);
	int y = 60;
	ofDrawBitmapString("Available sources:", 20, y);
	for (size_t i = 0; i < receiver.getSenderCount(); i++) {
		y += 15;
		std::string line = "  [" + ofToString(i) + "] " + receiver.getSenderName(i);
		if (receiver.getSenderName(i) == receiver.getCurrentSenderName()) {
			ofSetColor(0, 255, 0);
			line += " <--";
		} else {
			ofSetColor(200);
		}
		ofDrawBitmapString(line, 20, y);
	}
}

void ofApp::keyPressed(int key) {
	if (key >= '0' && key <= '9') {
		int idx = key - '0';
		if ((size_t)idx < receiver.getSenderCount()) {
			receiver.connect(idx);
		}
	}
}

void ofApp::exit() {
	receiver.close();
}
