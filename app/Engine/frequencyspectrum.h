#ifndef FREQUENCYSPECTRUM_H
#define FREQUENCYSPECTRUM_H

#include <QtCore/QVector>

/**
	��ʾƵ��Ϊһϵ��Ԫ�أ�����ÿһ���� 
	����һ��Ƶ�ʣ��������λ
 */
//Ƶ��Ԫ�ؼ���(ͨ��Ƶ�׷����Ƕ���Ƶ���п��ٸ���Ҷ�任ȡ����Ƶ��
//Ƶ�ʣ���λ����������һ��Ƶ��Ԫ�ؼ����ٴ��ݸ�Ƶ�׻����������)
class FrequencySpectrum 
{
public:
    FrequencySpectrum(int numPoints = 0);

    struct Element 
	{
        Element() : frequency(0.0), amplitude(0.0), phase(0.0), clipped(false)
        { }
        double frequency;		//����Ƶ��
        double amplitude;		//�����Χ[0.0, 1.0]
        double phase;			//��λ��Χ[0.0, 2*PI]
        bool clipped;			//ָʾ�Ƿ�ֵʱƵ�׷������ض�
    };

    typedef QVector<Element>::iterator iterator;
    typedef QVector<Element>::const_iterator const_iterator;

    void reset();	//����

    int count() const;
    Element& operator[](int index);
    const Element& operator[](int index) const;
    iterator begin();
    iterator end();
    const_iterator begin() const;
    const_iterator end() const;

private:
    QVector<Element> m_elements;

};

#endif // FREQUENCYSPECTRUM_H
