#include "ofApp.h"

void ofApp::setup() {
	ofSetWindowTitle("ofxNDI Receiver");
	ofBackground(20);

	if (!receiver.setup()) {
		ofLogError("ofApp") << "NDI receiver setup failed";
	}
}

void ofApp::update() {
	receiver.update();
}

void ofApp::draw() {
	// Draw received texture centered and scaled to fit
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
		std::string msg = "Waiting for NDI source...";
		if (receiver.getSenderCount() > 0) {
			msg = "Sources found: " + ofToString(receiver.getSenderCount()) + "\nPress SPACE to list";
		}
		ofDrawBitmapString(msg, 20, 30);
	}

	// Info overlay
	ofSetColor(255);
	std::string info = "Sources: " + ofToString(receiver.getSenderCount());
	if (receiver.isConnected()) {
		info += " | Connected: " + receiver.getCurrentSenderName();
		info += " | " + ofToString((int)receiver.getWidth()) + "x" + ofToString((int)receiver.getHeight());
	}
	ofDrawBitmapString(info, 20, ofGetHeight() - 20);
}

void ofApp::keyPressed(int key) {
	if (key == ' ') {
		auto count = receiver.getSenderCount();
		ofLogNotice("ofApp") << "--- NDI Sources: " << count << " ---";
		for (size_t i = 0; i < count; i++) {
			ofLogNotice("ofApp") << "  [" << i << "] " << receiver.getSenderName(i);
		}
		if (count > 0) {
			ofLogNotice("ofApp") << "Press 0-" << (count - 1) << " to switch";
		}
	}

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
