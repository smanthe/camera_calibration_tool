/*
 * QFileItem.cpp
 *
 *  Created on: 05.03.2014
 *      Author: Stephan Manthe
 */

#include <QFileItem.h>

QFileItem::QFileItem(const QFileInfo &fi):
	QStandardItem(fi.fileName()),
	fInfo(fi)
{
}

QFileItem::~QFileItem()
{
}

const QFileInfo& QFileItem::getInfo() const {
	return fInfo;
}

void QFileItem::setInfo(const QFileInfo& info) {
	fInfo = info;
}
