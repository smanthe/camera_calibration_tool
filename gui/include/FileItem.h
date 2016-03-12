/*
 * FileItem.h
 *
 *  Created on: 05.03.2014
 *      Author: Stephan Manthe
 */

#ifndef FILEITEM_H_
#define FILEITEM_H_

#include <QStandardItem>
#include <QFileInfo>


class FileItem: public QStandardItem
{
public:
	FileItem(const QFileInfo &fi);
	virtual ~FileItem();

	const QFileInfo& getInfo() const;
	void setInfo(const QFileInfo& info);
protected:
	QFileInfo fInfo;
};

#endif /* FILEITEM_H_ */
