#include "stdafx.h"
#include "widget.h"
#include "static.h"
#include "czplayerconfig.h"
#include "ClientBaseOperating.h"
#include "DBModule.h"
#include "SoundControl.h"

const int NullTimerId = -1;

/*

�������ݿ⹦��
                                                         -> ��������Ϣ¼�����ݿ�(���������·��¼��)
1.���г��� -> �������ļ� -> ��ȫ�������ļ����ص�ý��Դ
                                                         -> ��������Ϣ��ʾ�����ֱ��

                                       -> �������ļ���ʾ�����ֱ��
2.���г��� -> �������ļ������ݿ��ж���
                                       -> ���������ļ�·���������� -> �������ļ����ص�ý��Դ

*/

/*
CTRL_CLOSE_EVENT, CTRL_LOGOFF_EVENT��CTRL_SHUTDOWN_EVENTͨ������������һЩ�������������
Ȼ�����ExitProcess API�����⣬�������¼��г�ʱ���ƣ�CTRL_CLOSE_EVENT��5�룬����������20�롣
�������ʱ��ϵͳ���ᵯ���������̵ĶԻ�������û�ѡ���˽������̣�
�κ���������������������Ӧ���ڳ�ʱʱ������ɹ���
*/
//�¼�����ص�
BOOL HandlerRoutine(DWORD dwCtrlType)  
{  
	switch (dwCtrlType)  
	{  
	case CTRL_C_EVENT:						//Ctrl+C��ϵͳ�ᷢ�ʹ���Ϣ
		qDebug() << "ctrl+c--���򲻻�ر�";
		return TRUE;  
	case CTRL_CLOSE_EVENT:					 //���û��ر�Consoleʱ��ϵͳ�ᷢ�ʹ���Ϣ
		qDebug() << "ctrl close--�����ر�";
		return TRUE;  
	case CTRL_BREAK_EVENT:					//Ctrl+break��ϵͳ�ᷢ�ʹ���Ϣ
		qDebug() << "CTRL_BREAK_EVENT";  
		return TRUE;
	case CTRL_LOGOFF_EVENT:					//�û��˳���ע������ϵͳ�ᷢ�ʹ���Ϣ
		qDebug() << "CTRL_LOGOFF_EVENT--�û��˳���ע��";  
		return TRUE;
	case CTRL_SHUTDOWN_EVENT:				//ϵͳ�رգ�ϵͳ�ᷢ�ʹ���Ϣ
		qDebug() << "CTRL_SHUTDOWN_EVENT--ϵͳ�ر�";  
		return TRUE;
	default: 
		return FALSE;  
	}  
} 

//���캯��
Widget::Widget()
{
	m_strCurrentFilePath = "";
	m_strCurrentMusicName = "";
	m_strCurrentMusicTime = "00:00";
	m_nCurrentMusicRow = 0;
	nTime = 0;
	nPos = 170;

	//��װ�¼�����
	if (!SetConsoleCtrlHandler((PHANDLER_ROUTINE)HandlerRoutine, TRUE))
	{
		qDebug() << "��װ�¼�����ʧ�ܣ�";
	}

	//step = 0;
	//baseTimer.start(60, this);

	qsrand(time(NULL));							//��ʼ�����������
	ClientBaseOperating::touchConfigFile();		//����һ�������ļ�
	readyReadDb = 0;							//û�д����ݿ��ж�ȡ
	DBModule::initDB();							//��ʼ�����ݿ�
	ClientBaseOperating::mkdirDownloadDir();	//����һ�����ظ���Ŀ¼
	ClientBaseOperating::mkdirAlbumDir();		//����һ���ļ������洢���ص�ר��
	ClientBaseOperating::mkdirLrcDir();			//����һ�����ظ��Ŀ¼

	m_engine = new Engine(this);				//��������
	this ->createUI();							//��������
	this ->connectUI();							//�ź����
	m_state = NoState;							//�趨һ��״̬
}

//��������
Widget::~Widget()
{
	if (searchPanel) {delete searchPanel; searchPanel = 0;}
	if (playList) {delete playList; playList = 0;}
	if (musicListWidget) {delete musicListWidget; musicListWidget = 0;}
	if (lrc) {delete lrc; lrc = 0;}
	if (minMusicWidget) {delete minMusicWidget; minMusicWidget = 0;}

	//terminate:��ֹ�߳�
	if (hotpugWatcherThread ->isRunning()) {hotpugWatcherThread ->terminate(); delete hotpugWatcherThread; hotpugWatcherThread = 0;}
}

