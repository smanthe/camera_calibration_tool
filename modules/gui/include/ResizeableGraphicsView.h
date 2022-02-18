/*
 * ResizeableGraphicsView.h
 *
 *  Created on: 20.02.2014
 *      Author: Stephan Manthe
 */

#ifndef RESIZEABLEGRAPHICSVIEW_H_
#define RESIZEABLEGRAPHICSVIEW_H_

#include <QGraphicsView>

class ResizeableGraphicsView : public QGraphicsView
{
    Q_OBJECT
public:
    ResizeableGraphicsView(QWidget* parent = 0);
    virtual ~ResizeableGraphicsView();

protected:
    void resizeEvent(QResizeEvent* event);
};

#endif /* RESIZEABLEGRAPHICSVIEW_H_ */
