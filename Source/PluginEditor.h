#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "StringComponent.h"
#include "KnobStyle.h"

//==============================================================================
class StringUIdemoAudioProcessorEditor : public juce::AudioProcessorEditor, public juce::Timer
{
public:
    explicit StringUIdemoAudioProcessorEditor(StringUIdemoAudioProcessor&);
    ~StringUIdemoAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

    // Colori delle corde (verde scalare, basso → alto)
    static juce::Colour stringColour(int index)
    {
        const juce::Colour colours[6] = {
            juce::Colour(0xFF008000),  // E2
            juce::Colour(0xFF00CF00),  // A2
            juce::Colour(0xFF1FFF1F),  // D3
            juce::Colour(0xFF6FFF6F),  // G3
            juce::Colour(0xFF96FF96),  // B3
            juce::Colour(0xFFCCFFCC)   // E4
        };
        return colours[index % 6];
    }

private:

    /// Titoli delle sezioni
    static constexpr int numSezioni = 8;
	juce::Label titoloSezione[numSezioni];

    // Manopole
    KnobStyle stilePomello;
    // 0: Time, 1: Feedback, 2: Drive, 3: Gain, 4: Hardness, 5: Damping, 6: Sustain,
    // 7: Rev Mix, 8: Rev Size, 9: Master, 10: P. Rate, 11: P. Depth, 12: P. Mix,
    // 13: Attack, 14: Decay, 15: ADSR Sustain, 16: Release
    static constexpr int numManopole = 17;
    juce::Slider manopolaEffetto[numManopole];
    juce::Label titoloManopolaEffetto[numManopole];

	// Attachment per collegamenti con APVTS (Componente UI <-> Parametro)
	std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> driveAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> gainAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> timeAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> feedbackAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> hardnessAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> dampingAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> sustainAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> revMixAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> revSizeAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> phaserRateAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> phaserDepthAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> phaserMixAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> masterAttachment;

    // Attachment per ADSR
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attackAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> decayAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> adsrSustainAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> releaseAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> atcAdsrOn;

	// Callback del Timer (Per interazione Audio Thread -> UI Thread per la MIDI)
	void timerCallback() override;

    // Mouse
    void mouseDown(const juce::MouseEvent& e) override { handleMouseEvent(e); }
    void mouseDrag(const juce::MouseEvent& e) override { handleMouseEvent(e); }
    void mouseUp(const juce::MouseEvent& e) override
    {
        oldPosFret = -1;
        oldMidiNote = -1;
        audioProcessor.releaseAllStrings();
    }
    void handleMouseEvent(const juce::MouseEvent& e);

    // Paint helpers
    void SetTitle(juce::Graphics&);
    void SetLineaSeparatrice(juce::Graphics&);
    void SetStrings(juce::Graphics&);
    void SetSeparationFret(juce::Graphics&);

    // Tuning helpers
    void updateTuningLabel(int stringIndex);
    void updateAllTuningLabels();
    void populateTuningMenu();
    void promptSaveCustomTuning();
    void deleteSelectedCustomTuning();
    void applySelectedTuning(int itemId);

	// Sezioni della UI
    juce::Rectangle<int> areaOscilloscopio;
    juce::Rectangle<int> areaMaster;
    juce::Rectangle<int> areaParametriFisici;
    juce::Rectangle<int> areaADSR;
    juce::Rectangle<int> areaDelay;
    juce::Rectangle<int> areaDistortion;
    juce::Rectangle<int> areaReverb;
    juce::Rectangle<int> areaPhaser;
    juce::Rectangle<int> areaCordeSotto;

    // Dati
    StringUIdemoAudioProcessor& audioProcessor;

    juce::OwnedArray<StringComponent> stringComponents;

    // Controlli tuning: per ogni corda un pulsante "-", una label, un pulsante "+"
    juce::OwnedArray<juce::TextButton> tuningDownButtons;  // [−]
    juce::OwnedArray<juce::TextButton> tuningUpButtons;    // [+]
    juce::OwnedArray<juce::Label>      tuningLabels;       // "E2 (+0)"

    // Pulsanti e menu gestione accordatura
    juce::TextButton resetTuningButton;
    juce::ComboBox   tuningMenu;
    juce::TextButton saveTuningButton{ "Salva" };
    juce::TextButton deleteTuningButton{ "Elimina" };

    // Label nota suonata corrente
    juce::Label notaSuonataLabel;

    // Costanti layout
    const int numFret = 12;
    const int numCorde = 6;

    static constexpr int tuningPanelWidth = 110;

    int oldPosFret = -1;
    int oldMidiNote = -1;

    // Sezione oscilloscopio
    juce::AudioVisualiserComponent oscilloscopio{ 2 };

    // Rettangoli per disegnare i meter
	juce::Rectangle<int> meterLeftArea;
	juce::Rectangle<int> meterRightArea;

    float levelLeftScaled = 0.0f;
	float levelRightScaled = 0.0f;

	// Pulsanti ON/OFF per effetti
    juce::TextButton btnDelayOn{ "ON" };
    juce::TextButton btnDistOn{ "ON" };
    juce::TextButton btnRevOn{ "ON" };
    juce::TextButton btnPhaserOn{ "ON" };
    juce::TextButton btnAdsrOn{ "ON" };

    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> atcDelayOn;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> atcDistOn;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> atcRevOn;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> atcPhaserOn;

    // Sezione dei menu a tendina per i preset
    juce::ComboBox presetMenu;
    
    // Funzione per applicare i valori
    void applicaPreset(int presetId);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(StringUIdemoAudioProcessorEditor)
};