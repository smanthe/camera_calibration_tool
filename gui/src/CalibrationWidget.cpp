/*
 * CalibrationWidget.cpp
 *
 *
 *  Created on: 26.01.2014
 *      Author: Stephan Manthe
 */
#include "CalibrationWidget.h"
#include "ImageModel.h"
#include "qtOpenCVConversions.h"
#include "ui_CalibrationWidget.h"
#include "utils.h"
#include <ProgressState.h>
#include <QFileDialog>
#include <QGraphicsPixmapItem>
#include <QMessageBox>
#include <QWidget>
#include <QtConcurrent>
#include <QtCore>
#include <functional>
#include <regex>
#include <vector>

CalibrationWidget::CalibrationWidget(QWidget* parent)
    : QWidget(parent)
    , widget(new Ui::CalibrationWidget)
    , imgModel(new ImageModel)
    , currentImage(0)
    , calibrationRunning(false)
{
    setupUi();
    connectSignalsAndSlots();
}
//------------------------------------------------------------------------------------------------
CalibrationWidget::~CalibrationWidget()
{
    if (calibrationRunning)
        stopCalibration();
}
//------------------------------------------------------------------------------------------------
void CalibrationWidget::on_pushButton_kalibrieren_clicked()
{
    if (calibrationRunning) // calibration is already running --> stop it
        stopCalibration();
    else // calibration is not running --> start it
        startCalibration();
}
//------------------------------------------------------------------------------------------------
void CalibrationWidget::startCalibration()
{
    // Read settings
    bool ok = false;

    const int cornersHorizontal = this->widget->lineEdit_eckenHorizontal->text().toInt(&ok);
    if (!ok)
    {
        showError(
            trUtf8("Bitte das Eingabefeld für die Anzahl der horizontalen Ecken überprüfen. Es "
                   "muss eine positive ganze Zahl eingegeben werden."));
        errorDialog->show();
        return;
    }

    const int cornersVertical = this->widget->lineEdit_eckenVertikal->text().toInt(&ok);
    if (!ok)
    {
        errorDialog->setText(
            trUtf8("Bitte das Eingabefeld für die Anzahl der vertikalen Ecken überprüfen. Es muss "
                   "eine positive ganze Zahl eingegeben werden."));
        errorDialog->show();
        return;
    }

    const int32_t cornerRefinmentWindowSizeHorizontal
        = this->widget->lineEdit_cornerRefinmentWindowSizeHorizontal->text().toInt(&ok);
    if (!ok)
    {
        showError(
            trUtf8("Bitte das Eingabefeld für die horizontale Fenstergröße der Eckenverfeinerung "
                   "überprüfen. Es muss eine positive ganze Zahl eingegeben werden."));
        return;
    }

    const int32_t cornerRefinmentWindowSizeVertical
        = this->widget->lineEdit_cornerRefinmentWindowSizeVertical->text().toInt(&ok);
    if (!ok)
    {
        showError(
            trUtf8("Bitte das Eingabefeld für die vertikale Fenstergröße der Eckenverfeinerung "
                   "überprüfen. Es muss eine positive ganze Zahl eingegeben werden."));
        return;
    }

    double squareWidth = this->widget->lineEdit_quadratGroesse->text().toDouble(&ok);
    if (!ok)
    {
        errorDialog->setText(trUtf8("Bitte das Eingabefeld für die Quadratgröße überprüfen."));
        errorDialog->show();
        return;
    }

    std::vector<ImageModel::ImgData> imageData = imgModel->getImageData();
    if (imageData.size() <= 0)
    {
        showError(trUtf8("Es muss mindestens eine Datei für die Kalibrierung ausgewählt sein."));
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

        if (imageData[i].checked)
        {
            calibTool.addFile(imageData[i].filePath);
            filePathModelIndices.push_back(i);
        }
    }

    // Get filepath
    const QString filePath = QFileDialog::getSaveFileName(this, trUtf8("Datei speichern"),
        QDir::homePath() + "/", trUtf8("XML-Dateien (*.xml);;JSON-Dateien (*.json)"));

    if (!filePath.endsWith(".xml") && !filePath.endsWith(".json"))
    {
        showError(trUtf8("Die Datei muss die Endung \".xml\" oder \".json\" haben."));
        return;
    }

    if (filePath.isEmpty())
    {
        showError(trUtf8("Dateipfad ist leer."));
        return;
    }

    // set parameters
    calibTool.setChessboardSize(cv::Size2i(cornersHorizontal, cornersVertical));
    calibTool.setChessboardSquareWidth(squareWidth);
    calibTool.setCornerRefinmentWindowSize(
        cv::Size2i(cornerRefinmentWindowSizeHorizontal, cornerRefinmentWindowSizeVertical));

    int calibrationFlags = 0;
    const int distortionModelIndex = this->widget->comboBox_distortionModel->currentIndex();
    switch (distortionModelIndex)
    {
    case 0:
    {
        calibrationFlags = 0;
        break;
    }
    case 1:
    {
        calibrationFlags |= cv::CALIB_RATIONAL_MODEL;
        break;
    }
    default:
    {
        throw std::runtime_error("Unknown calibration model.");
    }
    }
    calibTool.setCalibrationFlags(calibrationFlags);

    widget->pushButton_kalibrieren->setText(trUtf8("Kalibrierung stoppen"));
    widget->tableView_images->setEditTriggers(QAbstractItemView::NoEditTriggers);
    imgModel->setCheckboxesEnabled(false);
    disableButtons();

    // http://qt-project.org/wiki/QtConcurrent-run-member-function
    calibrationFuture = QtConcurrent::run(
        this, &CalibrationWidget::doCalibration, filePath, filePathModelIndices);
    calibrationRunning = true;
}
//------------------------------------------------------------------------------------------------
void CalibrationWidget::doCalibration(
    const QString& filePath, const std::vector<int>& filePathModelIndices)
{
    namespace pl = std::placeholders;
    auto f = std::bind(&ProgressState::emitSignals, calibrationState, pl::_1, pl::_2, pl::_3);
    try
    {
        calibTool.calibrateCamera(f);
    }
    catch (const std::runtime_error& e)
    {
        emit calibrationDone(false, QString::fromStdString(e.what()));
        return;
    }
    catch (const cv::Exception& e)
    {
        emit calibrationDone(false, QString::fromStdString(e.what()));
        return;
    }
    catch (...)
    {
        emit calibrationDone(false, trUtf8("Unknown exception."));
        return;
    }

    if (calibTool.isStopRequested())
        return;

    calibTool.saveCameraParameters(filePath.toStdString());
    const std::vector<libba::CameraCalibration::CalibImgInfo> imgs = calibTool.getCalibInfo();

    // update the imagemodel
    for (size_t i = 0; i < imgs.size(); ++i)
    {
        int modelIdx = filePathModelIndices[i];
        ImageModel::ImgData data = imgModel->getImageData(modelIdx);
        data.found = imgs[i].patternFound;
        data.error = imgs[i].reprojectionError;
        data.boardCornersImg = imgs[i].boardCornersImg;

        imgModel->setImageData(modelIdx, data);
    }

    // signals must be used here because otherwise calibrationDone() and the gui would run in
    // different threads
    emit calibrationDone();
}
//------------------------------------------------------------------------------------------------
void CalibrationWidget::stopCalibration()
{
    calibTool.stopCalibration();

    // cancel the calibration and wait for thread has finished
    if (calibrationFuture.isRunning())
    {
        QMessageBox::information(this, trUtf8("Kalibrierung stoppen"),
            trUtf8("Die Kalibrierung wird abgebrochen, dies kann einen Moment dauern."));
        calibrationFuture.cancel();
        calibrationFuture.waitForFinished();
    }
    else
        QMessageBox::information(this, trUtf8("Kalibrierung abgeschlossen"),
            trUtf8("Das Kalibrieren der Kamera ist abegschlossen."));

    calibrationRunning = false;
    imgModel->setCheckboxesEnabled(true);
    widget->pushButton_kalibrieren->setText(trUtf8("Kalibrieren"));

    // reset progressbar
    widget->progressBar->setValue(0);
    enableButtons();
}
//------------------------------------------------------------------------------------------------
void CalibrationWidget::on_pushButton_loeschen_clicked()
{
    QModelIndex i = widget->tableView_images->currentIndex();
    widget->tableView_images->model()->removeRow(i.row(), QModelIndex());
}
//------------------------------------------------------------------------------------------------
void CalibrationWidget::on_pushButton_hinzufuegen_clicked()
{
    const QString filePath = QFileDialog::getOpenFileName(
        this, trUtf8("Datei öffnen"), QDir::homePath(), trUtf8("Images (*.png *.jpg)"));

    if (filePath == "")
        return;

    imgModel->addImage(filePath);
}
//------------------------------------------------------------------------------------------------
void CalibrationWidget::on_pushButton_ordnerHinzufuegen_clicked()
{
    const QString dirPath = QFileDialog::getExistingDirectory(
        this, trUtf8("Ordner öffnen"), QDir::homePath(), QFileDialog::ShowDirsOnly);
    const std::regex filter(".*\\.JPG|.*\\.PNG|.*\\.jpg||.*\\.png", std::regex::icase);
    std::vector<std::string> files = libba::readFilesFromDir(dirPath.toStdString(), filter);

    for (const auto& file : files)
        imgModel->addImage(QString::fromStdString(file));
}
//------------------------------------------------------------------------------------------------
void CalibrationWidget::setupUi()
{
    widget->setupUi(this);

    QIntValidator* qiv = new QIntValidator(2, 100, this);
    widget->lineEdit_eckenHorizontal->setValidator(qiv);
    widget->lineEdit_eckenVertikal->setValidator(qiv);

    QIntValidator* qiv2 = new QIntValidator(0, 100, this);
    widget->lineEdit_cornerRefinmentWindowSizeVertical->setValidator(qiv2);
    widget->lineEdit_cornerRefinmentWindowSizeHorizontal->setValidator(qiv2);

    QDoubleValidator* qdv = new QDoubleValidator(0, std::numeric_limits<double>::max(), 10, this);
    widget->lineEdit_quadratGroesse->setValidator(qdv);

    errorDialog = new QMessageBox(
        QMessageBox::Warning, trUtf8("Fehler"), trUtf8("Fehler"), QMessageBox::Ok, this);

    widget->graphicsView->setScene(new QGraphicsScene(widget->graphicsView));

    widget->tableView_images->setEditTriggers(QAbstractItemView::NoEditTriggers);
    widget->tableView_images->setSelectionBehavior(QAbstractItemView::SelectRows);
    widget->tableView_images->setModel(imgModel);
    widget->tableView_images->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    widget->tableView_images->horizontalHeader()->sectionResizeMode(QHeaderView::ResizeToContents);
    widget->tableView_images->resizeColumnsToContents();

    widget->lineEdit_eckenVertikal->setText(QString::number(calibTool.getChessboardSize().height));
    widget->lineEdit_eckenHorizontal->setText(QString::number(calibTool.getChessboardSize().width));
    widget->lineEdit_quadratGroesse->setText(QString::number(calibTool.getChessboardSquareWidth()));

    calibrationState = new ProgressState(widget->progressBar);
}
//------------------------------------------------------------------------------------------------
void CalibrationWidget::showImage(const QModelIndex& index)
{
    QGraphicsScene* scene = widget->graphicsView->scene();

    // delete all items in the scene (currentImage)
    scene->clear();
    QString filePath = QString::fromStdString(imgModel->getImageData(index.row()).filePath);

    if (!QFile::exists(filePath))
    {
        errorDialog->setText(
            trUtf8("Das Angeforderte Bild existiert nicht mehr im Dateisystem: ") + filePath);
        errorDialog->show();
        return;
    }

    switch (widget->comboBox_ansicht->currentIndex())
    {
    case 0:
    {
        QImage img(filePath);

        if (img.isNull())
        {
            showError(trUtf8("Das Bild konnte nicht geöffnet werden."));
            return;
        }

        currentImage = new QGraphicsPixmapItem(QPixmap::fromImage(img));
        break;
    }
    case 1:
    {
        if (!calibTool.isCalibrationDataAvailable())
        {
            showError(trUtf8(
                "Für diese Funktion müssen erst Kameraparameter berechnet oder geladen werden."));
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
        if (!calibTool.isCalibrationDataAvailable())
        {
            showError(trUtf8("Für diese Funktion müssen erst Kameraparameter geladen werden."));
            return;
        }

        try
        {
            if (!imgModel->getImageData(index.row()).found)
                currentImage = new QGraphicsPixmapItem(0);
            else
            {
                cv::Mat cvImg = cv::imread(filePath.toStdString(), CV_LOAD_IMAGE_COLOR);
                cv::drawChessboardCorners(cvImg, calibTool.getChessboardSize(),
                    imgModel->getImageData(index.row()).boardCornersImg, true);
                currentImage = new QGraphicsPixmapItem(qtOpenCvConversions::cvMatToQPixmap(cvImg));
            }
        }
        catch (const std::out_of_range& e)
        {
            showError(trUtf8("out_of_range exception. Dieses Bild existiert nicht mehr"));
            errorDialog->show();
            currentImage = new QGraphicsPixmapItem(0);
        }
        break;
    }
    }

    widget->graphicsView->fitInView(currentImage, Qt::KeepAspectRatio);
    scene->addItem(currentImage);
}
//------------------------------------------------------------------------------------------------
void CalibrationWidget::updateResults(bool success, const QString& errorMsg)
{
    if (!success)
    {
        errorDialog->setText(
            trUtf8("Es ist ein Fehler bei der Kalibrierung aufgetreten.\nFehler:") + errorMsg);
        errorDialog->show();
        return;
    }

    constexpr int precision = 5;
    const std::string tableStyle = "cellpadding=\"2\"";
    std::string tableHTML = libba::matrixToHTML(calibTool.getCameraMatrix(), tableStyle, precision);

    widget->label_cameraMatrix->setText(QString::fromStdString(tableHTML));

 
    tableHTML = "<table cellpadding=\"2\">";
    for (size_t i = 0; i < calibTool.getNumDistortionCoefficents(); ++i)
    {
        std::string label = "";
        switch (i)
        {
        case 0:
        {
            label = "kappa1";
            break;
        }
        case 1:
        {
            label = "kappa2";
            break;
        }
        case 2:
        {
            label = "p1";
            break;
        }
        case 3:
        {
            label = "p2";
            break;
        }
        case 4:
        {
            label = "kappa3";
            break;
        }
        case 5:
        {
            label = "kappa4";
            break;
        }
        case 6:
        {
            label = "kappa5";
            break;
        }
        case 7:
        {
            label = "kappa6";
            break;
        }
        }
        label += ":";
        tableHTML += "<tr><td>" + label + "</td><td>"
            + std::to_string(calibTool.getDistCoeffs().at<double>(i)) + "</td></tr>";
    }

    tableHTML += "</table>";
    widget->label_distoritionCoefficents->setText(QString::fromStdString(tableHTML));

    widget->label_reprojectionError->setText(
        QString::number(calibTool.getReprojectionError(), 'g', 4));
}
//------------------------------------------------------------------------------------------------
void CalibrationWidget::on_pushButton_kalibrierdatenLaden_clicked()
{
    QString filePath = QFileDialog::getOpenFileName(
        this, trUtf8("Datei öffnen"), QDir::homePath(), trUtf8("XML (*.xml);;JSON (*.json)"));
    if (filePath == "")
        return;

    std::smatch match_result;
    std::regex pattern("\\.(json|xml)?$");
    const std::string tmpStr = filePath.toStdString();
    if (!std::regex_search(tmpStr, match_result, pattern))
        throw std::runtime_error("Path does not match the pattern.");

    if (match_result[0] == ".json")
        calibTool.loadCameraParametersJSON(filePath.toStdString());
    else if (match_result[0] == ".xml")
        calibTool.loadCameraParametersXML(filePath.toStdString());
    else
        return;

    updateResults();
}
//------------------------------------------------------------------------------------------------
void CalibrationWidget::on_comboBox_ansicht_currentIndexChanged(int index)
{
    const QModelIndex i = widget->tableView_images->currentIndex();
    if (i.isValid())
        showImage(i);
}
//------------------------------------------------------------------------------------------------
void CalibrationWidget::connectSignalsAndSlots()
{
    connect(widget->tableView_images, SIGNAL(pressed(const QModelIndex&)), this,
        SLOT(showImage(const QModelIndex&)));
    connect(this, SIGNAL(calibrationDone(bool, QString)), this, SLOT(updateResults(bool, QString)));
    connect(this, SIGNAL(calibrationDone(bool, QString)), this, SLOT(stopCalibration()));
    connect(imgModel, SIGNAL(rowsRemoved(const QModelIndex&, int, int)), this->imgModel,
        SLOT(rowsRemoved(const QModelIndex&, int, int)));
}
//------------------------------------------------------------------------------------------------
void CalibrationWidget::closeEvent(QCloseEvent* event)
{
    if (calibrationRunning)
        stopCalibration();
}
//-------------------------------------------------------------------------------------------------
void CalibrationWidget::enableButtons()
{
    this->widget->pushButton_loeschen->setDisabled(false);
    this->widget->pushButton_ordnerHinzufuegen->setDisabled(false);
    this->widget->pushButton_hinzufuegen->setDisabled(false);
    this->widget->pushButton_kalibrierdatenLaden->setDisabled(false);
}
//-------------------------------------------------------------------------------------------------
void CalibrationWidget::disableButtons()
{
    this->widget->pushButton_loeschen->setDisabled(true);
    this->widget->pushButton_ordnerHinzufuegen->setDisabled(true);
    this->widget->pushButton_hinzufuegen->setDisabled(true);
    this->widget->pushButton_kalibrierdatenLaden->setDisabled(true);
}
//-------------------------------------------------------------------------------------------------
void CalibrationWidget::showError(const QString& msg)
{
    errorDialog->setText(msg);
    errorDialog->show();
}
