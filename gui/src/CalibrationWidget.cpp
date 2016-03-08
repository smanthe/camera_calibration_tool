/*
 * CalibrationWidget.cpp
 * 
 *
 *  Created on: 26.01.2014
 *      Author: Stephan Manthe
 */
#include <vector>
#include <functional>
#include <regex>

#include "CalibrationWidget.h"
#include <QFileDialog>
#include <QGraphicsPixmapItem>
#include <QWidget>
#include <QProgressState.h>
#include <QMessageBox>
#include "QImageModel.h"
#include <QtCore>
#include <QtConcurrent>

#include "utils.h"

#include "qtOpenCVConversions.h"
#include "ui_CalibrationWidget.h"
 
CalibrationWidget::CalibrationWidget(QWidget* parent):
	QWidget(parent),
	widget(new Ui::CalibrationWidget),
	imgModel(new QImageModel),
	currentImage(0),
	calibrationRunning(false)
{
	setupUi();
	connectSignalsAndSlots();
}

CalibrationWidget::~CalibrationWidget()
{
	if(calibrationRunning)
		stopCalibration();
}

void CalibrationWidget::on_pushButton_kalibrieren_clicked()
{
  // calibration is already running --> stop it
	if(calibrationRunning)
	{
		stopCalibration();
	}
  // calibration is not running --> start it
	else
	{
		startCalibration();
	}
}

void CalibrationWidget::startCalibration()
{
  // Read settings
	bool ok = false;

	int cornersHorizontal = this ->widget->lineEdit_eckenHorizontal->text().toInt(&ok);
	if(!ok)
	{
		errorDialog->setText(trUtf8("Bitte das Eingabefeld für die Anzahl der horizontalen Ecken überprüfen."));
		errorDialog->show();
		return;
	}

	int cornersVertical = this ->widget->lineEdit_eckenVertikal->text().toInt(&ok);
	if(!ok)
	{
		errorDialog->setText(trUtf8("Bitte das Eingabefeld für die Anzahl der vertikalen Ecken überprüfen."));
		errorDialog->show();
		return;
	}

	double squareWidth = this -> widget->lineEdit_quadratGroesse->text().toDouble(&ok);
	if(!ok)
	{
		errorDialog->setText(trUtf8("Bitte das Eingabefeld für die Quadratgröße überprüfen."));
		errorDialog->show();
		return;
	}

	std::vector<QImageModel::ImgData> imageData = imgModel -> getImageData();
	if(imageData.size() <= 0)
	{
		errorDialog->setText(trUtf8("Es muss mindestens eine Datei für die Kalibrierung ausgewählt sein."));
		errorDialog->show();
		return;
	}

	calibTool.clearFiles();
    std::vector<int> filePathModelIndices;
	for (size_t i = 0; i < imageData.size(); ++i)
    {
        imageData[i].found = false;
        imageData[i].error = 0;
        imageData[i].boardCornersImg.clear();
	    
        imgModel->setImageData(i, imageData[i]);

        if(imageData[i].checked)
        {
			calibTool.addFile(imageData[i].filePath);
            filePathModelIndices.push_back(i);
        }
	}

  // Get filepath
	QString filePath = QFileDialog::getSaveFileName(
			this,
			trUtf8("Datei speichern"),
			QDir::homePath() + "/calib.xml",
            trUtf8("XML-Dateien (*.xml)"));

  // @TODO check if filename ends  with .xml
	if(filePath.isEmpty())
	{
		errorDialog->setText(trUtf8("Dateipfad ist leer."));
		errorDialog->show();
		return;
	}

  // set parameters
	calibTool.setChessboardSize(cv::Size2i(cornersHorizontal, cornersVertical));
    calibTool.setChessboardSquareWidth(squareWidth);

	widget->pushButton_kalibrieren->setText(trUtf8("Kalibrierung stoppen"));
    widget->tableView_images->setEditTriggers(QAbstractItemView::NoEditTriggers);
    imgModel->setCheckboxesEnabled(false);
    disableButtons();

  // http://qt-project.org/wiki/QtConcurrent-run-member-function
	calibrationFuture = QtConcurrent::run(this, &CalibrationWidget::doCalibration, filePath, filePathModelIndices);
	calibrationRunning = true;
}

void CalibrationWidget::doCalibration(const QString& filePath,
                                      const std::vector<int>& filePathModelIndices)
{
	using namespace std::placeholders;
	auto f = std::bind(&QProgressState::emitSignals, calibrationState,
                        std::placeholders::_1, std::placeholders::_2,
                        std::placeholders::_3);
	try
	{
		calibTool.calibrateCamera(f);
	}
	catch(std::runtime_error &e)
	{
		std::cerr << "Error:" << e.what() << std::endl;
		emit calibrationDone(false, QString::fromStdString(e.what()));
		return;
	}

	if(calibTool.isStopRequested())
 		return;

	calibTool.saveCameraParameter(filePath.toStdString());
	const std::vector<libba::CameraCalibration::CalibImgInfo> imgs = calibTool.getCalibInfo();

  // update the imagemodel
	for(size_t i = 0; i < imgs.size(); ++i)
	{
        int modelIdx =  filePathModelIndices[i];
        QImageModel::ImgData data = imgModel->getImageData(modelIdx);
        data.found = imgs[i].patternFound;     
		data.error = imgs[i].reprojectionError;
        data.boardCornersImg = imgs[i].boardCornersImg;
        
		imgModel -> setImageData(modelIdx, data);
	}

  // signals must be used here because otherwise calibrationDone() and the gui would run in different threads
	emit calibrationDone();
}

