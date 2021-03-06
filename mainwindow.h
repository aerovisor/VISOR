#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include "imagestitcher.h"
#include "metadataparser.h"
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
    void saveCurrentImage();
    void stitchImagesClicked();
    void startImageStitchingClicked();
    void detectButtonClicked();
    void gaussianSdChanged(int value);
    void cannyLowChanged(int value);
    void cannyHighChanged(int value);
    void houghVoteChanged(int value);
    void houghMinLengthChanged(int value);
    void houghMinDistanceChanged(int value);
    void imageScaleChangedOR(int value);
    void polyErrorChangedOR(int value);
    void IS_scaleChanged(int value);
    void IS_radioButtonChanged();
    void stitchingUpdate(StitchingUpdateData *data);
    int getGaussianBlurValue();
    void displayRecognitionResult();
    void stitchingAngleChanged(double value);
    void stitchingDistanceChanged(double value);
    void stitchingHeuristicChanged(double value);
    void stitchingStepClicked();
    void stitchingMatchesUpdate(StitchingMatchesUpdateData data);
    void displayProposedMatches();
    void stitchStepRunClicked();
    void mouseMovedOnDisplay(int x, int y);

    
private:
    void detectObjects();
    //For getting frames from camera/video/picture
    cv::VideoCapture capWebcam;
    int curIndex;
    long int counter;
    Ui::MainWindow *ui;
    ObjectRecognizer objectRecognizer;
    ImageStitcher* stitcher;
    int saveImageCounter;
    StitchingUpdateData* lastData;
    RecognizerResults* lastResult;
    StitchingMatchesUpdateData currentMatches;
    MetaDataParser parser;
    MetaData currentORData;


};

#endif // MAINWINDOW_H