//��������
void Widget::createUI()
{
	//���ô��ڻ�������
	this ->resize(400, 212);//���ô����С
	this ->setWindowFlags(Qt::FramelessWindowHint);//ȥ������߿�
	this ->setAttribute(Qt::WA_TranslucentBackground);//���ñ���͸��
	this ->setWindowIcon(QIcon(":/images/CZPlayer.png"));//����logo
	this ->setAcceptDrops(true);//�����Ϸ��ļ�
	this ->setWindowTitle(tr("CZPlayer"));

	//    qDebug() << "������Ļ��С��" << QApplication::desktop() ->width() << ", " << QApplication::desktop() ->height();
	//    qDebug() << "��굱ǰλ�ã�" << QCursor::pos();
	//    qDebug() << "��������С��" << this ->frameGeometry();
	//    qDebug() << "��������������Ļ�ľ������꣺" << this ->mapToParent(QPoint(0, 0));
	//    qDebug() << "��������������Ļ�ľ������꣺" << this ->pos();
	//    qDebug() << frameGeometry().topRight();

	//���ý��潥����ʾ
	QPropertyAnimation *animation = new QPropertyAnimation(this, "windowOpacity");
	animation ->setDuration(1000);
	animation ->setStartValue(0);
	animation ->setEndValue(1);
	animation ->start();

	//ר������
	albumImage = new QLabel(this);
	albumImage ->setToolTip(tr("ר������"));
	albumImage ->setPixmap(QPixmap(tr(":/images/albumImage.png")));
	movie = new QMovie(tr(":/images/jiaZai.gif"));
	//    albumImage ->setMovie(movie);
	//    movie ->start();

	//��Ϣ��ʾ��ǩ
	InfoLabel = new QLabel(this);
	InfoLabel ->setObjectName("InfoLabel");
	InfoLabel ->setText(tr("Welecome to CZPlayer!"));

	//��ǰ���Ÿ���
	currentMusicLabel = new QLabel(this);
	currentMusicLabel ->setObjectName(tr("currentMusicLabel"));
	currentMusicLabel ->setText(tr("Welecome to CZPlayer!"));

	//Ƶ�׽���
	m_spectrograph = new Spectrograph(this);
	m_spectrograph ->setParams(SpectrumNumBands, SpectrumLowFreq, SpectrumHighFreq);	//��ʼ��Ƶ�ײ���

	//���ν���
	m_waveform = new Waveform(this);
	if (CZPlayerConfig::getValue("AUDIOSHOW").toString() == "spectrumShow")	//Ƶ����ʾ
	{
		m_spectrograph ->setVisible(true);
		m_waveform ->setVisible(false);
	}
	else if (CZPlayerConfig::getValue("AUDIOSHOW").toString() == "formShow")	//������ʾ
	{
		m_spectrograph ->setVisible(false);
		m_waveform ->setVisible(true);
	}

	//�������Ʋ��Ž��ȵĻ���
	seekSlider = new QSlider(Qt::Horizontal, this);
	seekSlider ->setStyleSheet("QSlider::groove:horizontal:!hover{border-image: url(:/images/progressBar.png);}"
		"QSlider::groove:horizontal:hover{border-image: url(:/images/progressBar2.png);}"
		"QSlider::sub-page{border-image: url(:/images/progressBarSub.png);}");
	seekSlider ->setObjectName(tr("seekSlider"));

	//�����������ڻ���
	volumeSlider = new QSlider(Qt::Horizontal, this);
	volumeSlider ->setStyleSheet("QSlider::groove:horizontal:!hover{border-image: url(:/images/volumeBar.png);}"
		"QSlider::groove:horizontal:hover{border-image: url(:/images/volumeBar2.png);}"
		"QSlider::sub-page{border-image: url(:/images/volumeBarSub.png);}");
	volumeSlider ->setObjectName(tr("volumeSlider"));
	volumeSlider ->setToolTip(tr("��������"));
	volumeSlider ->setRange(0, 100);
	volumeSlider ->setValue(CZPlayerConfig::getValue("VOLUMEVALUE").toInt());
	SoundControl::SetVolume(0, CZPlayerConfig::getValue("VOLUMEVALUE").toInt());	//��������

	//�������ڰ�ť
	volumeButton = new QPushButton(this);
	volumeButton ->setObjectName(tr("volumeButton"));
	if (CZPlayerConfig::getValue("SETMUTE").toString() == "false")
	{
		volumeButton ->setToolTip(tr("����"));
		volumeButton ->setStyleSheet("QPushButton:!hover{border-image: url(:/images/soundButton.png);}"
			"QPushButton:hover{border-image: url(:/images/soundButton2.png);}"
			"QPushButton:pressed{border-image: url(:/images/soundButton3.png);}");
	}
	else
	{
		SoundControl::SetVolume(0, 0);		//����
		volumeButton ->setToolTip(tr("�ָ�����"));
		volumeButton ->setStyleSheet("QPushButton:!hover{border-image: url(:/images/soundButton4.png);}"
			"QPushButton:hover{border-image: url(:/images/soundButton5.png);}"
			"QPushButton:pressed{border-image: url(:/images/soundButton6.png);}");
	}

	//����ģʽ��ť
	modeButton = new QPushButton(this);
	modeButton ->setObjectName(tr("modeButton"));
	if (ClientBaseOperating::initPlayMode() == "cyclePlay")	//ѭ������
	{
		modeButton ->setStyleSheet("QPushButton:!hover{border-image: url(:/images/cyclePlay.png);}"
			"QPushButton:hover{border-image: url(:/images/cyclePlay2.png);}"
			"QPushButton:pressed{border-image: url(:/images/cyclePlay3.png);}");
		modeButton ->setToolTip(tr("ѭ������"));
		this ->setMode(cyclePlay);							//����ѭ������ģʽ
	}
	else if (ClientBaseOperating::initPlayMode() == "randomPlay")	//�������
	{
		modeButton ->setStyleSheet("QPushButton:!hover{border-image: url(:/images/randomPlay.png);}"
			"QPushButton:hover{border-image: url(:/images/randomPlay2.png);}"
			"QPushButton:pressed{border-image: url(:/images/randomPlay3.png);}");
		modeButton ->setToolTip(tr("�������"));
		this ->setMode(randomPlay);							//�����������ģʽ
	}


	//������ʾʱ���ǩ
	timeLabel = new QLabel(tr("00:00/00:00"), this);
	timeLabel ->setToolTip(tr("��ǰʱ��/��ʱ��"));
	timeLabel ->setObjectName(tr("timeLabel"));
	timeLabel ->setAlignment(Qt::AlignCenter);//���ö����ʽ
	//QSizePolicy��������ˮƽ�ʹ�ֱ�޸Ĵ�С���Ե�һ������
	timeLabel ->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);//ˮƽ�����ϳߴ����չ����ֱ�����ѹ̶�

	//��ʾ����ҳ��
	showButton = new QPushButton(this);
	showButton ->setObjectName(tr("showButton"));
	showButton ->setToolTip(tr("���������"));
	showButton ->setStyleSheet("QPushButton:!hover{border-image: url(:/images/showButton.png);}"
		"QPushButton:hover{border-image: url(:/images/showButton2.png);}"
		"QPushButton:pressed{border-image: url(:/images/showButton3.png);}");

	//����
	playButton = new QPushButton(this);
	playButton ->setToolTip(tr("����"));
	playButton ->setObjectName(tr("playButton"));
	playButton ->setStyleSheet("QPushButton:!hover{border-image: url(:/images/playButton.png);}"
		"QPushButton:hover{border-image: url(:/images/playButton2.png);}"
		"QPushButton:pressed{border-image: url(:/images/playButton3.png);}");

	//ֹͣ
	endButton = new QPushButton(this);
	endButton ->setToolTip(tr("ֹͣ"));
	endButton ->setObjectName(tr("endButton"));

	//��һ��
	preButton = new QPushButton(this);
	preButton ->setToolTip(tr("��һ��"));
	preButton ->setObjectName(tr("preButton"));

	//��һ��
	nextButton = new QPushButton(this);
	nextButton ->setToolTip(tr("��һ��"));
	nextButton ->setObjectName(tr("nextButton"));

	//���ļ�
	openFileButton = new QPushButton(this);
	openFileButton ->setToolTip(tr("���ļ�"));
	openFileButton ->setObjectName(tr("openFileButton"));

	//�򿪲����б�
	musicListButton = new QPushButton(this);
	musicListButton ->setToolTip(tr("�򿪲����б�"));
	musicListButton ->setObjectName(tr("musicListButton"));

	//������
	lrcButton = new QPushButton(this);
	lrcButton ->setToolTip(tr("������"));
	lrcButton ->setObjectName(tr("lrcButton"));

	//�������˳�
	closeAction = new QAction(this);
	closeAction ->setIcon(QIcon(":/images/quitAction.png"));
	closeAction ->setText(tr("�˳�"));

	//��������С��
	minAction = new QAction(this);
	minAction ->setIcon(QIcon(":/images/minAction.png"));
	minAction ->setText(tr("��С��"));

	//���̽����˳�����
	quitAction = new QAction(this);
	quitAction ->setIcon(QIcon(":/images/quitAction.png"));
	quitAction ->setText(tr("�˳�"));

	//��ͣ(����)
	pauseAction = new QAction(this);
	pauseAction ->setIcon(QIcon(":/images/trayPlay.png"));
	pauseAction ->setText(tr("����"));

	//ֹͣ
	stopAction = new QAction(this);
	stopAction ->setIcon(QIcon(":/images/trayStop.png"));
	stopAction ->setText(tr("ֹͣ"));

	//��һ��
	lastAction = new QAction(this);
	lastAction ->setIcon(QIcon(":/images/trayLast.png"));
	lastAction ->setText(tr("��һ��"));

	//��һ��
	nextAction = new QAction(this);
	nextAction ->setIcon(QIcon(":/images/trayNext.png"));
	nextAction ->setText(tr("��һ��"));

	//�������
	unlockLrcAction = new QAction(this);
	unlockLrcAction ->setIcon(QIcon(":/images/unlock.png"));
	unlockLrcAction ->setText(tr("�������"));

	//�������ļ�
	openFileAction = new QAction(this);
	openFileAction ->setIcon(QIcon(":/images/folder.png"));
	openFileAction ->setText(tr("�������ļ�"));

	//�򿪲����б�
	openMusicListAction = new QAction(this);
	openMusicListAction ->setIcon(QIcon(":/images/openMusicList.png"));
	openMusicListAction ->setText(tr("�򿪲����б�"));

	//���������
	openSearchPanelAnimation = new QAction(this);
	openSearchPanelAnimation ->setIcon(QIcon(":/images/openMusicDownload.png"));
	openSearchPanelAnimation ->setText(tr("���������"));

	//��ʾ������
	openLrcAction = new QAction(this);
	openLrcAction ->setIcon(QIcon(":/images/lrc.png"));
	openLrcAction ->setText(tr("��ʾ������"));

	//ѡ��
	configAction = new QAction(this);
	configAction ->setIcon(QIcon(":/images/configButton.png"));
	configAction ->setText(tr("����"));

	//����Qt
	aboutQtAction = new QAction(this);
	aboutQtAction ->setIcon(QIcon(":/images/Qt.png"));
	aboutQtAction ->setText(tr("����Qt"));

	//����
	aboutAction = new QAction(this);
	aboutAction ->setIcon(QIcon(":/images/CZPlayer2.png"));
	aboutAction ->setText(tr("����CZPlayer"));

	//ѭ������
	cycleAction = new QAction(this);
	cycleAction ->setIcon(QIcon(":/images/cyclePlay.png"));

	//�������
	randomAction = new QAction(this);
	randomAction ->setIcon(QIcon(":/images/randomPlay.png"));

	//��Ƶ��ʾ
	audioShowMenu = new QMenu(this);
	audioShowMenu ->setTitle(tr("��Ƶ��ʾ"));
	audioShowMenu ->setIcon(QIcon(":/images/audioShow.png"));

	//Ƶ����ʾ
	spectrumAction = new QAction(this);

	//������ʾ
	formAction = new QAction(this);

	//����ϵͳ����ͼ��
	trayIcon = new QSystemTrayIcon(QIcon(":/images/CZPlayer.png"), this);
	trayIcon ->show();
	trayIcon ->showMessage(tr("CZPlayer"), tr("��ӭ��½www.highway-9.com"));
	trayIcon ->setToolTip(tr("CZPlayer"));

	//�����˵���ϵͳ����ͼ���һ����ֵĲ˵�
	trayMenu = new QMenu();
	trayMenu ->addAction(quitAction);
	trayMenu ->addAction(unlockLrcAction);
	trayMenu ->addSeparator();
	trayMenu ->addAction(pauseAction);
	trayMenu ->addAction(stopAction);
	trayMenu ->addSeparator();
	trayMenu ->addAction(lastAction);
	trayMenu ->addAction(nextAction);
	trayIcon ->setContextMenu(trayMenu);//��ϵͳ��������Ӳ˵�
	trayIcon ->show();

	//С���沥����
	minMusicWidget = new MinMusicWidget(this);
	//�����б�
	playList = new MusicList(this);
	musicListWidget = new MusicListWidget(playList, &m_mapMusicRows, this);
	//���
	lrc = new MusicLrc();
	//�������
	searchPanel = new SearchPanel(this);
	//���ý���
	configDialog = new ConfigDialog(this);
	//����CZPlayer
	aboutPanel = new AboutPanel;
	//ȫ���ȼ�����
	globalHotKey = new GlobalHotKey(this);

	//���ø������
	ClientBaseOperating::initLrcPropery();
	//����ȫ���ȼ�
	ClientBaseOperating::initHotKeyPropery();

	//�����豸�����߳�
	hotpugWatcherThread = new HotplugWatcherThread;
	//hotpugWatcherThread ->start();//���߳��ڲ��Ѿ�start

	//����һ����ʱ��
	timer = new QTimer(this);
	timer ->start(30);
	shortCutIimer = new QTimer(this);
	shortCutIimer ->start(500);
	upDateTimer = new QTimer(this);
	upDateTimer ->start(1000);

	//�����ݿ��ж�����Ϣ���������б���
	this ->readFromDb();

	//���������С��λ��
	albumImage ->setGeometry(25, 23, 110, 103);
	m_spectrograph ->setGeometry(155, 47, 190, 78);
	m_waveform ->setGeometry(155, 47, 190, 78);
	InfoLabel ->setGeometry(153, 69, 240, 39);
	timeLabel ->setGeometry(295, 23, 82, 18);
	currentMusicLabel ->setGeometry(170, 4, 290, 11);
	showButton ->setGeometry(365, 55, 17, 17);
	modeButton ->setGeometry(365, 80, 18, 15);

	preButton ->setGeometry(33, 164, 25, 25);
	playButton ->setGeometry(83, 164, 25, 25);
	endButton ->setGeometry(133, 164, 25, 25);
	nextButton ->setGeometry(183, 164, 25, 25);

	seekSlider ->setGeometry(22, 137, 361, 14);
	openFileButton ->setGeometry(152, 21, 49, 23);
	musicListButton ->setGeometry(201, 20, 35, 24);
	lrcButton ->setGeometry(236, 21, 44, 23);
	volumeButton ->setGeometry(249, 170, 16, 16);
	volumeSlider ->setGeometry(273, 170, 108, 15);
}

