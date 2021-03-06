#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "sharedfunctions.h"
#include <fstream>
#include <cmath>
#include <QFileDialog>

#include <opencv2/opencv.hpp>
#include <opencv2/core/core.hpp>
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
    ui(new Ui::MainWindow), stitcher(NULL), saveImageCounter(0), lastData(NULL), lastResult(NULL)
{
    ui->setupUi(this);
    ui->groupBox->hide();
    ui->groupBox_IS->hide();

    qRegisterMetaType<StitchingUpdateData*>();   // Allows us to use the custom class in signals/slots
    qRegisterMetaType<StitchingMatchesUpdateData>();

    parser.setFileName(QString("metaData.txt"));

    connect(ui->stitchButton, SIGNAL(clicked()), this, SLOT(stitchImagesClicked()));
    connect(ui->detectButton, SIGNAL(clicked()), this, SLOT(detectButtonClicked()));
    connect(ui->button_IS_select, SIGNAL(clicked()), this, SLOT(startImageStitchingClicked()));
    connect(ui->saveImageButton, SIGNAL(clicked()), this, SLOT(saveCurrentImage()));
    connect(ui->slider_gaussian_sd, SIGNAL(valueChanged(int)), this, SLOT(gaussianSdChanged(int)));
    connect(ui->slider_canny_low, SIGNAL(valueChanged(int)), this, SLOT(cannyLowChanged(int)));
    connect(ui->slider_canny_high, SIGNAL(valueChanged(int)), this, SLOT(cannyHighChanged(int)));
    connect(ui->slider_hough_vote, SIGNAL(valueChanged(int)), this, SLOT(houghVoteChanged(int)));
    connect(ui->slider_hough_minLength, SIGNAL(valueChanged(int)), this, SLOT(houghMinLengthChanged(int)));
    connect(ui->slider_hough_minDistance, SIGNAL(valueChanged(int)), this, SLOT(houghMinDistanceChanged(int)));
    connect(ui->slider_IS_resize, SIGNAL(valueChanged(int)), this, SLOT(IS_scaleChanged(int)));
    connect(ui->radioButtonMatches, SIGNAL(clicked()), this, SLOT(IS_radioButtonChanged()));
    connect(ui->radioButtonScene, SIGNAL(clicked()), this, SLOT(IS_radioButtonChanged()));
    connect(ui->radio_or_canny, SIGNAL(clicked()), this, SLOT(displayRecognitionResult()));
    connect(ui->radio_or_canny2, SIGNAL(clicked()), this, SLOT(displayRecognitionResult()));
    connect(ui->radio_or_gaussian, SIGNAL(clicked()), this, SLOT(displayRecognitionResult()));
    connect(ui->radio_or_hough, SIGNAL(clicked()), this, SLOT(displayRecognitionResult()));
    connect(ui->radio_or_input, SIGNAL(clicked()), this, SLOT(displayRecognitionResult()));
    connect(ui->radio_or_output, SIGNAL(clicked()), this, SLOT(displayRecognitionResult()));
    connect(ui->slider_IS_angle, SIGNAL(customValueChanged(double)), this, SLOT(stitchingAngleChanged(double)));
    connect(ui->slider_IS_heuristic, SIGNAL(customValueChanged(double)), this, SLOT(stitchingHeuristicChanged(double)));
    connect(ui->slider_IS_length, SIGNAL(customValueChanged(double)), this, SLOT(stitchingDistanceChanged(double)));
    connect(ui->buttonStitchStep, SIGNAL(clicked()), this, SLOT(stitchingStepClicked()));
    connect(ui->radio_IS_run, SIGNAL(clicked()), this, SLOT(stitchStepRunClicked()));
    connect(ui->radio_IS_step, SIGNAL(clicked()), this, SLOT(stitchStepRunClicked()));
    connect(ui->slider_OR_imageScale, SIGNAL(valueChanged(int)), this, SLOT(imageScaleChangedOR(int)));
    connect(ui->slider_OR_polyError, SIGNAL(valueChanged(int)), this, SLOT(polyErrorChangedOR(int)));
    connect(ui->display, SIGNAL(mouseMove(int,int)), this, SLOT(mouseMovedOnDisplay(int, int)));

    // set initial values
    ui->label_gaussian_sd->setText(QString::number(getGaussianBlurValue()));
    ui->label_canny_low->setText(QString::number(ui->slider_canny_low->value()));
    ui->label_canny_high->setText(QString::number(ui->slider_canny_high->value()));
    ui->label_hough_vote->setText(QString::number(ui->slider_hough_vote->value()));
    ui->label_hough_minLength->setText(QString::number(ui->slider_hough_minLength->value()));
    ui->label_hough_minDistance->setText(QString::number(ui->slider_hough_minDistance->value()));
    ui->label_OR_polyError->setText(QString::number(ui->slider_OR_polyError->value() / 10.0) + "%");
    ui->label_OR_imageScale->setText(QString::number(ui->slider_OR_imageScale->value()) + "%");
    objectRecognizer.gaussianSD = getGaussianBlurValue();
    objectRecognizer.cannyLow = ui->slider_canny_low->value();
    objectRecognizer.cannyHigh = ui->slider_canny_high->value();
    objectRecognizer.houghVote = ui->slider_hough_vote->value();
    objectRecognizer.houghMinLength = ui->slider_hough_minLength->value();
    objectRecognizer.houghMinDistance = ui->slider_hough_minDistance->value();
    objectRecognizer.imageScale = ui->slider_OR_imageScale->value() / 100.0;
    objectRecognizer.polyDPError = ui->slider_OR_polyError->value() / 1000.0;

    ui->slider_IS_angle->setCustomValues(0.1, 5.0, 1.0);
    ui->slider_IS_length->setCustomValues(0.1, 5.0, 1.0);
    ui->slider_IS_heuristic->setCustomValues(1.0, 5.0, 3.0);

}

