#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup(){

	ofSetFrameRate(60);
	ofEnableAlphaBlending();

	// download sample Hap file from 
	// http://www.renderheads.com/downloads/2014/sample-1080p30-Hap.avi
	dsPlayer.loadMovie("sample-1080p30-Hap.avi");

	dsPlayer.play();

	ofSetWindowShape(dsPlayer.getWidth(), dsPlayer.getHeight());
}

//--------------------------------------------------------------
void ofApp::update(){

	dsPlayer.update();
}

//--------------------------------------------------------------
void ofApp::draw(){

	ofBackground(255);

	ofSetColor(255);
	dsPlayer.draw(0, 0);

	ofDrawBitmapStringHighlight(ofToString(ofGetFrameRate(), 2), 10, 20);
	ofDrawBitmapStringHighlight(ofToString(dsPlayer.getPosition(), 3), 10, 70);
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){

}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){

}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y){

}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h){

}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg){

}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo){ 

}