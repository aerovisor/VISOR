#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include "imagestitcher.h"
#include "objectrecognizer.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void displayImage(cv::Mat& image);
    void stitchImagesClicked();
    void detectButtonClicked();
    void gaussianSdChanged(int value);
    void cannyLowChanged(int value);
    void cannyHighChanged(int value);
    void houghVoteChanged(int value);
    void houghMinLengthChanged(int value);
    void houghMinDistanceChanged(int value);
    void stitchingUpdate(StitchingUpdateData *data);
    
private:
    void detectObjects();
    //For getting frames from camera/video/picture
    cv::VideoCapture capWebcam;
    int curIndex;
    long int counter;
    Ui::MainWindow *ui;
    ObjectRecognizer objectRecognizer;
    ImageStitcher* stitcher;

};

#endif // MAINWINDOW_H
