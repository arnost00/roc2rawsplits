#include "CWorkerObject.h"
#include <QNetworkReply>
#include <QFile>

// manual :
// https://docs.oresults.eu/integrations/roc

CWorkerObject::CWorkerObject()
{
	connect(&m_netManager, &QNetworkAccessManager::finished, this, &CWorkerObject::dataDownloaded);
	connect(&m_netManager, &QNetworkAccessManager::sslErrors, this, &CWorkerObject::sslErrors);

	connect(this,&CWorkerObject::newPunch, this,&CWorkerObject::savePunch);
	connect(this,&CWorkerObject::toSaveFile,this,&CWorkerObject::saveFile,Qt::QueuedConnection);
}

void CWorkerObject::setApiUrl(QString url)
{
	m_apiURL = url;
}

void CWorkerObject::setRace(int race)
{
	m_raceNo = race;
}

bool CWorkerObject::downloadData()
{
	QString call = m_apiURL;
	call += "?";
	call += m_constUrlUnitId;
	call += QString::number(m_raceNo);
	if (m_useLastId)
	{
		call += "&";
		call += m_constUrlLastId;
		call += QString::number(m_lastId);
	}

	QNetworkRequest request(call);
	m_netManager.get(request);
	addLogLine(call);
	return true;
}

void CWorkerObject::sslErrors(QNetworkReply *reply, const QList<QSslError> &errors)
{
	QString err = "SSL errors:";
	for (auto& e : errors)
	{
		err += "\n";
		err+= e.errorString();
	}
	reply->deleteLater();
	addLogLine(err);
}

void CWorkerObject::dataDownloaded(QNetworkReply* reply)
{
	m_lastDownloadedData.clear();
	if (reply)
	{
		if (reply->error() == QNetworkReply::NetworkError::NoError)
			m_lastDownloadedData = reply->readAll();
		else
		{
			QString err = QString("Download Error [%1] : %2").arg(reply->error()).arg(reply->errorString());
			addLogLine(err);
		}
		reply->deleteLater();
	}

	parseData(m_lastDownloadedData);
}

void CWorkerObject::parseData(QString data)
{
	if (data.size() == 0)
	{
		addLogLine("Parse data - no new data");
		emit workDone();
		return;
	}
	//example line - 949706;2;8002711;2023-08-20 15:39:11
	auto lines = data.split("\n");
	addLogLine(QString("Parse data Beg - lines %1, data size %2").arg(lines.size()).arg(data.size()));
	int add_punches = 0;
	for (auto& l : lines)
	{
		l = l.simplified();
		auto cols = l.split(";");
		if (cols.size() != 4)
			continue;
		int id = cols[0].toInt();
		if (!m_punchData.contains(id))
		{
			SData dta;
			dta.id = id;
			dta.code = cols[1].toInt();
			dta.si = cols[2].toInt();
			dta.ts = QDateTime::fromString(cols[3],m_constDateTimeFormat);
			if (dta.id != 0 && dta.si > 0 && dta.ts.isValid())
			{
				m_punchData.insert(id,dta);
				add_punches++;
				emit newPunch(dta);
				addLogLine(QString("Add punch %1 | %2 | %3 | %4 == %5").arg(dta.id).arg(dta.code,3).arg(dta.si,7).arg(dta.ts.toString(m_constDateTimeFormat)).arg(cols[3]));
			}
			else
				addLogLine(QString("Invalid punch %1 | %2 | %3 | %4 == %5 | '%6'").arg(dta.id).arg(dta.code,3).arg(dta.si,7).arg(dta.ts.toString(m_constDateTimeFormat)).arg(cols[3]).arg(l));
		}
		if (m_useLastId && m_lastId < id)
			m_lastId = id;
	}
	addLogLine(QString("Parse data End - parsed punches %1, lastId %3, stored data size %2").arg(add_punches).arg(m_punchData.size()).arg(m_lastId));
	emit workDone();
}

void CWorkerObject::setFileName(QString filename)
{
	m_filename = filename;
}

void CWorkerObject::savePunch(SData punch)
{
	m_punchData.insert(punch.id,punch);
	toSave++;
	if (toSave == 1)	// first unsaved punch
		emit toSaveFile();
}

void CWorkerObject::saveFile()
{
	if (toSave == 0)
		return;
	QString data;
	for (auto& dta : m_punchData)
	{
		data += QString("%1: %2/%3\n").arg(dta.si,8).arg(dta.code,3).arg(dta.ts.time().toString("HH:mm:ss.z"));
	}

	QFile f(m_filename);
	if(!f.open(QFile::WriteOnly))
		return;
	QTextStream ts(&f);
	ts << data;
	ts.flush();
	f.close();
	addLogLine(QString("Txt Saved - %1 items (new %2 items)").arg(m_punchData.size()).arg(toSave));
	toSave = 0;
}

void CWorkerObject::addLogLine(QString line)
{
	QString text = "[";
	text += QDateTime::currentDateTime().toString(Qt::ISODate);
	text += "] ";
	text += line;
//	text += "\n";
	if (m_log)
		m_log->append(text);
}
