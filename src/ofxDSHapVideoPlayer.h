
//ofxDSHapVideoPlayer written by Jeremy Rotsztain and Philippe Laurheret for Second Story, 2015
//DirectShowVideo and ofDirectShowPlayer written by Theodore Watson, Jan 2014
//See the cpp file for the DirectShow implementation 

#pragma once 

#include "ofMain.h"
#include "HapShared.h"

class DirectShowHapVideo;

class ofxDSHapVideoPlayer : public ofBaseVideoPlayer {

	public:

		ofxDSHapVideoPlayer();
		~ofxDSHapVideoPlayer();

		bool loadMovie(string path);
		void update();
		void waitUpdate(long milliseconds);
		void writeToTexture(ofTexture& texture);
		void draw(float x, float y);

		 void				close();
	
		 void				play();
		 void				pause();
		 void				stop();		
	
		 bool 				isFrameNew();
		 unsigned char * 	getPixels(); // @NOTE: return uncompressed pixels
	     ofPixelsRef		getPixelsRef();
	
		 float 				getWidth();
		 float 				getHeight();
	
		 bool				isPaused();
		 bool				isLoaded();
		 bool				isPlaying();
	
		 bool setPixelFormat(ofPixelFormat pixelFormat);
		 ofPixelFormat 		getPixelFormat();
         ofShader           getShader();
         ofTexture *        getTexture();


		 float 				getPosition();
		 float 				getSpeed();
		 float 				getDuration();
		 bool				getIsMovieDone();
	
		 void 				setPaused(bool bPause);
		 void 				setPosition(float pct);
		 void 				setVolume(float volume); // 0..1
		 void 				setLoopState(ofLoopType state);
		 void   			setSpeed(float speed);
		 void				setFrame(int frame);  // frame 0 = first frame...
	
		 int				getCurrentFrame();
		 int				getTotalFrames();
		 ofLoopType			getLoopState();
	
		 void				firstFrame();
		 void				nextFrame();
		 void				previousFrame();

         enum               HapType {
                                HAP,
                                HAPALPHA,
                                HAPQ
                            };

         HapType            getHapType();

		 int				height;
		 int				width;

	protected:

		// using DS for grabbing frames
		DirectShowHapVideo * player;
		
		// hap shader (for YCoCg)
		ofShader shader; 
		bool bShaderInitialized;

		ofPixels pix; // copy of compressed pixels
		ofTexture tex; // texture for pix

		HapTextureFormat textureFormat;
};