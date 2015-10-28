#include "testApp.h"

//--------------------------------------------------------------
void testApp::setup(){

	ofSetFrameRate(60);
	ofEnableAlphaBlending();

	// download sample Hap file from 
	// http://www.renderheads.com/downloads/2014/sample-1080p30-Hap.avi
	dsPlayer.loadMovie("sample-1080p30-Hap.avi");

	dsPlayer.play();

	ofSetWindowShape(dsPlayer.getWidth(), dsPlayer.getHeight());
}

//--------------------------------------------------------------
void testApp::update(){

	dsPlayer.update();
}

//--------------------------------------------------------------
void testApp::draw(){

	ofBackground(255);

	ofSetColor(255);
	dsPlayer.draw(0, 0);

	ofDrawBitmapStringHighlight(ofToString(ofGetFrameRate(), 2), 10, 20);
	ofDrawBitmapStringHighlight(ofToString(dsPlayer.getPosition(), 3), 10, 70);
}

//--------------------------------------------------------------
void testApp::keyPressed(int key){

}

//--------------------------------------------------------------
void testApp::keyReleased(int key){

}

//--------------------------------------------------------------
void testApp::mouseMoved(int x, int y){

}

//--------------------------------------------------------------
void testApp::mouseDragged(int x, int y, int button){

}

//--------------------------------------------------------------
void testApp::mousePressed(int x, int y, int button){

}

//--------------------------------------------------------------
void testApp::mouseReleased(int x, int y, int button){

}

//--------------------------------------------------------------
void testApp::windowResized(int w, int h){

}

//--------------------------------------------------------------
void testApp::gotMessage(ofMessage msg){

}

//--------------------------------------------------------------
void testApp::dragEvent(ofDragInfo dragInfo){ 

}