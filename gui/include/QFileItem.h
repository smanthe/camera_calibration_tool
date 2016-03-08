/*
 * QFileItem.h
 *
 *  Created on: 05.03.2014
 *      Author: Stephan Manthe
 */

#ifndef QFILEITEM_H_
#define QFILEITEM_H_

#include <QStandardItem>
#include <QFileInfo>


class QFileItem: public QStandardItem
{
public:
	QFileItem(const QFileInfo &fi);
	virtual ~QFileItem();

	const QFileInfo& getInfo() const;
	void setInfo(const QFileInfo& info);
protected:
	QFileInfo fInfo;
};

#endif /* QFILEITEM_H_ */
