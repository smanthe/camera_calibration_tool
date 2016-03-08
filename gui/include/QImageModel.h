/*
 * QImageModel.h
 *
 *  Created on: 28.01.2014
 *      Author: Stephan Manthe
 */

#ifndef QIMAGEMODEL_H_
#define QIMAGEMODEL_H_

#include <vector>
#include <string>
#include <QStandardItemModel>
#include <QModelIndex>
#include <opencv2/opencv.hpp>

/**
 * This class implements a model which contains all needed data for the calibration widget.
 *
 * See http://programmingexamples.net/wiki/Qt/ModelView/QAbstractTableModel
 * http://paste.cdtag.de/view.php?id=23679
 */
class QImageModel:public QStandardItemModel
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

	QImageModel(QObject  *parent=0);
	virtual ~QImageModel();

	void addImage(QString imgPath);

	ImgData getImageData(int idx);
	std::vector<ImgData> getImageData();
	void setImageData(int idx, ImgData data);

    void setCheckboxesEnabled(bool enabled);
protected:
	void initHeader();
	void updateRow(int idx, const ImgData &data);
    
	std::vector<ImgData> imageData;

public slots:  
    void rowsRemoved(const QModelIndex & parent, int start, int end);
    void changeItem(QStandardItem* item);
};

Q_DECLARE_METATYPE(QImageModel::ImgData)
#endif /* QIMAGEMODEL_H_ */
