/*
 * ProgressState.h
 *
 *  Created on: 26.02.2014
 *      Author: Stephan Manthe
 */

#ifndef PROGRESSSTATE_H_
#define PROGRESSSTATE_H_

#include <QObject>

class QProgressBar;

class ProgressState:public QObject
{
	Q_OBJECT
public:
	ProgressState();
	virtual ~ProgressState();
	QProgressBar *progBar;

	ProgressState(QProgressBar* pBar, QObject* parent = 0);

	void emitSignals(int currentStep, int maxSteps, std::string fileName);

signals:
		void setValue(int );
		void setRange(int, int );
};

#endif /* PROGRESSSTATE_H_ */
