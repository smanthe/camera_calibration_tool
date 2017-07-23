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
    , calibrationWidget(new Ui::CalibrationWidget)
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

    const int cornersHorizontal = calibrationWidget->lineEdit_eckenHorizontal->text().toInt(&ok);
    if (!ok)
    {
        showError(
            trUtf8("Bitte das Eingabefeld für die Anzahl der horizontalen Ecken überprüfen. Es "
                   "muss eine positive ganze Zahl eingegeben werden."));
        errorDialog->show();
        return;
    }

    const int cornersVertical = calibrationWidget->lineEdit_eckenVertikal->text().toInt(&ok);
    if (!ok)
    {
        errorDialog->setText(
            trUtf8("Bitte das Eingabefeld für die Anzahl der vertikalen Ecken überprüfen. Es muss "
                   "eine positive ganze Zahl eingegeben werden."));
        errorDialog->show();
        return;
    }

    const int32_t cornerRefinmentWindowSizeHorizontal
        = calibrationWidget->lineEdit_cornerRefinmentWindowSizeHorizontal->text().toInt(&ok);
    if (!ok)
    {
        showError(
            trUtf8("Bitte das Eingabefeld für die horizontale Fenstergröße der Eckenverfeinerung "
                   "überprüfen. Es muss eine positive ganze Zahl eingegeben werden."));
        return;
    }

    const int32_t cornerRefinmentWindowSizeVertical
        = calibrationWidget->lineEdit_cornerRefinmentWindowSizeVertical->text().toInt(&ok);
    if (!ok)
    {
        showError(
            trUtf8("Bitte das Eingabefeld für die vertikale Fenstergröße der Eckenverfeinerung "
                   "überprüfen. Es muss eine positive ganze Zahl eingegeben werden."));
        return;
    }

    const double squareWidth = calibrationWidget->lineEdit_quadratGroesse->text().toDouble(&ok);
    if (!ok)
    {
        showError(trUtf8("Bitte das Eingabefeld für die Quadratgröße überprüfen."));
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
    switch (calibrationWidget->comboBox_distortionModel->currentIndex())
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
    case 2:
    {
        calibrationFlags |= cv::CALIB_THIN_PRISM_MODEL;
        break;
    }
    case 3:
    {
        calibrationFlags |= cv::CALIB_RATIONAL_MODEL | cv::CALIB_THIN_PRISM_MODEL;
        break;
    }
    default:
    {
        throw std::runtime_error("Unknown calibration model.");
    }
    }

    calibTool.setCalibrationFlags(calibrationFlags);

    calibrationWidget->pushButton_kalibrieren->setText(trUtf8("Kalibrierung stoppen"));
    calibrationWidget->tableView_images->setEditTriggers(QAbstractItemView::NoEditTriggers);
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
    calibrationWidget->pushButton_kalibrieren->setText(trUtf8("Kalibrieren"));

    // reset progressbar
    calibrationWidget->progressBar->setValue(0);
    enableButtons();
}
//------------------------------------------------------------------------------------------------
void CalibrationWidget::on_pushButton_loeschen_clicked()
{
    QModelIndex i = calibrationWidget->tableView_images->currentIndex();
    calibrationWidget->tableView_images->model()->removeRow(i.row(), QModelIndex());
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
    calibrationWidget->setupUi(this);

    QIntValidator* qiv = new QIntValidator(2, 100, this);
    calibrationWidget->lineEdit_eckenHorizontal->setValidator(qiv);
    calibrationWidget->lineEdit_eckenVertikal->setValidator(qiv);

    QIntValidator* qiv2 = new QIntValidator(0, 100, this);
    calibrationWidget->lineEdit_cornerRefinmentWindowSizeVertical->setValidator(qiv2);
    calibrationWidget->lineEdit_cornerRefinmentWindowSizeHorizontal->setValidator(qiv2);

    QDoubleValidator* qdv = new QDoubleValidator(0, std::numeric_limits<double>::max(), 10, this);
    calibrationWidget->lineEdit_quadratGroesse->setValidator(qdv);

    errorDialog = new QMessageBox(
        QMessageBox::Warning, trUtf8("Fehler"), trUtf8("Fehler"), QMessageBox::Ok, this);

    calibrationWidget->graphicsView->setScene(new QGraphicsScene(calibrationWidget->graphicsView));

    calibrationWidget->tableView_images->setEditTriggers(QAbstractItemView::NoEditTriggers);
    calibrationWidget->tableView_images->setSelectionBehavior(QAbstractItemView::SelectRows);
    calibrationWidget->tableView_images->setModel(imgModel);
    calibrationWidget->tableView_images->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    calibrationWidget->tableView_images->horizontalHeader()->sectionResizeMode(QHeaderView::ResizeToContents);
    calibrationWidget->tableView_images->resizeColumnsToContents();

    calibrationWidget->lineEdit_eckenVertikal->setText(QString::number(calibTool.getChessboardSize().height));
    calibrationWidget->lineEdit_eckenHorizontal->setText(QString::number(calibTool.getChessboardSize().width));
    calibrationWidget->lineEdit_quadratGroesse->setText(QString::number(calibTool.getChessboardSquareWidth()));

    calibrationState = new ProgressState(calibrationWidget->progressBar);
}
//------------------------------------------------------------------------------------------------
void CalibrationWidget::showImage(const QModelIndex& currentIndex)
{
    QGraphicsScene* scene = calibrationWidget->graphicsView->scene();

    // delete all items in the scene (currentImage)
    scene->clear();
    QString filePath = QString::fromStdString(imgModel->getImageData(currentIndex.row()).filePath);

    if (!QFile::exists(filePath))
    {
        errorDialog->setText(
            trUtf8("Das Angeforderte Bild existiert nicht mehr im Dateisystem: ") + filePath);
        errorDialog->show();
        return;
    }

    switch (calibrationWidget->comboBox_ansicht->currentIndex())
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
            if (!imgModel->getImageData(currentIndex.row()).found)
                currentImage = new QGraphicsPixmapItem(0);
            else
            {
                cv::Mat cvImg = cv::imread(filePath.toStdString(), CV_LOAD_IMAGE_COLOR);
                cv::drawChessboardCorners(cvImg, calibTool.getChessboardSize(),
                    imgModel->getImageData(currentIndex.row()).boardCornersImg, true);
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

    calibrationWidget->graphicsView->fitInView(currentImage, Qt::KeepAspectRatio);
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

    calibrationWidget->label_cameraMatrix->setText(QString::fromStdString(tableHTML));

 
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
        case 8:
        {
            label = "s1";
            break;
        }
        case 9:
        {
            label = "s2";
            break;
        }
        case 10:
        {
            label = "s3";
            break;
        }
        case 11:
        {
            label = "s4";
            break;
        }
        }
        label += ":";
        tableHTML += "<tr><td>" + label + "</td><td>"
            + std::to_string(calibTool.getDistCoeffs().at<double>(i)) + "</td></tr>";
    }

    tableHTML += "</table>";
    calibrationWidget->label_distoritionCoefficents->setText(QString::fromStdString(tableHTML));
    calibrationWidget->label_reprojectionError->setText(
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
    const QModelIndex i = calibrationWidget->tableView_images->currentIndex();
    if (i.isValid())
        showImage(i);
}
//------------------------------------------------------------------------------------------------
void CalibrationWidget::connectSignalsAndSlots()
{
    connect(calibrationWidget->tableView_images->selectionModel(),
        SIGNAL(currentRowChanged(const QModelIndex&, const QModelIndex&)), this,
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
    calibrationWidget->pushButton_loeschen->setDisabled(false);
    calibrationWidget->pushButton_ordnerHinzufuegen->setDisabled(false);
    calibrationWidget->pushButton_hinzufuegen->setDisabled(false);
    calibrationWidget->pushButton_kalibrierdatenLaden->setDisabled(false);
}
//-------------------------------------------------------------------------------------------------
void CalibrationWidget::disableButtons()
{
    calibrationWidget->pushButton_loeschen->setDisabled(true);
    calibrationWidget->pushButton_ordnerHinzufuegen->setDisabled(true);
    calibrationWidget->pushButton_hinzufuegen->setDisabled(true);
    calibrationWidget->pushButton_kalibrierdatenLaden->setDisabled(true);
}
//-------------------------------------------------------------------------------------------------
void CalibrationWidget::showError(const QString& msg)
{
    errorDialog->setText(msg);
    errorDialog->show();
}
