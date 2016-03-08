/*
 * ProgressState.cpp
 *
 *  Created on: 26.02.2014
 *      Author: Stephan Manthe
 */

#include <QProgressState.h>
#include <QProgressBar>
#include <cassert>


QProgressState::QProgressState(QProgressBar* pBar, QObject* parent):
	QObject(parent),
	progBar(pBar)
{
	assert(progBar != 0);
	connect(this, SIGNAL(setValue(int)), progBar, SLOT(setValue(int )));
	connect(this, SIGNAL(setRange(int,int )), progBar, SLOT(setRange(int, int )));
}

QProgressState::~QProgressState()
{
}

void QProgressState::emitSignals(int currentStep, int maxSteps, std::string fileName)
{
	emit setRange(0, maxSteps);
	emit setValue(currentStep);
}