//�ź����
void Widget::connectUI()
{
	connect(openFileButton, SIGNAL(clicked()), this, SLOT(slot_OpenFile()));									//�������ļ�
	connect(musicListButton, SIGNAL(clicked()), this, SLOT(slot_OpenMusicList()));								//�������б����
	connect(lrcButton, SIGNAL(clicked()), this, SLOT(slot_OpenMusicLrc()));										//�򿪸��
	connect(showButton, SIGNAL(clicked()), this, SLOT(slot_OpenSearchPanel()));								//�򿪸������ؽ���
	connect(trayIcon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)),
		this, SLOT(slot_TrayIconActivated(QSystemTrayIcon::ActivationReason)));									//����ͼ�걻�������д���
	connect(playList, SIGNAL(cellDoubleClicked(int,int)), this, SLOT(slot_TableDoubleClicked(int, int)));		//������˫�������б�
	connect(playList, SIGNAL(sig_PlayListClean()), this, SLOT(slot_ClearSources()));							//���ý��Դ
	connect(playList, SIGNAL(sig_RowSelected(int, QString)), this, SLOT(slot_DelSelectSource(int, QString)));	//ɾ��ѡ�е�Դ
	connect(lrc, SIGNAL(sig_LrcLocked()), this, SLOT(slot_ShowLrcLocked()));									//��ʾ����Ѿ�������Ϣ
	connect(lrc, SIGNAL(sig_LrcUnlocked()), this, SLOT(slot_ShowLrcUnlocked()));								//��ʾ����Ѿ�������Ϣ
	connect(unlockLrcAction, SIGNAL(triggered()), this, SLOT(slot_UnlockLrc()));								//�������
	connect(playButton, SIGNAL(clicked()), this, SLOT(slot_SetPlayPaused()));									//����/��ͣ
	connect(endButton, SIGNAL(clicked()), this, SLOT(slot_SetStop()));											//ֹͣ
	connect(preButton, SIGNAL(clicked()), this, SLOT(slot_SetPre()));											//��һ��
	connect(nextButton, SIGNAL(clicked()), this, SLOT(slot_SetNext()));											//��һ��
	connect(m_engine, SIGNAL(sig_Finished()), this, SLOT(slot_Finished()));										//�������
	connect(minAction, SIGNAL(triggered()), this, SLOT(slot_MinToTray()));										//��С��
	connect(closeAction, SIGNAL(triggered()), this, SLOT(slot_CloseAllProgress()));								//�˳�����
	connect(quitAction, SIGNAL(triggered()), this, SLOT(slot_CloseAllProgress()));								//�˳�����
	connect(pauseAction, SIGNAL(triggered()), this, SLOT(slot_SetPlayPaused()));								//��ͣ(����)
	connect(stopAction, SIGNAL(triggered()), this, SLOT(slot_SetStop()));										//ֹͣ
	connect(lastAction, SIGNAL(triggered()), this, SLOT(slot_SetPre()));										//��һ��
	connect(nextAction, SIGNAL(triggered()), this, SLOT(slot_SetNext()));										//��һ��
	connect(timer, SIGNAL(timeout()), this, SLOT(slot_TimeOut()));												//����һ����ʱ��
	connect(shortCutIimer, SIGNAL(timeout()), this, SLOT(slot_ShortCutsetDisable()));							//����ȫ���ȼ�����/����
	connect(upDateTimer, SIGNAL(timeout()), this, SLOT(slot_UpdateTime()));										//����ʱ��͸��
	connect(openFileAction, SIGNAL(triggered()), this, SLOT(slot_OpenFile()));									//�������ļ�
	connect(openMusicListAction, SIGNAL(triggered()), this, SLOT(slot_OpenMusicList()));						//�����ֲ����б�
	connect(openSearchPanelAnimation, SIGNAL(triggered()), this, SLOT(slot_OpenSearchPanel()));				//�����������б�
	connect(openLrcAction, SIGNAL(triggered()), this, SLOT(slot_OpenMusicLrc()));								//��ʾ������
	connect(configAction, SIGNAL(triggered()), this, SLOT(slot_ConfigCZPlayer()));								//����
	connect(aboutAction, SIGNAL(triggered()), this, SLOT(slot_AboutCZPlayer()));								//����CZPlayer
	connect(aboutQtAction, SIGNAL(triggered()), this, SLOT(slot_AboutQt()));									//����Qt
	connect(minMusicWidget ,SIGNAL(sig_ShowMusicPanel()), this, SLOT(slot_ShowMusicPanel()));					//��ʾ������
	connect(musicListWidget, SIGNAL(sig_AddMusic()), this, SLOT(slot_OpenFile()));								//��Ӹ���
	connect(musicListWidget, SIGNAL(sig_AddMusicList()), this, SLOT(slot_AddMusicList()));						//��Ӳ����б�
	connect(this, SIGNAL(sig_ShowMainWidget()), minMusicWidget, SLOT(slot_OpenMainWidget()));					//���͵����������ʾ������
	connect(volumeButton, SIGNAL(clicked()), this, SLOT(slot_SetMute()));										//���þ���
    connect(volumeSlider, SIGNAL(valueChanged(int)), this, SLOT(slot_SetVolume(int)));							//��������
	connect(modeButton, SIGNAL(clicked()), this, SLOT(slot_ShowModeMenu()));									//��ʾģʽ�˵�
	connect(cycleAction, SIGNAL(triggered()), this, SLOT(slot_CyclePlay()));									//ѭ������
	connect(randomAction, SIGNAL(triggered()), this, SLOT(slot_RandomPlay()));									//�������
	connect(spectrumAction, SIGNAL(triggered()), this, SLOT(slot_SpectrumShow()));								//Ƶ����ʾ
	connect(formAction, SIGNAL(triggered()), this, SLOT(slot_FormShow()));										//������ʾ

	////״̬�ı�
	//connect(m_engine, SIGNAL(stateChanged(QAudio::State,QAudio::State)), this, SLOT(slot_StateChanged(QAudio::State,QAudio::State)));
	////��ʽ�ı�
	connect(m_engine, SIGNAL(formatChanged(const QAudioFormat &)), this, SLOT(slot_FormatChanged(const QAudioFormat &)));
	////��ʾ��Ϣ
	//connect(m_engine, SIGNAL(infoMessage(QString, int)), this, SLOT(slot_InfoMessage(QString, int)));
	////��ʾ������Ϣ
	//connect(m_engine, SIGNAL(errorMessage(QString, QString)), this, SLOT(slot_ErrorMessage(QString, QString)));	//������Ϣ
	////��ʾ��Ϣ
	//connect(m_spectrograph, SIGNAL(infoMessage(QString, int)), this, SLOT(slot_InfoMessage(QString, int)));

	//Ƶ�׸ı�
	connect(m_engine, SIGNAL(spectrumChanged(long long, long long, const FrequencySpectrum &)), this, SLOT(slot_SpectrumChanged(long long, long long, const FrequencySpectrum &)));
	//���θı�
	connect(m_engine, SIGNAL(bufferChanged(long long, long long, const QByteArray &)), m_waveform, SLOT(bufferChanged(long long, long long, const QByteArray &)));
	connect(m_engine, SIGNAL(recordPositionChanged(long long)), m_waveform, SLOT(audioPositionChanged(long long)));
	connect(m_engine, SIGNAL(playPositionChanged(long long)), m_waveform, SLOT(audioPositionChanged(long long)));
}


//�����ݿ��ж��������ļ���Ϣ�������ֲ����б�
void Widget::readFromDb()
{
	QStringList list;
	DBModule::readFromDb(list);
	if (list.isEmpty())
	{
		return;
	}
	this ->loadMusicList(list);	//���ظ����б�
}

//����һ����ʱ��
void Widget::slot_TimeOut()
{
    if (lrc ->text().isEmpty())
    {
        lrc ->stopLrcMask();
    }

    //��������б�Ϊ�գ��򲥷Ű�ť������
    if (playList ->rowCount() == 0)
    {
		playButton ->setEnabled(false);
		endButton  ->setEnabled(false);
		preButton ->setEnabled(false);
		nextButton ->setEnabled(false);

		minMusicWidget ->minPlayButton ->setEnabled(false);
		minMusicWidget ->minPauseButton ->setEnabled(false);
		minMusicWidget ->minStopButton ->setEnabled(false);
		minMusicWidget ->minNextButton ->setEnabled(false);
		minMusicWidget ->minLastButton ->setEnabled(false);

		pauseAction ->setIcon(QIcon(":/images/trayPlay.png"));
		pauseAction ->setText(tr("����"));
		pauseAction ->setEnabled(false);
		stopAction ->setEnabled(false);
		lastAction ->setEnabled(false);
		nextAction ->setEnabled(false);
    }
    else
    {
        playButton ->setEnabled(true);
        endButton  ->setEnabled(true);
        preButton ->setEnabled(true);
        nextButton ->setEnabled(true);

        minMusicWidget ->minPlayButton ->setEnabled(true);
        minMusicWidget ->minPauseButton ->setEnabled(true);
        minMusicWidget ->minStopButton ->setEnabled(true);
        minMusicWidget ->minNextButton ->setEnabled(true);
        minMusicWidget ->minLastButton ->setEnabled(true);

        pauseAction ->setEnabled(true);
        stopAction ->setEnabled(true);
        lastAction ->setEnabled(true);
        nextAction ->setEnabled(true);
    }

	if (nPos == -290)
	{
		nPos = 400;
	}
	currentMusicLabel ->setGeometry(nPos, 7, 290, 11);
	nPos = nPos - 1;
}

//ѡ������
void Widget::slot_ConfigOptions()
{
    if (configDialog ->isHidden())
    {
        this ->slot_ConfigCZPlayer();
    }
}

//��ʾ/����������
void Widget::slot_ShowHideMainWidget()
{
    this ->slot_MinToTray();
    if (this ->isHidden())
    {
        this ->slot_TrayIconActivated(QSystemTrayIcon::Trigger);
    }
}

//����ģʽ/��������
void Widget::slot_MinMainWidget()
{
    this ->slot_OpenMinWidget();
    if (this ->isHidden())
    {
        emit sig_ShowMainWidget();//��ʾ������
    }
}

//�������ļ�
void Widget::slot_OpenMusicfile()
{
    this ->slot_OpenFile();
}

//��ʾ/���������б�
void Widget::slot_ShowDownload()
{
    this ->slot_OpenSearchPanel();
}

//��ʾ/���ظ����б�
void Widget::slot_ShowMusicList()
{
    this ->slot_OpenMusicList();
}

//��ʾ/����������
void Widget::slot_ShowLrc()
{
    this ->slot_OpenMusicLrc();
}

//����/��ͣ
void Widget::slot_PlayPause()
{
    this ->slot_SetPlayPaused();
}

//��һ��
void Widget::slot_Last()
{
    this ->slot_SetPre();
}

//��һ��
void Widget::slot_Next()
{
    this ->slot_SetNext();
}

//���ļ�
void Widget::slot_OpenFile()
{
    //����ͬʱ�򿪶����Ƶ�ļ�
#ifdef _WIN32_WINNT
    QString WINPATH = CZPlayerConfig::getValue("MUSICDIR_WIN").toString();
    QStringList list = QFileDialog::getOpenFileNames(this, tr("�������ļ�"),
                                                     WINPATH, tr("music Files(*.wav)"));
#else
    QString X11PATH =  QDir::homePath() + CZPlayerConfig::getValue("MUSICDIR_X11").toString();
    QStringList list = QFileDialog::getOpenFileNames(this, tr("�������ļ�"),
                                                     X11PATH, tr("music Files(*.wav)"));
#endif

    if (list.isEmpty())
    {
        return;
    }
	this ->loadMusicList(list);	//���ظ����б�
}

