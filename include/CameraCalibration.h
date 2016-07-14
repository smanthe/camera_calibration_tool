/*
 * CameraCalibration.h
 *
 *  Created on: 15.01.2014
 *      Author: Stephan Manthe
 */

#ifndef CAMERACALIBRATION_H_
#define CAMERACALIBRATION_H_

#include <vector>
#include <string>
#include <functional>
#include <opencv2/opencv.hpp>
#include <boost/property_tree/ptree.hpp>

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
	 */
	void calibrateCamera(std::function<void(int, int, std::string)> statusFunc);

	/**
	 * Stops the calibration.
	 */
	void stopCalibration();

	/**
	 * Stores the calculated camera parameters in opencv filestorage format.
	 */
	void saveCameraParameter(const std::string &filePath) const;

    void exportCameraParameterCv(const std::string &filePath) const;

    void exportCameraParameterJSON(const std::string &filePath) const;

    void exportCameraParameterROS(const std::string &filePath) const;

	/**
	 * Computes the reprojection error of the last camera calibration.
	 */
	float computeReprojectionError();

	/**
	 * Loads camera parameters from a file which was created with cv::Filestorage.
	 */
	void loadCameraParameter(const std::string& filePath);

	const cv::Mat& getCameraMatrix() const;
	const cv::Mat& getDistCoeffs() const;
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

	const std::vector<CalibImgInfo> &getCalibInfo() const;
	bool isCalibrationDataAvailable() const;

    cv::Size getChessboardSize() const;

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

    template <typename T> boost::property_tree::ptree matrix2PropertyTreeCv(const cv::Mat& matrix) const
    {
        namespace pt = boost::property_tree;
        pt::ptree tree;
    
        tree.put("rows", matrix.rows);
        tree.put("cols", matrix.cols);
    
        pt::ptree coefficents;
        for (int i = 0; i < matrix.rows; ++i)
            for (int j = 0; j < matrix.cols; ++j)
            {
                pt::ptree coefficent;
                coefficent.put("", matrix.at<T>(i, j));
                coefficents.push_back(std::make_pair("", coefficent));
            }
    
        tree.add_child("coefficents", coefficents);
        return tree;
    }
};

}

#endif /* CAMERACALIBRATION_H_ */
