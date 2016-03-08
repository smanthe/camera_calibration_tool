/*
 * QImageModel.cpp
 *
 *  Created on: 28.01.2014
 *      Author: Stephan Manthe
 */
#include <iostream>
#include <cassert>
#include <QFileInfo>

#include "QImageModel.h"
#include "CameraCalibration.h"

QImageModel::QImageModel(QObject  *parent):
	QStandardItemModel(parent)
{
	initHeader();
    connect(this, SIGNAL(itemChanged(QStandardItem* )), this, SLOT(changeItem(QStandardItem* )));
}

QImageModel::~QImageModel()
{

}

void QImageModel::changeItem(QStandardItem* item)
{
    int row = item->row();
    int col = item->column();

    if(col == 0)
        imageData.at(row).checked = item->checkState() == Qt::Checked;
}

void QImageModel::addImage(QString imgPath)
{
    ImgData imgData;
    imgData.checked = true;
    imgData.found = false;
    imgData.filePath = imgPath.toStdString();
    imgData.error = 0;
    imageData.push_back(imgData);

	setRowCount(rowCount() + 1);
	QStandardItem* item;
	item = new QStandardItem(false);
	item->setCheckable(true);
	item->setCheckState(Qt::Checked);
	
    setItem(rowCount() - 1,0, item);
	setItem(rowCount() - 1,1, new QStandardItem(tr("Nein")));
    
	QFileInfo info(imgPath);
	item = new QStandardItem(QString::fromStdString(imgData.filePath));
    item ->setData(QVariant( imgPath ));	
	
    setItem(rowCount() - 1,2, item);
	setItem(rowCount() - 1,3, new QStandardItem("0"));
}

void QImageModel::initHeader()
{
	QStringList header;
	header << tr("Nr.") << tr("Gefunden") << tr("Dateiname") << tr("Fehler");
	setHorizontalHeaderLabels(header);
}

QImageModel::ImgData QImageModel::getImageData(int idx)
{
	return imageData.at(idx);
}

std::vector<QImageModel::ImgData> QImageModel::getImageData()
{
	return imageData;
}

void QImageModel::setImageData(int idx, ImgData data)
{  
    imageData.at(idx) = data; 
    updateRow(idx, data);
}

void QImageModel::updateRow(int idx, const ImgData &data)
{
    QString foundText = "";    
    if(data.found)
        foundText = tr("Ja");
	else
        foundText = tr("Nein");

    setItem(idx,1, new QStandardItem(foundText));

	item(idx,3) -> setText(QString::fromStdString(std::to_string(data.error)));
}

void QImageModel::setCheckboxesEnabled(bool enabled)
{
	for (int idx = 0; idx < rowCount(); ++idx)
	{
		QStandardItem* itemTmp = this ->item(idx, 0);
        itemTmp->setEnabled(enabled);
    }
}

void QImageModel::rowsRemoved(const QModelIndex &parent, int start , int end)
{
    imageData.erase(imageData.begin() + start);
}