//��Ӳ����б�
void Widget::slot_AddMusicList()
{
	//ֻ�ܴ�һ�������б��ļ�
#ifdef _WIN32_WINNT
	QString WINPATH = CZPlayerConfig::getValue("MUSICDIR_WIN").toString();
	QString fileName = QFileDialog::getOpenFileName(this, tr("���벥���б�"),
		WINPATH, tr("music List(*.m3u *.pls)"));
#else
	QString X11PATH =  QDir::homePath() + CZPlayerConfig::getValue("MUSICDIR_X11").toString();
	QString fileName = QFileDialog::getOpenFileName(this, tr("���벥���б�"),
		WINPATH, tr("music List(*.m3u *.pls)"));
#endif

	if (fileName != "")
	{
		QString strSuffixName = fileName.right(3);//�õ���׺��
		////��ȡ��ǰý��Դ�б�Ĵ�С
		//int index = sources.size();
		char buf[256];
		string sBuf;
		int i = 0;
		int n = 1;
		ifstream file(fileName.toStdString());
		while (!file.eof())
		{
			file.getline(buf, 256, '\n');
			sBuf = buf;

			if (i == 0)		//��ȡ��ͷ
			{
				if (sBuf != "#EXTM3U" && sBuf != "[playlist]")
				{
					QMessageBox::information(NULL, tr("��Ϣ"), tr("������Ч��m3u��pls�ļ���"), QMessageBox::Yes);
					return;
				}
			}
			else
			{
				if ((strSuffixName == "m3u" || strSuffixName == "M3U") && i % 2 == 0)
				{
					this ->loadMusicList(QString::fromStdString(sBuf));	//���ظ����б�
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
							this ->loadMusicList(filePath);	//���ظ����б�
						}
					}
				}
			}
			++i;
		}
		file.close();
	}
}

//���ظ����б�
void Widget::loadMusicList( const QStringList &list )
{
	foreach (QString filePath, list)
	{
		this ->loadMusicList(filePath);
	}
}

//���ظ����б�
void Widget::loadMusicList( const QString &filePath )
{
	//���
	QTableWidgetItem *rowItem = new QTableWidgetItem;
	rowItem ->setTextAlignment(Qt::AlignCenter);
	rowItem ->setFont(QFont("΢���ź�", 10));

	//����
	QString fileName = QFileInfo(filePath).baseName();
	QTableWidgetItem *fileNameItem = new QTableWidgetItem(fileName);
	fileNameItem ->setTextAlignment(Qt::AlignCenter);
	fileNameItem ->setFont(QFont("΢���ź�", 10));

	//ʱ��
	WavFile wavFile;
	int nMusicTime = wavFile.getMusicTime(filePath);
	QTime time(0, nMusicTime / 60, nMusicTime % 60);
	QTableWidgetItem *timeItem = new QTableWidgetItem(time.toString("mm:ss"));
	timeItem ->setTextAlignment(Qt::AlignCenter);
	timeItem ->setFont(QFont("΢���ź�", 10));

	//���벥���б�
	int currentRows = playList ->rowCount();//�����б��е�����
	playList ->insertRow(currentRows);//�Ӳ����б��еĵ�ǰ�в���
	rowItem ->setText(QString::number(currentRows + 1));
	playList ->setItem(currentRows, 0, rowItem);
	playList ->setItem(currentRows, 1, fileNameItem);
	playList ->setItem(currentRows, 2, timeItem);

	//����ʱ���м��
	if (currentRows + 1 <= 12)
	{
		playList ->horizontalHeader() ->resizeSection(3, 80);
	}
	else
	{
		playList ->horizontalHeader() ->resizeSection(3, 65);
	}

	//��������Ϣ����vector
	m_mapMusicRows.insert(make_pair(fileName, currentRows));
	m_mapMusicFilePath.insert(make_pair(fileName, filePath));

	//�������ݿ�
	DBModule::insertLine(fileName, time.toString("mm:ss"), filePath);
}

//����С����
void Widget::slot_OpenMinWidget()
{
    if (minMusicWidget ->isHidden())
    {
        QPropertyAnimation *minWidgetdAnimation = new QPropertyAnimation(minMusicWidget, "windowOpacity");
        minWidgetdAnimation ->setDuration(500);
        minWidgetdAnimation ->setStartValue(0);
        minWidgetdAnimation ->setEndValue(1);
        minWidgetdAnimation ->start();
        minMusicWidget ->show();

        QPropertyAnimation *mainWidgetdAnimation = new QPropertyAnimation(this, "windowOpacity");
        mainWidgetdAnimation ->setDuration(500);
        mainWidgetdAnimation ->setStartValue(1);
        mainWidgetdAnimation ->setEndValue(0);
        mainWidgetdAnimation ->start();
        connect(mainWidgetdAnimation, SIGNAL(finished()), this, SLOT(slot_HideMainWidget()));

        QPropertyAnimation *searchPanelAnimation = new QPropertyAnimation(searchPanel, "windowOpacity");
        searchPanelAnimation ->setDuration(500);
        searchPanelAnimation ->setStartValue(1);
        searchPanelAnimation ->setEndValue(0);
        searchPanelAnimation ->start();
        connect(searchPanelAnimation, SIGNAL(finished()), this, SLOT(slot_HideSearchPanel()));

        QPropertyAnimation *musicListAnimation = new QPropertyAnimation(musicListWidget, "windowOpacity");
        musicListAnimation ->setDuration(500);
        musicListAnimation ->setStartValue(1);
        musicListAnimation ->setEndValue(0);
        musicListAnimation ->start();
        connect(musicListAnimation, SIGNAL(finished()), this, SLOT(slot_HideMusicList()));
    }
}

//�򿪲����б�
void Widget::slot_OpenMusicList()
{
    if (musicListWidget ->isHidden())
    {
        musicListWidget ->move(frameGeometry().bottomLeft());//��ʾ����������·�
        QPropertyAnimation *musicListAnimation = new QPropertyAnimation(musicListWidget, "windowOpacity");
        musicListAnimation ->setDuration(500);
        musicListAnimation ->setStartValue(0);
        musicListAnimation ->setEndValue(1);
        musicListAnimation ->start();
        musicListWidget ->show();
        openMusicListAction ->setText(tr("�رղ����б�"));
        musicListShowFlag = 1;
    }
    else
    {
        QPropertyAnimation *musicListAnimation = new QPropertyAnimation(musicListWidget, "windowOpacity");
        musicListAnimation ->setDuration(500);
        musicListAnimation ->setStartValue(1);
        musicListAnimation ->setEndValue(0);
        musicListAnimation ->start();
        connect(musicListAnimation, SIGNAL(finished()), this, SLOT(slot_HideMusicList()));
        openMusicListAction ->setText(tr("�򿪲����б�"));
        musicListShowFlag = 0;
    }
}

//�򿪸��
void Widget::slot_OpenMusicLrc()
{
    if (lrc ->isHidden())
    {
        //lrc ->show();
        lrc ->showNormal();
        openLrcAction ->setText(tr("�ر�������"));
        minMusicWidget ->minMusicLrcAction ->setText(tr("�ر�������"));
    }
    else
    {
        lrc ->hide();
        openLrcAction ->setText(tr("��ʾ������"));
        minMusicWidget ->minMusicLrcAction ->setText(tr("��ʾ������"));
    }
}

//�����ظ�������
void Widget::slot_OpenSearchPanel()
{
    if (searchPanel ->isHidden())
    {
        searchPanel ->move(frameGeometry().topRight());//��ʾ����������ҷ�
        QPropertyAnimation *searchPanelAnimation = new QPropertyAnimation(searchPanel, "windowOpacity");
        searchPanelAnimation ->setDuration(500);
        searchPanelAnimation ->setStartValue(0);
        searchPanelAnimation ->setEndValue(1);
        searchPanelAnimation ->start();
        searchPanel ->show();
        openSearchPanelAnimation ->setText(tr("�ر��������"));
        showButton ->setToolTip(tr("�ر��������"));
        showButton ->setStyleSheet("QPushButton:!hover{border-image: url(:/images/hideButton.png);}"
                                   "QPushButton:hover{border-image: url(:/images/hideButton2.png);}"
                                   "QPushButton:pressed{border-image: url(:/images/hideButton3.png);}");
        musicDownloadShowFlag = 1;
    }
    else
    {
        QPropertyAnimation *searchPanelAnimation = new QPropertyAnimation(searchPanel, "windowOpacity");
        searchPanelAnimation ->setDuration(500);
        searchPanelAnimation ->setStartValue(1);
        searchPanelAnimation ->setEndValue(0);
        searchPanelAnimation ->start();
        connect(searchPanelAnimation, SIGNAL(finished()), this, SLOT(slot_HideSearchPanel()));
        openSearchPanelAnimation ->setText(tr("���������"));
        showButton ->setToolTip(tr("���������"));
        showButton ->setStyleSheet("QPushButton:!hover{border-image: url(:/images/showButton.png);}"
                                   "QPushButton:hover{border-image: url(:/images/showButton2.png);}"
                                   "QPushButton:pressed{border-image: url(:/images/showButton3.png);}");
        musicDownloadShowFlag = 0;
    }
}

//������������
void Widget::slot_HideSearchPanel()
{
    searchPanel ->hide();
}

//�������ֲ������б�
void Widget::slot_HideMusicList()
{
    musicListWidget ->hide();
}

//����������
void Widget::slot_HideMainWidget()
{
    this ->hide();
}

//��ʾ������
void Widget::slot_ShowMusicPanel()
{
    if (musicDownloadShowFlag == 1)
    {
        this ->slot_OpenSearchPanel();
    }
    if (musicListShowFlag == 1)
    {
        this ->slot_OpenMusicList();
    }
}

//����
void Widget::slot_ConfigCZPlayer()
{
    ConfigDialog::contentsWidget ->setCurrentRow(0);//ѡ�е�һҳ
	ClientBaseOperating::initConfigDialog();//��ʼ�����öԻ���
    configDialog ->exec();//ģ̬��ʾ�Ի���
}

//����Qt
void Widget::slot_AboutQt()
{
    QMessageBox::aboutQt(this, "����Qt");
}

//����CZPlayer
void Widget::slot_AboutCZPlayer()
{
	AboutPanel::tabWidget ->setCurrentIndex(0);
	aboutPanel ->exec();//ģ̬��ʾ�Ի���
}

