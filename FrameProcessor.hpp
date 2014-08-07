#ifndef FRAMEPROCESSOR_HPP
#define FRAMEPROCESSOR_HPP

#include <opencv2/objdetect/objdetect.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/opencv.hpp>
#include <opencv2/core/core.hpp>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <queue>
#include <iostream>
#include <cstdio>

// ------------- PARAMS ------------------------

//#define onefile "7"

// process frame every ? frames
#define framePerAction 10

// the cascade xml path
const std::string _hog_path="/home/garzon/demo/hog_head.xml";

// -----------------------------------------------

class FrameProcessor{ // common interface: the base class of all frame processor
public:
    std::vector<cv::Rect> result;  // the var holds the results
    cv::Mat emptyMat; // == cv::Mat() , empty Matrix to pass to a member function
    // the interface of process function
    virtual void processFrame(int framePosition,const cv::Mat &frame,cv::Mat &output)=0;
};

class WatershedSegmenter{ // used in foreground segmentation
private:
    cv::Mat markers;
public:
    void setMarkers(const cv::Mat &markerImage){
        markerImage.convertTo(markers,CV_32S);
    }
    cv::Mat process(const cv::Mat &image){
        cv::watershed(image,markers);
        return markers;
    }
};

class FGSegmentation: public FrameProcessor{

    cv::BackgroundSubtractorMOG mog;
    WatershedSegmenter segmenter;
    cv::Mat fg,tmp,markers;

    std::queue<cv::Point> _queue;
    cv::Mat mark;
    cv::Point p,np;
    cv::Rect superPixel;

    template <typename T>
    inline T imin(T a,T b){
        if(a<b) return a; else return b;
    }

    template <typename T>
    inline T imax(T a,T b){
        if(a>b) return a; else return b;
    }

public:

    void segment(const cv::Mat &frame,cv::Mat &output){

        cv::cvtColor(frame,tmp,CV_BGR2GRAY);
        cv::equalizeHist(tmp,tmp);
        mog(tmp,tmp,0.01);  // mixture of gaussian algorithm
        cv::medianBlur(tmp,tmp,5);  //
        cv::medianBlur(tmp,tmp,5);

        cv::dilate(tmp,fg,cv::Mat(),cv::Point(-1,-1),20); //
        markers=fg+128;
        segmenter.setMarkers(markers);
        segmenter.process(frame).convertTo(fg,CV_8U);

        if(output.data==NULL) return;
        if(output.data!=frame.data)
            output=frame.clone();

        int w=frame.cols,h=frame.rows,i,j;

        uchar *row,*tmprow;

        for(i=0;i<h;i++){
            row=output.ptr<uchar>(i);
            tmprow=fg.ptr<uchar>(i);
            for(j=0;j<w;j++){
                if(tmprow[j]!=255){
                    row[j*3]=255;
                    row[j*3+1]=255;
                    row[j*3+2]=255;
                }
            }
        }

    }

    bool findNeighbor(int dir,const cv::Point& now,cv::Point &next,int w,int h){
        // find the neighbor in the specified direction
        next=now;
        switch(dir){
            case 0:
                next.y--;
                break;
            case 1:
                next.y++;
                break;
            case 2:
                next.x--;
                break;
            case 3:
                next.x++;
                break;
            default:
                assert(false);
        }
        // bounding
        if(next.x<0) return false;
        if(next.x==w) return false;
        if(next.y<0) return false;
        if(next.y==h) return false;
        return true;
    }

    void boundingBoxes(){
        result.clear();
        int typeNum=0;
        mark.create(fg.size(),CV_32S);
        int w=fg.size().width,h=fg.size().height,x,y,z;
        for(y=0;y<h;y++){
            for(x=0;x<w;x++){
                mark.at<int>(y,x)=0;  // set zeros
            }
        }
        uchar *pdata,tttmp;
        for(y=0;y<h;y++){
            pdata=fg.ptr<uchar>(y);
            for(x=0;x<w;x++){
                if(pdata[x]==255){
                    if(mark.at<int>(y,x)==0){
                        typeNum++;
                        int tlx=x,tly=y,brx=x,bry=y;
                        mark.at<int>(y,x)=typeNum;
                        p.x=x;
                        p.y=y;
                        _queue.push(p);
                        while(!(_queue.empty())){
                            p=_queue.front();
                            _queue.pop();
                            for(z=0;z<4;z++){
                                if(findNeighbor(z,p,np,w,h)){
                                    tttmp=fg.at<uchar>(np.y,np.x);
                                    if(tttmp==255){
                                        if(mark.at<int>(np.y,np.x)==0){
                                            tlx=imin(tlx,np.x);
                                            brx=imax(brx,np.x);
                                            tly=imin(tly,np.y);
                                            bry=imax(bry,np.y);
                                            mark.at<int>(np.y,np.x)=typeNum;
                                            _queue.push(np);
                                        }
                                    }
                                }
                            }
                        }
                        superPixel.x=tlx;
                        superPixel.y=tly;
                        superPixel.height=bry-tly;
                        superPixel.width=brx-tlx;
                        result.push_back(superPixel);
                    }
                }
            }
        }
    }

    void processFrame(int framePosition,const cv::Mat &frame,cv::Mat &output){
        segment(frame,emptyMat);
        boundingBoxes();
        if(output.data==NULL) return;
        if(output.data!=frame.data)
            output=frame.clone();
        for(const cv::Rect &FGBoundingBox: result){
            cv::rectangle(output,FGBoundingBox,cv::Scalar(255,0,0),2);
        }
    };
};  // output: frame with bounding boxes

class FaceDetection: public FrameProcessor{
    FGSegmentation FGSegmentor;
    cv::HOGDescriptor hog;
    cv::Mat fg,ROI;
    std::vector<cv::Rect> tmp;
public:

    FaceDetection():
        hog(cv::Size(48,48), cv::Size(16,16), cv::Size(8,8), cv::Size(8,8), 9),
        FGSegmentor()
    {
        hog.load(_hog_path);
        fg.create(cv::Size(1,1),CV_8UC3);
    }

    void processFrame(int framePosition,const cv::Mat &frame,cv::Mat &output){

        result.clear();

        FGSegmentor.processFrame(framePosition,frame,output);

        for(const cv::Rect &FGBoundingBox: FGSegmentor.result){
            ROI=frame(FGBoundingBox);
            tmp.clear();
            if(FGBoundingBox.width>=48)
                if(FGBoundingBox.height>=48)
                    hog.detectMultiScale(ROI, tmp, 1, cv::Size(8,8), cv::Size(0,0), 1.1, 3, false);
            for(cv::Rect &face: tmp){
                face.x+=FGBoundingBox.x;
                face.y+=FGBoundingBox.y;
                if(face.height*1.3+face.y<frame.rows)
                    face.height=floor(face.height*1.3);              // dilate
                result.push_back(face);
                if(output.data!=NULL)
                    cv::rectangle(output,face,cv::Scalar(0,255,0));  // draw bounding boxes
            }
        }

    }
};  // output: frame with bounding boxes (faces & moving objs)

#endif // FRAMEPROCESSOR_HPP
