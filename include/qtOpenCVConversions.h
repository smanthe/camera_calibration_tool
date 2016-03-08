#ifndef QTOPENCVCONVERSIONS_H
#define QTOPENCVCONVERSIONS_H
/*
   Functions to convert between OpenCV's cv::Mat and Qt's QImage and QPixmap.

   Andy Maloney
   23 November 2013
   http://asmaloney.com/2013/11/code/converting-between-cvmat-and-qimage-or-qpixmap
 */

#include <QImage>
#include <QPixmap>

#include <opencv2/imgproc/imgproc.hpp>

namespace qtOpenCvConversions
{
	QImage  cvMatToQImage(const cv::Mat &inMat);
	QPixmap cvMatToQPixmap(const cv::Mat &inMat);
	cv::Mat QImageToCvMat(const QImage &inImage, bool inCloneImageData = true);
	cv::Mat QPixmapToCvMat(const QPixmap &inPixmap, bool inCloneImageData = true);
}

#endif // QTOPENCVCONVERSIONS_H
