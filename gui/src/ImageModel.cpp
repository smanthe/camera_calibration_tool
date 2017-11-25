/*
 * ImageModel.cpp
 *
 *  Created on: 28.01.2014
 *      Author: Stephan Manthe
 */
#include <QFileInfo>

#include "CameraCalibration.h"
#include "ImageModel.h"

ImageModel::ImageModel(QObject* parent)
    : QStandardItemModel(parent)
{
    initHeader();
    connect(this, SIGNAL(itemChanged(QStandardItem*)), this, SLOT(changeItem(QStandardItem*)));
}

ImageModel::~ImageModel()
{
}

void ImageModel::changeItem(QStandardItem* item)
{
    const int row = item->row();
    const int col = item->column();
    
    if (col == 0)
        imageData.at(row).checked = item->checkState() == Qt::Checked;
}

void ImageModel::addImage(QString imgPath)
{
    ImgData imgData;
    imgData.checked = true;
    imgData.found = false;
    imgData.filePath = imgPath.toStdString();
    imgData.error = 0;
    imageData.push_back(imgData);

    setRowCount(rowCount() + 1);
    QStandardItem* item = new QStandardItem(false);
    item->setCheckable(true);
    item->setCheckState(Qt::Checked);

    setItem(rowCount() - 1, 0, item);
    setItem(rowCount() - 1, 1, new QStandardItem(tr("Nein")));

    item = new QStandardItem(QString::fromStdString(imgData.filePath));
    item->setData(QVariant(imgPath));

    setItem(rowCount() - 1, 2, item);
    setItem(rowCount() - 1, 3, new QStandardItem("0"));
}

void ImageModel::initHeader()
{
    QStringList header;
    header << trUtf8("Nr.") << trUtf8("Gefunden") << trUtf8("Dateiname") << trUtf8("Fehler");
    setHorizontalHeaderLabels(header);
}

ImageModel::ImgData ImageModel::getImageData(int idx)
{
    return imageData.at(idx);
}

std::vector<ImageModel::ImgData> ImageModel::getImageData()
{
    return imageData;
}

void ImageModel::setImageData(int idx, ImgData data)
{
    imageData.at(idx) = data;
    updateRow(idx, data);
}

void ImageModel::updateRow(int idx, const ImgData& data)
{
    QString foundText = "";
    if (data.found)
        foundText = trUtf8("Ja");
    else
        foundText = trUtf8("Nein");

    setItem(idx, 1, new QStandardItem(foundText));
    item(idx, 3)->setText(QString::fromStdString(std::to_string(data.error)));
}

void ImageModel::setCheckboxesEnabled(bool enabled)
{
    for (int idx = 0; idx < rowCount(); ++idx)
        this->item(idx, 0)->setEnabled(enabled);
}

void ImageModel::rowsRemoved(const QModelIndex& parent, int start, int end)
{
    imageData.erase(imageData.begin() + start);
}
