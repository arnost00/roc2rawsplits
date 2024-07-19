#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QScrollBar>
#include <QMessageBox>
#include <QSettings>
#include <QFileDialog>

const std::vector < std::pair <QString, QString >> predefinedApis =
{
	{ "OResults.eu", "https://api.oresults.eu/roc"},
	{ "OLResultat.se", "https://roc.olresultat.se/getpunches.asp"}
};

const QString keyRaceNumber("RaceNumber");
const QString keyOutputFile("OutputFile");
const QString keyUseLastId("UseLastId");
const QString keyRocApiUrl("RocApiUrl");
const QString keyLastIdValue("LastIdValue");

MainWindow::MainWindow(QWidget *parent)
	: QMainWindow(parent)
	, ui(new Ui::MainWindow)
{
	ui->setupUi(this);

	setWindowIcon(QIcon(":/main.png"));

	connect(ui->actionE_xit, &QAction::triggered, this, &MainWindow::close);

	connect(ui->actionLoad_Settings, &QAction::triggered, this, [this](){ this->LoadSettings(true); });
	connect(ui->actionSave_Settings, &QAction::triggered, this, [this](){ this->SaveSettings(true); });
	connect(ui->actionAbout, &QAction::triggered, this, &MainWindow::About);
	connect(ui->actionAbout_Qt, &QAction::triggered, this, &MainWindow::AboutQt);

	connect(ui->pushButton_Start, &QPushButton::clicked, this, &MainWindow::Start);
	connect(ui->pushButton_Stop, &QPushButton::clicked, this, &MainWindow::Stop);
	connect(ui->pushButton_Reset, &QPushButton::clicked, this, &MainWindow::Reset);
	connect(ui->pushButton_ChooseFile, &QPushButton::clicked, this, &MainWindow::ChooseOutputFile);
	connect(ui->pushButton_ResetId, &QPushButton::clicked, this, &MainWindow::ResetLastId);
	connect(ui->checkBox_UseLastId, &QCheckBox::clicked, this, &MainWindow::UseLastChecked);

	ui->pushButton_Stop->setEnabled(false);

	QMenu *apiList = new QMenu(this);
	for(auto&item : predefinedApis)
	{
		QAction *action = new QAction(this);
		connect(action, &QAction::triggered, this, &MainWindow::OnSelectAPI);
		action->setText(item.first);
		action->setData(item.second);
		apiList->addAction(action);
	}
	ui->pushButton_PredefinedAPI->setMenu(apiList);

	timer = new QTimer(this);
	connect(timer, &QTimer::timeout, this, &MainWindow::TimerUpdate);
	LoadSettings(false);
}

MainWindow::~MainWindow()
{
	SaveSettings(false);
	delete ui;
}

void MainWindow::Start()
{
	QApplication::setOverrideCursor(Qt::WaitCursor);
	if (!worker)
	{
		worker.reset(new CWorkerObject());
		worker->setLog(ui->textBrowser_Log);
		connect(worker.get(),&CWorkerObject::workDone, this,&MainWindow::WorkDone);
		ui->textBrowser_Log->clear();
	}
	worker->setLastId(ui->checkBox_UseLastId->isChecked());
	worker->setApiUrl(ui->lineEdit_ApiAddress->text());
	worker->setRace(ui->lineEdit_RaceNumber->text().toInt());
	worker->setFileName(ui->lineEdit_OutputFile->text());
	worker->setLastId(ui->spinBox_LastId->value());

	UpdateUI(true);
	timer->setInterval(ui->spinBox_Timer->value() * 1000); // from sec
	timer->singleShot(100,this,&MainWindow::TimerUpdate);
	timer->start();
	QApplication::restoreOverrideCursor();
}

void MainWindow::Stop()
{
	QApplication::setOverrideCursor(Qt::WaitCursor);
	timer->stop();
	UpdateUI(false);
	QApplication::restoreOverrideCursor();
}

void MainWindow::TimerUpdate()
{
	QApplication::setOverrideCursor(Qt::WaitCursor);
	statusBar()->showMessage(tr("Downloading ..."));
	worker->downloadData();
	QApplication::restoreOverrideCursor();
}

void MainWindow::WorkDone()
{
	QApplication::setOverrideCursor(Qt::WaitCursor);
	ui->lineEdit_QuickInfo->setText(QString("last Id %1, whole punches cnt %2  ").arg(worker->lastId()).arg(worker->punches()));
	QScrollBar *sb = ui->textBrowser_Log->verticalScrollBar();
	if (ui->checkBox_UseLastId->isChecked())
		ui->spinBox_LastId->setValue(worker->lastId());
	sb->setValue(sb->maximum());
	statusBar()->showMessage(tr("Done"),5000);
	QApplication::restoreOverrideCursor();
}

void MainWindow::Reset()
{
	disconnect(worker.get(),&CWorkerObject::workDone, this,&MainWindow::WorkDone);
	worker.reset();
}

void MainWindow::About()
{
	QString buildTime = QString("%1 - %2").arg(__DATE__).arg(__TIME__);

	QString aboutTitle = tr("About roc2rawsplits");
	QString aboutText = "<h1>roc2rawsplits</h1>";
	aboutText += tr("Download split times from ROC API<br>and save them as text file with \"RACOM raw splits\" format");

	QString version = "1.0.0";
	aboutText += "<br><br>";
	aboutText += tr("Version: %1");
	aboutText += "<br><br>";
	QString currentYear = QString("%1").arg(__DATE__).right(4);
	QString copyright = QString::fromUtf8("\xc2\xa9");
	aboutText += copyright + " 2023-" + currentYear + " KeAr s.r.o.";
	aboutText += "<br>";
	aboutText += tr("Distributed under the Boost Software License, Version 1.0");
	aboutText += "<br>";
	aboutText += tr("<br>Build time : %2");
#ifdef _M_X64
	aboutText += "<br>64bit\n";
#else
	aboutText += "<br>32bit\n";
#endif
#ifdef QT_DEBUG
	aboutText += "<br><i>Debug</i>";
#endif

	QMessageBox::about(this, aboutTitle, QString(aboutText).arg(version).arg(buildTime));
}

