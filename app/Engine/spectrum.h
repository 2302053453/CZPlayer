#ifndef SPECTRUM_H
#define SPECTRUM_H

#include <QtCore>
#include "utils.h"
#include "fftreal_wrapper.h"

//���ڼ���Ƶ����Ƶ������
const int SpectrumLengthSamples		 = PowerOfTwo<FFTLengthPowerOfTwo>::Result;
const int    SpectrumNumBands		 = 25;						//Ƶ����ʾ�������
const double  SpectrumLowFreq        = 0.0;						//Ƶ���½�,��λHZ
const double  SpectrumHighFreq       = 1000.0;					//Ƶ���Ͻ�,��λHZ
const long long WaveformWindowDuration = 500 * 1000;			//΢�벨�δ��ڴ�С

//waveform�ĳ��ȣ����ֽ�Ϊ��λ
//����������£���Щ��ƥ��QAudio*::bufferSize()�������ⲻ��
//�ã�ֱ��QAudio*::start()�����ã�������
//Ϊ�˳�ʼ��������ʾ��Ҫ���ֵ��
//��ˣ�����ֻҪѡ��һ�������ֵ
const int   WaveformTileLength  = 4096;
const double SpectrumAnalyserMultiplier = 0.15;					//���ڼ���Ƶ�����ĸ߶�
const int   NullMessageTimeout      = -1;						//������Ϣ��ʱ


//-----------------------------------------------------------------------------
// Types and data structures
//-----------------------------------------------------------------------------

enum WindowFunction
{
    NoWindow,
    HannWindow
};

const WindowFunction DefaultWindowFunction = HannWindow;

struct Tone 
{
    Tone(double freq = 0.0, double amp = 0.0)
    :   frequency(freq), amplitude(amp)
    { }

    // Start and end frequencies for swept tone generation
    double   frequency;

    // Amplitude in range [0.0, 1.0]
    double   amplitude;
};

struct SweptTone 
{
    SweptTone(double start = 0.0, double end = 0.0, double amp = 0.0)
    :   startFreq(start), endFreq(end), amplitude(amp)
    { 
		Q_ASSERT(end >= start);
	}

    SweptTone(const Tone &tone)
    :   startFreq(tone.frequency), endFreq(tone.frequency), amplitude(tone.amplitude)
    {
	}

    // Start and end frequencies for swept tone generation
    double   startFreq;
    double   endFreq;

    // Amplitude in range [0.0, 1.0]
    double   amplitude;
};


//-----------------------------------------------------------------------------
// Macros
//-----------------------------------------------------------------------------

// Macro which connects a signal to a slot, and which causes application to
// abort if the connection fails.  This is intended to catch programming errors
// such as mis-typing a signal or slot name.  It is necessary to write our own
// macro to do this - the following idiom
//     Q_ASSERT(connect(source, signal, receiver, slot));
// will not work because Q_ASSERT compiles to a no-op in release builds.

#define connect(source, signal, receiver, slot) \
    if(!connect(source, signal, receiver, slot)) \
        qt_assert_x(Q_FUNC_INFO, "connect failed", __FILE__, __LINE__);

// Handle some dependencies between macros defined in the .pro file

#ifdef DISABLE_WAVEFORM
#undef SUPERIMPOSE_PROGRESS_ON_WAVEFORM
#endif

#endif // SPECTRUM_H

