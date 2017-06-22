/*
 * ResizeableGraphicsView.cpp
 *
 *  Created on: 20.02.2014
 *      Author: Stephan Manthe
 */

#include "ResizeableGraphicsView.h"
ResizeableGraphicsView::ResizeableGraphicsView(QWidget* parent) : QGraphicsView(parent) {}

ResizeableGraphicsView::~ResizeableGraphicsView() {}

void ResizeableGraphicsView::resizeEvent(QResizeEvent* event)
{
    QList<QGraphicsItem*> list = this->items();
    for (int i = 0; i < list.size(); ++i)
    {
        this->fitInView(list[i], Qt::KeepAspectRatio);
    }
}
