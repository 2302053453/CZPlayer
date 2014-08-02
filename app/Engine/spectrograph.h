#ifndef SPECTROGRAPH_H
#define SPECTROGRAPH_H

#include <QtCore>
#include <QtGui>
#include "frequencyspectrum.h"

QT_FORWARD_DECLARE_CLASS(QMouseEvent)

//Ƶ�׻���
class Spectrograph : public QLabel
{
    Q_OBJECT

public:
    Spectrograph(QWidget *parent = 0);
    ~Spectrograph();

public:
	//numBars:Ƶ����ʾ�������
	//lowFreq:Ƶ���½�
	//highFreq:Ƶ���Ͻ�
    void setParams(int numBars, double lowFreq, double highFreq);//���ò���
	
protected:
	//����ʵ������event����
    //void timerEvent(QTimerEvent *event);
    void paintEvent(QPaintEvent *event);
    //void mousePressEvent(QMouseEvent *event);

signals:
    void infoMessage(const QString &message, int intervalMs);	//��Ϣ�ź�

public slots:
    void reset();												//����
    void spectrumChanged(const FrequencySpectrum &spectrum);	//�ı�Ƶ��

private:
    int barIndex(double frequency) const;						//����Ƶ��bar
    QPair<double, double> barRange(int barIndex) const;			//Ƶ��bar�ķ�Χ
    void updateBars();											//ˢ��Ƶ��bar
    //void selectBar(int index);									//ѡ��Ƶ��bar

private:
	//Ƶ��bar�ṹ��
    struct Bar 
	{
        Bar() : value(0.0), clipped(false) { }
        double   value;
        bool    clipped;										//Ƶ��bar�Ƿ񱻽ض�
    };

    QVector<Bar>        m_bars;									//Ƶ��bar vector
    int                 m_barSelected;							//Ƶ��bar�Ƿ�ѡ��
    int                 m_timerId;								//ʱ��id
    double              m_lowFreq;								//Ƶ���½�
    double              m_highFreq;								//Ƶ���Ͻ�
    FrequencySpectrum   m_spectrum;								//ͨ�����ٸ���Ҷ�任�������Ĳ���

	QPoint dragPosition;
};

#endif // SPECTROGRAPH_H