//����LRC��ʣ���stateChanged()������State::Play��(������ǰ���Ÿ�ĸ��)
//��slot_Finished()�����е����˸ú���(��ʼ������һ�׸�ĸ��)
void Widget::resolveLrc()
{
	lrcMap.clear();

	//����ר��ͼƬ
	if (m_strCurrentMusicName.contains(" - "))//�������ַ�������ʽΪ��������� - �ƼҾԣ�
	{
		//��ר��ͼƬ
		QStringList ablumList = m_strCurrentMusicName.split(" - ");
		QString name = ablumList.at(1);//������
		QString artist = ablumList.at(0);//����

#ifdef _WIN32_WINNT
		QString albumImagesName = CZPlayerConfig::getValue("ALBUMDIR_WIN").toString();
#else
		QString albumImagesName = QDir::homePath() + CZPlayerConfig::getValue("ALBUMDIR_X11").toString();
#endif

		QString albumFileName = albumImagesName + "/" + m_strCurrentMusicName + ".jpg";
		QFile fileAlbum(albumFileName);
		if (fileAlbum.open(QIODevice::ReadOnly))//���ļ��ɹ�
		{
			//�������س���û��������
			if (fileAlbum.size() <  4500)
			{
				//����ר��ͼƬ
				AlbumThread *albumThread = new AlbumThread(name, artist, albumImage);
				albumThread ->start();//����һ���߳�
			}
			else
			{
				albumImage ->setPixmap(QPixmap(albumFileName));
			}
		}
		else
		{
			//����ר��ͼƬ
			AlbumThread *albumThread = new AlbumThread(name, artist, albumImage);
			albumThread ->start();//����һ���߳�
		}
	}
	else
	{
		albumImage ->setPixmap(QPixmap(tr(":/images/albumImage.png")));
	}

#ifdef _WIN32_WINNT
	QString lrcDirName = CZPlayerConfig::getValue("LRCDIR_WIN").toString();
#else
	QString lrcDirName = QDir::homePath() + CZPlayerConfig::getValue("LRCDIR_X11").toString();
#endif
	QString lrcFileName = lrcDirName + "/" + m_strCurrentMusicName + ".lrc";
	m_lrcFileName = lrcFileName;

	//�򿪸��
	QFile file(lrcFileName);
	if (!file.open(QIODevice::ReadOnly))//���ظ����ļ�
	{
		lrc ->setText(m_strCurrentMusicName + tr("----�������ظ���ļ�!"));

		if (m_strCurrentMusicName.contains(" - "))
		{
			QStringList list = m_strCurrentMusicName.split(" - ");
			QString musicName = list.at(1);//������
			QString musicArtist = list.at(0);//����

			//���ظ��
			LrcThread *lrcThread = new LrcThread(musicName, musicArtist, m_lrcFileName, lrc);
			lrcThread ->start();//����һ���߳�
			connect(lrcThread, SIGNAL(sig_ResolveLrc()), this, SLOT(slot_ResolveLrc()));
			return;
		}
	}

	/*�������*/

	QString allText = QString(file.readAll());
	file.close();

	//����ʰ��з�Ϊ����б�
	QStringList lrcLines = allText.split("\n");

	//�����ʱ���ǩ�ĸ�ʽ[00:05.54]
	//������ʽ\d{2}��ʾƥ����������
	QRegExp rx("\\[\\d{2}:\\d{2}\\.\\d{2}\\]");

	foreach (QString oneLine, lrcLines)
	{
		QString temp = oneLine;
		temp.replace(rx, "");//�ÿ��ַ����滻������ʽ����ƥ��ĵط�,�����ͻ���˸���ı�

		// Ȼ�����λ�ȡ��ǰ���е�����ʱ���ǩ�����ֱ������ı�����QMap��
		//indexIn()Ϊ���ص�һ��ƥ���λ�ã��������Ϊ-1�����ʾû��ƥ��ɹ�
		//���������pos����Ӧ�ö�Ӧ���Ǹ���ļ�

		int pos = rx.indexIn(oneLine, 0);
		while (pos != -1)//��ʾƥ��ɹ�
		{
			QString cap = rx.cap(0);//���ص�0�����ʽƥ�������

			//��ʱ���ǩת��Ϊʱ����ֵ���Ժ���Ϊ��λ
			QRegExp regexp;

			//��÷���
			regexp.setPattern("\\d{2}(?=:)");
			regexp.indexIn(cap);
			int minute = regexp.cap(0).toInt();

			//�����
			regexp.setPattern("\\d{2}(?=\\.)");
			regexp.indexIn(cap);
			int second = regexp.cap(0).toInt();

			//��ú���
			regexp.setPattern("\\d{2}(?=\\])");
			regexp.indexIn(cap);
			int milliseSecond = regexp.cap(0).toInt();

			//�����ʱ��
			long long totalTime = minute * 60000 + second * 1000 + milliseSecond * 10;

			//��ÿһ�е�ʱ��͸�ʲ��뵽lrcMap��
			lrcMap.insert(totalTime, temp);
			pos += rx.matchedLength();
			pos = rx.indexIn(oneLine, pos);//ƥ��ȫ��
		}
	}

	//���lrcMapΪ��
	if (lrcMap.isEmpty())
	{
		lrc ->setText(m_strCurrentMusicName + tr("----����ļ����ݴ���!"));
		return;
	}
}

//���ý������
void Widget::slot_ResolveLrc()
{
	this ->resolveLrc();//������ǰ���ظ��
}

//����/��ͣ
void Widget::slot_SetPlayPaused()
{
	if (m_state == Widget::Play)							//��ǰ״̬Ϊ����״̬
	{
		m_engine ->suspend();							//��ͣ����
		this ->setState(Pause);							//���ò���״̬
	}
	else if (m_state == Widget::Pause)						//��ǰ״̬Ϊ��ͣ״̬
	{
		m_engine ->slot_StartPlayback();				//��ʼ����
		this ->setState(Play);							//���ò���״̬
	}
	else if (m_state == Widget::Stop)						//��ǰ״̬Ϊֹͣ״̬
	{
		this ->reset();									//����
		m_engine ->loadFile(m_strCurrentFilePath);		//������������ļ�
		m_engine ->slot_StartPlayback();				//��ʼ����
		this ->setState(Play);							//����״̬
	}
	else if (m_state == Widget::NoState)				//��ǰ״̬Ϊ��״̬
	{
		QTableWidgetItem *item = playList ->item(0, 1);
		map<QString, QString>::iterator iter2 = m_mapMusicFilePath.find(item ->text());
		if (iter2 != m_mapMusicFilePath.end())
		{
			this ->reset();									//����
			nTime = 0;				

			m_engine ->loadFile(iter2 ->second);			//������������ļ�
			m_engine ->slot_StartPlayback();				//��ʼ����
			this ->setState(Play);							//����״̬
			m_strCurrentMusicName = item ->text();			//��ǰ���ŵĸ�����
			m_strCurrentFilePath = iter2 ->second;			//��ǰ���ŵĸ���·��
			m_nCurrentMusicRow = 0;							//��ǰ���ŵĸ����к�
			m_strCurrentMusicTime = playList ->item(m_nCurrentMusicRow, 2) ->text();//��ǰ���ŵĸ���ʱ��
			this ->stateChanged();							//״̬�ı䣬���½���
			this ->rowChanged();							//ѡ�в���ý����
			playList ->setCurrentMusicRow(m_nCurrentMusicRow);	//���õ�ǰ���Ÿ����к�
		}
	}
	this ->stateChanged();								//״̬�ı䣬���½���
}

//����
void Widget::slot_SetPlay()
{
	if (m_state == Widget::Pause)						//��ǰ״̬Ϊ��ͣ״̬
	{
		m_engine ->slot_StartPlayback();				//��ʼ����
		this ->setState(Play);							//���ò���״̬
	}
	else if (m_state == Widget::Stop)					//��ǰ״̬Ϊֹͣ״̬
	{
		this ->reset();									//����
		m_engine ->loadFile(m_strCurrentFilePath);		//������������ļ�
		m_engine ->slot_StartPlayback();				//��ʼ����
		this ->setState(Play);							//����״̬
	}
	else if (m_state == Widget::NoState)				//��ǰ״̬Ϊ��״̬
	{
		QTableWidgetItem *item = playList ->item(0, 1);
		map<QString, QString>::iterator iter2 = m_mapMusicFilePath.find(item ->text());
		if (iter2 != m_mapMusicFilePath.end())
		{
			this ->reset();									//����
			nTime = 0;				

			m_engine ->loadFile(iter2 ->second);			//������������ļ�
			m_engine ->slot_StartPlayback();				//��ʼ����
			this ->setState(Play);							//����״̬
			m_strCurrentMusicName = item ->text();			//��ǰ���ŵĸ�����
			m_strCurrentFilePath = iter2 ->second;			//��ǰ���ŵĸ���·��
			m_nCurrentMusicRow = 0;							//��ǰ���ŵĸ����к�
			m_strCurrentMusicTime = playList ->item(m_nCurrentMusicRow, 2) ->text();//��ǰ���ŵĸ���ʱ��
			this ->stateChanged();							//״̬�ı䣬���½���
			this ->rowChanged();							//ѡ�в���ý����
			playList ->setCurrentMusicRow(m_nCurrentMusicRow);	//���õ�ǰ���Ÿ����к�
		}
	}
	this ->stateChanged();								//״̬�ı䣬���½���
}

//��ͣ
void Widget::slot_SetPause()
{
	if (m_state == Widget::Play)							//��ǰ״̬Ϊ����״̬
	{
		m_engine ->suspend();							//��ͣ����
		this ->setState(Pause);							//���ò���״̬
	}
	this ->stateChanged();								//״̬�ı䣬���½���
}

//ֹͣ����
void Widget::slot_SetStop()
{
	if (m_state == Widget::Play || m_state == Widget::Pause)
	{
		this ->reset();
		this ->setState(Stop);
		this ->stateChanged();							//״̬�ı䣬���½���
	}
}