MainWindow::~MainWindow()
{
    if (lastData) {
        delete lastData;
    }
    if (lastResult) {
        delete lastResult;
    }
    delete ui;
}

void MainWindow::saveCurrentImage() {
    //if (ui->display->pixmap() == NULL || ui->display->pixmap()->isNull()) return;
    QString fileName = QString("save") + QString::number(saveImageCounter) + ".jpg";
    saveImageCounter++;
    ui->display->getImage().save(fileName);
    std::cout << "saved image " << fileName.toStdString() << std::endl;
}

void MainWindow::displayImage(cv::Mat& image) {
    cvtColor(image, image,CV_BGR2RGB);
    QImage qimgOrig((uchar*)image.data, image.cols, image.rows, image.step, QImage::Format_RGB888);
    ui->display->setImage(qimgOrig);
    cvtColor(image, image, CV_RGB2BGR);
}

void MainWindow::IS_scaleChanged(int value) {
    ui->label_IS_resize->setText(QString::number(value));
    // value from the slider is read directly when creating the stitcher object
}

void MainWindow::IS_radioButtonChanged() {
    if (lastData) {
        if (ui->radioButtonMatches->isChecked()) {
            displayImage(lastData->currentFeatureMatches);
        } else {
            displayImage(lastData->currentScene);
        }
    }
}

void MainWindow::displayRecognitionResult() {
    if (lastResult == NULL) return;
    if (ui->radio_or_canny->isChecked()) {
        displayImage(lastResult->canny);
    } else if (ui->radio_or_canny2->isChecked()) {
        displayImage(lastResult->canny2);
    } else if (ui->radio_or_gaussian->isChecked()) {
        displayImage(lastResult->gaussianBlur);
    } else if (ui->radio_or_hough->isChecked()) {
        displayImage(lastResult->hough);
    } else if (ui->radio_or_input->isChecked()) {
        displayImage(lastResult->input);
    } else if (ui->radio_or_output->isChecked()) {
        displayImage(lastResult->output);
    }
}

void MainWindow::stitchingAngleChanged(double value)
{
    ui->label_IS_angle->setText(QString::number(value));
    displayProposedMatches();
}

void MainWindow::stitchingDistanceChanged(double value)
{
    ui->label_IS_length->setText(QString::number(value));
    displayProposedMatches();
}

void MainWindow::stitchingHeuristicChanged(double value)
{
    ui->label_IS_heuristic->setText(QString::number(value));
    displayProposedMatches();
}

void MainWindow::stitchStepRunClicked() {
    if (stitcher) {
        stitcher->setStepMode(ui->radio_IS_step->isChecked());
    }
}

