/*
 * ProgressState.cpp
 *
 *  Created on: 26.02.2014
 *      Author: Stephan Manthe
 */

#include <ProgressState.h>
#include <QProgressBar>
#include <cassert>


ProgressState::ProgressState(QProgressBar* pBar, QObject* parent) : QObject(parent), progBar(pBar)
{
    assert(progBar != 0);
    connect(this, SIGNAL(setValue(int)), progBar, SLOT(setValue(int)));
    connect(this, SIGNAL(setRange(int, int)), progBar, SLOT(setRange(int, int)));
}

ProgressState::~ProgressState() {}

void ProgressState::emitSignals(int currentStep, int maxSteps, std::string fileName)
{
    emit setRange(0, maxSteps);
    emit setValue(currentStep);
}
