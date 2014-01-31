#include "stdafx.h"
#include "nofocusdelegate.h"
#include "musiclist.h"
#include "DBModule.h"
#include "MusicListSaveFormatEntity.h"

MusicList::MusicList(QWidget *widget) : QTableWidget(widget), parent(widget)
{
    //���ô��ڻ�������
    this ->resize(380, 360);//���ô����С
    //this ->setWindowFlags(Qt::FramelessWindowHint);//ȥ������߿�
    this ->setWindowFlags(Qt::Tool | Qt::FramelessWindowHint);//ȥ������߿�,������������ʾͼ��

    //this ->setAttribute(Qt::WA_TranslucentBackground);//���ñ���͸��
    this ->setWindowIcon(QIcon(":/images/CZPlayer.png"));//����logo
    this ->setWindowTitle(tr("���ֲ����б�"));

    this ->setRowCount(0);//��ʼ��������Ϊ0
    this ->setColumnCount(3);//��ʼ��������Ϊ3

    QStringList headList;
    headList << tr("���") << tr("����") << tr("ʱ��");
    this ->setHorizontalHeaderLabels(headList);//����ͷ��Ϣ

    this ->horizontalHeader() ->setVisible(false);
    this ->verticalHeader() ->setVisible(false);
    //this ->horizontalHeader() ->setResizeMode(QHeaderView::Stretch);//���������Զ�����������������
    this ->horizontalHeader() ->resizeSection(0, 30);
    this ->horizontalHeader() ->resizeSection(1, 270);
    this ->horizontalHeader() ->resizeSection(2, 65);
    this ->horizontalHeader() ->setHighlightSections(false);//�����ʱ���Ա�ͷ�й�������ȡ���㣩
    this ->horizontalHeader() ->setClickable(false);//����Ӧ��굥��
    this ->setSelectionMode(QAbstractItemView::SingleSelection);//����ֻ��ѡ�е���
    this ->setSelectionBehavior(QAbstractItemView::SelectRows);//����ѡ��ϰ��Ϊѡ����
    this ->setEditTriggers(QAbstractItemView::NoEditTriggers);//�������ݲ��ɱ༭
    this ->setShowGrid(false);//���ò���ʾ����
    this ->setItemDelegate(new NoFocusDelegate());//ȥ��ѡ�е�Ԫ�������

//    QTableWidgetItem *columnHeadItem = this ->horizontalHeaderItem(0);
//    columnHeadItem ->setFont(QFont("��Բ"));
//    columnHeadItem ->setBackground(Qt::gray);
//    columnHeadItem ->setTextColor(Qt::white);

//    QTableWidgetItem *columnHeadItem2 = this ->horizontalHeaderItem(1);
//    columnHeadItem2 ->setFont(QFont("��Բ"));
//    columnHeadItem2 ->setBackground(Qt::gray);
//    columnHeadItem2 ->setTextColor(Qt::white);

//    QTableWidgetItem *columnHeadItem3 = this ->horizontalHeaderItem(2);
//    columnHeadItem3 ->setFont(QFont("��Բ"));
//    columnHeadItem3 ->setBackground(Qt::gray);
//    columnHeadItem3 ->setTextColor(Qt::white);

    //����header��ʽ
    this ->horizontalHeader() ->setStyleSheet("QHeaderView::section{background: gray; color:white; font-family:'΢���ź�'}");
    this ->setStyleSheet("QTableCornerButton::section{background:gray};");//���ñ��ֱ�Ƿ�����ʽ
    this ->verticalHeader() ->setStyleSheet("QHeaderView::section{background: gray; color:white; font-family:'΢���ź�';}");

    clearMusicList = new QAction(this);
    clearMusicList ->setIcon(QIcon(":/images/clearMusicList.png"));
    clearMusicList ->setText(tr("����б�"));

    closeAction = new QAction(this);
    closeAction ->setIcon(QIcon(":/images/quitAction"));
    closeAction ->setText(tr("�ر������б�"));

    delCurrentMusicAction = new QAction(this);
    delCurrentMusicAction ->setIcon(QIcon(":/images/delCurrentMusic.png"));
    delCurrentMusicAction ->setText(tr("ɾ����ǰ����"));

	saveListAction = new QAction(this);
	saveListAction ->setIcon(QIcon(":/images/saveList.png"));
	saveListAction ->setText(tr("�����б�"));

    connect(clearMusicList, SIGNAL(triggered()), this, SLOT(slot_ClearAllMusicList()));
    connect(closeAction, SIGNAL(triggered()), parent, SLOT(slot_OpenMusicList()));
    connect(delCurrentMusicAction, SIGNAL(triggered()), this, SLOT(slot_DelCurrentMusic()));
	connect(saveListAction, SIGNAL(triggered()), this, SLOT(slot_SaveList()));
    connect(this, SIGNAL(cellClicked(int,int)), this, SLOT(slot_TableClicked(int)));
}

MusicList::~MusicList()
{
}

