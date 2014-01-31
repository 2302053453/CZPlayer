#include "stdafx.h"
#include "engine.h"
#include "tonegenerator.h"
#include "utils.h"
#include <math.h>
#include <QMetaObject>
#include <QSet>
#include <QThread>

const long long BufferDurationUs = 10 * 1000000;	//������ʱ��
const int NotifyIntervalMs = 100;					//֪ͨ���(Ҳ��Ƶ�׸���ʱ����)
const int LevelWindowUs = 0.1 * 1000000;			//��΢�뼶���㴰�ڵĴ�С

//����ʵ��"<<"������
QDebug& operator<<(QDebug &debug, const QAudioFormat &format)
{
    debug << format.frequency() << "Hz"
          << format.channels() << "channels";
    return debug;
}

//�������湹�캯��
Engine::Engine(QObject *parent)
    :   QObject(parent)
    ,   m_mode(QAudio::AudioInput)
    ,   m_state(QAudio::StoppedState)
    ,   m_generateTone(false)
    ,   m_file(0)
    ,   m_analysisFile(0)
    ,   m_audioInputDevice(QAudioDeviceInfo::defaultInputDevice())
    ,   m_audioInput(0)
    ,   m_audioOutputDevice(QAudioDeviceInfo::defaultOutputDevice())
    ,   m_audioOutput(0)
    ,   m_playPosition(0)
	,	m_recordPosition(0)
    ,   m_bufferPosition(0)
    ,   m_bufferLength(0)
    ,   m_dataLength(0)
    ,   m_levelBufferLength(0)
    ,   m_rmsLevel(0.0)
    ,   m_peakLevel(0.0)
    ,   m_spectrumBufferLength(0)
    ,   m_spectrumAnalyser()
    ,   m_spectrumPosition(0)
    ,   m_count(0)
{
	//ע��Qt�źŲ۲���
    qRegisterMetaType<FrequencySpectrum>("FrequencySpectrum");
    qRegisterMetaType<WindowFunction>("WindowFunction");
    connect(&m_spectrumAnalyser, SIGNAL(spectrumChanged(FrequencySpectrum)), this, SLOT(spectrumChanged(FrequencySpectrum)));	//Ƶ�׸ı�
    this ->initialize();	//��ʼ����������
}

Engine::~Engine()
{

}

//�����ļ�
bool Engine::loadFile(const QString &fileName)
{
    this ->reset();	//���ò�������
    bool result = false;
    m_file = new WavFile(this);						//�½�һ��wav�ļ�
    if (m_file ->open(fileName))					//���ļ�
	{
        if (isPCMS16LE(m_file ->fileFormat()))		//�ж��ļ���ʽ�Ƿ�Ϊpcm16le
		{
            result = this ->initialize();			//��ʼ����������
        }
		else 
		{
            emit errorMessage(tr("��֧�ֵ���Ƶ��ʽ"), formatToString(m_file ->fileFormat()));
        }
    }
	else
	{
        emit errorMessage(tr("���ܹ����ļ�"), fileName);
    }
    if (result)		//��ʼ����������ɹ�
	{	
		//�����ļ�ָ��
        m_analysisFile = new WavFile(this);
        m_analysisFile ->open(fileName);
    }
    return result;
}

