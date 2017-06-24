/*
 * utils.cpp
 *
 *  Created on: 28.01.2014
 *      Author: Stephan Manthe
 */

#include "utils.h"
#include <boost/filesystem.hpp>
#include <opencv2/opencv.hpp>
#include <sstream>

namespace libba
{
std::vector<std::string> readFilesFromDir(
    const std::string& dirPath, const std::regex& extensionFilter)
{
    namespace fs = boost::filesystem;
    fs::path someDir(dirPath);
    fs::directory_iterator end_iter;
    std::vector<std::string> files;

    if (fs::exists(someDir) && fs::is_directory(someDir))
    {
        for (fs::directory_iterator dir_iter(someDir); dir_iter != end_iter; ++dir_iter)
        {
            if (!fs::is_regular_file(dir_iter->status()))
                continue;

            try
            {
                if (!std::regex_match(dir_iter->path().string(), extensionFilter))
                    continue;
            }
            catch (std::regex_error& e)
            {
                std::cerr << e.what() << std::endl;
                continue;
            }

            files.push_back(dir_iter->path().string());
        }
    }
    return files;
}
//------------------------------------------------------------------------------------------------
std::string matrixToHTML(const cv::Mat matrix, const std::string& tableStyle, const int precision)
{
    if (matrix.empty())
        return "";

    std::stringstream stream;
    stream.precision(precision);
    stream << "<table " << tableStyle << " >" << std::endl;
    int type = matrix.type();

    for (int i = 0; i < matrix.rows; ++i)
    {
        stream << "<tr>" << std::endl;

        for (int j = 0; j < matrix.cols; ++j)
        {
            stream << "<td>" << std::endl;

            switch (type)
            {
            case CV_32FC1:
                stream << matrix.at<float>(i, j);
                break;
            case CV_64FC1:
                stream << matrix.at<double>(i, j);
                break;
            default:
                throw std::runtime_error("Unhandled type in function matrixToHTML");
                break;
            }
            stream << "</td>";
        }
        stream << "</tr>";
    }
    stream << "</table>";
    return stream.str();
}
}
