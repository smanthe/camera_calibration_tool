/*
 * FileItem.cpp
 *
 *  Created on: 05.03.2014
 *      Author: Stephan Manthe
 */

#include <FileItem.h>

FileItem::FileItem(const QFileInfo &fi):
	QStandardItem(fi.fileName()),
	fInfo(fi)
{
}

FileItem::~FileItem()
{
}

const QFileInfo& FileItem::getInfo() const {
	return fInfo;
}

void FileItem::setInfo(const QFileInfo& info) {
	fInfo = info;
}