//������һ��
void Widget::slot_SetPre()
{
	albumImage ->setPixmap(QPixmap(tr("")));
	albumImage ->setMovie(movie);
	movie ->start();

	//ѭ������ģʽ
	if (m_mode == Widget::cyclePlay)
	{
		if (m_nCurrentMusicRow - 1 != -1)
		{
			QTableWidgetItem *item = playList ->item(m_nCurrentMusicRow - 1, 1);
			map<QString, QString>::iterator iter2 = m_mapMusicFilePath.find(item ->text());
			if (iter2 != m_mapMusicFilePath.end())
			{
				this ->reset();									//����
				nTime = 0;

				m_engine ->loadFile(iter2 ->second);			//������������ļ�
				m_engine ->slot_StartPlayback();				//��ʼ����
				this ->setState(Play);							//����״̬
				m_strCurrentMusicName = item ->text();			//��ǰ���ŵĸ�����
				m_strCurrentFilePath = iter2 ->second;			//��ǰ���ŵĸ���·��
				m_nCurrentMusicRow = m_nCurrentMusicRow - 1;	//��ǰ���ŵĸ����к�
				m_strCurrentMusicTime = playList ->item(m_nCurrentMusicRow, 2) ->text();//��ǰ���ŵĸ���ʱ��
				this ->stateChanged();							//״̬�ı䣬���½���
				this ->rowChanged();							//ѡ�в���ý����
				playList ->setCurrentMusicRow(m_nCurrentMusicRow);	//���õ�ǰ���Ÿ����к�
			}
		}
		else	//�������һ��
		{
			QTableWidgetItem *item = playList ->item(playList ->rowCount() - 1, 1);
			map<QString, QString>::iterator iter2 = m_mapMusicFilePath.find(item ->text());
			if (iter2 != m_mapMusicFilePath.end())
			{
				this ->reset();									//����
				nTime = 0;

				m_engine ->loadFile(iter2 ->second);			//������������ļ�
				m_engine ->slot_StartPlayback();				//��ʼ����
				this ->setState(Play);							//����״̬
				m_strCurrentMusicName = item ->text();			//��ǰ���ŵĸ�����
				m_strCurrentFilePath = iter2 ->second;			//��ǰ���ŵĸ���·��
				m_nCurrentMusicRow = playList ->rowCount() - 1;	//��ǰ���ŵĸ����к�
				m_strCurrentMusicTime = playList ->item(m_nCurrentMusicRow, 2) ->text();//��ǰ���ŵĸ���ʱ��
				this ->stateChanged();							//״̬�ı䣬���½���
				this ->rowChanged();							//ѡ�в���ý����
				playList ->setCurrentMusicRow(m_nCurrentMusicRow);	//���õ�ǰ���Ÿ����к�
			}
		}
	}
	//�������
	else if (m_mode == Widget::randomPlay)
	{
		int nRow = qrand() % playList ->rowCount();				//����������ŵ�����
		QTableWidgetItem *item = playList ->item(nRow, 1);
		map<QString, QString>::iterator iter2 = m_mapMusicFilePath.find(item ->text());
		if (iter2 != m_mapMusicFilePath.end())
		{
			this ->reset();									//����
			nTime = 0;				

			m_engine ->loadFile(iter2 ->second);			//������������ļ�
			m_engine ->slot_StartPlayback();				//��ʼ����
			this ->setState(Play);							//����״̬
			m_strCurrentMusicName = item ->text();			//��ǰ���ŵĸ�����
			m_strCurrentFilePath = iter2 ->second;			//��ǰ���ŵĸ���·��
			m_nCurrentMusicRow = nRow;						//��ǰ���ŵĸ����к�
			m_strCurrentMusicTime = playList ->item(m_nCurrentMusicRow, 2) ->text();//��ǰ���ŵĸ���ʱ��
			this ->stateChanged();							//״̬�ı䣬���½���
			this ->rowChanged();							//ѡ�в���ý����
			playList ->setCurrentMusicRow(m_nCurrentMusicRow);	//���õ�ǰ���Ÿ����к�
		}
	}
}

//������һ��
void Widget::slot_SetNext()
{
	albumImage ->setPixmap(QPixmap(tr("")));
	albumImage ->setMovie(movie);
	movie ->start();

	//ѭ������ģʽ
	if (m_mode == Widget::cyclePlay)
	{
		if (m_nCurrentMusicRow + 1 != playList ->rowCount())
		{
			QTableWidgetItem *item = playList ->item(m_nCurrentMusicRow + 1, 1);
			map<QString, QString>::iterator iter2 = m_mapMusicFilePath.find(item ->text());
			if (iter2 != m_mapMusicFilePath.end())
			{
				this ->reset();									//����
				nTime = 0;				

				m_engine ->loadFile(iter2 ->second);			//������������ļ�
				m_engine ->slot_StartPlayback();				//��ʼ����
				this ->setState(Play);							//����״̬
				m_strCurrentMusicName = item ->text();			//��ǰ���ŵĸ�����
				m_strCurrentFilePath = iter2 ->second;			//��ǰ���ŵĸ���·��
				m_nCurrentMusicRow = m_nCurrentMusicRow + 1;	//��ǰ���ŵĸ����к�
				m_strCurrentMusicTime = playList ->item(m_nCurrentMusicRow, 2) ->text();//��ǰ���ŵĸ���ʱ��
				this ->stateChanged();							//״̬�ı䣬���½���
				this ->rowChanged();							//ѡ�в���ý����
				playList ->setCurrentMusicRow(m_nCurrentMusicRow);	//���õ�ǰ���Ÿ����к�
			}
		}
		else	//����ѭ���б�
		{
			QTableWidgetItem *item = playList ->item(0, 1);
			map<QString, QString>::iterator iter2 = m_mapMusicFilePath.find(item ->text());
			if (iter2 != m_mapMusicFilePath.end())
			{
				this ->reset();									//����
				nTime = 0;				

				m_engine ->loadFile(iter2 ->second);			//������������ļ�
				m_engine ->slot_StartPlayback();				//��ʼ����
				this ->setState(Play);							//����״̬
				m_strCurrentMusicName = item ->text();			//��ǰ���ŵĸ�����
				m_strCurrentFilePath = iter2 ->second;			//��ǰ���ŵĸ���·��
				m_nCurrentMusicRow = 0;							//��ǰ���ŵĸ����к�
				m_strCurrentMusicTime = playList ->item(m_nCurrentMusicRow, 2) ->text();//��ǰ���ŵĸ���ʱ��
				this ->stateChanged();							//״̬�ı䣬���½���
				this ->rowChanged();							//ѡ�в���ý����
				playList ->setCurrentMusicRow(m_nCurrentMusicRow);	//���õ�ǰ���Ÿ����к�
			}
		}
	}
	//�������
	else if (m_mode == Widget::randomPlay)
	{
		int nRow = qrand() % playList ->rowCount();				//����������ŵ�����
		QTableWidgetItem *item = playList ->item(nRow, 1);
		map<QString, QString>::iterator iter2 = m_mapMusicFilePath.find(item ->text());
		if (iter2 != m_mapMusicFilePath.end())
		{
			this ->reset();									//����
			nTime = 0;				

			m_engine ->loadFile(iter2 ->second);			//������������ļ�
			m_engine ->slot_StartPlayback();				//��ʼ����
			this ->setState(Play);							//����״̬
			m_strCurrentMusicName = item ->text();			//��ǰ���ŵĸ�����
			m_strCurrentFilePath = iter2 ->second;			//��ǰ���ŵĸ���·��
			m_nCurrentMusicRow = nRow;						//��ǰ���ŵĸ����к�
			m_strCurrentMusicTime = playList ->item(m_nCurrentMusicRow, 2) ->text();//��ǰ���ŵĸ���ʱ��
			this ->stateChanged();							//״̬�ı䣬���½���
			this ->rowChanged();							//ѡ�в���ý����
			playList ->setCurrentMusicRow(m_nCurrentMusicRow);	//���õ�ǰ���Ÿ����к�
		}
	}
}

//������˫�����ֲ����б�ѡ����
void Widget::slot_TableDoubleClicked(int row, int /*column*/)
{
	albumImage ->setPixmap(QPixmap(tr("")));
	albumImage ->setMovie(movie);
	movie ->start();

	QTableWidgetItem *item = playList ->item(row, 1);
	map<QString, QString>::iterator iter = m_mapMusicFilePath.find(item ->text());
	if (iter != m_mapMusicFilePath.end())
	{
		this ->reset();									//����
		nTime = 0;

		m_engine ->loadFile(iter ->second);				//������������ļ�
		m_engine ->slot_StartPlayback();				//��ʼ����
		this ->setState(Play);							//����״̬
		m_strCurrentMusicName = item ->text();			//��ǰ���ŵĸ�����
		m_strCurrentFilePath = iter ->second;			//��ǰ���ŵĸ���·��
		m_nCurrentMusicRow = row;						//��ǰ���ŵĸ����к�
		m_strCurrentMusicTime = playList ->item(m_nCurrentMusicRow, 2) ->text();//��ǰ���ŵĸ���ʱ��
		this ->stateChanged();							//״̬�ı䣬���½���
		playList ->setCurrentMusicRow(m_nCurrentMusicRow);	//���õ�ǰ���Ÿ����к�
	}
}

//�������
void Widget::slot_Finished()
{
	this ->slot_SetNext();	//������һ��
}

//ѡ�е�ǰ����ý��Դ����
void Widget::rowChanged()
{
	playList ->selectRow(m_nCurrentMusicRow);
}

//���ý��Դ����ɾ�����ݿ�ȫ�����ݺ�map
void Widget::slot_ClearSources()
{
	//ֹͣ��ǰ����
	this ->slot_SetStop();

	if (!DBModule::clearDB())//������ݿ�
	{
		QMessageBox::information(NULL, tr("��Ϣ"), tr("������ݿ�ʧ�ܣ�"), QMessageBox::Yes);
	}

    m_mapMusicRows.clear();
    albumImage ->setPixmap(QPixmap(tr(":/images/albumImage.png")));
	currentMusicLabel ->setText(tr("Welecome to CZPlayer!"));
}

//ɾ�����ݿ���Ӧ������
void Widget::slot_DelSelectSource(int row, QString musicName)
{
    //���ɾ���ĵ�ǰ��Ϊ���ڲ��ŵ���һ�У���ֹͣ�����ֵĲ���
	if (row == m_nCurrentMusicRow)
	{
		this ->slot_SetStop();
	}
	if (!DBModule::delLine(musicName))//ɾ��һ��
	{
		QMessageBox::information(NULL, tr("��Ϣ"), tr("ɾ���ø���ʧ�ܣ�"), QMessageBox::Yes);
		return;
	}
    //����map
    this ->updateMap();
}

//����map
void Widget::updateMap()
{
    m_mapMusicRows.clear();
    for (int index = 0; index < playList ->rowCount(); ++index)
    {
        QTableWidgetItem *musicNameItem = playList ->item(index, 1);
        QString musicName = musicNameItem ->text();//������
        m_mapMusicRows.insert(make_pair(musicName, index));
    }
}

//��ý�岥�ſ����ʱ���ᷢ��aboutToFininsh()�ź�
//void Widget::slot_AboutMusicFinish()
//{
//    int index = sources.indexOf(mediaObject ->currentSource()) + 1;
//    if (sources.size() > index)
//    {
//        mediaObject ->setCurrentSource(sources.at(index));
//        mediaObject ->play();
//        lrc ->stopLrcMask();//ֹͣ�������
//        resolveLrc(sources.at(index).fileName());//������һ�׸�ĸ��
//    }
//    else
//    {
//        mediaObject ->enqueue(sources.at(0));
//        mediaObject ->play();
//        lrc ->stopLrcMask();//ֹͣ�������
//        resolveLrc(sources.at(0).fileName());//������һ�׸�ĸ��
//        QString fileName = QFileInfo(sources.at(0).fileName()).baseName();
//    }
//}

