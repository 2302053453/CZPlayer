#include "stdafx.h"
#include <QtCore/qendian.h>
#include <QVector>
#include <QDebug>
#include "utils.h"
#include "wavfile.h"

//�ļ�������
struct chunk
{
    char			 id[4];			    //id
    unsigned int     size;				//�ļ���С
};

//����ͷ
struct DATAHeader
{
	chunk       descriptor;				//�ļ�������
};

//RIFFͷ
struct RIFFHeader
{
    chunk       descriptor;				// "RIFF"(��Դ�����ļ���־)
    char        type[4];				// "WAVE"
};

//wave��ʽ�ļ�ͷ
struct WAVEHeader
{
    chunk			   descriptor;		//�ļ�������
    unsigned short	   audioFormat;		//��Ƶ���ݸ�ʽ
    unsigned short	   numChannels;		//ͨ����
    unsigned int	   sampleRate;		//������
    unsigned int	   byteRate;		//ÿ��������
    unsigned short     blockAlign;		//�����
    unsigned short     bitsPerSample;	//����λ��
};

//�ļ�ͷ
struct CombinedHeader
{
    RIFFHeader  riff;
    WAVEHeader  wave;
};

WavFile::WavFile(QObject *parent) : QFile(parent) , m_headerLength(0)
{
}

//���ļ�
bool WavFile::open(const QString &fileName)
{
    close();														//���ļ�֮ǰ�ȹر��ļ�
    setFileName(fileName);											//���ô��ļ���·��
    return QFile::open(QIODevice::ReadOnly) && this ->readHeader();	//���ļ����Ҷ�ȡ�ļ�ͷ
}

//��ȡ�ļ�ͷ
bool WavFile::readHeader()
{
    seek(0);					//���ļ�ָ��ָ���ļ���
    CombinedHeader header;
    bool result = read(reinterpret_cast<char *>(&header), sizeof (CombinedHeader)) == sizeof (CombinedHeader);
    if (result) 
	{
        if ((memcmp(header.riff.descriptor.id, "RIFF", 4) == 0					//�ж��ļ��������Ƿ�Ϊ"RIFF"
            || memcmp(header.riff.descriptor.id, "RIFX", 4) == 0)				//�ж��ļ��������Ƿ�Ϊ"RIFX"
            && memcmp(header.riff.type, "WAVE", 4) == 0							//�ж��Ƿ���wave��ʽ�ļ�
            && memcmp(header.wave.descriptor.id, "fmt ", 4) == 0				//�ж��ļ����θ�ʽ��־
            && (header.wave.audioFormat == 1 || header.wave.audioFormat == 0))	//�ж���Ƶ���ݸ�ʽ
		{
			qDebug() << "�ļ�������:" << header.riff.descriptor.id;
			qDebug() << "�ļ���ʽ:" << header.riff.type;
			qDebug() << "�ļ�������2:" << header.wave.descriptor.id;
			qDebug() << "��Ƶ���ݸ�ʽ:" << header.wave.audioFormat;

   //         DATAHeader dataHeader;	//����ͷ
   //         if (qFromLittleEndian<unsigned int>(header.wave.descriptor.size) > sizeof (WAVEHeader)) 
			//{
   //             unsigned short extraFormatBytes;	//����չ������
   //             if (peek((char*)&extraFormatBytes, sizeof(unsigned short)) != sizeof (unsigned short))
			//	{
   //                 return false;
			//	}
   //             const long long throwAwayBytes = sizeof(unsigned short) + qFromLittleEndian<unsigned short>(extraFormatBytes);
   //             if (read(throwAwayBytes).size() != throwAwayBytes)
			//	{
   //                 return false;
			//	}
   //         }

   //         if (read((char*)&dataHeader, sizeof(DATAHeader)) != sizeof(DATAHeader))
			//{
   //             return false;
			//}

            //������ʽ
            if (memcmp(header.riff.descriptor.id, "RIFF", 4) == 0)
			{
                m_fileFormat.setByteOrder(QAudioFormat::LittleEndian);	//�����ֽ�˳��
			}
            else
			{
                m_fileFormat.setByteOrder(QAudioFormat::BigEndian);
			}

            int bps = qFromLittleEndian<unsigned short>(header.wave.bitsPerSample);
            m_fileFormat.setChannels(qFromLittleEndian<unsigned short>(header.wave.numChannels));			//ͨ��
            m_fileFormat.setCodec("audio/pcm");																//�����ʽ
            m_fileFormat.setFrequency(qFromLittleEndian<unsigned int>(header.wave.sampleRate));				//Ƶ��
            m_fileFormat.setSampleSize(qFromLittleEndian<unsigned short>(header.wave.bitsPerSample));		//������С
            m_fileFormat.setSampleType(bps == 8 ? QAudioFormat::UnSignedInt : QAudioFormat::SignedInt);		//��������
        } 
		else
		{
            result = false;
        }
    }
    m_headerLength = pos();
    return result;
}

//�õ��ļ���ʽ
const QAudioFormat &WavFile::fileFormat() const
{
	return m_fileFormat;
}

//�õ��ļ�ͷ����
long long WavFile::headerLength() const
{
	return m_headerLength;
}

//�õ�����ʱ��
long int WavFile::getMusicTime(const QString &fileName)
{
	long int lnMusicDataSize = 0;
	long int lnByteRate = 0;

	close();
	setFileName(fileName);
	QFile::open(QIODevice::ReadOnly);

	//�������ݴ������ʣ�ÿ��ƽ���ֽ���,��λ:Byte��(1CH~1FH)
	seek(28L);
	read(reinterpret_cast<char*>(&lnByteRate), 4);

	//�������ݵ��ܵ��ֽ���(��λ:Byte)(ƫ�Ƶ�ַ:28H~2BH)
	seek(40L);
	read(reinterpret_cast<char*>(&lnMusicDataSize), 4);
	close();

	return lnMusicDataSize / lnByteRate;
}
