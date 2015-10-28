#include "testApp.h"

//--------------------------------------------------------------
void testApp::setup(){

	ofSetFrameRate(60);
	ofEnableAlphaBlending();

	//dsPlayer.loadMovie("touchdown1.wmv");
	//dsPlayer.loadMovie( "CokeRebootHap.avi" );
	//dsPlayer.loadMovie("CokeRebootHapNoAlpha.avi");
	//dsPlayer.loadMovie("sample-1080p30-Hap.avi"); // no workie, sample size isn't even
	//dsPlayer.loadMovie("sample-1080p30-Hap_1.avi");
	dsPlayer.loadMovie("sample-1080p30-Hap_2.avi"); // hapQ
	//dsPlayer.loadMovie("ElectricFields.avi"); // audio

	dsPlayer.play();

	ofSetWindowShape(dsPlayer.getWidth()*0.5, dsPlayer.getHeight()*0.5);
}

//--------------------------------------------------------------
void testApp::update(){

	if (ofGetFrameNum() % 5 == -1){

		int numFrames = dsPlayer.getTotalNumFrames();

		int randFrame = ofRandom(numFrames);

		dsPlayer.setFrame(randFrame);

		ofLogNotice() << "fr " << randFrame;
	}

	dsPlayer.update();
}

//--------------------------------------------------------------
void testApp::draw(){

	ofBackground(0, 255, 255);

	ofPushMatrix();

	ofScale(0.5, 0.5);

	ofSetColor(255);
	dsPlayer.draw(0, 0);

	ofPopMatrix();

	ofDrawBitmapStringHighlight(ofToString(ofGetFrameRate(), 2), 10, 20);
	//ofDrawBitmapStringHighlight(ofToString(dsPlayer.getCurrentFrame(), 3), 10, 45);
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