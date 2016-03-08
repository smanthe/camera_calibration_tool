/**
 * CalibrationWidget.h
 *
 *  Created on: 26.01.2014
 *      Author: Stephan Manthe
 */

#ifndef CALIBRATIONWIDGET_H_
#define CALIBRATIONWIDGET_H_

#include <QFuture>
#include <QWidget>
#include "CameraCalibration.h"
class QMessageBox;
class QImageModel;
class QProgressState;
class QGraphicsItem;

namespace Ui
{
    class CalibrationWidget;
}

class CalibrationWidget: public QWidget
{
	Q_OBJECT
public:
	CalibrationWidget(QWidget* parent = 0);
	virtual ~CalibrationWidget();

public slots:
	void on_pushButton_kalibrieren_clicked();
	void on_pushButton_kalibrierdatenLaden_clicked();
	void on_pushButton_loeschen_clicked();
	void on_pushButton_hinzufuegen_clicked();
	void on_pushButton_ordnerHinzufuegen_clicked();
	void on_comboBox_ansicht_currentIndexChanged(int index);

	void showImage(const QModelIndex &index);
	void updateResults(bool success = true, const QString& errorMsg = "");
	void stopCalibration();

signals:
	void calibrationDone(bool success = true, const QString& errorMsg = "");

protected:
	Ui::CalibrationWidget *widget;
	QMessageBox *errorDialog;

	QImageModel *imgModel;
	QGraphicsItem *currentImage;

	QProgressState* calibrationState;
	
    libba::CameraCalibration calibTool;
    QFuture<void> calibrationFuture;

	bool calibrationRunning;

	void startCalibration();
	void doCalibration(const QString& filePath,
                       const std::vector<int>& filePathModelIndices);

	void connectSignalsAndSlots();

	void setupUi();

	void closeEvent(QCloseEvent *event);

    void enableButtons();
    void disableButtons();
};

#endif /* CALIBRATIONWIDGET_H_ */