//��ʼ����������
bool Engine::initialize()
{
	bool result = false;			//��ʼ����������ʧ��
	QAudioFormat format = m_format;	//��Ƶ���ݸ�ʽ

	if (this ->selectFormat())		//ѡ����Ƶ���ݸ�ʽ
	{
		if (m_format != format)
		{
			this ->resetAudioDevices();	//������Ƶ�豸
			if (m_file)
			{
				emit bufferLengthChanged(bufferLength());
				emit dataLengthChanged(dataLength());
				emit bufferChanged(0, 0, m_buffer);
				this ->setRecordPosition(bufferLength());
				result = true;
			}
			else
			{
				m_bufferLength = audioLength(m_format, BufferDurationUs);	//����������
				m_buffer.resize(m_bufferLength);							//���û�����
				m_buffer.fill(0);											//��ʼ��������
				emit bufferLengthChanged(bufferLength());
				if (m_generateTone)
				{
					if (0 == m_tone.endFreq) 
					{
						const double nyquist = nyquistFrequency(m_format);
						m_tone.endFreq = qMin(double(SpectrumHighFreq), nyquist);
					}
					//��utils.h���ú����ж��壬��ȫ�ַ�Χ��
					::generateTone(m_tone, m_format, m_buffer);
					m_dataLength = m_bufferLength;
					emit dataLengthChanged(dataLength());
					emit bufferChanged(0, m_dataLength, m_buffer);
					this ->setRecordPosition(m_bufferLength);
					result = true;
				} 
				else
				{
					emit bufferChanged(0, 0, m_buffer);
					m_audioInput = new QAudioInput(m_audioInputDevice, m_format, this);		//��Ƶ����
					m_audioInput ->setNotifyInterval(NotifyIntervalMs);						//�趨֪ͨʱ����
					result = true;
				}
			}
			m_audioOutput = new QAudioOutput(m_audioOutputDevice, m_format, this);			//��Ƶ���
			m_audioOutput->setNotifyInterval(NotifyIntervalMs);								//�趨֪ͨʱ����
		}
	}
	else	//ѡ����Ƶ��ʽʧ��
	{
		if (m_file)
		{
			emit errorMessage(tr("��֧�ֵ���Ƶ��ʽ"), formatToString(m_format));
		}
		else if (m_generateTone)
		{
			emit errorMessage(tr("û���ҵ����ʵĸ�ʽ"), "");
		}
		else
		{
			emit errorMessage(tr("�Ҳ�����ͬ������/�����ʽ"), "");
		}
	}

	qDebug() << "Engine::initialize" << "��������С:" << m_bufferLength;
	qDebug() << "Engine::initialize" << "��������С:" << m_dataLength;
	qDebug() << "Engine::initialize" << "��Ƶ���ݸ�ʽ:" << m_format;

	return result;
}

//ѡ����Ƶ���ݸ�ʽ
bool Engine::selectFormat()
{
	bool foundSupportedFormat = false;		//Ĭ�ϵ���Ƶ����豸��֧�ָ���Ƶ��ʽ
	if (m_file || QAudioFormat() != m_format) 
	{
		QAudioFormat format = m_format;
		if (m_file)
		{
			//��ͷ�Ǵ�WAV�ļ���ȡ��ֻ��Ҫ����Ƿ���֧�ֵ���Ƶ���װ��
			format = m_file ->fileFormat();
		}
		if (m_audioOutputDevice.isFormatSupported(format)) 
		{
			this ->setFormat(format);		//������Ƶ���ݸ�ʽ
			foundSupportedFormat = true;	//Ĭ�ϵ���Ƶ����豸֧�ָ���Ƶ��ʽ
		}
	}
	else 
	{
		QList<int> frequenciesList;											//Ƶ���б�

#ifdef Q_OS_WIN
		//Windows����Ƶ��˲���ȷ�����ʽ֧�֣���QTBUG-9100�������⣬��Ȼ��Ƶ��ϵͳ������11025Hz���ɴ˲�������Ƶ����
		frequenciesList += 8000;
#endif

		if (!m_generateTone)
		{
			frequenciesList += m_audioInputDevice.supportedFrequencies();	//��ȡ�����豸֧�ֵ�Ƶ��
		}

		frequenciesList += m_audioOutputDevice.supportedFrequencies();		//��ȡ����豸֧�ֵ�Ƶ��
		frequenciesList = frequenciesList.toSet().toList();					//ɾ���ظ�
		qSort(frequenciesList);												//��Ƶ���б�����
		qDebug() << "Engine::initialize ��������豸֧�ֵ�ȡ��Ƶ��:" << frequenciesList;

		QList<int> channelsList;											//ͨ���б�
		channelsList += m_audioInputDevice.supportedChannels();				//��ȡ�����豸֧�ֵ�ͨ��
		channelsList += m_audioOutputDevice.supportedChannels();			//��ȡ����豸֧�ֵ�ͨ��
		channelsList = channelsList.toSet().toList();						//ɾ���ظ�
		qSort(channelsList);												//��ͨ���б�����
		qDebug() << "Engine::initialize ��������豸֧�ֵ�ͨ��:" << channelsList;

		//������Ƶ���ݸ�ʽ
		QAudioFormat format;
		format.setByteOrder(QAudioFormat::LittleEndian);					//�����ֽ�˳��
		format.setCodec("audio/pcm");										//�����ʽ
		format.setSampleSize(16);											//������С
		format.setSampleType(QAudioFormat::SignedInt);						//��������

		int frequency, channels;
		//����Ƶ���б�
		foreach (frequency, frequenciesList)
		{
			if (foundSupportedFormat)
			{
				break;
			}
			format.setFrequency(frequency);									//Ƶ��

			//����ͨ���б�
			foreach (channels, channelsList) 
			{
				format.setChannels(channels);								//ͨ��
				const bool inputSupport = m_generateTone || m_audioInputDevice.isFormatSupported(format);	//�����豸֧��
				const bool outputSupport = m_audioOutputDevice.isFormatSupported(format);					//����豸֧��
				qDebug() << "Engine::initialize ��Ƶ���ݸ�ʽ:" << format << ",����:" << inputSupport << ",���:" << outputSupport;
				if (inputSupport && outputSupport)
				{
					foundSupportedFormat = true;	//Ĭ�ϵ���Ƶ����豸֧�ָ���Ƶ��ʽ
					break;
				}
			}
		}
		if (!foundSupportedFormat)
		{
			format = QAudioFormat();
		}
		this ->setFormat(format);		//������Ƶ���ݸ�ʽ
	}
	return foundSupportedFormat;
}

