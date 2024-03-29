/*
 * CameraCalibration.cpp
 *
 *  Created on: 15.01.2014
 *      Author: Stephan Manthe
 */

#include "camera_calibration/CameraCalibration.h"
#include "nlohmann/json.hpp"
#include <filesystem>
#include <fstream>
#include <opencv2/core.hpp>
#include <stdexcept>

namespace libba
{

CameraCalibration::CameraCalibration()
    : chessboardCorners(7, 6)
    , cornerRefinmentWindowSize(10, 10)
    , chessboardSquareWidth(0.06)
    , stopRequested(false)
    , reprojectionError(0)
    , calibDataAvailabel(false)
    , calibrationFlags(0)
{
}
//-------------------------------------------------------------------------------------------------
void CameraCalibration::calibrateCamera(
    const std::function<void(int, int, std::string)> progressFunc)
{
    stopRequested = false;
    calibDataAvailabel = false;

    imgCorners.clear();
    patternCorners.clear();
    rotationVector.clear();
    translationVector.clear();

    // calculate corners from the calibration pattern
    std::vector<cv::Point3f> chessboardCorners3d;
    for (int i = 0; i < chessboardCorners.height; ++i)
        for (int j = 0; j < chessboardCorners.width; ++j)
            chessboardCorners3d.emplace_back(
                float(j * chessboardSquareWidth), float(i * chessboardSquareWidth), 0);

    if (calibImages.size() == 0)
        throw std::runtime_error("No images for calibration provided.");

    const int maxNumberSteps = calibImages.size() + 1;
    int currentStep = 0;

    imageSize.width = -1;
    imageSize.height = -1;
    for (size_t i = 0; i < calibImages.size(); ++i)
    {
        calibImages[i].reprojectionError = 0;
        calibImages[i].patternFound = false;

        currentStep++;
        const cv::Mat img = cv::imread(calibImages[i].filePath, cv::IMREAD_GRAYSCALE);

        if (i == 0)
            imageSize = img.size();

        if (img.cols != imageSize.width || img.rows != imageSize.height)
        {
            std::string errorMsg = "This image had the wrong size for the calibration: "
                + calibImages[i].filePath + " expected: " + std::to_string(imageSize.width) + "x"
                + std::to_string(imageSize.height) + " img: " + std::to_string(img.cols) + "x"
                + std::to_string(img.rows);
            throw std::runtime_error(errorMsg);
        }

        std::vector<cv::Point2f> cornersTemp;
        const bool patternFound = cv::findChessboardCorners(img, chessboardCorners, cornersTemp,
            cv::CALIB_CB_ADAPTIVE_THRESH | cv::CALIB_CB_FILTER_QUADS);

        if (stopRequested)
            return;

        if (!patternFound)
        {
            calibImages[i].patternFound = false;
            progressFunc(currentStep, maxNumberSteps, calibImages[i].filePath);
            continue;
        }

        if (cornerRefinmentWindowSize.width > 0 && cornerRefinmentWindowSize.height > 0)
        {
            try
            {
                // TODO make this an option for the gui
                if (true)
                    cv::cornerSubPix(img, cornersTemp, cornerRefinmentWindowSize, cv::Size(-1, -1),
                        cv::TermCriteria(cv::TermCriteria::COUNT + cv::TermCriteria::EPS, 30, 0.1));
                else
                    cv::find4QuadCornerSubpix(img, cornersTemp, cornerRefinmentWindowSize);
            }

            catch (const cv::Exception& e)
            {
                calibImages[i].patternFound = false;
                progressFunc(currentStep, maxNumberSteps, calibImages[i].filePath);

                std::cout << "OpenCV exception for image " << i
                          << " during subpixel refinment: " << e.what() << std::endl;
                continue;
            }
        }

        calibImages[i].patternFound = true;
        calibImages[i].boardCornersImg = cornersTemp;

        if (stopRequested)
            return;

        imgCorners.push_back(std::move(cornersTemp));
        patternCorners.push_back(chessboardCorners3d);
        progressFunc(currentStep, maxNumberSteps, calibImages[i].filePath);
    }

    try
    {
        calibrationMatrix = cv::Mat::eye(3, 3, CV_64F);
        distortionCoefficients = cv::Mat::zeros(12, 1, CV_64F);

        // TODO make the number of iterations changeable
        cv::calibrateCamera(patternCorners, imgCorners, imageSize, calibrationMatrix,
            distortionCoefficients, rotationVector, translationVector, calibrationFlags,
            cv::TermCriteria(cv::TermCriteria::COUNT + cv::TermCriteria::EPS, 30, DBL_EPSILON));
        currentStep++;
        progressFunc(currentStep, maxNumberSteps, "");
    }
    catch (const cv::Exception& ex)
    {
        std::cout << "ex.what() = " << ex.what() << std::endl;
    }

    reprojectionError = computeReprojectionError();
    // computeDistortUndistortError(); // TODO Display this error inside of the
    // gui
    calibDataAvailabel = true;
}
//-------------------------------------------------------------------------------------------------
void CameraCalibration::saveCameraParameters(const std::string& filePath) const
{
    namespace fs = std::filesystem;
    const auto extension = fs::path(filePath).extension().string();
    if (extension == ".xml")
        exportCameraParametersCv(filePath);
    else if (extension == ".json")
        exportCameraParametersJSON(filePath);
}
//-------------------------------------------------------------------------------------------------
void CameraCalibration::exportCameraParametersCv(const std::string& filePath) const
{
    cv::FileStorage fs(filePath, cv::FileStorage::WRITE); // Read the settings
    if (!fs.isOpened())
        throw std::runtime_error("Could not open the configuration file: \"" + filePath + "\"");

    fs << "fx" << calibrationMatrix.at<double>(0, 0);
    fs << "fy" << calibrationMatrix.at<double>(1, 1);
    fs << "cx" << calibrationMatrix.at<double>(0, 2);
    fs << "cy" << calibrationMatrix.at<double>(1, 2);
    fs << "distortion_coefficients" << distortionCoefficients;
    fs << "vertical_resolution" << imageSize.height;
    fs << "horizontal_resolution" << imageSize.width;
    fs << "reprojection_error" << reprojectionError;
    fs.release();
}
//-------------------------------------------------------------------------------------------------
void CameraCalibration::exportCameraParametersJSON(const std::string& filePath) const
{
    nlohmann::json camJson;
    camJson["fx"] = calibrationMatrix.at<double>(0, 0);
    camJson["fy"] = calibrationMatrix.at<double>(1, 1);
    camJson["cx"] = calibrationMatrix.at<double>(0, 2);
    camJson["cy"] = calibrationMatrix.at<double>(1, 2);
    camJson["horizontal_resolution"] = imageSize.width;
    camJson["vertical_resolution"] = imageSize.height;

    camJson["distortion_coefficients"] = nlohmann::json::array();
    for (size_t i = 0; i < getNumDistortionCoefficents(); ++i)
        camJson["distortion_coefficients"].push_back(distortionCoefficients.at<double>(i, 0));

    camJson["reprojection_error"] = reprojectionError;

    std::ofstream outStream(filePath);
    outStream << std::setw(4) << camJson << std::endl;
}
//-------------------------------------------------------------------------------------------------
void CameraCalibration::loadCameraParametersJSON(const std::string& filePath)
{
    std::ifstream inFileStram(filePath);
    nlohmann::json camJson;
    inFileStram >> camJson;

    calibrationMatrix = cv::Mat::eye(3, 3, CV_64F);
    calibrationMatrix.at<double>(0, 0) = camJson.at("fx").get<double>();
    calibrationMatrix.at<double>(1, 1) = camJson.at("fy").get<double>();
    calibrationMatrix.at<double>(0, 2) = camJson.at("cx").get<double>();
    calibrationMatrix.at<double>(1, 2) = camJson.at("cy").get<double>();

    imageSize.width = camJson.at("horizontal_resolution").get<int>();
    imageSize.height = camJson.at("vertical_resolution").get<int>();

    const size_t rows = camJson["distortion_coefficients"].size();
    distortionCoefficients = cv::Mat::zeros(rows, 1, CV_64F);
    for (size_t i = 0; i < rows; ++i)
        distortionCoefficients.at<double>(i, 0) = camJson["distortion_coefficients"][i];

    reprojectionError = camJson.at("reprojection_error").get<double>();
    calibDataAvailabel = true;
}
//-------------------------------------------------------------------------------------------------
void CameraCalibration::loadCameraParametersXML(const std::string& filePath)
{
    cv::FileStorage fs(filePath, cv::FileStorage::READ); // Read the settings
    if (!fs.isOpened())
        throw std::runtime_error("Could not open the configuration file: \"" + filePath + "\"");

    calibrationMatrix = cv::Mat::eye(3, 3, CV_64FC1);
    fs["fx"] >> calibrationMatrix.at<double>(0, 0);
    fs["fy"] >> calibrationMatrix.at<double>(1, 1);
    fs["cx"] >> calibrationMatrix.at<double>(0, 2);
    fs["cy"] >> calibrationMatrix.at<double>(1, 2);

    fs["distortion_coefficients"] >> distortionCoefficients;
    fs["vertical_resolution"] >> imageSize.height;
    fs["horizontal_resolution"] >> imageSize.width;
    fs["reprojection_error"] >> reprojectionError;

    calibDataAvailabel = true;

    fs.release();
}
//-------------------------------------------------------------------------------------------------
double CameraCalibration::computeReprojectionError()
{
    assert(patternCorners.size() == rotationVector.size());
    assert(patternCorners.size() == translationVector.size());
    assert(patternCorners.size() == imgCorners.size());

    size_t idx = 0;
    int totalPoints = 0;
    double totalErr = 0;
    for (size_t i = 0; i < calibImages.size(); i++)
    {
        if (!calibImages[i].patternFound)
            continue;

        std::vector<cv::Point2f> projectedPoints;
        cv::projectPoints(cv::Mat(patternCorners[idx]), rotationVector[idx], translationVector[idx],
            calibrationMatrix, distortionCoefficients, projectedPoints);

        double error = 0;
        for (size_t j = 0; j < projectedPoints.size(); ++j)
        {
            const double x = projectedPoints[j].x - imgCorners[idx][j].x;
            const double y = projectedPoints[j].y - imgCorners[idx][j].y;

            error += sqrt(x * x + y * y);
        }

        totalErr += error;

        calibImages[i].reprojectionError = error / patternCorners[idx].size();
        totalPoints += (int)patternCorners[idx].size();

        idx++;
    }

    if (totalPoints == 0)
        return 0.0f;

    return totalErr / totalPoints;
}
//-------------------------------------------------------------------------------------------------
double CameraCalibration::computeDistortUndistortError()
{
    std::vector<cv::Point3d> points_Iu;
    for (double i = -2; i <= 2; i += 0.1)
        for (double j = -2; j <= 2; j += 0.1)
            points_Iu.emplace_back(i, j, 1);

    cv::Mat rRod, tVec;
    tVec = cv::Mat::zeros(3, 1, CV_64F);
    cv::Rodrigues(cv::Mat::eye(3, 3, CV_64F), rRod);

    std::vector<cv::Point2d> points_P;
    cv::projectPoints(points_Iu, rRod, tVec, calibrationMatrix, distortionCoefficients, points_P);

    std::vector<cv::Point2d> points2_Iu;
    cv::undistort(points_P, points2_Iu, calibrationMatrix, distortionCoefficients);

    double error = 0;
    for (size_t i = 0; i < points_Iu.size(); ++i)
    {
        const double x = points_Iu[i].x - points2_Iu[i].x;
        const double y = points_Iu[i].y - points2_Iu[i].y;
        error += sqrt(x * x + y * y);
    }

    error /= points_Iu.size();
    return error;
}
//-------------------------------------------------------------------------------------------------
const cv::Mat& CameraCalibration::getCameraMatrix() const
{
    return calibrationMatrix;
}
//-------------------------------------------------------------------------------------------------
const cv::Mat& CameraCalibration::getDistCoeffs() const
{
    return distortionCoefficients;
}
//-------------------------------------------------------------------------------------------------
size_t CameraCalibration::getNumDistortionCoefficents() const
{
    if (calibrationFlags & cv::CALIB_THIN_PRISM_MODEL)
        return 12;
    else if (calibrationFlags & cv::CALIB_RATIONAL_MODEL)
        return 8;
    else
        return 5;
}
//-------------------------------------------------------------------------------------------------
std::vector<std::string> CameraCalibration::getFiles() const
{
    std::vector<std::string> files(calibImages.size());
    for (size_t i = 0; i < calibImages.size(); ++i)
        files[i] = calibImages[i].filePath;

    return files;
}
//-------------------------------------------------------------------------------------------------
void CameraCalibration::setFiles(const std::vector<std::string>& files)
{
    calibImages.resize(files.size());
    for (size_t i = 0; i < calibImages.size(); ++i)
    {
        calibImages[i].filePath = files[i];
        calibImages[i].patternFound = false;
    }
}
//-------------------------------------------------------------------------------------------------
void CameraCalibration::addFile(const std::string& file)
{
    CalibImgInfo imgInfo;
    imgInfo.patternFound = false;
    imgInfo.filePath = file;
    calibImages.push_back(std::move(imgInfo));
}
//-------------------------------------------------------------------------------------------------
void CameraCalibration::stopCalibration()
{
    stopRequested = true;
}
//-------------------------------------------------------------------------------------------------
void CameraCalibration::removeFile(const int index)
{
    calibImages.erase(calibImages.begin() + index);
}
//-------------------------------------------------------------------------------------------------
void CameraCalibration::clearFiles()
{
    calibImages.clear();
}
//-------------------------------------------------------------------------------------------------
void CameraCalibration::setChessboardSize(const cv::Size2i& chessboardSize)
{
    this->chessboardCorners = chessboardSize;
}
//-------------------------------------------------------------------------------------------------
void CameraCalibration::setCornerRefinmentWindowSize(const cv::Size2i& cornerRefinmentWindowSize)
{
    this->cornerRefinmentWindowSize = cornerRefinmentWindowSize;
}
//-------------------------------------------------------------------------------------------------
void CameraCalibration::setChessboardSquareWidth(const float chessboardSquareWidth)
{
    this->chessboardSquareWidth = chessboardSquareWidth;
}
//-------------------------------------------------------------------------------------------------
bool CameraCalibration::isStopRequested() const
{
    return stopRequested;
}
//-------------------------------------------------------------------------------------------------
float CameraCalibration::getReprojectionError() const
{
    return reprojectionError;
}
//-------------------------------------------------------------------------------------------------
const std::vector<CameraCalibration::CalibImgInfo>& CameraCalibration::getCalibInfo() const
{
    return calibImages;
}
//-------------------------------------------------------------------------------------------------
bool CameraCalibration::isCalibrationDataAvailable() const
{
    return this->calibDataAvailabel;
}
//-------------------------------------------------------------------------------------------------
cv::Size2i CameraCalibration::getChessboardSize() const
{
    return chessboardCorners;
}
//-------------------------------------------------------------------------------------------------
float CameraCalibration::getChessboardSquareWidth() const
{
    return chessboardSquareWidth;
}
//-------------------------------------------------------------------------------------------------
void CameraCalibration::setCalibrationFlags(const int calibrationFlags)
{
    this->calibrationFlags = calibrationFlags;
}
} // namespace libba
