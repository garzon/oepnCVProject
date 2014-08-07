#ifndef VIDEOPROCESSOR_HPP
#define VIDEOPROCESSOR_HPP

#include "FrameProcessor.hpp"

class VideoProcessor{

    cv::VideoCapture capture;
    FrameProcessor *_frameProcessor;
    std::string _input,_output;
    bool _debugMode;

    int frameId,imgId,fps;
    cv::Mat frame,output;

    std::string int2str(int n){
        std::stringstream ss;
        ss<<n;
        std::string res;
        ss>>res;
        return res;
    }

public:

    // -------------- error classes -------------------
    class Error{
    public:
        std::string errorMsg;
        Error(const std::string &err):errorMsg(err){}
    };
    class InputError:public Error{
    public:
        InputError():Error("Input Error"){}
    };
    class OutputError:public Error{
    public:
        OutputError():Error("Output Error"){}
    };
    // -------------- error classes -------------------

    void writeFaces(const cv::Mat &frame,const std::vector<cv::Rect> &faces,const std::string &outputDir,int &imgId){
        cv::Mat tmp;
        for(const cv::Rect &face: faces){
            tmp=frame(face);
            cv::imwrite(outputDir+int2str(imgId)+".jpg",tmp);
            imgId++;
        }
    }

    VideoProcessor(FrameProcessor &frameProcessor,
                   const std::string& openFileName="",
                   const std::string& saveFileName="",
                   bool debugMode=false):
        _input(openFileName),
        _output(saveFileName),
        _frameProcessor(&frameProcessor),
        _debugMode(debugMode),
        imgId(0)
    {}

    ~VideoProcessor(){
        cv::destroyAllWindows();
        capture.release();
    }

    void process(){
        cv::destroyAllWindows();
        capture.release();
        if(_input.length()==0){
            // read frames from the camera
            if(!capture.open(0)) throw InputError();
            fps=50;
        }else{
            // read frames from the file
            if(!capture.open(_input)) throw InputError();
            fps=capture.get(CV_CAP_PROP_FPS);
        }
        frameId=0;
        if(!capture.read(frame)) throw InputError();
        if(_debugMode){
            cv::namedWindow("Output");
        }

        output.create(frame.rows,frame.cols,CV_8UC3);

        while(true){
            if(frameId%framePerAction==0){

                // resize to accelerate
                cv::resize(frame,frame,cv::Size(640,480));
                // process the frame
                _frameProcessor->processFrame(frameId,frame,output);

                // show result in a window
                if(_debugMode) cv::imshow("Output",output);
                // write to files
                if(_output.length()!=0)
                    writeFaces(frame,_frameProcessor->result,_output,imgId);

                if(cv::waitKey(1)==27)  break;  // press esc to break the program
            }

            frameId++;
            if(!capture.read(frame)) break;

        }
        if(_debugMode) cv::destroyAllWindows();
    }
};

#endif // VIDEOPROCESSOR_HPP
