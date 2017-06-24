/*
 * utils.h
 *
 *  Created on: 28.01.2014
 *      Author: Stephan Manthe
 */

#ifndef UTILS_H_
#define UTILS_H_

#include <opencv2/opencv.hpp>
#include <regex>
#include <vector>

namespace libba
{
/**
 * Returns all filepaths from a specified directory.
 */
std::vector<std::string> readFilesFromDir(
    const std::string& dirPath, const std::regex& extensionFilter);

/**
 * Converts a cv::Mat to html code which can be displayed in a QLabel or a webpage.
 */
std::string matrixToHTML(
    const cv::Mat matrix, const std::string& tableStyle = "", const int precision = 2);

}

#endif /* UTILS_H_ */