//����
void Engine::slot_StartPlayback()
{
	if (m_audioOutput) 
	{
		//�����ǰģʽΪ���ģʽ���ҵ�ǰ״̬Ϊ��ͣ״̬
		if (QAudio::AudioOutput == m_mode && QAudio::SuspendedState == m_state)
		{
			m_audioOutput ->resume();	//���¿�ʼ,��������
		}
		//��ǰ״̬Ϊֹͣ״̬
		else if (QAudio::StoppedState == m_state)
		{
			//��һ�β���
			m_spectrumAnalyser.cancelCalculation();					//ȡ��Ƶ�׼���
			this ->spectrumChanged(0, 0, FrequencySpectrum());		//��ʼ��Ƶ��
			this ->setPlayPosition(0, true);						//������Ƶ��ʼλ��
			m_mode = QAudio::AudioOutput;							//��ǰģʽΪ���ģʽ

			connect(m_audioOutput, SIGNAL(stateChanged(QAudio::State)), this, SLOT(audioStateChanged(QAudio::State)));	//��Ƶ״̬�ı�
			connect(m_audioOutput, SIGNAL(notify()), this, SLOT(audioNotify()));										//��Ƶ֪ͨ(ÿ100ms֪ͨһ��)
			m_count = 0;
			if (m_file) 
			{
				m_file ->seek(0);				//�ļ�ָ��ָ���ļ�ͷ
				m_bufferPosition = 0;			//���û�����λ��
				m_dataLength = 0;				//�������ݳ���
				m_audioOutput ->start(m_file);	//��ʼ����
			} 
			else 
			{
				m_audioOutputIODevice.close();
				m_audioOutputIODevice.setBuffer(&m_buffer);
				m_audioOutputIODevice.open(QIODevice::ReadOnly);
				m_audioOutput ->start(&m_audioOutputIODevice);
			}
		}
	}
}

//��ͣ����
void Engine::suspend()
{
	//�����ǰ״̬Ϊ����״̬
	if (QAudio::ActiveState == m_state || QAudio::IdleState == m_state) 
	{
		switch (m_mode) 
		{
		case QAudio::AudioInput:	//����ģʽ
			m_audioInput ->suspend();
			break;
		case QAudio::AudioOutput:	//���ģʽ
			m_audioOutput ->suspend();
			break;
		default:
			break;
		}
	}
}

//ֹͣ����
void Engine::stopPlayback()
{
	if (m_audioOutput)
	{
		m_audioOutput ->stop();		//ֹͣ����
		QCoreApplication::instance() ->processEvents();
		m_audioOutput ->disconnect();
		this ->setPlayPosition(0);	//���ò���λ��
	}
}

//������Ƶ�豸
void Engine::resetAudioDevices()
{
	delete m_audioInput;
	m_audioInput = 0;
	this ->setRecordPosition(0);
	delete m_audioOutput;
	m_audioOutput = 0;
	this ->setPlayPosition(0);
	m_spectrumPosition = 0;
}