void MainWindow::AboutQt()
{
	qApp->aboutQt();
}

void MainWindow::LoadSettings(bool askForFile)
{
	std::shared_ptr<QSettings> settings;

	if (askForFile)
	{
		QString filter(tr("Ini files (*.ini);;All files (*.*)"));
		QString path = QFileDialog::getOpenFileName(this, tr("Open settings file"), lastPath, filter);
		if(!path.isEmpty())
		{
			settings.reset(new QSettings(path, QSettings::IniFormat));
			QFileInfo fi(path);
			lastPath = fi.absolutePath();
		}
		else
			return;
	}
	else
		settings.reset(new QSettings(QSettings::IniFormat, QSettings::UserScope, QCoreApplication::organizationName(), QCoreApplication::applicationName()));

	if (!settings)
		return;
	settings->beginGroup(applicationName);
	if(settings->contains(keyRaceNumber))
		ui->lineEdit_RaceNumber->setText(settings->value(keyRaceNumber).toString());
	if(settings->contains(keyOutputFile))
		ui->lineEdit_OutputFile->setText(settings->value(keyOutputFile).toString());
	if(settings->contains(keyUseLastId))
		ui->checkBox_UseLastId->setChecked(settings->value(keyUseLastId).toBool());
	if(settings->contains(keyRocApiUrl))
		ui->lineEdit_ApiAddress->setText(settings->value(keyRocApiUrl).toString());
	if(settings->contains(keyLastIdValue))
		ui->spinBox_LastId->setValue(settings->value(keyLastIdValue).toInt());
}

void MainWindow::SaveSettings(bool askForFile)
{
	std::shared_ptr<QSettings> settings;

	if (askForFile)
	{
		QString filter(tr("Ini files (*.ini);;All files (*.*)"));
		QString path = QFileDialog::getSaveFileName(this, tr("Save settings file"), lastPath, filter);
		if(!path.isEmpty())
		{
			settings.reset(new QSettings(path, QSettings::IniFormat));
			QFileInfo fi(path);
			lastPath = fi.absolutePath();
		}
		else
			return;
	}
	else
		settings.reset(new QSettings(QSettings::IniFormat, QSettings::UserScope, QCoreApplication::organizationName(), QCoreApplication::applicationName()));

	if (!settings)
		return;
	settings->beginGroup(applicationName);
	if(!ui->lineEdit_ApiAddress->text().isEmpty())
		settings->setValue(keyRocApiUrl, ui->lineEdit_ApiAddress->text());
	if(!ui->lineEdit_RaceNumber->text().isEmpty())
		settings->setValue(keyRaceNumber, ui->lineEdit_RaceNumber->text());
	if(!ui->lineEdit_OutputFile->text().isEmpty())
		settings->setValue(keyOutputFile, ui->lineEdit_OutputFile->text());
	settings->setValue(keyUseLastId, ui->checkBox_UseLastId->isChecked());
	settings->setValue(keyLastIdValue, ui->spinBox_LastId->value());

	settings->endGroup();
}

void MainWindow::ChooseOutputFile()
{
	QString filter(tr("Text files (*.txt);;All files (*.*)"));
	QString path = ui->lineEdit_OutputFile->text();
	if (path.isEmpty())
		path = lastPath;
	path = QFileDialog::getSaveFileName(this, tr("Save rawsplit file"), path, filter);
	if(!path.isEmpty())
	{
		ui->lineEdit_OutputFile->setText(path);
		QFileInfo fi(path);
		lastPath = fi.absolutePath();
	}
}

void MainWindow::ResetLastId()
{
	ui->spinBox_LastId->setValue(0);
}

void MainWindow::UpdateUI(bool running)
{
	ui->pushButton_Start->setEnabled(!running);
	ui->pushButton_Stop->setEnabled(running);
	ui->pushButton_ResetId->setEnabled(!running && ui->checkBox_UseLastId->isChecked());
	ui->pushButton_Reset->setEnabled(!running);
	ui->lineEdit_OutputFile->setEnabled(!running);
	ui->lineEdit_RaceNumber->setEnabled(!running);
	ui->lineEdit_ApiAddress->setEnabled(!running);
	ui->checkBox_UseLastId->setEnabled(!running);
	ui->spinBox_LastId->setEnabled(!running && ui->checkBox_UseLastId->isChecked());
	ui->pushButton_PredefinedAPI->setEnabled(!running);
	ui->spinBox_Timer->setEnabled(!running);
	ui->pushButton_ChooseFile->setEnabled(!running);
}

void MainWindow::UseLastChecked(bool checked)
{
	ui->spinBox_LastId->setEnabled(checked);
	ui->pushButton_ResetId->setEnabled(checked);
}

void MainWindow::OnSelectAPI()
{
	QAction* action = qobject_cast<QAction*>(sender());
	if(action)
	{
		QString addr = action->data().toString();
		if (!addr.isEmpty())
			ui->lineEdit_ApiAddress->setText(addr);
	}
}
