#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFileDialog>
#include <opencv2/opencv.hpp>
#include <opencv2/stitching/stitcher.hpp>
#include "opencv2/features2d/features2d.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/nonfree/nonfree.hpp"
#include "opencv2/calib3d/calib3d.hpp"
#include "opencv2/imgproc/imgproc.hpp"

using cv::Mat;
using namespace cv;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    ui->display->setScaledContents(true);

    connect(ui->buttonOpenCamera, SIGNAL(clicked()), this, SLOT(openCameraClicked()));
    connect(ui->buttonOpenImage, SIGNAL(clicked()), this, SLOT(openImageClicked()));
    connect(ui->stitchButton, SIGNAL(clicked()), this, SLOT(stitchImagesClicked()));

    timer = new QTimer(this);
   // connect(timer, SIGNAL(timeout()),this, SLOT(processFrameAndUpdateGui()));
    timer->start(20);
}

MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::processFrameAndUpdateGui() {
    Mat matOriginal;

    if (inputImages.size() > 0) {
        inputImages[0].copyTo(matOriginal);
        /*++counter;
        if (counter > 1000) {
            counter = 0;
            ++curIndex;
            inputImages[curIndex % inputImages.size()].copyTo(matOriginal);
        }*/
    } else {
        capWebcam.read(matOriginal);
        if(matOriginal.empty() == true) return;
    }
    if (matOriginal.empty() == true) return;
    // images are stored by default as Blue Gree Red, convert to RGB for display
    cvtColor(matOriginal, matOriginal,CV_BGR2RGB);

    QImage qimgOrig((uchar*)matOriginal.data, matOriginal.cols, matOriginal.rows, matOriginal.step, QImage::Format_RGB888);

    ui->display->setPixmap(QPixmap::fromImage(qimgOrig));
}

void MainWindow::openCameraClicked() {
    capWebcam.open(0);
    inputImages.clear();
}

void MainWindow::openImageClicked() {
    capWebcam.release();
    QString file = QFileDialog::getOpenFileName();
    capWebcam.open(file.toStdString());
    inputImages.clear();
}

cv::Mat result;
void stitchImages(Mat &image1, Mat &image2 ) {

    //-- Step 1: Detect the keypoints using SURF Detector
     int minHessian = 400;

    SurfFeatureDetector detector( minHessian );

    std::vector< KeyPoint > keypoints_object, keypoints_scene;

    detector.detect( image1, keypoints_object );
    detector.detect( image2, keypoints_scene );

    //-- Step 2: Calculate descriptors (feature vectors)
     SurfDescriptorExtractor extractor;

    Mat descriptors_object, descriptors_scene;

    extractor.compute( image1, keypoints_object, descriptors_object );
     extractor.compute( image2, keypoints_scene, descriptors_scene );

    //-- Step 3: Matching descriptor vectors using FLANN matcher
     FlannBasedMatcher matcher;
     std::vector< DMatch > matches;
     matcher.match( descriptors_object, descriptors_scene, matches );

    double max_dist = 0; double min_dist = 100;

    //-- Quick calculation of max and min distances between keypoints
    for( int i = 0; i < descriptors_object.rows; i++ )
    { double dist = matches[i].distance;
        if( dist < min_dist ) min_dist = dist;
        if( dist > max_dist ) max_dist = dist;
    }

    printf("-- Max dist : %f \n", max_dist );
     printf("-- Min dist : %f \n", min_dist );

    //-- Use only "good" matches (i.e. whose distance is less than 3*min_dist )
     std::vector< DMatch > good_matches;

     for( int i = 0; i < descriptors_object.rows; i++ )
     {
         if( matches[i].distance < 3*min_dist )
         { good_matches.push_back( matches[i]); }
     }
     std::vector< Point2f > obj;
     std::vector< Point2f > scene;

    for( int i = 0; i < good_matches.size(); i++ )
     {
     //-- Get the keypoints from the good matches
     obj.push_back( keypoints_object[ good_matches[i].queryIdx ].pt );
     scene.push_back( keypoints_scene[ good_matches[i].trainIdx ].pt );
     }

    // Find the Homography Matrix
     Mat H = findHomography( obj, scene, CV_RANSAC );
     // Use the Homography Matrix to warp the images

     warpPerspective(image1,result,H,cv::Size(image1.cols+image2.cols,image1.rows+image2.rows));
     cv::Mat half(result,cv::Rect(0,0,image2.cols,image2.rows));
     image2.copyTo(half);

     //crop the black part of the image
     cv::Mat mask;
     vector<vector<Point> > contours; //no c++ 11 rabble rabble

     //need grey image to remove black box
     Mat gray_result;
     // Convert to Grayscale
     cvtColor( result, gray_result, CV_RGB2GRAY );

     threshold(gray_result, mask, 1.0, 255.0, CHAIN_APPROX_SIMPLE);
     findContours(mask, contours, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);

     double maxContourArea = 0.0;
     unsigned maxContourIndex, i;
     for (i  = 0; i < contours.size(); ++i) {
         double a = contourArea(contours[i]);
         if (a > maxContourArea) {
             maxContourArea  = a;
             maxContourIndex = i;
         }
     }

     cv::Rect boundingBox = boundingRect(contours[maxContourIndex]);
     result = result(boundingBox);
}

void MainWindow::stitchImagesClicked() {

    QStringList names = QFileDialog::getOpenFileNames();
    //Mat output;


    Mat image1= imread( names.at(0).toStdString() );
    Mat image2= imread( names.at(1).toStdString() );


    stitchImages(image1, image2 );


    cvtColor(result, result,CV_BGR2RGB);
    QImage qimgOrig((uchar*)result.data, result.cols, result.rows, result.step, QImage::Format_RGB888);
    ui->display->setPixmap(QPixmap::fromImage(qimgOrig));
    qimgOrig.save("output.png");
}

