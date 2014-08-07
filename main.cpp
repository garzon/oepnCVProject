/*

#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.show();

    return a.exec();
}


*/

#include "VideoProcessor.hpp"
#include "FrameProcessor.hpp"

using namespace cv;
using namespace std;

int main(){
    FaceDetection FD;
#ifdef onefile  // define @ FrameProcessor.hpp, the name of one image file to process
    Mat img=imread("/home/garzon/QtCreatorProjects/openCVProject/data/people/"onefile".jpg");
    Mat tmp;
    tmp.create(img.rows,img.cols,CV_8UC3);
    FD.processFrame(0,img,tmp);

    //cv::resize(img,img,cv::Size(640,480));
    namedWindow("output");
    imshow("output",tmp);
    waitKey();
    int id=0,maxid; long area,maxarea=0;

    for(Rect &face:FD.result){  // find the max size ROI to output as the found face
        area=face.height*face.width;
        if(maxarea<area){
            maxarea=area;
            maxid=id;
        }
        id++;
    }

    tmp=img(FD.result[maxid]);
    imwrite(
            "/home/garzon/QtCreatorProjects/openCVProject/data/people/t"onefile".jpg",
            tmp);
#else

    // set up the manager
    /* VideoProcessor Constructor:
     * params: VideoProcessor
     *          (FrameProcessor &frameProcessor,
                const std::string& openFileName="",
                const std::string& saveFileName="",
                bool debugMode=false);
          frameProcessor - the frame processor to call
          openFileName - the avi file path
          saveFileName - the DIRECTORY where the files of result are
          debugMode - show the result directly in a window?
    */
    VideoProcessor VP(FD,
                      "/home/garzon/QtCreatorProjects/openCVProject/data/.avi",
                      ""
                      //"./faces/"
                      ,true);

    try{
        VP.process();  // run
    }catch(VideoProcessor::Error e){
        cout<<e.errorMsg<<endl;
        // print the error msg if catch error
    }
#endif
    return 0;
}

//