void MainWindow::stitchingStepClicked()
{
    ui->frame_IS_step_controls->setEnabled(false);
    ui->frame_IS_showResults->setEnabled(true);
    ui->progressBar->setEnabled(true);
    if (stitcher) {
        double angleParam = ui->slider_IS_angle->getCurrentCustomValue();
        double lengthParam = ui->slider_IS_length->getCurrentCustomValue();
        double heuristicParam = ui->slider_IS_heuristic->getCurrentCustomValue();
        stitcher->nextStep(angleParam, lengthParam, heuristicParam);
    }
}

void MainWindow::stitchingMatchesUpdate(StitchingMatchesUpdateData data) {
    ui->frame_IS_step_controls->setEnabled(true);
    ui->frame_IS_showResults->setEnabled(false);
    ui->progressBar->setEnabled(false);
    currentMatches = data;
    displayProposedMatches();
}

void MainWindow::displayProposedMatches() {
    if (!currentMatches.object.empty() && !currentMatches.scene.empty()) {
        std::vector<cv::DMatch> goodMatches = ImageStitcher::pruneMatches(currentMatches.matches,
             currentMatches.objFeatures, currentMatches.sceneFeatures,
             ui->slider_IS_angle->getCurrentCustomValue(),
             ui->slider_IS_length->getCurrentCustomValue(),
             ui->slider_IS_heuristic->getCurrentCustomValue());
        Mat imgMatches;
        drawMatches( currentMatches.object, currentMatches.objFeatures,
                     currentMatches.scene, currentMatches.sceneFeatures,
                     goodMatches, imgMatches, Scalar(0, 255, 0), Scalar(255, 0, 0),
                     vector<char>(), DrawMatchesFlags::DEFAULT );
        cv::Rect crop = SharedFunctions::findBoundingBox(imgMatches);
        imgMatches = imgMatches(crop);
        displayImage(imgMatches);
        ui->label_IS_numMatches->setText(QString("Good Matches: ") + QString::number(goodMatches.size()) + "/" + QString::number(currentMatches.matches.size()));
    }
}

void MainWindow::stitchingUpdate(StitchingUpdateData* data) {

    displayImage(data->currentScene);
    if (data->totalImages > 0) {
        ui->progressBar->setValue(((double)data->curIndex)/ data->totalImages * 100);
    }
    ui->label_IS_progress->setText(QString::number(data->curIndex) + "/" + QString::number(data->totalImages));
    //saveImage(data->currentScene, "resultAfter.png");
    if (lastData) {
        delete lastData;
    }
    lastData = data;
}

void MainWindow::startImageStitchingClicked() {
    QStringList inputFiles = QFileDialog::getOpenFileNames();
    if (inputFiles.size() < 2) return;    // don't crash on one input image

    if (stitcher) {
        stitcher->terminate();    //possibly risky way to terminate an already running stitcher
        delete stitcher;
        ui->progressBar->setEnabled(true);
        ui->frame_IS_showResults->setEnabled(true);
        ui->frame_IS_step_controls->setEnabled(false);
    }
    //TODO make these take input instead of just being constant
    double angleParam = ui->slider_IS_angle->getCurrentCustomValue();
    double lengthParam = ui->slider_IS_length->getCurrentCustomValue();
    double heuristicParam = ui->slider_IS_heuristic->getCurrentCustomValue();
    bool stepMode = ui->radio_IS_step->isChecked();
    ImageStitcher::AlgorithmType algorithm = ImageStitcher::COMPOUND_HOMOGRAPHY;
    if (ui->radio_IS_wholeImageMatching->isChecked()) {
        algorithm = ImageStitcher::FULL_MATCHES;
    } else if (ui->radio_IS_reduce->isChecked()) {
        algorithm = ImageStitcher::REDUCE;
    } else if (ui->radio_IS_ROI->isChecked()) {
        algorithm = ImageStitcher::CUMULATIVE;
    }
    stitcher = new ImageStitcher(inputFiles, ui->slider_IS_resize->value() / 100.0, 1.25, angleParam, lengthParam, heuristicParam, ImageStitcher::SURF, ImageStitcher::BRUTE_FORCE, stepMode, algorithm);
    connect(stitcher, SIGNAL(stitchingUpdate(StitchingUpdateData*)), this, SLOT(stitchingUpdate(StitchingUpdateData*)), Qt::QueuedConnection);
    connect(stitcher, SIGNAL(stitchingUpdateMatches(StitchingMatchesUpdateData)), this, SLOT(stitchingMatchesUpdate(StitchingMatchesUpdateData)));
    stitcher->start();
}

