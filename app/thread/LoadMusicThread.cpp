#include "StdAfx.h"
#include "LoadMusicThread.h"
#include "DBModule.h"
#include "wavfile.h"

LoadMusicThread::LoadMusicThread(void)
{

}

LoadMusicThread::~LoadMusicThread(void)
{

}

void LoadMusicThread::run()
{
	this->startFunc();
}

//�������ļ� ���ò���
void LoadMusicThread::setParam( const QStringList &musicList, map<QString, int> *mapMusicRows, map<QString, QString> *mapMusicFilePath, QTableWidget *tableWidget )
{
	m_MusicList = musicList;
	m_mapMusicRows = mapMusicRows;
	m_mapMusicFilePath = mapMusicFilePath;
	m_TableWidget = tableWidget;
	loadMusicMode = LoadMusicThread::openMusic;				//�������ļ�����ģʽ
}

//�϶������ļ������ָ�ʽ�ļ� ���ò���
void LoadMusicThread::setParam( const QList<QUrl> &urls, map<QString, int> *mapMusicRows, map<QString, QString> *mapMusicFilePath, QTableWidget *tableWidget )
{
	m_urls = urls;
	m_mapMusicRows = mapMusicRows;
	m_mapMusicFilePath = mapMusicFilePath;
	m_TableWidget = tableWidget;
	loadMusicMode = LoadMusicThread::dragEnterMusic;		//�϶������ļ������ָ�ʽ�ļ�����ģʽ
}

//�������ָ�ʽ�ļ����ļ��� ���ò���
void LoadMusicThread::setParam( const QString &strFileOrDir, map<QString, int> *mapMusicRows, map<QString, QString> *mapMusicFilePath, QTableWidget *tableWidget )
{
	QFileInfo fileInfo(strFileOrDir);
	if (fileInfo.isFile())
	{
		m_strMusicFormatFile = strFileOrDir;
		m_mapMusicRows = mapMusicRows;
		m_mapMusicFilePath = mapMusicFilePath;
		m_TableWidget = tableWidget;
		loadMusicMode = LoadMusicThread::openMusicFormatFile;	//���ָ�ʽ�ļ�����ģʽ
	}
	else if (fileInfo.isDir())
	{
		m_strMusicDirectory = strFileOrDir;
		m_mapMusicRows = mapMusicRows;
		m_mapMusicFilePath = mapMusicFilePath;
		m_TableWidget = tableWidget;
		loadMusicMode = LoadMusicThread::openDirectory;			//�����ļ��м���ģʽ
	}
}

//�����ݿ��ȡ�����ļ� ���ò���
void LoadMusicThread::setParam( map<QString, int> *mapMusicRows, map<QString, QString> *mapMusicFilePath, QTableWidget *tableWidget )
{
	m_mapMusicRows = mapMusicRows;
	m_mapMusicFilePath = mapMusicFilePath;
	m_TableWidget = tableWidget;
	loadMusicMode = LoadMusicThread::readFromDB;			//���ݿ��ȡ�����ļ�����ģʽ
}

void LoadMusicThread::startFunc()
{
	if (loadMusicMode == LoadMusicThread::openMusic)
	{
		this->openMusicModeLoad();
	}
	else if (loadMusicMode == LoadMusicThread::dragEnterMusic)
	{
		this->dragEnterMusicModeLoad();
	}
	else if (loadMusicMode == LoadMusicThread::openMusicFormatFile)
	{
		this->openMusicFormatFileLoad();
	}
	else if (loadMusicMode == LoadMusicThread::readFromDB)
	{
		this->readFromDBModeLoad();
	}
	else if (loadMusicMode == LoadMusicThread::openDirectory)
	{
		this->openMusicDirectoryMode();
	}

	emit sig_LoadMusicListFinished();	//�����겥���б�
	this->deleteLater();
}