//����״̬�ı�(���½���)
void Widget::stateChanged()
{
	//����״̬Ϊ����״̬ʱ������һЩ�ؼ���״̬
	if (m_state == Widget::Play)
	{
		playButton ->setStyleSheet("QPushButton:!hover{border-image: url(:/images/stopButton.png);}"
			"QPushButton:hover{border-image: url(:/images/stopButton2.png);}"
			"QPushButton:pressed{border-image: url(:/images/stopButton3.png);}");
		playButton ->setToolTip(tr("��ͣ"));
		pauseAction ->setIcon(QIcon(":/images/trayPause.png"));
		pauseAction ->setText(tr("��ͣ"));
		InfoLabel ->setVisible(false);
		trayIcon ->setToolTip(tr("���ڲ���:%1").arg(m_strCurrentMusicName));
		currentMusicLabel ->setText(tr("���ڲ��ţ�") + m_strCurrentMusicName);
		this ->resolveLrc();//�������ڲ��ŵĸ����ĸ��
	}
	else if (m_state == Widget::Pause)	//��״̬Ϊ��ͣ״̬
	{
		playButton ->setStyleSheet("QPushButton:!hover{border-image: url(:/images/playButton.png);}"
			"QPushButton:hover{border-image: url(:/images/playButton2.png);}"
			"QPushButton:pressed{border-image: url(:/images/playButton3.png);}");
		playButton ->setToolTip(tr("����"));
		pauseAction ->setIcon(QIcon(":/images/trayPlay.png"));
		pauseAction ->setText(tr("����"));
		InfoLabel ->setText(false);
		trayIcon ->setToolTip(tr("CZPlayer"));
		currentMusicLabel ->setText(tr("��ͣ���ţ�") + m_strCurrentMusicName);
		lrc ->stopLrcMask();
		lrc ->setText(m_strCurrentMusicName);
	}
	else if (m_state == Widget::Stop)	//��״̬Ϊֹͣ����
	{
		playButton ->setStyleSheet("QPushButton:!hover{border-image: url(:/images/playButton.png);}"
			"QPushButton:hover{border-image: url(:/images/playButton2.png);}"
			"QPushButton:pressed{border-image: url(:/images/playButton3.png);}");
		playButton ->setToolTip(tr("����"));
		pauseAction ->setIcon(QIcon(":/images/trayPlay.png"));
		pauseAction ->setText(tr("����"));
		InfoLabel ->setVisible(true);
		trayIcon ->setToolTip(tr("CZPlayer"));
		InfoLabel ->setText(tr("Welcome to CZPlayer!"));
		currentMusicLabel ->setText(tr("ֹͣ���ţ�") + m_strCurrentMusicName);
		lrc ->stopLrcMask();
		lrc ->setText(tr("CZPlayer"));
	}
	this ->slot_UpdateTime();	//����ʱ��͸��
}

//����ʱ��͸��
void Widget::slot_UpdateTime()
{
	//����״̬Ϊ����״̬ʱ������һЩ�ؼ���״̬
	if (m_state == Widget::Play)
	{
		//���¹�����
		QTime qTime;
		int ntotalTime = qTime.secsTo(QTime::fromString(m_strCurrentMusicTime, "mm:ss"));
		seekSlider ->setRange(0, ntotalTime);
		seekSlider ->setValue(nTime);

		//����ʱ��
		 QTime currentTime(0, nTime / 60, nTime % 60);
		 timeLabel ->setText(currentTime.toString("mm:ss") + "/" + m_strCurrentMusicTime);
		 if (currentTime.toString("mm:ss") != m_strCurrentMusicTime)
		 {
			++nTime;
		 }
	}
	else if (m_state == Widget::Pause)	//��״̬Ϊ��ͣ״̬
	{
		//���¹�����
		QTime qTime;
		int ntotalTime = qTime.secsTo(QTime::fromString(m_strCurrentMusicTime, "mm:ss"));
		seekSlider ->setRange(0, ntotalTime);
		seekSlider ->setValue(nTime);

		//����ʱ��
		QTime currentTime(0, nTime / 60, nTime % 60);
		timeLabel ->setText(currentTime.toString("mm:ss") + "/" + m_strCurrentMusicTime);
	}
	else if (m_state == Widget::Stop)	//��״̬Ϊֹͣ����
	{
		seekSlider ->setValue(0);
		timeLabel ->setText(tr("00:00/00:00"));
		nTime = 0;
	}

	//��ȡ��ǰʱ���Ӧ�ĸ��
	if (!lrcMap.isEmpty())
	{
		//��ȡ��ǰʱ���ڸ���е�ǰ������ʱ���
		long long previousTime = 0;
		long long laterTime = 0;

		//keys()��������lrcMap�б�
		foreach (long long value, lrcMap.keys())
		{
			if (nTime >= value / 1000)
			{
				previousTime = value;
			}
			else
			{
				laterTime = value;
				break;
			}
		}

		//��ȡ��ǰʱ������Ӧ�ĸ������
		QString currentLrc = lrcMap.value(previousTime);

		// ������µ�һ�и�ʣ���ô���¿�ʼ��ʾ�������
		if (currentLrc != lrc ->text())
		{
			lrc ->setText(currentLrc);
			long long intervalTime = laterTime - previousTime;//ʱ����
			lrc ->startLrcMask(intervalTime);//�����������
		}
	}
	else
	{
		//lrc ->setText(m_strCurrentMusicName + tr("----δ�ҵ�����ļ�!"));
		lrc ->setText("Welcome to CZPlayer!");
	}
}

//��С��������
void Widget::slot_MinToTray()
{
    if (musicListWidget ->isVisible())
    {
        QPropertyAnimation *musicListAnimation = new QPropertyAnimation(musicListWidget, "windowOpacity");
        musicListAnimation ->setDuration(500);
        musicListAnimation ->setStartValue(1);
        musicListAnimation ->setEndValue(0);
        musicListAnimation ->start();
        connect(musicListAnimation, SIGNAL(finished()), this, SLOT(slot_HideMusicList()));
    }
    if (searchPanel ->isVisible())
    {
        QPropertyAnimation *searchPanelAnimation = new QPropertyAnimation(searchPanel, "windowOpacity");
        searchPanelAnimation ->setDuration(500);
        searchPanelAnimation ->setStartValue(1);
        searchPanelAnimation ->setEndValue(0);
        searchPanelAnimation ->start();
        connect(searchPanelAnimation, SIGNAL(finished()), this, SLOT(slot_HideSearchPanel()));
    }
    if (this ->isVisible())
    {
        QPropertyAnimation *mainWidgetAnimation = new QPropertyAnimation(this, "windowOpacity");
        mainWidgetAnimation ->setDuration(500);
        mainWidgetAnimation ->setStartValue(1);
        mainWidgetAnimation ->setEndValue(0);
        mainWidgetAnimation ->start();
        connect(mainWidgetAnimation, SIGNAL(finished()), this, SLOT(slot_HideMainWidget()));
        trayIcon ->showMessage(tr("CZPlayer"), tr("���������»ص�������"));
    }
}

//�ر����г���
void Widget::slot_CloseAllProgress()
{
    //    mediaObject ->stop();
    //    sources.clear();

    QPropertyAnimation *mainWidgetAnimation = new QPropertyAnimation(this, "windowOpacity");
    mainWidgetAnimation ->setDuration(500);
    mainWidgetAnimation ->setStartValue(1);
    mainWidgetAnimation ->setEndValue(0);
    mainWidgetAnimation ->start();
    connect(mainWidgetAnimation, SIGNAL(finished()), this, SLOT(close()));

    QPropertyAnimation *musicListAnimation = new QPropertyAnimation(musicListWidget, "windowOpacity");
    musicListAnimation ->setDuration(500);
    musicListAnimation ->setStartValue(1);
    musicListAnimation ->setEndValue(0);
    musicListAnimation ->start();
    connect(musicListAnimation, SIGNAL(finished()), this, SLOT(close()));

    QPropertyAnimation *searchPanelAnimation = new QPropertyAnimation(searchPanel, "windowOpacity");
    searchPanelAnimation ->setDuration(500);
    searchPanelAnimation ->setStartValue(1);
    searchPanelAnimation ->setEndValue(0);
    searchPanelAnimation ->start();
    connect(searchPanelAnimation, SIGNAL(finished()), this, SLOT(close()));

    QPropertyAnimation *lrcAnimation = new QPropertyAnimation(lrc, "windowOpacity");
    lrcAnimation ->setDuration(500);
    lrcAnimation ->setStartValue(1);
    lrcAnimation ->setEndValue(0);
    lrcAnimation ->start();
    connect(lrcAnimation, SIGNAL(finished()), this, SLOT(close()));

    minMusicWidget ->close();
    //    QPropertyAnimation *minWidgetAnimation = new QPropertyAnimation(minMusicWidget, "windowOpacity");
    //    minWidgetAnimation ->setDuration(500);
    //    minWidgetAnimation ->setStartValue(1);
    //    minWidgetAnimation ->setEndValue(0);
    //    minWidgetAnimation ->start();
    //    connect(minWidgetAnimation, SIGNAL(finished()), this, SLOT(close()));
}

//ϵͳ����ͼ�걻����
void Widget::slot_TrayIconActivated(QSystemTrayIcon::ActivationReason activationReason)
{
    if (activationReason == QSystemTrayIcon::Trigger)
    {
        QPropertyAnimation *mainWidgetAnimation = new QPropertyAnimation(this, "windowOpacity");
        mainWidgetAnimation ->setDuration(500);
        mainWidgetAnimation ->setStartValue(0);
        mainWidgetAnimation ->setEndValue(1);
        mainWidgetAnimation ->start();
        this ->show();//��ʾ������
        this ->slot_ShowMusicPanel();
    }
}

//��С��
void Widget::slot_ShowMinSize()
{
    this ->showMinimized();
    musicListWidget ->showMinimized();
    searchPanel ->showMinimized();
}

//��ʾ����Ѿ�������Ϣ
void Widget::slot_ShowLrcLocked()
{
    trayIcon ->showMessage(tr("��ʾ"), tr("����������Ѿ�����,���������ͼ���Ҽ����Խ���"));
}

//��ʾ����Ѿ�������Ϣ
void Widget::slot_ShowLrcUnlocked()
{
    trayIcon ->showMessage(tr("��ʾ"), tr("����������Ѿ�����"));
}

//�������
void Widget::slot_UnlockLrc()
{
    lrc ->unlockLrc();//�������
}

////��λ����ǰ����
//void Widget::locateCurrentMusic()
//{
//    int index = sources.indexOf(mediaObject ->currentSource());
//    playList ->selectRow(index);
//}

//����ȫ���ȼ�����/����
void Widget::slot_ShortCutsetDisable()
{
	if (minMusicWidget ->isHidden())
	{
		//���������ȼ�
		GlobalHotKey::showHideMainWidgetDisabled(false);
		GlobalHotKey::showDownloadDisabled(false);
		GlobalHotKey::showMusicListDisabled(false);
	}
	else
	{
		//���������ȼ�
		GlobalHotKey::showHideMainWidgetDisabled(true);
		GlobalHotKey::showDownloadDisabled(true);
		GlobalHotKey::showMusicListDisabled(true);
	}
}

