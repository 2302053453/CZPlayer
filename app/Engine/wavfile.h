#ifndef WAVFILE_H
#define WAVFILE_H

#include <QtCore/qobject.h>
#include <QtCore/qfile.h>
#include <QtMultimedia/qaudioformat.h>

class WavFile : public QFile
{
public:
    WavFile(QObject *parent = 0);

    bool open(const QString &fileName);			//���ļ�
    const QAudioFormat &fileFormat() const;		//�õ��ļ���ʽ
    long long headerLength() const;				//�õ��ļ�ͷ����

	long int getMusicTime(const QString &fileName);					//�õ�����ʱ��

private:
    bool readHeader();							//��ȡ�ļ�ͷ

private:
    QAudioFormat m_fileFormat;					//�ļ���Ƶ���ݸ�ʽ
    long long m_headerLength;					//�ļ�ͷ����
};

#endif

