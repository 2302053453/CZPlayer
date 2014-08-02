#include "stdafx.h"
#include "spectrograph.h"

const int NullTimerId = -1;
const int NullIndex = -1;
const int BarSelectionInterval = 2000;	//Ƶ��bar�����ѡ��

Spectrograph::Spectrograph(QWidget *parent) : QLabel(parent) 
	, m_barSelected(NullIndex) , m_timerId(NullTimerId) , m_lowFreq(0.0) , m_highFreq(0.0)
{
	//����Ƶ��label�Ĵ�С
	this->resize(190, 78);
}

Spectrograph::~Spectrograph()
{
}

//����Ƶ�ײ���
void Spectrograph::setParams(int numBars, double lowFreq, double highFreq)
{
    Q_ASSERT(numBars > 0);
    Q_ASSERT(highFreq > lowFreq);
    m_bars.resize(numBars);
    m_lowFreq = lowFreq;		//Ƶ���½�
    m_highFreq = highFreq;		//Ƶ���Ͻ�
    this->updateBars();		//����Ƶ��
}

//����ʵ��ʱ���¼�
//void Spectrograph::timerEvent(QTimerEvent *event)
//{
//    Q_ASSERT(event->timerId() == m_timerId);
//    Q_UNUSED(event) // suppress warnings in release builds
//    killTimer(m_timerId);
//    m_timerId = NullTimerId;
//    m_barSelected = NullIndex;
//    update();
//}

//����Ƶ��
void Spectrograph::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)

    QPainter painter(this);
	//QPixmap backgroundImage;
	//backgroundImage.load(":/images/screen.png");
	////��ͨ��pix�ķ������ͼƬ�Ĺ��˵�͸���Ĳ��ֵõ���ͼƬ����ΪloginPanel�Ĳ�����߿�
	//this->setMask(backgroundImage.mask());
	//painter.drawPixmap(0, 0, 190, 78, backgroundImage);

    const int numBars = m_bars.count();

	QColor barColor(5, 184, 204);		//Ƶ��bar��ɫ
	QColor clipColor(255, 0, 0);		//Ƶ�ױ��ضϺ����ɫ

    barColor = barColor.lighter();
    barColor.setAlphaF(0.75);		//����alphaͨ��
    clipColor.setAlphaF(0.75);

    //����Ƶ��
    if (numBars)
	{
        //�����ȵ����Ϳհ�
		const int widgetWidth = this->width();										//Ƶ��widget���
        const int barPlusGapWidth = widgetWidth / numBars;							//ÿһ��Ƶ�׼ӿհ׼�϶�Ŀ��
        const int barWidth = 0.8 * barPlusGapWidth;									//ÿһ��Ƶ��bar�Ŀ��
        const int gapWidth = barPlusGapWidth - barWidth;							//ÿһ���հ׼�϶���
        const int paddingWidth = widgetWidth - numBars * (barWidth + gapWidth);		//��Ե���
        const int leftPaddingWidth = (paddingWidth + gapWidth) / 2;					//���Ե���
        const int barHeight = this->height() - 2 * gapWidth;						//ÿһ��Ƶ��bar�ĸ߶�

		//����ÿһ��Ƶ��bar
        for (int i = 0; i < numBars; ++i)
		{
            const double value = m_bars[i].value;		//vlaue��ֵ��0��1֮��
            Q_ASSERT(value >= 0.0 && value <= 1.0);

            QRect bar = rect();
			//����Ƶ��bar��λ�úʹ�С
            bar.setLeft(rect().left() + leftPaddingWidth + (i * (gapWidth + barWidth)));
            bar.setWidth(barWidth);
            bar.setTop(rect().top() + gapWidth + (1.0 - value) * barHeight);
            bar.setBottom(rect().bottom() - gapWidth);

            QColor color = barColor;
            if (m_bars[i].clipped)
			{
                color = clipColor;
			}
			
			//������ɫ����
			//QLinearGradient linearGradient(bar.topLeft(), bar.bottomRight());
			//linearGradient.setColorAt(0.1, QColor(247, 104, 9));
			//linearGradient.setColorAt(1.0, QColor(238, 17, 128)); 
			//painter.fillRect(bar, QBrush(linearGradient));

            painter.fillRect(bar, color);
        }
    }
	event->accept();
}

//ˢ��Ƶ��bar
void Spectrograph::updateBars()
{
	m_bars.fill(Bar());

	//����Ƶ�ײ���
	for (FrequencySpectrum::const_iterator iter = m_spectrum.begin(); iter != m_spectrum.end(); ++iter)
	{
		const FrequencySpectrum::Element element = *iter;
		//����������ٸ���Ҷ����Ƶ������ڡ�0.0-1.0��֮��
		if (element.frequency >= m_lowFreq && element.frequency < m_highFreq) 
		{
			Bar &bar = m_bars[barIndex(element.frequency)];
			bar.value = qMax(bar.value, element.amplitude);	//Ƶ��bar�����Χ
			bar.clipped |= element.clipped;
		}
	}
	update();
}

//����ʵ��������¼�
//void Spectrograph::mousePressEvent(QMouseEvent *event)
//{
//    const QPoint pos = event->pos();
//    const int index = m_bars.count() * (pos.x() - rect().left()) / rect().width();
//    this->selectBar(index);		//ѡ�е�ǰƵ��bar
//}

//ѡ��Ƶ��bar
//void Spectrograph::selectBar(int index) 
//{
//	const QPair<double, double> frequencyRange = barRange(index);
//	const QString message = QString("%1 - %2 Hz")
//		.arg(frequencyRange.first)
//		.arg(frequencyRange.second);
//	emit infoMessage(message, BarSelectionInterval);
//
//	if (NullTimerId != m_timerId)
//	{
//		killTimer(m_timerId);
//	}
//	m_timerId = startTimer(BarSelectionInterval);
//	m_barSelected = index;
//	update();
//}

//����Ƶ��
void Spectrograph::reset()
{
    m_spectrum.reset();
    this->spectrumChanged(m_spectrum);	//�ı�Ƶ��
}

//�ı�Ƶ��
void Spectrograph::spectrumChanged(const FrequencySpectrum &spectrum)
{
    m_spectrum = spectrum;
    this->updateBars();	//ˢ��Ƶ��
}

//����Ƶ��bar
int Spectrograph::barIndex(double frequency) const
{
    Q_ASSERT(frequency >= m_lowFreq && frequency < m_highFreq);
    const double bandWidth = (m_highFreq - m_lowFreq) / m_bars.count();
    const int index = (frequency - m_lowFreq) / bandWidth;
    if(index <0 || index >= m_bars.count())
	{
        Q_ASSERT(false);
	}
    return index;
}

//Ƶ��bar�ķ�Χ
QPair<double, double> Spectrograph::barRange(int index) const
{
    Q_ASSERT(index >= 0 && index < m_bars.count());
    const double bandWidth = (m_highFreq - m_lowFreq) / m_bars.count();
    return QPair<double, double>(index * bandWidth, (index+1) * bandWidth);
}