//���ظ����б�
void LoadMusicThread::loadMusicList( const QString &filePath )
{
	//���
	QTableWidgetItem *rowItem = new QTableWidgetItem;
	rowItem->setTextAlignment(Qt::AlignCenter);
	rowItem->setFont(QFont("΢���ź�", 10));

	//����
	QString fileName = QFileInfo(filePath).baseName();
	QTableWidgetItem *fileNameItem = new QTableWidgetItem(fileName);
	fileNameItem->setTextAlignment(Qt::AlignCenter);
	fileNameItem->setFont(QFont("΢���ź�", 10));

	//ʱ��
	WavFile wavFile;
	int nMusicTime = wavFile.getMusicTime(filePath);
	QTime time(0, nMusicTime / 60, nMusicTime % 60);
	QTableWidgetItem *timeItem = new QTableWidgetItem(time.toString("mm:ss"));
	timeItem->setTextAlignment(Qt::AlignCenter);
	timeItem->setFont(QFont("΢���ź�", 10));

	//���벥���б�
	int currentRows = m_TableWidget->rowCount();//�����б��е�����
	m_TableWidget->insertRow(currentRows);//�Ӳ����б��еĵ�ǰ�в���
	rowItem->setText(QString::number(currentRows + 1));
	m_TableWidget->setItem(currentRows, 0, rowItem);
	m_TableWidget->setItem(currentRows, 1, fileNameItem);
	m_TableWidget->setItem(currentRows, 2, timeItem);

	//����ʱ���м��
	if (currentRows + 1 <= 12)
	{
		m_TableWidget->horizontalHeader()->resizeSection(3, 80);
	}
	else
	{
		m_TableWidget->horizontalHeader()->resizeSection(3, 65);
	}

	//��������Ϣ����vector
	m_mapMusicRows->insert(make_pair(fileName, currentRows));
	m_mapMusicFilePath->insert(make_pair(fileName, filePath));

	//�������ݿ�
	DBModule::insertLine(fileName, time.toString("mm:ss"), filePath);
}

//�������ļ�����ģʽ
void LoadMusicThread::openMusicModeLoad()
{
	foreach (QString filePath, m_MusicList)
	{
		this->loadMusicList(filePath);
	}
}

//�϶������ļ������ָ�ʽ�ļ�����ģʽ
void LoadMusicThread::dragEnterMusicModeLoad()
{
	foreach (QUrl url, m_urls)
	{
		QString filePath = url.toLocalFile();

		QFileInfo fileInfo(filePath);
		
		if (fileInfo.isFile())	//�����ļ�
		{
			QString strSuffixName = filePath.right(3);//�õ���׺��
			//���ز����б�
			if (strSuffixName == "m3u" || strSuffixName == "M3U" || strSuffixName == "pls" || strSuffixName == "PLS")
			{
				char buf[256];
				string sBuf;
				int i = 0;
				int n = 1;
				ifstream file(filePath.toStdString());
				while (!file.eof())
				{
					file.getline(buf, 256, '\n');
					sBuf = buf;

					if (i == 0)		//��ȡ��ͷ
					{
						if (sBuf != "#EXTM3U" && sBuf != "[playlist]")
						{
							emit sig_LoadMusicListError();
							this->deleteLater();
						}
					}
					else
					{
						if ((strSuffixName == "m3u" || strSuffixName == "M3U") && i % 2 == 0)
						{
							this->loadMusicList(QString::fromStdString(sBuf));	//���ظ����б�
						}
						else if ((strSuffixName == "pls" || strSuffixName == "PLS") && i == 1 + (n - 1) * 3)
						{
							if (!QString::fromStdString(sBuf).contains("NumberOfEntries"))
							{
								++n;
								if (QString::fromStdString(sBuf).contains("="))
								{
									QStringList list = QString::fromStdString(sBuf).split("=");
									QString filePath = list.at(1);
									this->loadMusicList(filePath);	//���ظ����б�
								}
							}
						}
					}
					++i;
				}
				file.close();
			}
			else	//���ص��׸���
			{
				this->loadMusicList(filePath);
			}
		}
		else if (fileInfo.isDir())	//�����ļ���
		{
			m_strMusicDirectory = filePath;
			this->openMusicDirectoryMode();
		}
	}
}