//���ò�������
void Engine::reset()
{
	this ->stopPlayback();											//ֹͣ����
	this ->setState(QAudio::AudioInput, QAudio::StoppedState);		//����״̬
	this ->setFormat(QAudioFormat());								//������Ƶ��ʽ
	m_generateTone = false;											//û����������
	if (m_file) { delete m_file; m_file = 0; }
	if (m_analysisFile) { delete m_analysisFile; m_analysisFile = 0; }
	m_buffer.clear();												//��ջ�����
	m_bufferPosition = 0;											//������λ��
	m_bufferLength = 0;												//����������
	m_dataLength = 0;												//���ݲ��ֳ���
	emit dataLengthChanged(0);										//�����������ı��ź�
	this ->resetAudioDevices();										//������Ƶ�豸
}

//���ü�¼λ��
void Engine::setRecordPosition(qint64 position, bool forceEmit)
{
	const bool changed = (m_recordPosition != position);
	m_recordPosition = position;
	if (changed || forceEmit)
	{
		emit recordPositionChanged(m_recordPosition);
	}
}

//���ò���λ��
void Engine::setPlayPosition(long long position, bool forceEmit)
{
	const bool changed = (m_playPosition != position);
	m_playPosition = position;
	if (changed || forceEmit)
	{
		emit playPositionChanged(m_playPosition);
	}
}

//��������
bool Engine::generateTone(const Tone &tone)
{
    this ->reset();
    Q_ASSERT(!m_generateTone);
    Q_ASSERT(!m_file);
    m_generateTone = true;
    m_tone = tone;
    qDebug() << "Engine::generateTone"
                 << "startFreq" << m_tone.startFreq
                 << "endFreq" << m_tone.endFreq
                 << "amp" << m_tone.amplitude;
    return this ->initialize();
}

//�������
bool Engine::generateSweptTone(double amplitude)
{
    Q_ASSERT(!m_generateTone);
    Q_ASSERT(!m_file);
    m_generateTone = true;
    m_tone.startFreq = 1;
    m_tone.endFreq = 0;
    m_tone.amplitude = amplitude;
    qDebug() << "Engine::generateSweptTone"
                 << "startFreq" << m_tone.startFreq
                 << "amp" << m_tone.amplitude;
    return this ->initialize();
}

//��ȡ���������ڲ�����������
long long Engine::bufferLength() const
{
    return m_file ? m_file->size() : m_bufferLength;
}

//���ô��ڹ���
void Engine::setWindowFunction(WindowFunction type)
{
    m_spectrumAnalyser.setWindowFunction(type);
}

//��Ƶ֪ͨ(ÿ100ms֪ͨһ��)
void Engine::audioNotify()
{
    switch (m_mode) 
	{
    case QAudio::AudioOutput:	//���ģʽ
		{
            const long long playPosition = audioLength(m_format, m_audioOutput->processedUSecs());	//��ȡ����λ��
            this ->setPlayPosition(qMin(bufferLength(), playPosition));								//���ò���λ��
            const long long levelPosition = playPosition - m_levelBufferLength;						//��ȡˮƽλ��
            const long long spectrumPosition = playPosition - m_spectrumBufferLength;				//��ȡƵ��λ��
            if (m_file) 
			{
                if (levelPosition > m_bufferPosition || spectrumPosition > m_bufferPosition || qMax(m_levelBufferLength, m_spectrumBufferLength) > m_dataLength)
				{
                    m_bufferPosition = 0;	//������λ��
                    m_dataLength = 0;		//���ݳ���

                    //������Ҫ�����뵽m_buffer�Դ�����
                    const long long readPos = qMax(long long(0), qMin(levelPosition, spectrumPosition));		//��ȡ�Ķ���ʼλ��
                    const long long readEnd = qMin(m_analysisFile ->size(), qMax(levelPosition + m_levelBufferLength, spectrumPosition + m_spectrumBufferLength));	//��ȡ�Ķ�����λ��
                    const long long readLen = readEnd - readPos + audioLength(m_format, WaveformWindowDuration);//��ȡ�Ķ�����
                    //qDebug() << "Engine::audioNotify [1]"
                    //         << "�ļ���С:" << m_analysisFile ->size()
                    //         << "�Ѷ�λ��:" << readPos
                    //         << "��ȡ����:" << readLen;
                    if (m_analysisFile ->seek(readPos + m_analysisFile ->headerLength())) 
					{
                        m_buffer.resize(readLen);
                        m_bufferPosition = readPos;
                        m_dataLength = m_analysisFile ->read(m_buffer.data(), readLen);
                        //qDebug() << "Engine::audioNotify [2]" << "������λ��:" << m_bufferPosition << "������:" << m_dataLength;
                    } 
					else 
					{
                        qDebug() << "Engine::audioNotify [2]" << "�ļ�Ѱ��ʧ��";
                    }
					emit bufferChanged(m_bufferPosition, m_dataLength, m_buffer);
                }
            } 
			else
			{	
				//������ŵ�λ�ô��ڵ������ݳ�����ֹͣ����
                if (playPosition >= m_dataLength)
				{
                    this ->stopPlayback();
				}
            }
            if (spectrumPosition >= 0 && spectrumPosition + m_spectrumBufferLength < m_bufferPosition + m_dataLength)
			{
                this ->calculateSpectrum(spectrumPosition);	//����Ƶ��
			}
        }
        break;
    }
}