void MainWindow::stitchImagesClicked() {
    ui->groupBox->hide();
    ui->groupBox_IS->show();
    ui->progressBar->setValue(0);
    ui->display->scene()->clear();
    ui->label_IS_progress->setText("");
    ui->label_IS_resize->setText(QString::number(ui->slider_IS_resize->value()));
}

void MainWindow::detectObjects() {
    ui->groupBox_IS->hide();
    ui->groupBox->show();
    ui->display->scene()->clear();
    RecognizerResults* results = objectRecognizer.recognizeObjects();
    if (lastResult != NULL) {
        delete lastResult;
    }
    lastResult = results;
    displayRecognitionResult();
}

void MainWindow::detectButtonClicked(){
    QString name = QFileDialog::getOpenFileName();
    currentORData = parser.searchForImage(name);
    objectRecognizer.fullSizeInputImage = imread( name.toStdString() );
    detectObjects();
}

int MainWindow::getGaussianBlurValue() {
    int value = ui->slider_gaussian_sd->value();
    if (value % 2 == 0) ++value;   // always has to be odd
    return value;
}

void MainWindow::gaussianSdChanged(int value) {
    int oddValue = getGaussianBlurValue();
    ui->label_gaussian_sd->setText(QString::number(oddValue));
    objectRecognizer.gaussianSD = oddValue;
    detectObjects();
}

void MainWindow::cannyLowChanged(int value) {
    ui->label_canny_low->setText(QString::number(value));
    objectRecognizer.cannyLow = value;
    detectObjects();
}

void MainWindow::cannyHighChanged(int value) {
    ui->label_canny_high->setText(QString::number(value));
    objectRecognizer.cannyHigh = value;
    detectObjects();
}

void MainWindow::houghVoteChanged(int value) {
    ui->label_hough_vote->setText(QString::number(value));
    objectRecognizer.houghVote = value;
    detectObjects();
}

void MainWindow::houghMinLengthChanged(int value) {
    ui->label_hough_minLength->setText(QString::number(value));
    objectRecognizer.houghMinLength = value;
    detectObjects();
}

void MainWindow::houghMinDistanceChanged(int value) {
    ui->label_hough_minDistance->setText(QString::number(value));
    objectRecognizer.houghMinDistance = value;
    detectObjects();
}

void MainWindow::imageScaleChangedOR(int value) {
    double scaleValue = ((double)value) / 100.0;
    ui->label_OR_imageScale->setText(QString::number(value) + "%");
    objectRecognizer.imageScale = scaleValue;
    detectObjects();
}

void MainWindow::polyErrorChangedOR(int value) {
    double error = ((double)value) / 1000.0;
    ui->label_OR_polyError->setText(QString::number(value / 10.0) + "%");
    objectRecognizer.polyDPError = error;
    detectObjects();
}

const double FOV_degrees = 40.0;
const double FOV = FOV_degrees * M_PI / 180.0; // c libs use radians
void MainWindow::mouseMovedOnDisplay(int x, int y) {
    if (currentORData.dataIsValid && ui->groupBox->isEnabled()) {

        double metersPerPix = (double) currentORData.data[ALT] * atan(FOV) / ui->display->getImage().width();

        double yaw     = -1 * currentORData.data[YAW] * M_PI / 180.0; // convert to radians
        double offsetX =    cos(yaw)*x + sin(yaw)*y;
        double offsetY = -1*sin(yaw)*x + cos(yaw)*y;

        // offsets in meters
        offsetX *= metersPerPix;
        offsetY *= metersPerPix;

        //Earth’s radius
        int R = 6378137;

        // delta lat/long in radians
        double dLat = (double) offsetY/R;
        double dLon = (double) offsetX/(R*cos(M_PI*currentORData.data[LAT]/180.0));

        //OffsetPosition, decimal degrees
        double latitude  = currentORData.data[LAT] + dLat * 180.0/M_PI;
        double longitude = currentORData.data[LON] + dLon * 180.0/M_PI;

        QLocale c(QLocale::C);

        ui->label_OR_coords->setText(QString("lat: ") + c.toString(latitude, 'f', 9) + "\nlong: " + c.toString(longitude, 'f', 9));
        // dunno if we want both
        //ui->label_OR_coords->setText(QString("x: ") + QString::number(x) + "\nyg: " + QString::number(y));
    }
}






