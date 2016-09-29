/*
 * CameraCalibration.cpp
 *
 *  Created on: 15.01.2014
 *      Author: Stephan Manthe
 */

#include "CameraCalibration.h"
#include <stdexcept>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

namespace libba
{

CameraCalibration::CameraCalibration():
	chessboardCorners(8,6),
    cornerRefinmentWindowSize(10, 10),
	chessboardSquareWidth(0.0068),
	stopRequested(false),
	reprojectionError(0),
	calibDataAvailabel(false)
{
}

CameraCalibration::~CameraCalibration()
{
}

void CameraCalibration::calibrateCamera(std::function<void(int, int, std::string)> statusFunc)
{
	stopRequested = false;
	calibDataAvailabel = false;

	imgCorners.clear();
	patternCorners.clear();
	rotationVector.clear();
	translationVector.clear();

  // calculate corners from the calibration pattern
	std::vector<cv::Point3f> chessboardCorners3d;
	for( int i = 0; i < chessboardCorners.height; ++i)
		for( int j = 0; j < chessboardCorners.width; ++j)
			chessboardCorners3d.emplace_back(float( j*chessboardSquareWidth ), float( i*chessboardSquareWidth ), 0);
            
	if(calibImages.size() == 0)
		throw std::runtime_error("No images for calibration provided.");

	int maxNumberSteps = calibImages.size() + 1;
	int currentStep = 0;

    imageSize.width = -1;
    imageSize.height = -1;
	cv::Mat img;
	for (size_t i = 0; i < calibImages.size(); ++i)
	{
		calibImages[i].reprojectionError = 0;
		calibImages[i].patternFound = false;
		
        currentStep++;
		img = cv::imread(calibImages[i].filePath, CV_LOAD_IMAGE_GRAYSCALE);

        if(imageSize.width == -1)
        {
            imageSize.width = img.cols;
            imageSize.height = img.rows;
        }

		if( img.cols != imageSize.width ||
			img.rows != imageSize.height)
        {
			std::string errorMsg = "This image had the wrong size for the calibration: "+ calibImages[i].filePath;
			errorMsg += " expected: " + std::to_string(imageSize.width)+ "x"+ std::to_string(imageSize.height);
			errorMsg += " img: " + std::to_string(img.cols)+ "x"+ std::to_string(img.rows);
			std::cerr << errorMsg << std::endl;
			throw std::runtime_error(errorMsg);
		}

		std::vector<cv::Point2f> cornersTemp;
		bool patternFound = cv::findChessboardCorners(img, chessboardCorners, cornersTemp, CV_CALIB_CB_ADAPTIVE_THRESH | CV_CALIB_CB_FILTER_QUADS);

		if(stopRequested)
			return;

		if(!patternFound)
		{
			calibImages[i].patternFound = false;
			statusFunc(currentStep, maxNumberSteps, calibImages[i].filePath);
			continue;
		}
        
		if (cornerRefinmentWindowSize.width > 0 &&
            cornerRefinmentWindowSize.height > 0)
        {
            // TODO make this a option for the gui
            if (true)
            {
                cv::cornerSubPix(img, cornersTemp, cornerRefinmentWindowSize, cv::Size(-1, -1), cv::TermCriteria(CV_TERMCRIT_EPS + CV_TERMCRIT_ITER, 30, 0.1));
            }
            else
            {
                try
                {
                    cv::find4QuadCornerSubpix(img, cornersTemp, cornerRefinmentWindowSize);
                }
                catch (cv::Exception& e)
                {
                    calibImages[i].patternFound = false;
                    statusFunc(currentStep, maxNumberSteps, calibImages[i].filePath);
                    
                    std::cout << "OpenCV exception for image " << i
                              << " in find4QuadCornerSubpix : " << e.what() << std::endl;
                    continue;
                }
            }
        }

		calibImages[i].patternFound = true;
		calibImages[i].boardCornersImg = cornersTemp;
 
		if (stopRequested)
			return;

		imgCorners.push_back(std::move(cornersTemp));
		patternCorners.push_back(chessboardCorners3d);

		statusFunc(currentStep, maxNumberSteps, calibImages[i].filePath);
	}

	try
	{
		calibrationMatrix = cv::Mat::eye(3, 3, CV_64F);
		distortionCoefficients = cv::Mat::zeros(12, 1, CV_64F);
        // TODO Show this option at the gui 
        //const int flags = CV_CALIB_RATIONAL_MODEL;
		//cv::calibrateCamera(patternCorners, imgCorners, img.size(), calibrationMatrix, distortionCoefficients, rotationVector, translationVector, flags);
		cv::calibrateCamera(patternCorners, imgCorners, img.size(), calibrationMatrix, distortionCoefficients, rotationVector, translationVector);
		currentStep++;
		statusFunc(currentStep, maxNumberSteps, "");
	}
	catch(const cv::Exception &ex)
	{
		std::cout << "ex.what() = " << ex.what() << std::endl;
	}

	reprojectionError = computeReprojectionError();
    //computeDistortUndistortError(); // TODO Display this error inside of the gui
	calibDataAvailabel = true;
}

void CameraCalibration::saveCameraParameter(const std::string& filePath) const
{
    exportCameraParameterCv(filePath);
}

void CameraCalibration::exportCameraParameterCv(const std::string &filePath) const
{
    cv::FileStorage fs(filePath, cv::FileStorage::WRITE); // Read the settings
	if (!fs.isOpened())
	{
		std::cerr<< "Could not open the configuration file: \"" << filePath << "\"" << std::endl;
		return;
	}
    
	fs << "fx" << calibrationMatrix.at<double>(0,0);
	fs << "fy" << calibrationMatrix.at<double>(1,1);
	fs << "cx" << calibrationMatrix.at<double>(0,2);
	fs << "cy" << calibrationMatrix.at<double>(1,2);
	fs << "distortion_coefficients" << distortionCoefficients;
	fs << "vertical_resolution" << imageSize.height;
	fs << "horizontal_resolution" << imageSize.width;
	fs << "reprojection_error" << reprojectionError;

	fs.release();
}

void CameraCalibration::exportCameraParameterJSON(const std::string &filePath) const
{
    namespace pt = boost::property_tree;
    pt::ptree root;
    root.put("fx", calibrationMatrix.at<double>(0,0));
	root.put("fy", calibrationMatrix.at<double>(1,1));
	root.put("cx", calibrationMatrix.at<double>(0,2));
	root.put("cy", calibrationMatrix.at<double>(1,2));
    pt::ptree distortionCoefficientsPt = matrix2PropertyTreeCv<double>(distortionCoefficients);
	root.add_child("distortion_coefficients", distortionCoefficientsPt);
	root.put("vertical_resolution", imageSize.height);
	root.put("horizontal_resolution", imageSize.width);
	root.put("reprojection_error", reprojectionError);
    pt::write_json(filePath, root);
}
void CameraCalibration::loadCameraParameterJSON(const std::string& filePath)
{
    namespace pt = boost::property_tree;
    calibrationMatrix = cv::Mat::eye(3, 3, CV_64F);

    pt::ptree ptree;
    pt::read_json(filePath, ptree);

    calibrationMatrix.at<double>(0, 0) = ptree.get<double>("fx");
    calibrationMatrix.at<double>(1, 1) = ptree.get<double>("fy");
    calibrationMatrix.at<double>(0, 2) = ptree.get<double>("cx");
    calibrationMatrix.at<double>(1, 2) = ptree.get<double>("cy");

    const pt::ptree distPtree = ptree.get_child("distortion_coefficients");
    const pt::ptree coefficentsPtree = distPtree.get_child("coefficents");

    const int rows = distPtree.get<int>("rows");
    distortionCoefficients = cv::Mat::zeros(rows, 1, CV_64F);
    int i = 0;
    for (const auto& c : coefficentsPtree)
    {
        distortionCoefficients.at<double>(i, 0) = c.second.get_value<double>();
        i++;
    }

    imageSize.width = ptree.get<int>("horizontal_resolution");
    imageSize.height = ptree.get<int>("vertical_resolution");
}

void CameraCalibration::exportCameraParameterROS(const std::string &filePath) const
{
}

void CameraCalibration::loadCameraParameter(const std::string& filePath)
{
	cv::FileStorage fs(filePath, cv::FileStorage::READ); // Read the settings
	if(!fs.isOpened())
	{
		std::cerr<< "Could not open the configuration file: \"" << filePath << "\"" << std::endl;
		return;
	}

    calibrationMatrix = cv::Mat::eye(3,3, CV_64FC1);
	fs["fx"] >> calibrationMatrix.at<double>(0,0);
	fs["fy"] >> calibrationMatrix.at<double>(1,1);
	fs["cx"] >> calibrationMatrix.at<double>(0,2);
	fs["cy"] >> calibrationMatrix.at<double>(1,2);

	fs["distortion_coefficients"] >> distortionCoefficients;
	fs["vertical_resolution"] >> imageSize.height;
	fs["horizontal_resolution"] >> imageSize.width;
	fs["reprojection_error"] >> reprojectionError;
    
    this->calibDataAvailabel = true;

	fs.release();
}

double CameraCalibration::computeReprojectionError()
{
	assert(patternCorners.size() == rotationVector.size());
	assert(patternCorners.size() == translationVector.size());
	assert(patternCorners.size() == imgCorners.size());

	size_t idx = 0;
    int totalPoints = 0;
	double totalErr = 0;
	for(size_t i = 0; i < calibImages.size(); i++)
	{
		if(!calibImages[i].patternFound)
			continue;

	    std::vector<cv::Point2f> projectedPoints;
        cv::projectPoints(cv::Mat(patternCorners[idx]), rotationVector[idx], translationVector[idx], calibrationMatrix, distortionCoefficients, projectedPoints);

        double error = 0;
        for (size_t j = 0; j < projectedPoints.size(); ++j)
        {
            const double x = projectedPoints[j].x - imgCorners[idx][j].x;
            const double y = projectedPoints[j].y - imgCorners[idx][j].y;

            error += sqrt(x*x + y*y);
        }

		totalErr += error;

		calibImages[i].reprojectionError = error/patternCorners[idx].size();
		totalPoints += (int)patternCorners[idx].size();

		idx++;
	}

	if (totalPoints == 0)
		return 0.0f;

	return totalErr/totalPoints;
}

double CameraCalibration::computeDistortUndistortError()
{
    double error = 0;
   
    std::vector<cv::Point3d> points_Iu; 
    for (double i = -2; i <= 2; i += 0.1)
    {
        for (double j = -2; j <= 2; j += 0.1)
        {
            points_Iu.emplace_back(i, j, 1);
        }
    }

    cv::Mat rRod, tVec;
    tVec = cv::Mat::zeros(3, 1,CV_64F);
    cv::Rodrigues(cv::Mat::eye(3,3,CV_64F), rRod);
   
    std::vector<cv::Point2d> points_P;
    cv::projectPoints(points_Iu, rRod, tVec, calibrationMatrix, distortionCoefficients, points_P);
 
    std::vector<cv::Point2d> points2_Iu;
    cv::undistort(points_P, points2_Iu, calibrationMatrix, distortionCoefficients);

    for (size_t i = 0; i < points_Iu.size(); ++i)
    {
        const double x = points_Iu[i].x - points2_Iu[i].x;
        const double y = points_Iu[i].y - points2_Iu[i].y;

        error += sqrt(x * x + y * y);
    }
    
    error /= points_Iu.size();
    return error;
}

const cv::Mat& CameraCalibration::getCameraMatrix() const
{
	return calibrationMatrix;
}

const cv::Mat& CameraCalibration::getDistCoeffs() const
{
	return distortionCoefficients;
}

std::vector<std::string> CameraCalibration::getFiles() const
{
	std::vector<std::string> files(calibImages.size());
	for(size_t  i = 0; i < calibImages.size(); ++i)
		files[i] = calibImages[i].filePath;

	return files;
}

void CameraCalibration::setFiles(const std::vector<std::string>& files)
{
	calibImages.resize(files.size());
	for (size_t i = 0; i < calibImages.size(); ++i)
	{
		calibImages[i].filePath = files[i];
		calibImages[i].patternFound = false;
	}
}

void CameraCalibration::addFile(const std::string& file)
{
	CalibImgInfo imgInfo;
	imgInfo.patternFound = false;
    imgInfo.filePath = file;
    calibImages.push_back(std::move(imgInfo));
}

void CameraCalibration::removeFile(int index)
{
	calibImages.erase(calibImages.begin() + index);
}

void CameraCalibration::clearFiles()
{
	calibImages.clear();
}

void CameraCalibration::setChessboardSize(const cv::Size2i& chessboardSize)
{
	this->chessboardCorners = chessboardSize;
}

void CameraCalibration::setCornerRefinmentWindowSize(const cv::Size2i& cornerRefinmentWindowSize)
{
    this->cornerRefinmentWindowSize = cornerRefinmentWindowSize;
}

void CameraCalibration::setChessboardSquareWidth(float chessboardSquareWidth)
{
	this->chessboardSquareWidth = chessboardSquareWidth;
}

void CameraCalibration::stopCalibration()
{
	stopRequested = true;
}

bool CameraCalibration::isStopRequested() const
{
	return stopRequested;
}

float CameraCalibration::getReprojectionError() const
{
	return reprojectionError;
}

const std::vector<CameraCalibration::CalibImgInfo> &CameraCalibration::getCalibInfo() const
{
	return calibImages;
}

bool CameraCalibration::isCalibrationDataAvailable() const
{
	return this -> calibDataAvailabel;
}

cv::Size2i CameraCalibration::getChessboardSize() const
{
    return this ->chessboardCorners;
}

}