void CalibrationWidget::stopCalibration()
{
	calibTool.stopCalibration();

  // cancel the calibration and wait for thread has finished
	if(calibrationFuture.isRunning())
	{
		QMessageBox::information(this, trUtf8("Kalibrierung stoppen"), trUtf8("Die Kalibrierung wird abgebrochen, dies kann einen Moment dauern.") );
		calibrationFuture.cancel();
		calibrationFuture.waitForFinished();
	}
	else
	{
		QMessageBox::information(this, trUtf8("Kalibrierung abgeschlossen"), trUtf8("Das Kalibrieren der Kamera ist abegschlossen.") );
	}

	calibrationRunning = false;
    imgModel->setCheckboxesEnabled(true);
	widget->pushButton_kalibrieren->setText(trUtf8("Kalibrieren"));

  // reset progressbar
	widget->progressBar->setValue(0);
    enableButtons();
}

void CalibrationWidget::on_pushButton_loeschen_clicked()
{
	QModelIndex i = widget->tableView_images->currentIndex();
	widget->tableView_images->model()->removeRow(i.row(), QModelIndex());
}

void CalibrationWidget::on_pushButton_hinzufuegen_clicked()
{
	QString filePath = QFileDialog::getOpenFileName(this, trUtf8("Datei öffnen"), QDir::homePath(), trUtf8("Images (*.png *.jpg)"));
    
    if(filePath == "")
        return;

	imgModel->addImage(filePath);
}

void CalibrationWidget::on_pushButton_ordnerHinzufuegen_clicked()
{
	QString dirPath = QFileDialog::getExistingDirectory(this, trUtf8("Ordner öffnen"), QDir::homePath(), QFileDialog::ShowDirsOnly);
	const std::regex filter( ".*\\.JPG|.*\\.PNG|.*\\.jpg||.*\\.png", std::regex::icase);
	std::vector<std::string> files = libba::readFilesFromDir(dirPath.toStdString(), filter);

	for (int i = 0; i < files.size(); ++i)
	{
		imgModel->addImage(QString::fromStdString(files[i]));
	}
}

void CalibrationWidget::setupUi()
{
	widget->setupUi(this);
	QIntValidator *qiv = new QIntValidator(0, 999999, this);

    qiv = new QIntValidator(2, 100, this);
	widget->lineEdit_eckenHorizontal->setValidator(qiv);
	widget->lineEdit_eckenVertikal->setValidator(qiv);

	QDoubleValidator *qdv = new QDoubleValidator(0, 9999999999999, 10, this);
	widget->lineEdit_quadratGroesse->setValidator(qdv);

	errorDialog = new QMessageBox(QMessageBox::Warning, trUtf8("Fehler"), trUtf8("Fehler"), QMessageBox::Ok, this);

	widget->graphicsView->setScene(new QGraphicsScene(widget->graphicsView));

	widget->tableView_images->setEditTriggers(QAbstractItemView::NoEditTriggers);
	widget->tableView_images->setSelectionBehavior(QAbstractItemView::SelectRows);
	widget->tableView_images->setModel(imgModel);
	widget->tableView_images->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
	widget->tableView_images->horizontalHeader()->sectionResizeMode(QHeaderView::ResizeToContents);
	widget->tableView_images->resizeColumnsToContents();

	calibrationState = new QProgressState(widget->progressBar);
}

