/*
 * utils.h
 *
 *  Created on: 28.01.2014
 *      Author: Stephan Manthe
 */

#ifndef UTILS_H_
#define UTILS_H_

#include <boost/property_tree/ptree.hpp>
#include <opencv2/opencv.hpp>
#include <regex>
#include <vector>

namespace libba
{
/**
 * Returns all filepaths from a specified directory.
 */
std::vector<std::string> readFilesFromDir(const std::string& dirPath,
                                          const std::regex& extensionFilter);

/**
 * Converts a cv::Mat to html code which can be displayed in a QLabel or a webpage.
 */
std::string matrixToHTML(const cv::Mat matrix, const std::string& tableStyle = "",
                         int precision = 2);

/**
 * Converts an OpenCV matrix into an property tree
 */
template <typename T> boost::property_tree::ptree matrix2PropertyTreeCv(const cv::Mat& matrix)
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
}

#endif /* UTILS_H_ */
