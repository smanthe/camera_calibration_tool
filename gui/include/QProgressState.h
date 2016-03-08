/*
 * QProgressState.h
 *
 *  Created on: 26.02.2014
 *      Author: Stephan Manthe
 */

#ifndef QPROGRESSSTATE_H_
#define QPROGRESSSTATE_H_

#include <QObject>

class QProgressBar;

class QProgressState:public QObject
{
	Q_OBJECT
public:
	QProgressState();
	virtual ~QProgressState();
	QProgressBar *progBar;

	QProgressState(QProgressBar* pBar, QObject* parent = 0);

	void emitSignals(int currentStep, int maxSteps, std::string fileName);

signals:
		void setValue(int );
		void setRange(int, int );
};

#endif /* QPROGRESSSTATE_H_ */
