#pragma once

#include <QMainWindow>
#include "CWorkerObject.h"
#include <QTimer>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

const QString applicationName("roc2rawsplits");

class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	MainWindow(QWidget *parent = nullptr);
	~MainWindow();

	void Start();
	void Stop();
	void TimerUpdate();
	void WorkDone();
	void Reset();
	void About();
	void AboutQt();
	void LoadSettings(bool askForFile);
	void SaveSettings(bool askForFile);
	void ChooseOutputFile();
	void ResetLastId();
	void UpdateUI(bool running);
	void UseLastChecked(bool checked);
	void OnSelectAPI();
private:
	Ui::MainWindow *ui = nullptr;
	QTimer *timer = nullptr;

	QString lastPath;

	std::shared_ptr <CWorkerObject> worker;
};
