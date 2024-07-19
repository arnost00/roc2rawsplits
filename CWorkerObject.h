#pragma once
#include <QString>
#include <QNetworkAccessManager>
#include <QDateTime>
#include <QTextBrowser>

class CWorkerObject : public QObject
{
	Q_OBJECT
protected:
	struct SData
	{
		int id = 0;
		int code = 0;
		int si = 0;
		QDateTime ts;
	};
public:
	explicit CWorkerObject();
	virtual ~CWorkerObject() = default;

	bool downloadData();

	void setApiUrl(QString url);
	void setRace(int race);
	void setLastId(int value) { m_lastId = value; }
	void setUseLastId(bool value) { m_useLastId = value; }
	const QString lastData() { return m_lastDownloadedData; }
	const int lastId() { return m_lastId; }
	int punches() { return m_punchData.size(); }

	void savePunch(SData punch);
	void setFileName(QString filename);

	void setLog(QTextBrowser* log) { m_log = log; }

signals:
	void workDone();
	void newPunch(SData punch);

	void toSaveFile();
protected:
	int m_lastId = 0;
	int m_raceNo = 0;
	bool m_useLastId = true;
	QString m_apiURL;
	QString m_lastDownloadedData;

	QMap<int,SData> m_punchData;

	const QString m_constUrlUnitId = "unitId=";
	const QString m_constUrlLastId = "lastId=";
	const QString m_constDateTimeFormat = "yyyy-MM-dd HH:mm:ss";

	QTextBrowser* m_log = nullptr;

	void parseData(QString data);
	void dataDownloaded(QNetworkReply* reply);
	void sslErrors(QNetworkReply *reply, const QList<QSslError> &errors);

	QNetworkAccessManager m_netManager;

	QString m_filename;
	void saveFile();
	int toSave = 0;

	void addLogLine(QString line);
};
