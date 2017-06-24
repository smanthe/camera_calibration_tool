/*
 * ImageModel.h
 *
 *  Created on: 28.01.2014
 *      Author: Stephan Manthe
 */

#ifndef IMAGEMODEL_H_
#define IMAGEMODEL_H_

#include <QModelIndex>
#include <QStandardItemModel>
#include <opencv2/opencv.hpp>
#include <string>
#include <vector>

/**
 * This class implements a model which contains all needed data for the calibration widget.
 *
 * See http://programmingexamples.net/wiki/Qt/ModelView/QAbstractTableModel
 * http://paste.cdtag.de/view.php?id=23679
 */
class ImageModel : public QStandardItemModel
{
    Q_OBJECT
public:
    struct ImgData
    {
        bool checked = false;
        bool found = false;
        std::string filePath;
        float error = 0.f;
        int row = -1;
        std::vector<cv::Point2f> boardCornersImg;
    };

    ImageModel(QObject* parent = 0);
    virtual ~ImageModel();

    void addImage(QString imgPath);

    ImgData getImageData(int idx);
    std::vector<ImgData> getImageData();
    void setImageData(int idx, ImgData data);

    void setCheckboxesEnabled(bool enabled);

protected:
    void initHeader();
    void updateRow(int idx, const ImgData& data);

    std::vector<ImgData> imageData;

public slots:
    void rowsRemoved(const QModelIndex& parent, int start, int end);
    void changeItem(QStandardItem* item);
};

Q_DECLARE_METATYPE(ImageModel::ImgData)
#endif /* QIMAGEMODEL_H_ */