void CalibrationWidget::showImage(const QModelIndex& index)
{
	QGraphicsScene* scene = widget->graphicsView->scene();

  // delete all items in the scene (currentImage)
	scene->clear();
	QString filePath = QString::fromStdString(imgModel->getImageData(index.row()).filePath);

	if(!QFile::exists(filePath))
	{
		errorDialog->setText(trUtf8("Das Angeforderte Bild existiert nicht mehr im Dateisystem: ") +filePath);
		errorDialog->show();
		return;
	}

	switch(widget->comboBox_ansicht->currentIndex())
	{
		case 0:
		{
			QImage img(filePath);

			if(img.isNull())
			{
				errorDialog->setText(trUtf8("Das Bild konnte für die Vorschau nicht geöffnet werden."));
				errorDialog->show();
				return;
			}

			currentImage = new QGraphicsPixmapItem(QPixmap::fromImage(img));
			break;
		}
		case 1:
		{
			if(!calibTool.isCalibrationDataAvailable())
			{
				errorDialog->setText(trUtf8("Für diese Funktion müssen erst Kameraparameter berechnet oder geladen werden."));
				errorDialog->show();
				return;
			}

            cv::Mat cvImg = cv::imread(filePath.toStdString(), CV_LOAD_IMAGE_COLOR);
            cv::Mat imgUndist;
            cv::undistort(cvImg, imgUndist, calibTool.getCameraMatrix(), calibTool.getDistCoeffs());

            currentImage = new QGraphicsPixmapItem(qtOpenCvConversions::cvMatToQPixmap(imgUndist));
			break;
		}
		case 2:
		{
			if(!calibTool.isCalibrationDataAvailable())
			{
				errorDialog->setText(trUtf8("Für diese Funktion müssen erst Kameraparameter geladen werden."));
				errorDialog->show();
				return;
			}
        
            try {
                if(!imgModel->getImageData(index.row()).found)
                {
                    currentImage = new QGraphicsPixmapItem(0);
                }
               else
               {
                    cv::Mat cvImg = cv::imread(filePath.toStdString(), CV_LOAD_IMAGE_COLOR);
                    cv::drawChessboardCorners(cvImg, calibTool.getChessboardSize(), imgModel->getImageData(index.row()).boardCornersImg ,true);
                    currentImage = new QGraphicsPixmapItem(qtOpenCvConversions::cvMatToQPixmap(cvImg));
               }
            }
            catch(const std::out_of_range& e)
            {
                std::cerr << trUtf8("out_of_range exception. Dieses Bild existiert nicht mehr").toStdString() << std::endl;
                currentImage = new QGraphicsPixmapItem(0);
            }
            break;
		}
	}

	widget->graphicsView->fitInView(currentImage, Qt::KeepAspectRatio);
	scene->addItem(currentImage);
}

void CalibrationWidget::updateResults(bool success, const QString& errorMsg)
{
	if(!success)
	{
		errorDialog -> setText(trUtf8("Es ist ein Fehler bei der Kalibrierung aufgetreten.\nFehler:") + errorMsg);
		errorDialog -> show();
		return;
	}

	int precision = 5;

	std::string tableStyle = "cellpadding=\"2\"";
	std::string tableHTML = libba::matrixToHTML(calibTool.getCameraMatrix(), tableStyle, precision);

	widget->label_cameraMatrix->setText(QString::fromStdString(tableHTML));

	cv::Mat transpDistCoefs = calibTool.getDistCoeffs().clone();
	cv::transpose(transpDistCoefs, transpDistCoefs);
	tableHTML = libba::matrixToHTML(transpDistCoefs, tableStyle, precision);
	widget->label_distoritionCoefficents->setText( QString::fromStdString(tableHTML) );

	widget->label_reprojectionError->setText( QString::number(calibTool.getReprojectionError(), 'g', 4) );
}

void CalibrationWidget::on_pushButton_kalibrierdatenLaden_clicked()
{
	QString filePath = QFileDialog::getOpenFileName(this, trUtf8("Datei öffnen"), QDir::homePath(), trUtf8("XML (*.xml)"));
	calibTool.loadCameraParameter(filePath.toStdString());
	updateResults();
}

void CalibrationWidget::on_comboBox_ansicht_currentIndexChanged(int index)
{
	const QModelIndex i = widget->tableView_images->currentIndex();
	if (i.isValid())
		showImage(i);
}

void CalibrationWidget::connectSignalsAndSlots()
{
	connect(widget->tableView_images, SIGNAL( pressed(const QModelIndex &)), this, SLOT( showImage(const QModelIndex &)));
	connect(this, SIGNAL(calibrationDone(bool, QString)), this, SLOT(updateResults(bool, QString)) );
	connect(this, SIGNAL(calibrationDone(bool)), this, SLOT(stopCalibration()) );
    connect(imgModel, SIGNAL(rowsRemoved(const QModelIndex &, int, int)), this -> imgModel, SLOT(rowsRemoved(const QModelIndex &, int, int)));
}

void CalibrationWidget::closeEvent(QCloseEvent* event)
{
	if(calibrationRunning)
		stopCalibration();
}

void CalibrationWidget::enableButtons()
{ 
    this ->widget->pushButton_loeschen ->setDisabled(false);
    this ->widget->pushButton_ordnerHinzufuegen->setDisabled(false);
    this ->widget->pushButton_hinzufuegen->setDisabled(false);
    this ->widget->pushButton_kalibrierdatenLaden->setDisabled(false);
}

void CalibrationWidget::disableButtons()
{
    this ->widget->pushButton_loeschen ->setDisabled(true);
    this ->widget->pushButton_ordnerHinzufuegen->setDisabled(true);
    this ->widget->pushButton_hinzufuegen->setDisabled(true);
    this ->widget->pushButton_kalibrierdatenLaden->setDisabled(true);
}