//������ֲ����б�
void MusicList::slot_ClearAllMusicList()
{
	if (this ->rowCount())
	{
		if (QMessageBox::Yes == QMessageBox::information(NULL, tr("��Ϣ"),
			tr("�Ƿ�Ҫ��ղ����б�"), QMessageBox::Yes | QMessageBox::No))
		{
			while (this ->rowCount())
			{
				removeRow(0);
			}
			emit sig_PlayListClean();//�����󣬷�����ճɹ��ź�
		}
	}
	else
	{
		QMessageBox::information(NULL, tr("��Ϣ"), tr("��ǰ�����б�Ϊ�գ�"), QMessageBox::Yes);
	}
}

//ɾ����ǰ����
void MusicList::slot_DelCurrentMusic()
{
    if (this ->rowCount())
    {
        if (QMessageBox::Yes == QMessageBox::information(NULL, tr("��Ϣ"),
                tr("�Ƿ�ɾ���ø�����"), QMessageBox::Yes | QMessageBox::No))
        {
            if (row < this ->rowCount())
            {
                QString musicName =  this ->item(row, 1) ->text();
                this ->removeRow(row);
                emit sig_RowSelected(row, musicName);//���ݿ���ɾ��
                //���²����б����
                this ->updateMusicList();
            }
        }
    }
	else
	{
		QMessageBox::information(NULL, tr("��Ϣ"), tr("��ǰ�����б�Ϊ�գ�"), QMessageBox::Yes);
	}
}

//���²����б����
void MusicList::updateMusicList()
{
    for (int index = row; index < this ->rowCount(); ++index)
    {
        QTableWidgetItem *rowItem = this ->item(index, 0);
        rowItem ->setText(QString::number(index + 1));
    }
}

//���ѡ�е���
void MusicList::slot_TableClicked(int index)
{
    row = index;
}

//��Ӧ����Ҽ�����¼�
void MusicList::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu menu;
    menu.addAction(QIcon(":/images/locateButton.png"), tr("��λ����ǰ���Ÿ���"), this, SLOT(slot_LocateCurrentMusic()));
    menu.addSeparator();
    menu.addAction(delCurrentMusicAction);
    menu.addAction(clearMusicList);
	menu.addAction(saveListAction);
    menu.addSeparator();
    menu.addAction(closeAction);
    menu.exec(event ->globalPos());//�������ָ���ȫ��λ��
    event ->accept();
}

//��λ����ǰ����
void MusicList::slot_LocateCurrentMusic()
{
	if (m_nCurrentMusicRow == 0)
	{
		this ->selectRow(m_nCurrentMusicRow + 1);
	}
	else
	{
		this ->selectRow(m_nCurrentMusicRow - 1);
	}
	this ->selectRow(m_nCurrentMusicRow);
}

//���浱ǰ�б�
void MusicList::slot_SaveList()
{
	QString fileName = QFileDialog::getSaveFileName(this, tr("���沥���б�"), tr("�����б�"), tr("*.m3u *.pls"));
	if (!fileName.isEmpty())
	{
		ofstream file(fileName.toStdString());

		QString strSuffixName = fileName.right(3);//�õ���׺��
		if (strSuffixName == "m3u" || strSuffixName == "M3U")
		{
			file << "#EXTM3U" << endl;			//��ʾһ��M3U�ļ�
		}
		else if (strSuffixName == "pls" || strSuffixName == "PLS")
		{
			file << "[playlist]" << endl;		//��ʾһ��pls�ļ�
		}

		vector<MusicListSaveFormatEntity> vec;
		if (DBModule::readFromDb2(vec))
		{
			int i = 0;
			foreach (MusicListSaveFormatEntity entity, vec)
			{
				++i;
				QTime qTime;
				int nTime = qTime.secsTo(QTime::fromString(QString::fromStdString(entity.getTime()), "mm:ss"));

				if (strSuffixName == "m3u" || strSuffixName == "M3U")
				{
					QString sFileName = QString::fromStdString(entity.getFileName());
					QString strMusicName = "";
					if (sFileName.contains(" - "))
					{
						strMusicName = sFileName.split(" - ").at(1);
					}
					file << "#EXTINF:" << nTime << "," << strMusicName.toStdString() << "\n" << entity.getFilePath() << endl;
				}
				else if (strSuffixName == "pls" || strSuffixName == "PLS")
				{
					QString sFileName = QString::fromStdString(entity.getFileName());
					QString strMusicName = "";
					if (sFileName.contains(" - "))
					{
						strMusicName = sFileName.split(" - ").at(1);
					}
					file << "File" << i << "=" << entity.getFilePath() << "\n"
						 << "Title" << i << "=" << strMusicName.toStdString() << "\n"
						 << "Length" << i << "=" << nTime << endl;
				}
			}
			if (strSuffixName == "pls" || strSuffixName == "PLS")
			{
				file << "NumberOfEntries=" << i <<  endl;
			}
		}
	}
}

//���õ�ǰ�������ŵ��к�
void MusicList::setCurrentMusicRow( int row )
{
	m_nCurrentMusicRow = row;
}
