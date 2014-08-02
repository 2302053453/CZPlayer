#ifndef ENGINE_H
#define ENGINE_H

#include "spectrum.h"
#include "spectrumanalyser.h"
#include "wavfile.h"
#include <QtCore>
#include <QtMultimedia>
#include <stdio.h>
#include <stdlib.h>

class FrequencySpectrum;
QT_FORWARD_DECLARE_CLASS(QAudioInput)
QT_FORWARD_DECLARE_CLASS(QAudioOutput)
QT_FORWARD_DECLARE_CLASS(QFile)

//��������
class Engine : public QObject 
{
    Q_OBJECT

public:
    Engine(QObject *parent = 0);
    ~Engine();

    QAudio::Mode mode() const { return m_mode; }					//��ȡ����ģʽ
    QAudio::State state() const { return m_state; }					//��ȡ����״̬
    const QAudioFormat& format() const  { return m_format; }		//���ص�ǰ����Ƶ��ʽ
    void reset();													//���ò�������
    bool loadFile(const QString &fileName);							//�����ļ�
    bool generateTone(const Tone &tone);							//��������
    bool generateSweptTone(double amplitude);						//��������
    double rmsLevel() const { return m_rmsLevel; }					//��������һ����Ƶ������RMS��ƽ,�ڷ�Χ�ڵĻر�ˮƽ��0.0��1.0��
    double peakLevel() const  { return m_peakLevel; }				//��������һ����Ƶ�����ķ�ֵ��ƽ,�ڷ�Χ�ڵĻر�ˮƽ��0.0��1.0��
    long long playPosition() const  { return m_playPosition; }		//��Ƶ����豸��λ��,���ֽ�Ϊ��λ���ص�λ��
    long long bufferLength() const;									//���������ڲ�����������
    long long dataLength() const  { return m_dataLength; }			//�ڻ������б�����������
    void setWindowFunction(WindowFunction type);					//���ô��ڹ���
	void stopPlayback();											//ֹͣ����

public slots:
    void slot_StartPlayback();											//��ʼ����
    void suspend();														//��ͣ����

signals:
    void stateChanged(QAudio::Mode mode, QAudio::State state);			//״̬�ı�
    void infoMessage(const QString &message, int durationMs);			//��Ϣ
    void errorMessage(const QString &heading, const QString &detail);	//������Ϣ
	void bufferLengthChanged(long long duration);
    void formatChanged(const QAudioFormat &format);						//��Ƶ���ݸ�ʽ�ı�
    void dataLengthChanged(long long duration);							//�������������ı�
	void recordPositionChanged(long long position);						//��¼λ�÷����ı�
    void playPositionChanged(long long position);							//��Ƶ���װ�õ�λ�÷����˱仯
	//Ƶ�ײ����ı�
    void spectrumChanged(long long position, long long length, const FrequencySpectrum &spectrum);
	void bufferChanged(long long position, long long length, const QByteArray &buffer);
	void sig_Finished();												//�������

private slots:
    void audioNotify();													//��Ƶ֪ͨ
    void audioStateChanged(QAudio::State state);						//��Ƶ״̬�ı�
    void spectrumChanged(const FrequencySpectrum &spectrum);			//Ƶ�׸ı�

private:
	void resetAudioDevices();											//������Ƶ�豸
    bool initialize();													//��ʼ����������
    bool selectFormat();												//ѡ����Ƶ���ݸ�ʽ
    void setState(QAudio::State state);									//����״̬
    void setState(QAudio::Mode mode, QAudio::State state);				//����״̬
    void setFormat(const QAudioFormat &format);							//������Ƶ���ݸ�ʽ
	void setRecordPosition(long long position, bool forceEmit = false);	//���ü�¼λ��
    void setPlayPosition(long long position, bool forceEmit = false);	//������Ƶ���λ��
    void calculateSpectrum(long long position);							//����Ƶ��

private:
    QAudio::Mode        m_mode;							//��Ƶģʽ
    QAudio::State       m_state;						//��Ƶ״̬
    bool                m_generateTone;					//�Ƿ���������
    SweptTone           m_tone;							//��������
    WavFile*            m_file;							//wav�ļ�ָ��
    WavFile*            m_analysisFile;					//�ڶ���wav�ļ�ָ�룬��Ҫ���ڰ��ļ���������ݶ�������m_buffer
    QAudioFormat        m_format;						//��Ƶ���ݸ�ʽ
    QAudioDeviceInfo    m_audioInputDevice;				//��Ƶ�����豸��Ϣ
    QAudioInput*        m_audioInput;					//��Ƶ����
    QAudioDeviceInfo    m_audioOutputDevice;			//��Ƶ����豸��Ϣ
    QAudioOutput*       m_audioOutput;					//��Ƶ���
    long long           m_playPosition;					//��Ƶ����λ��
    QBuffer             m_audioOutputIODevice;			//��Ƶ����豸IO������
    QByteArray          m_buffer;						//���������黺����
    long long           m_bufferPosition;				//������λ��
    long long           m_bufferLength;					//����������
    long long           m_dataLength;					//���ݲ��ֳ���
	long long             m_recordPosition;				//��¼λ��
    int                 m_levelBufferLength;			//ˮƽ����������
    double              m_rmsLevel;						//RMS��ƽ
    double              m_peakLevel;					//��ֵ��ƽ
    int                 m_spectrumBufferLength;			//Ƶ�׻���������
    QByteArray          m_spectrumBuffer;				//Ƶ�׻�����
    SpectrumAnalyser    m_spectrumAnalyser;				//Ƶ�׷�����
    long long           m_spectrumPosition;				//Ƶ��λ��
    int					m_count;						//����
};

#endif // ENGINE_H
