#pragma once

#include <JuceHeader.h>
#include "StringSynthesiser.h"
#include <atomic>

//==============================================================================
class StringUIdemoAudioProcessor : public juce::AudioProcessor
{
public:
    static const int numStrings = 6;

    // Accordatura di default (E2 A2 D3 G3 B3 E4) - usata come riferimento iniziale
    static const int defaultMidiNotes[numStrings];

    // Range MIDI consentito per il tuning (±12 semitoni rispetto al default)
    static const int tuningRangeSemitones = 12;

    // Variabili per il meter del volume in decibel
    std::atomic<float> masterRmsLeft{ -60.0f }; // Valore RMS in decibel per il canale sinistro
    std::atomic<float> masterRmsRight{ -60.0f }; // Valore RMS in decibel per il canale destro

    #pragma region Variabile APVTS per controllo Effettistica (UI)

    juce::AudioProcessorValueTreeState apvts;

    #pragma endregion

    #pragma region Variabili Atomic

	// Atomic per gestire in maniera thread-safe le modifiche alla UI da parte del processBlock
    std::atomic<bool> uiStringWasPlucked[numStrings];
	std::atomic<float> uiPluckPosition[numStrings];


    #pragma endregion



    StringUIdemoAudioProcessor();
    ~StringUIdemoAudioProcessor() override;

    //==========================================================================
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

#ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
#endif

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    // Puntatore all'oscilloscopio, usato per visualizzare le onde sonore
    juce::AudioVisualiserComponent* puntatoreOscilloscopio = nullptr;

    const juce::String getName() const override;
    bool acceptsMidi()  const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    //==========================================================================
    // Pizzica la corda (la frequenza viene calcolata da currentMidiNotes + fret)
    void pluckString(int stringIndex, float position);

    // Tuning: imposta la nota MIDI base per la corda stringIndex
    void setStringMidiNote(int stringIndex, int newMidiNote);

    // Restituisce la nota MIDI base corrente per la corda stringIndex
    int getStringMidiNote(int stringIndex) const;

    // Reimposta tutte le corde all'accordatura di default
    void resetTuning();

    // Gestione ADSR: rilascio note
    void releaseAllStrings();
    void noteOffString(int stringIndex);

    // Struttura e metodi per accordature personalizzate
    struct CustomTuning
    {
        juce::String name;
        std::array<int, numStrings> notes;
    };

    static juce::File getCustomTuningsFile();
    std::vector<CustomTuning> loadCustomTunings();
    void saveCustomTuning(const juce::String& name, const std::array<int, numStrings>& notes);
    void deleteCustomTuning(const juce::String& name);
    void applyTuning(const std::array<int, numStrings>& notes);

private:

    #pragma region Parametri per Effettistica (UI)

    // Funzione per creare i parametri da gestire con l'APVTS
    juce::AudioProcessorValueTreeState::ParameterLayout createParameters();

	// Puntatori atomici (thread-safe) per i parametri di controllo dell'effettistica
	std::atomic<float>* driveParameter = nullptr;
    std::atomic<float>* gainParameter = nullptr;
    std::atomic<float>* hardnessParameter = nullptr;
    std::atomic<float>* dampingParameter = nullptr;
    std::atomic<float>* sustainParameter = nullptr;
    std::atomic<float>* revMixParameter = nullptr;
    std::atomic<float>* revSizeParameter = nullptr;
    std::atomic<float>* delayTimeParameter = nullptr;
    std::atomic<float>* delayFbParameter = nullptr;
    std::atomic<float>* masterVolumeParameter = nullptr;
    // Parametri del Phaser
    std::atomic<float>* phaserRateParameter = nullptr;
    std::atomic<float>* phaserDepthParameter = nullptr;
    std::atomic<float>* phaserMixParameter = nullptr;
    std::atomic<float>* phaserOnParameter = nullptr;

    // Parametri dell'inviluppo ADSR
    std::atomic<float>* adsrAttackParameter = nullptr;
    std::atomic<float>* adsrDecayParameter = nullptr;
    std::atomic<float>* adsrSustainParameter = nullptr;
    std::atomic<float>* adsrReleaseParameter = nullptr;
    std::atomic<float>* adsrOnParameter = nullptr;

    // Istanza dell'effetto Phaser (dal modulo DSP di JUCE)
    juce::dsp::Phaser<float> phaser;

    // Puntatori per accensione / spegnimento effetti
	// Si usano i float come bool perchè l'APVTS gestisce i parametri come float, ma si considerano "on" se >= 0.5f
	std::atomic<float>* distOnParameter = nullptr;
	std::atomic<float>* delayOnParameter = nullptr;
	std::atomic<float>* revOnParameter = nullptr;

    // Tracciamento note MIDI attive per ogni corda (per gestione NoteOff ADSR)
    int activeMidiNoteForString[numStrings] = { -1, -1, -1, -1, -1, -1 };

    #pragma endregion

    // Modulo Riverbero di JUCE
    juce::Reverb reverb;
    juce::Reverb::Parameters reverbParams;

    // Variabili per il Delay
    juce::AudioBuffer<float> delayBuffer;
    int delayWritePosition = 0;
    double currentSampleRate = 44100.0;

    // Nota MIDI base per ciascuna corda (modificabile dal tuning UI)
    int currentMidiNotes[numStrings];

    juce::OwnedArray<StringSynthesiser> stringSynths;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(StringUIdemoAudioProcessor)
};