//�����ָ�ʽ�ļ�����ģʽ
void LoadMusicThread::openMusicFormatFileLoad()
{
	QString strSuffixName = m_strMusicFormatFile.right(3);//�õ���׺��
	////��ȡ��ǰý��Դ�б�Ĵ�С
	//int index = sources.size();
	char buf[256];
	string sBuf;
	int i = 0;
	int n = 1;
	ifstream file(m_strMusicFormatFile.toStdString());
	while (!file.eof())
	{
		file.getline(buf, 256, '\n');
		sBuf = buf;

		if (i == 0)		//��ȡ��ͷ
		{
			if (sBuf != "#EXTM3U" && sBuf != "[playlist]")
			{
				emit sig_LoadMusicListError();
				this->deleteLater();
			}
		}
		else
		{
			if ((strSuffixName == "m3u" || strSuffixName == "M3U") && i % 2 == 0)
			{
				this->loadMusicList(QString::fromStdString(sBuf));	//���ظ����б�
			}
			else if ((strSuffixName == "pls" || strSuffixName == "PLS") && i == 1 + (n - 1) * 3)
			{
				if (!QString::fromStdString(sBuf).contains("NumberOfEntries"))
				{
					++n;
					if (QString::fromStdString(sBuf).contains("="))
					{
						QStringList list = QString::fromStdString(sBuf).split("=");
						QString filePath = list.at(1);
						this->loadMusicList(filePath);	//���ظ����б�
					}
				}
			}
		}
		++i;
	}
	file.close();
}

//���ݿ��ȡ�����ļ�����ģʽ
void LoadMusicThread::readFromDBModeLoad()
{
	vector<MusicInfoEntity> vec;
	if (DBModule::readMusicInfoEntity(vec))
	{
		foreach (MusicInfoEntity entity, vec)
		{
			this->loadMusicList(entity);	//���ظ����б�
		}
	}
}

//���ظ����б�
void LoadMusicThread::loadMusicList( const QStringList &list )
{
	foreach (QString filePath, list)
	{
		this->loadMusicList(filePath);
	}
}

//���ظ����б�
void LoadMusicThread::loadMusicList( const MusicInfoEntity &entity )
{
	//���
	QTableWidgetItem *rowItem = new QTableWidgetItem;
	rowItem->setTextAlignment(Qt::AlignCenter);
	rowItem->setFont(QFont("΢���ź�", 10));

	//����
	QTableWidgetItem *fileNameItem = new QTableWidgetItem(QString::fromStdString(entity.getFileName()));
	fileNameItem->setTextAlignment(Qt::AlignCenter);
	fileNameItem->setFont(QFont("΢���ź�", 10));

	//ʱ��
	QTableWidgetItem *timeItem = new QTableWidgetItem(QString::fromStdString(entity.getTime()));
	timeItem->setTextAlignment(Qt::AlignCenter);
	timeItem->setFont(QFont("΢���ź�", 10));

	//���벥���б�
	int currentRows = m_TableWidget->rowCount();//�����б��е�����
	m_TableWidget->insertRow(currentRows);//�Ӳ����б��еĵ�ǰ�в���
	rowItem->setText(QString::number(currentRows + 1));
	m_TableWidget->setItem(currentRows, 0, rowItem);
	m_TableWidget->setItem(currentRows, 1, fileNameItem);
	m_TableWidget->setItem(currentRows, 2, timeItem);

	//����ʱ���м��
	if (currentRows + 1 <= 12)
	{
		m_TableWidget->horizontalHeader()->resizeSection(3, 80);
	}
	else
	{
		m_TableWidget->horizontalHeader()->resizeSection(3, 65);
	}

	//��������Ϣ����vector
	m_mapMusicRows->insert(make_pair(QString::fromStdString(entity.getFileName()), currentRows));
	m_mapMusicFilePath->insert(make_pair(QString::fromStdString(entity.getFileName()), QString::fromStdString(entity.getFilePath())));

	//�������ݿ�
	//DBModule::insertLine(fileName, time.toString("mm:ss"), filePath);
}

//�����ļ��м���ģʽ
void LoadMusicThread::openMusicDirectoryMode()
{
	QDir dir(m_strMusicDirectory);
	QFileInfoList fileInfoList = dir.entryInfoList(QDir::Files | QDir::Hidden | QDir::NoDotAndDotDot | QDir::NoSymLinks);
	foreach (QFileInfo fileInfo, fileInfoList)
	{
		if (fileInfo.isFile() && (fileInfo.absoluteFilePath().right(3) == "wav" || fileInfo.absoluteFilePath().right(3) == "WAV"))
		{
			this->loadMusicList(fileInfo.absoluteFilePath());
		}
	}
}