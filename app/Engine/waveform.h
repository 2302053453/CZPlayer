#ifndef WAVEFORM_H
#define WAVEFORM_H

#include <QtGui>
#include <QtMultimedia/QAudioFormat>
#include <QPixmap>
#include <QScopedPointer>

QT_FORWARD_DECLARE_CLASS(QByteArray)

//������
class Waveform : public QLabel
{
    Q_OBJECT

public:
    Waveform(QWidget *parent = 0);
    ~Waveform();

    void paintEvent(QPaintEvent *event);
    void resizeEvent(QResizeEvent *event);

	//��ʼ������
    void initialize(const QAudioFormat &format, long long audioBufferSize, long long windowDurationUs);		
    void reset();									//���ò���
    void setAutoUpdatePosition(bool enabled);		//�����Զ�����λ��

public slots:
    void bufferChanged(long long position, long long length, const QByteArray &buffer);		//�������ı�
    void audioPositionChanged(long long position);											//��Ƶλ�øı�

private:
    static const int NullIndex = -1;

    void deletePixmaps();							//ɾ������ͼ
    void createPixmaps(const QSize &newSize);		//������������ͼ���ػ�͸�����ʾ��������
    void setWindowPosition(long long position);		//���´���λ�ô�������
    long long tilePosition(int index) const;		//��ש�Ļ�����λ

	//��ʶ���ש����
    struct TilePoint
    {
        TilePoint(int idx = 0, long long pos = 0, long long pix = 0)
        :   index(idx), positionOffset(pos), pixelOffset(pix)
        { }
        int index;					//��שָ��
        long long positionOffset;	//����Ƭ�Ŀ�ʼ���ֽ���
        int pixelOffset;			//�����ض�Ӧ������ͼ�������
    };

	//ת����m_bufferλ�ó���Ƭ������������Ϊ��λ��ƫ���� 
	//����Ӧ������ͼ����λ��ƫ�Ƶ�m_buffer��
	//���ֽ�Ϊ��λ ���λ����tile�����⣬ָ��NullIndex��ƫ��Ϊ�㡣
    TilePoint tilePoint(long long position) const;

	//���ֽ�Ϊ��λ��שƫ��ת��������ƫ��
    int tilePixelOffset(long long positionOffset) const;

	//���ֽ�Ϊ��λ������ƫ����ת��������ƫ��
    int windowPixelOffset(long long positionOffset) const;

    bool paintTiles();						//���ƴ�ש
    void paintTile(int index);				//���ƴ�ש
	void shuffleTiles(int n);				//�ƶ���һn��Ƭ�������ĩβ���������Ǳ��Ϊ��Ϳ
    void resetTiles(long long newStartPos);	//��λ��ש����

private:
	long long  m_bufferPosition;
	long long  m_bufferLength;
    QByteArray m_buffer;
    long long m_audioPosition;
    QAudioFormat m_format;
    bool m_active;
    QSize m_pixmapSize;
    QVector<QPixmap*> m_pixmaps;

    struct Tile 
	{
        QPixmap *pixmap;			//ָ�뵽��m_pixmaps����
        bool painted;				//��־����ʾ�ô�ש�Ƿ��Ѿ���
    };

    QVector<Tile> m_tiles;
    long long m_tileLength;			//��ÿ������ֽڵ���Ƶ���ݵĳ���
    long long m_tileArrayStart;		//��������һ��Ƭ���ֽ�λ���ϣ������m_buffer
    long long m_windowPosition;
    long long m_windowLength;
};

#endif // WAVEFORM_H
