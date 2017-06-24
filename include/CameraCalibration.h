/*
 * CameraCalibration.h
 *
 *  Created on: 15.01.2014
 *      Author: Stephan Manthe
 */

#ifndef CAMERACALIBRATION_H_
#define CAMERACALIBRATION_H_

#include <boost/property_tree/ptree.hpp>
#include <functional>
#include <opencv2/opencv.hpp>
#include <regex>
#include <string>
#include <vector>

namespace libba
{

class CameraCalibration
{
public:
    CameraCalibration();
    virtual ~CameraCalibration();

    struct CalibImgInfo
    {
        std::string filePath = "";
        bool patternFound = false;
        std::vector<cv::Point2f> boardCornersImg;
        float reprojectionError = -1;
    };

    /**
     * Executes the camera calibration with the current files.
     * @param statusFunc A function which is called if the progress changes.
     */
    void calibrateCamera(const std::function<void(int, int, std::string)> progressFunc);

    /**
     * Stops the calibration.
     */
    void stopCalibration();

    /**
     * Stores the calculated camera parameters in opencv filestorage format.
     */
    void saveCameraParameters(const std::string& filePath) const;

    void exportCameraParametersCv(const std::string& filePath) const;

    void exportCameraParametersJSON(const std::string& filePath) const;

    /**
     * Computes the reprojection error of the last camera calibration.
     */
    double computeReprojectionError();

    double computeDistortUndistortError();

    /**
     * Loads camera parameters from a file which was created with cv::Filestorage.
     */
    void loadCameraParametersXML(const std::string& filePath);

    void loadCameraParametersJSON(const std::string& filePath);

    const cv::Mat& getCameraMatrix() const;
    const cv::Mat& getDistCoeffs() const;

    /**
     * Returns the number of distortion coefficents for a specific distortion model.
     */
    size_t getNumDistortionCoefficents() const;
    std::vector<std::string> getFiles() const;

    void setFiles(const std::vector<std::string>& files);
    void addFile(const std::string& file);
    void removeFile(int index);
    void clearFiles();

    void setChessboardSize(const cv::Size2i& chessboardSize);
    void setCornerRefinmentWindowSize(const cv::Size2i& cornerRefinmentWindowSize);
    void setChessboardSquareWidth(float chessboardSquareWidth);
    bool isStopRequested() const;
    float getReprojectionError() const;


    const std::vector<CalibImgInfo>& getCalibInfo() const;
    bool isCalibrationDataAvailable() const;

    cv::Size getChessboardSize() const;
    float getChessboardSquareWidth() const;

    void setCalibrationFlags(const int calibrationFlags);

protected:
    /**
     * Contains the filepaths to the calibration images.
     */
    std::vector<CalibImgInfo> calibImages;

    /**
     * The number of inner checkerboard corners.
     */
    cv::Size2i chessboardCorners;

    /**
     * The size of the images which where used.
     */
    cv::Size2i imageSize;

    /**
     * Size of the corner refinment window
     */
    cv::Size2i cornerRefinmentWindowSize;

    /**
     * The size of the squares on the calibration pattern.
     */
    float chessboardSquareWidth;

    /**
     * The calculated calibration matrix.
     */
    cv::Mat calibrationMatrix;

    /**
     * Distortion coefficients
     */
    cv::Mat distortionCoefficients;

    /**
     * Rotation vectors for every camera
     */
    std::vector<cv::Mat> rotationVector;

    /**
     * Translation vectors for every camera
     */
    std::vector<cv::Mat> translationVector;

    /**
     * Contains the checkerboard corners which where found on the different images.
     */
    std::vector<std::vector<cv::Point2f> > imgCorners;

    /**
     * Contains the corners of the checkerboard in 3d.
     */
    std::vector<std::vector<cv::Point3f> > patternCorners;

    /**
     * If set to true the calibration process is stopped at the next possible date.
     */
    bool stopRequested;

    /**
     * Contains the reprojection error of the current camera calibration.
     */
    float reprojectionError;

    /**
     * Indicates if calibration data is available.
     */
    bool calibDataAvailabel;

    /**
     * Flags that are passed to the function cv::calibrateCamera .
     */
    size_t calibrationFlags;
};
}

#endif /* CAMERACALIBRATION_H_ */
