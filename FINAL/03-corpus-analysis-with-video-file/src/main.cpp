/*
 Corpus Based Concatenative Synthesis
 
 1. analysis:
    - load a corpus of sound
    - store them using frames (e.g. 2048 samples of audio)
    - associate each frame with some audio features
 2. matching:
    - look at each frame of audio (2048 samples)
    - calculate the audio features
    - find the k-NN, NN - find the nearest audio segment based on
      the smallest distance to every possible audio feature.
    - playback the nearest segment(s).
 
 extend to do, e.g.:
    mfcc, delta mfcc, delta delta mfcc features
    granular synthesis
    circular buffers w/ smaller frame sizes
    onset detection to determine recordings rather than using every frame
    for variable length recordings, you could e.g. use the mean MFCC as well as the first frame's MFCC to have 26 instead of 13 features
    you could add chromagrams
    andrew's idea of detecting pitched content and using or not using chromas/mfccs.
 
 */

#include "ofMain.h"
#include "pkmFFT.h"
#include "pkmSTFT.h"
#include "pkmCircularRecorder.h"
#include "pkmAudioFeatures.h"
#include "pkmMatrix.h"
#include "pkmEXTAudioFileReader.h"

class Recording {
public:
    Recording(pkmMatrix buf, pkmMatrix feats) {
        buffer = buf;
        features = feats;
    }
    pkmMatrix buffer, features;
};

class Corpus {
public:
    void setup(int segment_size = 2048){
        analyzer.setup(44100, segment_size);
    }
    
    float * getNearestRecording(float *buf, int size) {
        pkmMatrix buffer(1, size, buf);
        pkmMatrix features(1, 36);
        analyzer.compute36DimAudioFeaturesF(buffer.data, features.data);
        
            // look at every single recording's features
            // calculate the distance to it
        
        float best_distance = HUGE_VALF;

        for(int recording_i = 0; recording_i < corpora.size(); recording_i++) {
            float this_distance = 0;
            for(int feature_i = 0; feature_i < 36; feature_i++){
                this_distance += abs(features[feature_i] - corpora[recording_i].features[feature_i]);
            }
            if (this_distance < best_distance) {
                best_distance = this_distance;
                best_idx = recording_i;
            }
        }
        
        if (corpora.size()) {
            return corpora[best_idx].buffer.data;
        }
        else {
            return NULL;
        }
    }
    
    int getBestIdx() {
        return best_idx;
    }
    
    void addRecording(float *buf, int size){
        pkmMatrix buffer(1, size, buf);
        pkmMatrix features(1, 36);
        analyzer.compute36DimAudioFeaturesF(buffer.data, features.data);
        Recording r(buffer, features);
        corpora.push_back(r);
    }
    
    int size() {
        return corpora.size();
    }
private:
    int best_idx;
    pkmAudioFeatures analyzer;
    vector<Recording> corpora;
};

class ofApp : public ofBaseApp {
public:
    void setup() {
        width = 500;
        height = 500;
        
        int frame_size = 1024;
        buffer = pkmMatrix(1, frame_size);
        corpus.setup(frame_size);
        
        player.load("zappa.mp4");
//        player.play();
        player.setVolume(0.0);
        
        ofSetWindowShape(1920 / 2.0, 1080 / 2.0);
        video_rate = player.getTotalNumFrames() / player.getDuration();
        
        reader1.open(ofToDataPath("zappa.wav"));
        int total_frames = reader1.mNumSamples / frame_size;

        for (int frame = 0; frame < total_frames; frame++) {
            pkmMatrix recording(1, frame_size);
            reader1.read(recording.data, frame * frame_size, frame_size);
            corpus.addRecording(recording.data, frame_size);
        }
        audio_rate = total_frames / (reader1.mNumSamples / 44100.0);
        
        cout << video_rate << "," << audio_rate << endl;

        ofSoundStreamSetup(1, 1, 44100, frame_size, 3);
    }
    
    void update() {
        int video_frame = corpus.getBestIdx() * (video_rate / audio_rate);
        player.setFrame(video_frame);
        player.update();
    }
    
    void draw() {
        ofDrawBitmapString(ofToString(corpus.size()), 20, 20);
        player.draw(0, 0, ofGetWidth(), ofGetHeight());
    }
    
    void audioIn(float *buf, int size, int ch) {
        for (int i = 0; i < size; i++) {
            buffer[i] = buf[i];
        }
    }
    
    void audioOut(float *buf, int size, int ch) {

            // get 2048 samples of audio and
            // play back the nearest audio segments
            // in my corpus
        float *recording = corpus.getNearestRecording(buffer.data, size);
        for (int i = 0; i < size; i++) {
            buf[i] = recording[i];
        }

    }
    
    void keyPressed(int k) {

    }
    
private:
    
    pkmMatrix buffer;
    
    pkmEXTAudioFileReader reader1;
    
    Corpus corpus;
    
    ofVideoPlayer player;
    
    int width, height;
    float video_rate, audio_rate;
    
    bool is_matching;
    bool is_recording;
};


//========================================================================
int main( ){
	ofSetupOpenGL(1024, 768, OF_WINDOW);
	ofRunApp(new ofApp());

}
