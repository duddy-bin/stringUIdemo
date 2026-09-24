#include <JuceHeader.h>

class StringSynthesiser
{
public:
    StringSynthesiser(double sampleRate, double frequencyInHz,float hardness) : fs(sampleRate),currentHardness(hardness)
    {
        doPluckForNextBuffer.set(false);
        maxDelayLength = (size_t)juce::roundToInt(sampleRate / 20.0);
        delayLine.resize(maxDelayLength, 0.0f);
        excitationSample.resize(maxDelayLength, 0.0f);
        setFrequency(frequencyInHz);

        adsr.setSampleRate(sampleRate);
        juce::ADSR::Parameters defaultAdsrParams;
        defaultAdsrParams.attack = 0.01f;
        defaultAdsrParams.decay = 0.5f;
        defaultAdsrParams.sustain = 1.0f;
        defaultAdsrParams.release = 1.0f;
        adsr.setParameters(defaultAdsrParams);
    }

    void SetHardness(float h) {
        currentHardness = juce::jlimit(0.01f, 1.0f, h);
    }

    void SetDamping(float d) {
        currentDamping = juce::jlimit(0.0f,1.0f,d);
    }

    void SetSustain(float s) {
        currentSustain = juce::jlimit(0.0f, 1.0f, s);
    }

    void setAdsrParameters(float attackSec, float decaySec, float sustainLevel, float releaseSec)
    {
        juce::ADSR::Parameters p;
        p.attack = attackSec;
        p.decay = decaySec;
        p.sustain = sustainLevel;
        p.release = releaseSec;
        adsr.setParameters(p);
    }

    void setAdsrEnabled(bool enabled)
    {
        adsrEnabled = enabled;
    }

    void noteOff()
    {
        adsr.noteOff();
    }

    void resetAdsr()
    {
        adsr.reset();
    }

    void stringPlucked(float pluckPosition)
    {
        if (doPluckForNextBuffer.compareAndSetBool(1, 0))
            amplitude = std::sin(juce::MathConstants<float>::pi * pluckPosition);
    }

    void generateAndAddData(float* outBuffer, int numSamples)
    {
        if (doPluckForNextBuffer.compareAndSetBool(0, 1))
        {
            exciteInternalBuffer();
            adsr.reset();
            adsr.noteOn();
        }

        float dampCoeff = currentDamping * 0.5f;
        float feedbackGain = 0.9f + currentSustain * 0.099f;

        for (int i = 0; i < numSamples; ++i)
        {
            auto nextPos = (pos + 1) % currentDelayLength;

            // 1) filtro di damping (passa-basso)
            float filtered = delayLine[pos] * (1.0f - dampCoeff) + delayLine[nextPos] * dampCoeff;

            // 2) allpass per il fractional delay (intonazione fine)
            float allpassOut = allpassCoeff * filtered + allpassInputPrec - allpassCoeff * allpassOutputPrec;
            allpassInputPrec = filtered; // aggiorno valore input precedente allpass
            allpassOutputPrec = allpassOut; // aggiorno valore output precedente allpass

            // 3) feedback nel buffer
            delayLine[nextPos] = allpassOut * feedbackGain;

            float sample = delayLine[pos];
            if (adsrEnabled)
            {
                sample *= adsr.getNextSample();
            }

            outBuffer[i] += sample;
            pos = nextPos;
        }
    }

    void setFrequency(double newFrequencyInHz)
    {
        double exactDelay = fs / newFrequencyInHz;
        size_t N = (size_t)std::floor(exactDelay);
        float frac = (float)(exactDelay - (double)N);

        // l'allpass gestisce la frazione, quindi il buffer usa solo la parte intera
        currentDelayLength = juce::jlimit((size_t)2, maxDelayLength, N);

        // coefficiente allpass (Jaffe-Smith)
        allpassCoeff = (1.0f - frac) / (1.0f + frac);

        pos = 0;
        allpassInputPrec = 0.0f;
        allpassOutputPrec = 0.0f;
        generateExcitation();
    }

private:

    /// <summary>
    /// Riempie excitationSample con un buffer di rumore che simula il colpo iniziale sulla corda 
    /// Il "burst" di energia che poi il delay line di Karplus-Strong fa rimbalzare in loop.
    /// </summary>
    void generateExcitation() {
        float lastSample = 0.0f;
        float maxVal = 0.0f;

        for (size_t i = 0; i < currentDelayLength; ++i)
        {
            //genera rumore bianco casuale tra -1 e 1 (ogni campione è indipendente dal precedente)
            float noise = (juce::Random::getSystemRandom().nextFloat() * 2.0f) - 1.0f;
            //media pesata tra il campione nuovo e il precedente
            //dove 1 = plettro e 0 = dito
            //un po' come un filtro passa-basso di primo ordine
            //dove più l'hardness è bassa e più taglia gli acuti
            float shaped = noise * currentHardness + lastSample * (1.0f - currentHardness);
            excitationSample[i] = shaped;
            //tengo traccia del valore precedente (per il filtro)
            lastSample = shaped;
            //e del valore massimo raggiunto per la normalizzazione
            maxVal = std::max(maxVal, std::abs(shaped));
        }

        //normalizzo per evitare l'abbassamento di volume
        if (maxVal > 0.0f)
        {
            for (size_t i = 0; i < currentDelayLength; ++i)
                excitationSample[i] /= maxVal;//divido il buffer per il valore massimo così da avere il picco sempre a 1 
        }
    }

    void exciteInternalBuffer()
    {
        generateExcitation();
        for (size_t i = 0; i < currentDelayLength; ++i)
            delayLine[i] = excitationSample[i] * (float)amplitude;
    }

    double fs;
    size_t maxDelayLength;
    size_t currentDelayLength = 0;

    const double decay = 0.998;
    double amplitude = 0.0;

    float currentHardness = 0.5f;
    float currentDamping = 0.5f;
    float currentSustain = 0.8f;

    float allpassCoeff = 0.0f;
    float allpassInputPrec = 0.0f;
    float allpassOutputPrec = 0.0f;

    juce::Atomic<int> doPluckForNextBuffer;
    std::vector<float> excitationSample, delayLine;
    size_t pos = 0;

    juce::ADSR adsr;
    bool adsrEnabled = true;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(StringSynthesiser)
};