//����������Ҽ�����¼�
void Widget::contextMenuEvent(QContextMenuEvent *event)
{
	if (CZPlayerConfig::getValue("AUDIOSHOW").toString() == "spectrumShow")	//Ƶ����ʾ
	{
		spectrumAction ->setText(tr("��ʾƵ�� ��"));
		formAction ->setText(tr("��ʾ����"));
	}
	else if (CZPlayerConfig::getValue("AUDIOSHOW").toString() == "formShow")	//������ʾ
	{
		spectrumAction ->setText(tr("��ʾƵ��"));
		formAction ->setText(tr("��ʾ���� ��"));
	}

	audioShowMenu ->addAction(spectrumAction);
	audioShowMenu ->addAction(formAction);

    QMenu menu;
    menu.addAction(closeAction);
    menu.addAction(minAction);
    menu.addAction(QIcon(":/images/minMainWidget.png"), tr("����ģʽ"), this, SLOT(slot_OpenMinWidget()));
	menu.addMenu(audioShowMenu);
    menu.addSeparator();
    menu.addAction(openFileAction);
    menu.addAction(openMusicListAction);
    menu.addAction(openSearchPanelAnimation);
    menu.addAction(openLrcAction);
    menu.addSeparator();
    menu.addAction(configAction);
	menu.addAction(aboutAction);
    menu.addAction(aboutQtAction);
    menu.exec(event ->globalPos());//globalPos()Ϊ��ǰ����λ������
    //menu.exec(QCursor::pos());
    event ->accept();
}

//�Ͻ��¼�
void Widget::dragEnterEvent(QDragEnterEvent *event)
{
    QList<QUrl> urls = event ->mimeData() ->urls();
    foreach (QUrl url, urls)
    {
        QString filePath = url.toLocalFile();
        QStringList fileList = filePath.split('.');
        QString kzName = fileList.at(1);
        if (kzName == "wav" || kzName == "WAV" || kzName == "m3u" || kzName == "M3U" || kzName == "pls" || kzName == "PLS")
        {
            event ->accept();
        }
        else
        {
            event ->ignore();
        }
    }
}

//�����¼�
void Widget::dropEvent(QDropEvent *event)
{
    QList<QUrl> urls = event ->mimeData() ->urls();
    if (urls.isEmpty())
    {
        return;
    }

	foreach (QUrl url, urls)
	{
		QString filePath = url.toLocalFile();

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
						QMessageBox::information(NULL, tr("��Ϣ"), tr("������Ч��m3u��pls�ļ���"), QMessageBox::Yes);
						return;
					}
				}
				else
				{
					if ((strSuffixName == "m3u" || strSuffixName == "M3U") && i % 2 == 0)
					{
						this ->loadMusicList(QString::fromStdString(sBuf));	//���ظ����б�
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
								this ->loadMusicList(filePath);	//���ظ����б�
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
			this ->loadMusicList(filePath);
		}
	}

    event ->accept();
}

//��дpaintEvent,��ӱ���ͼƬ
void Widget::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    QPixmap backgroundImage;
    backgroundImage.load(":/images/mainBg3.png");

    //��ͨ��pix�ķ������ͼƬ�Ĺ��˵�͸���Ĳ��ֵõ���ͼƬ����ΪloginPanel�Ĳ�����߿�
    this ->setMask(backgroundImage.mask());
    painter.drawPixmap(0, 0, 400, 212, backgroundImage);

	////���ұ�
	//static const int sineTable[16] = { 0, 38, 71, 92, 100, 92, 71, 38,	0, -38, -71, -92, -100, -92, -71, -38};

	//QFontMetrics metrics(font());	//����ָ��
	//int x = (430 - metrics.width(lrcText)) / 2;
	//int y = (250 + metrics.ascent() - metrics.descent()) / 2;//���� - ����
	//QColor color;

	//for (int i = 0; i < lrcText.size(); ++i)
	//{
	//	int index = (step + i) % 16;
	//	color.setHsv((15 - index) * 16, 255, 191);
	//	painter.setPen(color);
	//	painter.drawText(x, y - ((sineTable[index] * metrics.height()) / 400), QString(lrcText[i]));
	//	x += metrics.width(lrcText[i]);
	//}

    event ->accept();
}

//��дmousePressEvent��mouseMoveEventʵ�ִ�����ƶ�
void Widget::mousePressEvent(QMouseEvent *event)
{
    if (event ->button() == Qt::LeftButton)
    {
        dragPosition = event ->globalPos() - frameGeometry().topLeft();
        event ->accept();
    }
}

//��дmousePressEvent��mouseMoveEventʵ�ִ�����ƶ�
void Widget::mouseMoveEvent(QMouseEvent *event)
{
    if (event ->buttons() == Qt::LeftButton)
    {
        this ->move(event ->globalPos() - dragPosition);//�ƶ�����
        musicListWidget ->move(frameGeometry().bottomLeft());//�ƶ������ʱ�����ֲ����б�����ƶ�
        searchPanel ->move(frameGeometry().topRight());//�ƶ������ʱ���������ؽ�������ƶ�
        event ->accept();
    }
}

//���ò���״̬
void Widget::setState( State state )
{
	m_state = state;
}

//���ò���ģʽ
void Widget::setMode( Mode mode )
{
	m_mode = mode;
}

//����
void Widget::reset()
{
	m_engine ->reset();			//���ò�������
	m_spectrograph ->reset();	//����Ƶ�׻�������
	m_waveform ->reset();		//���ò��λ�������
}

//״̬�ı�
//void Widget::slot_StateChanged( QAudio::State mode, QAudio::State state )
//{
//
//}

//audio��ʽ�ı�
void Widget::slot_FormatChanged( const QAudioFormat &format )
{
	if (QAudioFormat() != format) 
	{
		//��ʼ�����λ�������
		m_waveform ->initialize(format, WaveformTileLength, WaveformWindowDuration);
	}
}

//Ƶ�׸ı�
void Widget::slot_SpectrumChanged( long long position, long long length, const FrequencySpectrum &spectrum )
{
	m_spectrograph ->spectrumChanged(spectrum);	//�ı�Ƶ��
}

//��Ϣ
void Widget::slot_InfoMessage( const QString &message, int timeoutMs )
{

}

void Widget::slot_ErrorMessage( const QString &heading, const QString &detail )
{

}

//��������
void Widget::slot_SetVolume(int value)
{
	if (CZPlayerConfig::getValue("SETMUTE").toString() == "true")
	{
		volumeButton ->setToolTip(tr("����"));
		volumeButton ->setStyleSheet("QPushButton:!hover{border-image: url(:/images/soundButton.png);}"
			"QPushButton:hover{border-image: url(:/images/soundButton2.png);}"
			"QPushButton:pressed{border-image: url(:/images/soundButton3.png);}");
	}
	SoundControl::SetVolume(0, value);
	CZPlayerConfig::setValue("VOLUMEVALUE", QString::number(value));
	volumeSlider ->setToolTip(tr("��ǰ������%1%").arg(value));
}

//���þ���
void Widget::slot_SetMute()
{
	if (CZPlayerConfig::getValue("SETMUTE").toString() == "false")
	{
		SoundControl::SetVolume(0, 0);	//����
		volumeButton ->setToolTip(tr("�ָ�����"));
		volumeButton ->setStyleSheet("QPushButton:!hover{border-image: url(:/images/soundButton4.png);}"
			"QPushButton:hover{border-image: url(:/images/soundButton5.png);}"
			"QPushButton:pressed{border-image: url(:/images/soundButton6.png);}");
		CZPlayerConfig::setValue("SETMUTE", "true");
	}
	else
	{
		SoundControl::SetVolume(0, volumeSlider ->value());	//�ָ�
		volumeButton ->setToolTip(tr("����"));
		volumeButton ->setStyleSheet("QPushButton:!hover{border-image: url(:/images/soundButton.png);}"
			"QPushButton:hover{border-image: url(:/images/soundButton2.png);}"
			"QPushButton:pressed{border-image: url(:/images/soundButton3.png);}");
		CZPlayerConfig::setValue("SETMUTE", "false");
	}
}

//��ʾģʽ�˵�
void Widget::slot_ShowModeMenu()
{
	if (ClientBaseOperating::initPlayMode() == "cyclePlay")	//ѭ������
	{
		cycleAction ->setText(tr("ѭ������ ��"));
		randomAction ->setText(tr("�������"));
	}
	else if (ClientBaseOperating::initPlayMode() == "randomPlay")	//�������
	{
		cycleAction ->setText(tr("ѭ������"));
		randomAction ->setText(tr("������� ��"));
	}

	QMenu menu;
	menu.addAction(cycleAction);
	menu.addAction(randomAction);
	menu.exec(QCursor::pos());
}

//ѭ������
void Widget::slot_CyclePlay()
{
	CZPlayerConfig::setValue("PLAYMODE", "cyclePlay");
	cycleAction ->setText(tr("ѭ������ ��"));
	randomAction ->setText(tr("�������"));
	modeButton ->setStyleSheet("QPushButton:!hover{border-image: url(:/images/cyclePlay.png);}"
		"QPushButton:hover{border-image: url(:/images/cyclePlay2.png);}"
		"QPushButton:pressed{border-image: url(:/images/cyclePlay3.png);}");
	modeButton ->setToolTip(tr("ѭ������"));
	this ->setMode(cyclePlay);					//����ѭ������ģʽ
}

//�������
void Widget::slot_RandomPlay()
{
	CZPlayerConfig::setValue("PLAYMODE", "randomPlay");
	cycleAction ->setText(tr("ѭ������"));
	randomAction ->setText(tr("������� ��"));
	modeButton ->setStyleSheet("QPushButton:!hover{border-image: url(:/images/randomPlay.png);}"
		"QPushButton:hover{border-image: url(:/images/randomPlay2.png);}"
		"QPushButton:pressed{border-image: url(:/images/randomPlay3.png);}");
	modeButton ->setToolTip(tr("�������"));
	this ->setMode(randomPlay);					//�����������ģʽ
}

//��ʾƵ��
void Widget::slot_SpectrumShow()
{
	CZPlayerConfig::setValue("AUDIOSHOW", "spectrumShow");
	spectrumAction ->setText(tr("��ʾƵ�� ��"));
	formAction ->setText(tr("��ʾ����"));
	m_spectrograph ->setVisible(true);
	m_waveform ->setVisible(false);
}

//��ʾ����
void Widget::slot_FormShow()
{
	CZPlayerConfig::setValue("AUDIOSHOW", "formShow");
	spectrumAction ->setText(tr("��ʾƵ�� ��"));
	formAction ->setText(tr("��ʾ����"));
	m_spectrograph ->setVisible(false);
	m_waveform ->setVisible(true);
}