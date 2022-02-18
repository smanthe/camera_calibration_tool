/*
 * main.cpp
 *
 *  Created on: 30.12.2013
 *      Author: Stephan Manthe
 */

#include "CalibrationWidget.h"
#include <QApplication>
#include <QtGui>

int main(int argc, char* argv[])
{
    setenv("LC_ALL", "C", 1);

    QApplication app(argc, argv);
    CalibrationWidget calibWidget;
    calibWidget.show();
    calibWidget.raise();

    return app.exec();
}