//����Ƶ��
void Engine::calculateSpectrum(long long position)
{
	//qDebug() << "Engine::calculateSpectrum" << QThread::currentThread()
	//	<< "����:" << m_count << "Ƶ��λ��:" << position << "Ƶ�׻���������:" << m_spectrumBufferLength
	//	<< "Ƶ�׷������Ƿ����:" << m_spectrumAnalyser.isReady();

	//�ж�Ƶ�׷������Ƿ�ʼ����
	if (m_spectrumAnalyser.isReady()) 
	{
		m_spectrumBuffer = QByteArray::fromRawData(m_buffer.constData() + position - m_bufferPosition, m_spectrumBufferLength);
		m_spectrumPosition = position;
		m_spectrumAnalyser.calculate(m_spectrumBuffer, m_format);	//Ƶ�׷����ǽ��м���
	}
}

//��Ƶ״̬�ı�
void Engine::audioStateChanged(QAudio::State state)
{
    qDebug() << "Engine::audioStateChanged from" << m_state << "to" << state;

    if (QAudio::IdleState == state && m_file && m_file ->pos() == m_file->size())
	{
        this ->stopPlayback();	//ֹͣ����
		emit sig_Finished();	//���Ͳ�������ź�
		qDebug() << "���!";
    } 
	else
	{
        if (QAudio::StoppedState == state) 
		{
            // Check error
            QAudio::Error error = QAudio::NoError;
            switch (m_mode) 
			{
            case QAudio::AudioInput:
                error = m_audioInput->error();
                break;
            case QAudio::AudioOutput:
                error = m_audioOutput->error();
                break;
            }
            if (QAudio::NoError != error) 
			{
                this ->reset();
                return;
            }
        }
        this ->setState(state);	//����״̬
    }
}

//Ƶ�׸ı�
void Engine::spectrumChanged(const FrequencySpectrum &spectrum)
{
    //qDebug() << "Engine::spectrumChanged" << "λ��:" << m_spectrumPosition;
    emit spectrumChanged(m_spectrumPosition, m_spectrumBufferLength, spectrum);	//����Ƶ�׸ı��ź�
}

//����״̬
void Engine::setState(QAudio::State state)
{
    const bool changed = (m_state != state);
    m_state = state;
    if (changed)
	{
        emit stateChanged(m_mode, m_state);
	}
}

//����״̬
void Engine::setState(QAudio::Mode mode, QAudio::State state)
{
    const bool changed = (m_mode != mode || m_state != state);
    m_mode = mode;
    m_state = state;
    if (changed)
	{
        emit stateChanged(m_mode, m_state);
	}
}

//������Ƶ��ʽ
void Engine::setFormat(const QAudioFormat &format)
{
    const bool changed = (format != m_format);						//�жϸ�ʽ�Ƿ��Ѿ��ı�
    m_format = format;												//������Ƶ��ʽ
    m_levelBufferLength = audioLength(m_format, LevelWindowUs);		//���ˮƽ����������
    m_spectrumBufferLength = SpectrumLengthSamples * (m_format.sampleSize() / 8) * m_format.channels();	//Ƶ�׻���������
    if (changed)
	{
        emit formatChanged(m_format);		//������Ƶ��ʽ�ı��ź�
	}
}
