/*
 * QTextWindowWidget.h
 *
 *  Created on: 17.04.2014
 *      Author: Stephan Manthe
 */

#ifndef QTEXTWINDOWWIDGET_H_
#define QTEXTWINDOWWIDGET_H_

#include <QWidget>
#include <ui_textWindow.h>

class QTextWindowWidget: public QWidget
{
public:
	QTextWindowWidget();
	virtual ~QTextWindowWidget();

	void setText(const QString &text);
	void setupUi();

protected:
	Ui::TextWindow *ui;

};

#endif /* QTEXTWINDOWWIDGET_H_ */
