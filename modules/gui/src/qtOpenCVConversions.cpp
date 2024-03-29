/*
   Functions to convert between OpenCV's cv::Mat and Qt's QImage and QPixmap.

   Andy Maloney
   23 November 2013
   http://asmaloney.com/2013/11/code/converting-between-cvmat-and-qimage-or-qpixmap
 */

#include "qtOpenCVConversions.h"
#include <QDebug>
#include <string>

namespace qtOpenCvConversions
{
QImage cvMatToQImage(const cv::Mat& inMat)
{
    switch (inMat.type())
    {
    // 8-bit, 4 channel
    case CV_8UC4:
    {
        QImage image(inMat.data, inMat.cols, inMat.rows, inMat.step, QImage::Format_RGB32);
        return image;
    }

    // 8-bit, 3 channel
    case CV_8UC3:
    {
        QImage image(inMat.data, inMat.cols, inMat.rows, inMat.step, QImage::Format_RGB888);
        return image.rgbSwapped();
    }

    // 8-bit, 1 channel
    case CV_8UC1:
    {
        static QVector<QRgb> sColorTable;

        // only create our color table once
        if (sColorTable.isEmpty())
        {
            for (int i = 0; i < 256; ++i)
                sColorTable.push_back(qRgb(i, i, i));
        }

        QImage image(inMat.data, inMat.cols, inMat.rows, inMat.step, QImage::Format_Indexed8);
        image.setColorTable(sColorTable);

        return image;
    }

    default:
        throw std::runtime_error("ASM::cvMatToQImage() - cv::Mat image type not handled in switch: "
            + std::to_string(inMat.type()));
        break;
    }

    return QImage();
}

QPixmap cvMatToQPixmap(const cv::Mat& inMat)
{
    return QPixmap::fromImage(cvMatToQImage(inMat));
}

// If inImage exists for the lifetime of the resulting cv::Mat, pass false to inCloneImageData to
// share inImage's
// data with the cv::Mat directly
//    NOTE: Format_RGB888 is an exception since we need to use a local QImage and thus must clone
//    the data regardless
cv::Mat QImageToCvMat(const QImage& inImage, const bool inCloneImageData)
{
    switch (inImage.format())
    {
    // 8-bit, 4 channel
    case QImage::Format_RGB32:
    {
        cv::Mat mat(inImage.height(), inImage.width(), CV_8UC4, const_cast<uchar*>(inImage.bits()),
            inImage.bytesPerLine());
        return (inCloneImageData ? mat.clone() : mat);
    }

    // 8-bit, 3 channel
    case QImage::Format_RGB888:
    {
        if (!inCloneImageData)
            throw std::runtime_error("ASM::QImageToCvMat() - Conversion requires cloning since "
                                     "we use a temporary QImage");

        QImage swapped = inImage.rgbSwapped();
        return cv::Mat(swapped.height(), swapped.width(), CV_8UC3,
            const_cast<uchar*>(swapped.bits()), swapped.bytesPerLine())
            .clone();
    }

    // 8-bit, 1 channel
    case QImage::Format_Indexed8:
    {
        cv::Mat mat(inImage.height(), inImage.width(), CV_8UC1, const_cast<uchar*>(inImage.bits()),
            inImage.bytesPerLine());
        return (inCloneImageData ? mat.clone() : mat);
    }

    default:
        throw std::runtime_error("ASM::QImageToCvMat() - QImage format not handled in switch: "
            + std::to_string(inImage.format()));
        break;
    }

    return cv::Mat();
}

// If inPixmap exists for the lifetime of the resulting cv::Mat, pass false to inCloneImageData to
// share inPixmap's data
// with the cv::Mat directly
//    NOTE: Format_RGB888 is an exception since we need to use a local QImage and thus must clone
//    the data regardless
cv::Mat QPixmapToCvMat(const QPixmap& inPixmap, const bool inCloneImageData)
{
    return QImageToCvMat(inPixmap.toImage(), inCloneImageData);
}
} // namespace qtOpenCvConversions
