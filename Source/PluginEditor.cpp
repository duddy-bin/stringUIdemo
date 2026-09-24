#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
StringUIdemoAudioProcessorEditor::StringUIdemoAudioProcessorEditor(StringUIdemoAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{

    #pragma region Visibilita sfondo corde
        // Corde visive
        for (int i = 0; i < StringUIdemoAudioProcessor::numStrings; ++i)
        {
            auto* sc = stringComponents.add(new StringComponent(stringColour(i)));
            addAndMakeVisible(sc);
        }
    #pragma endregion

    #pragma region accordatura corde setup
        // Controlli tuning (uno per corda)
        for (int i = 0; i < StringUIdemoAudioProcessor::numStrings; ++i)
        {
            // Pulsante [−]
            auto* btnDown = tuningDownButtons.add(new juce::TextButton("-"));
            // quando premo il pulsante...
            btnDown->onClick = [this, i]()
                {
                    int current = audioProcessor.getStringMidiNote(i);
                    audioProcessor.setStringMidiNote(i, current - 1);
                    updateTuningLabel(i);
                };
            addAndMakeVisible(btnDown);

            // Label con nome nota + delta semitoni
            auto* lbl = tuningLabels.add(new juce::Label());
            lbl->setJustificationType(juce::Justification::centred);
            lbl->setFont(juce::FontOptions(11.0f, juce::Font::bold));
            lbl->setColour(juce::Label::textColourId, stringColour(i));
            addAndMakeVisible(lbl);

            // Pulsante [+]
            auto* btnUp = tuningUpButtons.add(new juce::TextButton("+"));
            //quando premo il pulsante...
            btnUp->onClick = [this, i]()
                {
                    int current = audioProcessor.getStringMidiNote(i);
                    audioProcessor.setStringMidiNote(i, current + 1);
                    updateTuningLabel(i);
                };
            addAndMakeVisible(btnUp);

        }  

        // Pulsante Reset
        resetTuningButton.setButtonText("Reset");
        resetTuningButton.onClick = [this]()
            {
                audioProcessor.resetTuning();
                updateAllTuningLabels();
                tuningMenu.setSelectedId(1, juce::dontSendNotification);
                deleteTuningButton.setEnabled(false);
            };
        addAndMakeVisible(resetTuningButton);

        // Menu e pulsanti accordatura personalizzata
        tuningMenu.setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xFF242424));
        tuningMenu.setColour(juce::ComboBox::textColourId, juce::Colours::white);
        tuningMenu.setColour(juce::ComboBox::outlineColourId, juce::Colour(0xFF4D453A));
        tuningMenu.setJustificationType(juce::Justification::centred);
        tuningMenu.onChange = [this]() { applySelectedTuning(tuningMenu.getSelectedId()); };
        addAndMakeVisible(tuningMenu);

        saveTuningButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xFF242424));
        saveTuningButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
        saveTuningButton.onClick = [this]() { promptSaveCustomTuning(); };
        addAndMakeVisible(saveTuningButton);

        deleteTuningButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xFF242424));
        deleteTuningButton.setColour(juce::TextButton::textColourOffId, juce::Colours::lightgrey);
        deleteTuningButton.onClick = [this]() { deleteSelectedCustomTuning(); };
        addAndMakeVisible(deleteTuningButton);
        deleteTuningButton.setEnabled(false);

        populateTuningMenu();
        tuningMenu.setSelectedId(1, juce::dontSendNotification);

        // Inizializza tutte le label di tuning con i valori correnti
        updateAllTuningLabels();
    #pragma endregion

    // Label nota suonata
    addAndMakeVisible(notaSuonataLabel);
    notaSuonataLabel.setText("Nota", juce::NotificationType::dontSendNotification);
    notaSuonataLabel.setFont(juce::FontOptions(13.0f));
    notaSuonataLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    notaSuonataLabel.setJustificationType(juce::Justification::centred);

    #pragma region Setup Preset Menu
        // Setup preset menu
        addAndMakeVisible(presetMenu);

        // Aggiungiamo le voci (Il primo parametro è il nome, il secondo è l'ID univoco che deve partire da 1)
        presetMenu.addItem("Default", 1);
		presetMenu.addItem("Dream Harp", 2);
		presetMenu.addItem("Electric", 3);
		presetMenu.addItem("Bass", 4);

		applicaPreset(1); // Applico il preset di default all'avvio

		// Setup dei colori e dell'allineamento del testo
        presetMenu.setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xFF242424));
        presetMenu.setColour(juce::ComboBox::textColourId, juce::Colours::white);
        presetMenu.setColour(juce::ComboBox::outlineColourId, juce::Colour(0xFF4D453A));
        presetMenu.setJustificationType(juce::Justification::centred);

		// Impostiamo il preset di default come selezionato all'avvio
        presetMenu.setSelectedId(1);

		// Gestione di quando cambia il preset selezionato
        presetMenu.onChange = [this]() {applicaPreset(presetMenu.getSelectedId());};
    #pragma endregion

    #pragma region Setup Titoli Sezioni
            // Definiamo i nomi delle 8 macro-aree
            juce::String nomiSezioni[numSezioni] = {
                "OSCILLOSCOPIO", "MASTER VOLUME", "PARAMETRI FISICI", "ENVELOPE ADSR", "DISTORTION", "DELAY", "PHASER", "REVERB"
            };

            for (int i = 0; i < numSezioni; ++i)
            {
                titoloSezione[i].setText(nomiSezioni[i], juce::dontSendNotification);
                titoloSezione[i].setJustificationType(juce::Justification::centred);
                titoloSezione[i].setColour(juce::Label::textColourId, juce::Colours::grey);
                addAndMakeVisible(titoloSezione[i]);
            }
    #pragma endregion

    #pragma region Setup monopole
                // Definizione dei nomi delle manopole (17 manopole totali)
                juce::String nomiManopole[numManopole] = {
                    "Time", "Feedback",               // Delay (0, 1)
                    "Drive", "Gain",                  // Distortion (2, 3)
                    "Hardness", "Damping", "Sustain", // Physical (4, 5, 6)
                    "Rev Mix", "Rev Size",            // Reverb (7, 8)
                    "Master",                         // Master Section (9)
                    "P. Rate", "P. Depth", "P. Mix",  // Phaser (10, 11, 12)
                    "Attack", "Decay", "Sustain", "Release" // ADSR (13, 14, 15, 16)
                };

                // Manopole
                for (int i = 0; i < numManopole; ++i)
                {
                    manopolaEffetto[i].setSliderStyle(juce::Slider::Rotary);
                    manopolaEffetto[i].setTextBoxStyle(juce::Slider::TextBoxBelow, false, 40, 15);
                    manopolaEffetto[i].setColour(juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
                    manopolaEffetto[i].setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
                    manopolaEffetto[i].setColour(juce::Slider::textBoxTextColourId, juce::Colours::white);

                    // Sezione per le unità di misura
                    if (nomiManopole[i] == "Time" || nomiManopole[i] == "Attack" || nomiManopole[i] == "Decay" || nomiManopole[i] == "Release") {
                        manopolaEffetto[i].setTextValueSuffix(" s");
                    }
                    else if (nomiManopole[i] == "P. Rate") {
                        manopolaEffetto[i].setTextValueSuffix(" Hz");
                    }
                    else if (nomiManopole[i] != "Drive" && nomiManopole[i] != "Gain" &&
                        nomiManopole[i] != "Hardness" && nomiManopole[i] != "P. Depth") {
                        // Per esclusione (Feedback, Damping, Sustain, Mix vari)
                        manopolaEffetto[i].setTextValueSuffix(" %");
                    }

                    manopolaEffetto[i].setLookAndFeel(&stilePomello);
                    addAndMakeVisible(manopolaEffetto[i]);

                    titoloManopolaEffetto[i].setText(nomiManopole[i], juce::dontSendNotification);
                    titoloManopolaEffetto[i].setJustificationType(juce::Justification::centred);
                    titoloManopolaEffetto[i].setColour(juce::Label::textColourId, juce::Colours::white);
                    addAndMakeVisible(titoloManopolaEffetto[i]);
                }
    #pragma endregion

    setSize(1280, 720);

    #pragma region Timer

	// Avvio il timer per controllare le interazioni Audio Thread -> UI Thread (per la MIDI)
	startTimerHz(60); // Timer che scade 60 volte al secondo (ogni ~16ms)

    #pragma endregion

    #pragma region Setup bottoni On / Off

        juce::TextButton* bypassButtons[] = { &btnDelayOn, &btnDistOn, &btnRevOn, &btnPhaserOn, &btnAdsrOn };
        juce::String bypassIDs[] = { "delayOn", "distOn", "revOn", "phaserOn", "adsrOn" };

        for (int i = 0; i < 5; ++i)
        {
            bypassButtons[i]->setClickingTogglesState(true);
            // Colore da spento (grigio scuro)
            bypassButtons[i]->setColour(juce::TextButton::buttonColourId, juce::Colour(0xFF242424));
            // Colore da acceso (viola neon)
            bypassButtons[i]->setColour(juce::TextButton::buttonOnColourId, juce::Colours::purple);

            addAndMakeVisible(bypassButtons[i]);
        }

    #pragma endregion

    #pragma region Attachments

        timeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            audioProcessor.apvts, "delayTime", manopolaEffetto[0]);
        feedbackAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            audioProcessor.apvts, "delayFb", manopolaEffetto[1]);
        driveAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            audioProcessor.apvts, "drive", manopolaEffetto[2]);
        gainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            audioProcessor.apvts, "gain", manopolaEffetto[3]);
        hardnessAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            audioProcessor.apvts, "hardness", manopolaEffetto[4]);
        dampingAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            audioProcessor.apvts, "damping", manopolaEffetto[5]);
        sustainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            audioProcessor.apvts, "sustain", manopolaEffetto[6]);
        revMixAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            audioProcessor.apvts, "revMix", manopolaEffetto[7]);
        revSizeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            audioProcessor.apvts, "revSize", manopolaEffetto[8]);
        masterAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            audioProcessor.apvts, "masterVolume", manopolaEffetto[9]);
        phaserRateAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            audioProcessor.apvts, "phaserRate", manopolaEffetto[10]);
        phaserDepthAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            audioProcessor.apvts, "phaserDepth", manopolaEffetto[11]);
        phaserMixAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            audioProcessor.apvts, "phaserMix", manopolaEffetto[12]);

        // ADSR Attachments
        attackAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            audioProcessor.apvts, "attack", manopolaEffetto[13]);
        decayAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            audioProcessor.apvts, "decay", manopolaEffetto[14]);
        adsrSustainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            audioProcessor.apvts, "adsrSustain", manopolaEffetto[15]);
        releaseAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            audioProcessor.apvts, "release", manopolaEffetto[16]);

        // Button Attachment
        atcDelayOn = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(audioProcessor.apvts, "delayOn", btnDelayOn);
        atcDistOn = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(audioProcessor.apvts, "distOn", btnDistOn);
        atcRevOn = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(audioProcessor.apvts, "revOn", btnRevOn);
        atcPhaserOn = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(audioProcessor.apvts, "phaserOn", btnPhaserOn);
        atcAdsrOn = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(audioProcessor.apvts, "adsrOn", btnAdsrOn);

    #pragma endregion
}

StringUIdemoAudioProcessorEditor::~StringUIdemoAudioProcessorEditor() 
{
	audioProcessor.puntatoreOscilloscopio = nullptr; // Rimuovo il puntatore all'oscilloscopio dal processor
	stopTimer(); // Ferma il timer quando l'editor viene distrutto
}

//==============================================================================

// Override del Callback del timer come specificato in PluginEditor.h
// Controllo la corda suonata da tastiera MIDI
void StringUIdemoAudioProcessorEditor::timerCallback() 
{
    // Controllo per ogni corda se la rispettiva flag è stata alzata dall'Audio Thread (processBlock)
	// Tutto tramite Polling dell'atomic <bool> uiStringWasPlucked[numStrings]
    for (int i = 0; i < StringUIdemoAudioProcessor::numStrings; ++i) 
    {
        // Check della flag, utilizzando il valore attuale e portandola a false
        if (audioProcessor.uiStringWasPlucked[i].exchange(false)) 
        {
            float position = audioProcessor.uiPluckPosition[i].load(); // Posizione del pizzico
			stringComponents.getUnchecked(i)->stringPlucked(position); // Aggiorna la visualizzazione della corda

            #pragma region Aggiornamento label nota suonata

            // Calcolo la fret della posizione relativa [0.0, 1.0]
			int fret = juce::jlimit(0, numFret, (int)(position * numFret));

			// Recupero la nota MIDI base della corda e vi sommo la fret per la nota MIDI effettiva suonata
			int midiNote = audioProcessor.getStringMidiNote(i) + fret;

            // Per il nome della nota riprendo la logica usata per esempio in HandleMouseEvent:
            // (sempre attraverso la utility di JUCE)
			juce::String nomeNota = juce::MidiMessage::getMidiNoteName(midiNote, true, true, 3);

            // Aggiorno la label di conseguenza
            notaSuonataLabel.setText("Nota: " + nomeNota + "  Tasto: " + juce::String(fret),
				juce::dontSendNotification);

            #pragma endregion
        }
    }
    #pragma region Aggiornamento testo bottoni On / Off
        // Il ButtonAttachment aggiorna in automatico il ToggleState
        btnDelayOn.setButtonText(btnDelayOn.getToggleState() ? "ON" : "OFF");
        btnDistOn.setButtonText(btnDistOn.getToggleState() ? "ON" : "OFF");
        btnRevOn.setButtonText(btnRevOn.getToggleState() ? "ON" : "OFF");
		btnPhaserOn.setButtonText(btnPhaserOn.getToggleState() ? "ON" : "OFF");
		btnAdsrOn.setButtonText(btnAdsrOn.getToggleState() ? "ON" : "OFF");
    #pragma endregion
    
    #pragma region Lettura Volume Meter
        // Leggiamo i decibel (-60 dB silenzio, 0 dB massimo)
        float rmsL = audioProcessor.masterRmsLeft.load();
        float rmsR = audioProcessor.masterRmsRight.load();

        // Mappiamo i decibel in un valore visivo da 0.0 (vuoto) a 1.0 (pieno)
        // Il range standard è da -60 dB a +3 dB
        float newLevelL = juce::jmap(rmsL, -60.0f, +3.0f, 0.0f, 1.0f);
        float newLevelR = juce::jmap(rmsR, -60.0f, +3.0f, 0.0f, 1.0f);

        // Limitiamo tra 0 e 1 per evitare errori grafici
        levelLeftScaled = juce::jlimit(0.0f, 1.0f, newLevelL);
        levelRightScaled = juce::jlimit(0.0f, 1.0f, newLevelR);

        // Diciamo a JUCE di ridisegnare solo la zona dei meter, per non appesantire la CPU
        repaint(meterLeftArea.expanded(2));
        repaint(meterRightArea.expanded(2));
    #pragma endregion

}

//==============================================================================

#pragma region paint UI

void StringUIdemoAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xFF201513));

    SetTitle(g);
    SetLineaSeparatrice(g);
    SetStrings(g);
    SetSeparationFret(g);

    #pragma region Disegno suddivisione delle aree
    g.setColour(juce::Colour(0xFF4D453A).withAlpha(0.4f));

    // Riempiamo le aree con rettangoli arrotondati
    // Nota: fillRoundedRectangle richiede coordinate float, quindi usiamo .toFloat()
    float cornerRadius = 8.0f; // Smussatura degli angoli (più è alto, più è rotondo)

    g.fillRoundedRectangle(areaOscilloscopio.reduced(4).toFloat(), cornerRadius);
    g.fillRoundedRectangle(areaMaster.reduced(4).toFloat(), cornerRadius);
    g.fillRoundedRectangle(areaParametriFisici.reduced(4).toFloat(), cornerRadius);
    g.fillRoundedRectangle(areaADSR.reduced(4).toFloat(), cornerRadius);
    g.fillRoundedRectangle(areaDelay.reduced(4).toFloat(), cornerRadius);
    g.fillRoundedRectangle(areaDistortion.reduced(4).toFloat(), cornerRadius);
    g.fillRoundedRectangle(areaReverb.reduced(4).toFloat(), cornerRadius);
    g.fillRoundedRectangle(areaPhaser.reduced(4).toFloat(), cornerRadius);
    #pragma endregion

    #pragma region Disegno Volume Meter
        // Sfondo scuro delle barrette (senza suono)
        g.setColour(juce::Colours::black.withAlpha(0.5f));
        g.fillRoundedRectangle(meterLeftArea.toFloat(), 2.0f);
        g.fillRoundedRectangle(meterRightArea.toFloat(), 2.0f);

        // Creiamo un gradiente verde per il volume (Verde in basso -> Giallo -> Rosso in alto)
        juce::ColourGradient meterGrad(
            juce::Colours::red, meterLeftArea.getX(), meterLeftArea.getY(),
            juce::Colours::limegreen, meterLeftArea.getX(), meterLeftArea.getBottom(),
            false
        );
        meterGrad.addColour(0.3, juce::Colours::yellow); // Aggiunge il giallo a 3/4 di altezza
        g.setGradientFill(meterGrad);

        // Calcoliamo l'altezza "piena" in base al volume
        int heightL = (int)(meterLeftArea.getHeight() * levelLeftScaled);
        int heightR = (int)(meterRightArea.getHeight() * levelRightScaled);

		// Copie locali delle aree dei meter, così da poterle modificare senza alterare le originali usate per i bordi
        auto localMeterL = meterLeftArea;
        auto localMeterR = meterRightArea;

        // Disegniamo il volume vero e proprio tagliando le copie
        auto fillL = localMeterL.removeFromBottom(heightL);
        auto fillR = localMeterR.removeFromBottom(heightR);

        g.fillRoundedRectangle(fillL.toFloat(), 2.0f);
        g.fillRoundedRectangle(fillR.toFloat(), 2.0f);
    #pragma endregion
}

#pragma endregion

#pragma region resized UI

void StringUIdemoAudioProcessorEditor::resized()
{
    auto area = getLocalBounds();

    // Calcola quanto la finestra è stata ingrandita o rimpicciolita
    float scale = (float)getWidth() / 750.0f;

    #pragma region Area corde scalata
        // Area corde scalata
        int stringH = 17 * scale;
        int gap = 4 * scale;
        int rightMargin = 10 * scale;
        int scaledTuningPanelWidth = tuningPanelWidth * scale;
        const int totalStrings = StringUIdemoAudioProcessor::numStrings;

        int stringsAreaH = totalStrings * stringH + (totalStrings - 1) * gap + (16 * scale);

        auto bottomArea = area.removeFromBottom(stringsAreaH);
        areaCordeSotto = bottomArea;
    

        bottomArea.removeFromTop(8 * scale);

        for (int i = 0; i < totalStrings; ++i)
        {
            // Separazione della zona inferiore
            auto row = bottomArea.removeFromTop(stringH);
            bottomArea.removeFromTop(gap);

            auto tuningRow = row.removeFromLeft(scaledTuningPanelWidth);
            row.removeFromRight(rightMargin);
            stringComponents.getUnchecked(i)->setBounds(row);

            int btnW = 22 * scale;
            int lblW = 44 * scale;

            tuningRow.removeFromLeft(4 * scale);
            tuningDownButtons.getUnchecked(i)->setBounds(tuningRow.removeFromLeft(btnW).reduced(0, 1));

            tuningRow.removeFromLeft(2 * scale);
            tuningLabels.getUnchecked(i)->setBounds(tuningRow.removeFromLeft(lblW).reduced(0, 1));

		    // Scala la grandezza del testo delle label di tuning
            tuningLabels.getUnchecked(i)->setFont(juce::FontOptions(11.0f * scale, juce::Font::bold));

            tuningRow.removeFromLeft(4 * scale);
            tuningUpButtons.getUnchecked(i)->setBounds(tuningRow.removeFromLeft(btnW).reduced(0, 1));
        }
    #pragma endregion

    #pragma region Area superiore scalata

        // Setup area superiore scalata
        area.removeFromTop(30 * scale); // Rimuove lo spazio per il titolo principale

        // Creiamo una "Toolbar" orizzontale sopra le corde
        auto toolbarArea = area.removeFromBottom(30 * scale);

        // Sinistra: Accordatura (Reset, Menu preset accordatura, Salva, Elimina)
        auto leftToolbar = toolbarArea.removeFromLeft(380 * scale);
        resetTuningButton.setBounds(leftToolbar.removeFromLeft(45 * scale).withSizeKeepingCentre(45 * scale, 18 * scale));
        leftToolbar.removeFromLeft(5 * scale);
        tuningMenu.setBounds(leftToolbar.removeFromLeft(175 * scale).withSizeKeepingCentre(175 * scale, 20 * scale));
        leftToolbar.removeFromLeft(5 * scale);
        saveTuningButton.setBounds(leftToolbar.removeFromLeft(50 * scale).withSizeKeepingCentre(50 * scale, 18 * scale));
        leftToolbar.removeFromLeft(5 * scale);
        deleteTuningButton.setBounds(leftToolbar.removeFromLeft(55 * scale).withSizeKeepingCentre(55 * scale, 18 * scale));

        // Preset Menu a Destra (con un po' di margine dal bordo per non attaccarlo)
        auto presetArea = toolbarArea.removeFromRight(150 * scale).reduced(5 * scale, 5 * scale);
        presetMenu.setBounds(presetArea);

        // Nota Suonata al Centro (prende tutto lo spazio rimanente tra il Reset e i Preset)
        notaSuonataLabel.setBounds(toolbarArea);
        notaSuonataLabel.setFont(juce::FontOptions(12.0f * scale));

        // Divisione Principale: 1/3 a Sinistra, 2/3 a Destra
        auto leftArea = area.removeFromLeft(area.getWidth() / 3);
        auto rightArea = area;

        // Sub-divisione Sinistra (Oscilloscopio e Master)
        areaOscilloscopio = leftArea.removeFromTop(leftArea.getHeight() / 2);
        areaMaster = leftArea;

        // Sub-divisione Destra (Dividiamo in Riga Superiore e Riga Inferiore)
        auto rightTopArea = rightArea.removeFromTop(rightArea.getHeight() / 2);
        auto rightBottomArea = rightArea;

        // --- RIGA SUPERIORE --- 
        // Parametri Fisici (~28%), ADSR (~44%), Distorsione (~28%)
        int totalTopW = rightTopArea.getWidth();
        areaParametriFisici = rightTopArea.removeFromLeft((totalTopW * 28) / 100);
        areaADSR = rightTopArea.removeFromLeft((totalTopW * 44) / 100);
        areaDistortion = rightTopArea;

        // --- RIGA INFERIORE --- 
        // Delay, Phaser, Reverb (divisi in 3 sezioni perfettamente uguali)
        areaDelay = rightBottomArea.removeFromLeft(rightBottomArea.getWidth() / 3);
        areaPhaser = rightBottomArea.removeFromLeft(rightBottomArea.getWidth() / 2);
        areaReverb = rightBottomArea;
    #pragma endregion

    #pragma region Griglia manopole scalata
        // Assegnazione titoli manopole (17 manopole)
        juce::Rectangle<int> celle[17];

        // Creiamo delle "copie di lavoro" delle aree. 
        // In questo modo non rimpiccioliamo le aree originali usate dal paint() per i bordi
        auto workOsc = areaOscilloscopio;
        auto workMaster = areaMaster;
        auto workPhys = areaParametriFisici;
        auto workAdsr = areaADSR;
        auto workDist = areaDistortion;
        auto workDelay = areaDelay;
        auto workPhaser = areaPhaser;
        auto workRev = areaReverb;

        // Ritagliamo 35 pixel dall'alto di ogni area per far spazio ai titoli
        int titleHeight = 35 * scale;
        titoloSezione[0].setBounds(workOsc.removeFromTop(titleHeight));
        titoloSezione[1].setBounds(workMaster.removeFromTop(titleHeight));
        titoloSezione[2].setBounds(workPhys.removeFromTop(titleHeight));
        titoloSezione[3].setBounds(workAdsr.removeFromTop(titleHeight).reduced(30 * scale, 0));
        titoloSezione[4].setBounds(workDist.removeFromTop(titleHeight).reduced(30 * scale, 0));
        titoloSezione[5].setBounds(workDelay.removeFromTop(titleHeight).reduced(30 * scale, 0));
        titoloSezione[6].setBounds(workPhaser.removeFromTop(titleHeight).reduced(30 * scale, 0));
        titoloSezione[7].setBounds(workRev.removeFromTop(titleHeight).reduced(30 * scale, 0));

        // Scaliamo il font dei titoli
        for (int i = 0; i < numSezioni; ++i)
            titoloSezione[i].setFont(juce::FontOptions(11.0f * scale, juce::Font::bold));

        // Distribuzione nelle aree di lavoro

        // Delay (Celle 0, 1)
        auto delayArea = workDelay.reduced(5 * scale, 5 * scale);
        celle[0] = delayArea.removeFromLeft(delayArea.getWidth() / 2);
        celle[1] = delayArea;

        // Distortion (Celle 2, 3)
        auto distArea = workDist.reduced(5 * scale, 5 * scale);
        celle[2] = distArea.removeFromLeft(distArea.getWidth() / 2);
        celle[3] = distArea;

        // Parametri fisici (Celle 4, 5, 6)
        auto physArea = workPhys.reduced(5 * scale, 5 * scale);
        celle[4] = physArea.removeFromLeft(physArea.getWidth() / 3);
        celle[5] = physArea.removeFromLeft(physArea.getWidth() / 2);
        celle[6] = physArea;

        // Reverb (Celle 7, 8)
        auto verbArea = workRev.reduced(5 * scale, 5 * scale);
        celle[7] = verbArea.removeFromLeft(verbArea.getWidth() / 2);
        celle[8] = verbArea;

        // Master Volume (Cella 9)
        celle[9] = workMaster.reduced(5 * scale, 5 * scale);

        // Phaser (Celle 10, 11, 12)
        auto phasArea = workPhaser.reduced(5 * scale, 5 * scale);
        celle[10] = phasArea.removeFromLeft(phasArea.getWidth() / 3); // Rate
        celle[11] = phasArea.removeFromLeft(phasArea.getWidth() / 2); // Depth
        celle[12] = phasArea; // Mix

        // ADSR (Celle 13, 14, 15, 16)
        auto adsrArea = workAdsr.reduced(5 * scale, 5 * scale);
        celle[13] = adsrArea.removeFromLeft(adsrArea.getWidth() / 4); // Attack
        celle[14] = adsrArea.removeFromLeft(adsrArea.getWidth() / 3); // Decay
        celle[15] = adsrArea.removeFromLeft(adsrArea.getWidth() / 2); // Sustain
        celle[16] = adsrArea;                                        // Release

        // Ciclo di posizionamento finale
        // Impostiamo un diametro fisso e uguale per tutte le manopole.
        int uniformKnobSize = 45 * scale;

        for (int i = 0; i < numManopole; ++i)
        {
            // Centriamo la manopola nella sua cella usando la dimensione fissa
            auto bounds = celle[i].withSizeKeepingCentre(uniformKnobSize, uniformKnobSize).translated(0, 8 * scale);
            manopolaEffetto[i].setBounds(bounds);

            // Grandezza della casella di testo col numero sotto la manopola
            manopolaEffetto[i].setTextBoxStyle(juce::Slider::TextBoxBelow, false, 37 * scale, 13 * scale);

            // Posizioniamo il titolo della manopola centrato rispetto alla larghezza totale della cella
            titoloManopolaEffetto[i].setBounds(celle[i].getX(), bounds.getY() - (16 * scale), celle[i].getWidth(), 20 * scale);
            titoloManopolaEffetto[i].setFont(juce::FontOptions(11.0f * scale, juce::Font::plain));
        }
    #pragma endregion

    #pragma region Posizionamento bottoni On/Off
        int btnW = 30 * scale;
        int btnH = 15 * scale;
        int marginX = 10 * scale;

        // Calcoliamo la Y esatta per centrare il bottone verticalmente rispetto al testo del titolo.
        int adsrY = titoloSezione[3].getBounds().getCentreY() - (btnH / 2);
        int distY = titoloSezione[4].getBounds().getCentreY() - (btnH / 2);
        int delayY = titoloSezione[5].getBounds().getCentreY() - (btnH / 2);
        int phasY = titoloSezione[6].getBounds().getCentreY() - (btnH / 2);
        int revY = titoloSezione[7].getBounds().getCentreY() - (btnH / 2);

        btnAdsrOn.setBounds(areaADSR.getRight() - btnW - marginX, adsrY, btnW, btnH);
        btnDistOn.setBounds(areaDistortion.getRight() - btnW - marginX, distY, btnW, btnH);
        btnDelayOn.setBounds(areaDelay.getRight() - btnW - marginX, delayY, btnW, btnH);
        btnPhaserOn.setBounds(areaPhaser.getRight() - btnW - marginX, phasY, btnW, btnH);
        btnRevOn.setBounds(areaReverb.getRight() - btnW - marginX, revY, btnW, btnH);
    #pragma endregion

    #pragma region Posizionamento Volume Meter
            // Prendiamo uno spazio a destra della manopola Master
            auto meterBox = celle[9].removeFromRight(15 * scale).reduced(0, 10 * scale).translated(-25 * scale, 0);

            // Dividiamo in due barrette (Left e Right) con 2 pixel di spazio in mezzo
            meterLeftArea = meterBox.removeFromLeft(meterBox.getWidth() / 2 - 1);
            meterRightArea = meterBox.removeFromRight(meterLeftArea.getWidth());
    #pragma endregion

    #pragma region Sezione oscilloscopio
			// Setup Oscilloscopio
            // Parametri dell'onda visualizzata
            oscilloscopio.setBufferSize(1024);
            oscilloscopio.setSamplesPerBlock(128);
            oscilloscopio.setRepaintRate(120); // Aggiornamento a 120 FPS

            audioProcessor.puntatoreOscilloscopio = &oscilloscopio;

            // Colori: Sfondo e Colore dell'Onda
            oscilloscopio.setColours(juce::Colour(0xFF201513), juce::Colours::cyan);
            oscilloscopio.setOpaque(true); // Per migliorare le prestazioni, dato che disegniamo tutto lo sfondo
            addAndMakeVisible(oscilloscopio);

            // Posizionamento dell'oscilloscopio
            oscilloscopio.setBounds(workOsc.reduced(10 * scale));    
    #pragma endregion
}

#pragma endregion

//==============================================================================
void StringUIdemoAudioProcessorEditor::handleMouseEvent(const juce::MouseEvent& e)
{
    for (int i = 0; i < stringComponents.size(); ++i)
    {
        auto* sc = stringComponents.getUnchecked(i);

        if (sc->getBounds().contains(e.getPosition()))
        {
            float relPos = (e.position.x - (float)sc->getX()) / (float)sc->getWidth();
            relPos = juce::jlimit(0.0f, 1.0f, relPos);

            int posFret = juce::jlimit(0, numFret, (int)(relPos * numFret));

            // La nota suonata dipende dalla nota base corrente (accordatura attuale)
            int baseMidi = audioProcessor.getStringMidiNote(i);
            int midiNote = baseMidi + posFret;

            if (oldPosFret != posFret || oldMidiNote != midiNote)
            {
                oldPosFret = posFret;
                oldMidiNote = midiNote;

                juce::String nomeNota = juce::MidiMessage::getMidiNoteName(midiNote, true, true, 3);
                notaSuonataLabel.setText("Nota: " + nomeNota + "  Tasto: " + juce::String(posFret),
                    juce::dontSendNotification);

                sc->stringPlucked(relPos);
                audioProcessor.pluckString(i, relPos);
                break;
            }
        }
    }
}

//==============================================================================
#pragma region Metodi Accordatura
/// <summary>
/// Tuning label: mostra nome nota base + delta in semitoni rispetto al default.
/// Es.: "E2 (+2)" oppure "E2 (-1)" oppure "E2"
/// </summary>
void StringUIdemoAudioProcessorEditor::updateTuningLabel(int stringIndex)
{
    if (stringIndex < 0 || stringIndex >= StringUIdemoAudioProcessor::numStrings)
        return;

    int currentNote = audioProcessor.getStringMidiNote(stringIndex);
    int defaultNote = StringUIdemoAudioProcessor::defaultMidiNotes[stringIndex];
    int delta = currentNote - defaultNote;

    // Nome della nota base (senza numero di ottava nei pulsanti, con ottava nella label)
    juce::String noteName = juce::MidiMessage::getMidiNoteName(currentNote, true, true, 3);

    juce::String deltaStr;
    if (delta > 0) deltaStr = "(+" + juce::String(delta) + ")";
    else if (delta < 0) deltaStr = "(" + juce::String(delta) + ")";
    // se delta == 0 non mostriamo nulla

    juce::String labelText = noteName;
    if (deltaStr.isNotEmpty())
        labelText += " " + deltaStr;

    tuningLabels.getUnchecked(stringIndex)->setText(labelText, juce::dontSendNotification);
}

void StringUIdemoAudioProcessorEditor::updateAllTuningLabels()
{
    for (int i = 0; i < StringUIdemoAudioProcessor::numStrings; ++i)
        updateTuningLabel(i);
}

void StringUIdemoAudioProcessorEditor::populateTuningMenu()
{
    tuningMenu.clear(juce::dontSendNotification);

    tuningMenu.addItem("Standard (E A D G B E)", 1);
    tuningMenu.addItem("Drop D (D A D G B E)", 2);
    tuningMenu.addItem("Half Step Down (Eb)", 3);
    tuningMenu.addItem("Open G (D G D G B D)", 4);
    tuningMenu.addItem("DADGAD (D A D G A D)", 5);

    auto customList = audioProcessor.loadCustomTunings();
    if (!customList.empty())
    {
        tuningMenu.addSeparator();
        for (size_t i = 0; i < customList.size(); ++i)
        {
            tuningMenu.addItem(customList[i].name, 100 + (int)i);
        }
    }
}

void StringUIdemoAudioProcessorEditor::applySelectedTuning(int itemId)
{
    if (itemId <= 0) return;

    if (itemId == 1) // Standard
    {
        audioProcessor.applyTuning({ 64, 59, 55, 50, 45, 40 });
        deleteTuningButton.setEnabled(false);
    }
    else if (itemId == 2) // Drop D
    {
        audioProcessor.applyTuning({ 62, 59, 55, 50, 45, 40 });
        deleteTuningButton.setEnabled(false);
    }
    else if (itemId == 3) // Half Step Down
    {
        audioProcessor.applyTuning({ 63, 58, 54, 49, 44, 39 });
        deleteTuningButton.setEnabled(false);
    }
    else if (itemId == 4) // Open G
    {
        audioProcessor.applyTuning({ 62, 57, 55, 50, 47, 38 });
        deleteTuningButton.setEnabled(false);
    }
    else if (itemId == 5) // DADGAD
    {
        audioProcessor.applyTuning({ 62, 59, 55, 50, 47, 38 });
        deleteTuningButton.setEnabled(false);
    }
    else if (itemId >= 100)
    {
        int idx = itemId - 100;
        auto customList = audioProcessor.loadCustomTunings();
        if (idx >= 0 && idx < (int)customList.size())
        {
            audioProcessor.applyTuning(customList[(size_t)idx].notes);
            deleteTuningButton.setEnabled(true);
        }
    }

    updateAllTuningLabels();
}

void StringUIdemoAudioProcessorEditor::promptSaveCustomTuning()
{
    auto* aw = new juce::AlertWindow("Salva Accordatura", "Inserisci il nome per l'accordatura personalizzata:", juce::MessageBoxIconType::QuestionIcon);
    aw->addTextEditor("name", "Custom " + juce::String(audioProcessor.loadCustomTunings().size() + 1));
    aw->addButton("Salva", 1, juce::KeyPress(juce::KeyPress::returnKey));
    aw->addButton("Annulla", 0, juce::KeyPress(juce::KeyPress::escapeKey));

    aw->enterModalState(true, juce::ModalCallbackFunction::create([this, aw](int result)
    {
        if (result == 1)
        {
            auto name = aw->getTextEditorContents("name").trim();
            if (name.isNotEmpty())
            {
                std::array<int, StringUIdemoAudioProcessor::numStrings> notes;
                for (int i = 0; i < StringUIdemoAudioProcessor::numStrings; ++i)
                    notes[i] = audioProcessor.getStringMidiNote(i);

                audioProcessor.saveCustomTuning(name, notes);
                populateTuningMenu();

                // Seleziona la nuova accordatura salvata
                for (int i = 0; i < tuningMenu.getNumItems(); ++i)
                {
                    if (tuningMenu.getItemText(i) == name)
                    {
                        tuningMenu.setSelectedItemIndex(i, juce::dontSendNotification);
                        deleteTuningButton.setEnabled(true);
                        break;
                    }
                }
            }
        }
        delete aw;
    }));
}

void StringUIdemoAudioProcessorEditor::deleteSelectedCustomTuning()
{
    int selectedId = tuningMenu.getSelectedId();
    if (selectedId >= 100)
    {
        juce::String name = tuningMenu.getText();
        audioProcessor.deleteCustomTuning(name);
        populateTuningMenu();
        tuningMenu.setSelectedId(1);
    }
}
#pragma endregion


//==============================================================================
#pragma region Metodi disegni UI
void StringUIdemoAudioProcessorEditor::SetTitle(juce::Graphics& g)
{
    float scale = (float)getWidth() / 750.0f;

    g.setColour(juce::Colours::white.withAlpha(0.85f));
    // Scaliamo il font del titolo (da 18 a 18 * scale)
    g.setFont(juce::FontOptions(18.0f * scale, juce::Font::bold));

    g.drawText("MODELLAZIONE FISICA CHITARRA", 0, 5 * scale, getWidth(), 30 * scale, juce::Justification::centred);      
}

void StringUIdemoAudioProcessorEditor::SetLineaSeparatrice(juce::Graphics& g)
{
    // Usiamo il bordo dell'areaCordeSotto calcolata nel resized
    float lineaY = areaCordeSotto.getY() - 4.0f;
    g.setColour(juce::Colour(0xFF4D453A));
    g.drawHorizontalLine((int)lineaY, 10.0f, (float)getWidth() - 10.0f);
}

void StringUIdemoAudioProcessorEditor::SetStrings(juce::Graphics& g)
{
    // Le etichette testuali (E2, A2...) vengono mostrate dalla tuningLabel,
    // quindi qui disegniamo solo una eventuale riga di sfondo per le corde
    for (int i = 0; i < StringUIdemoAudioProcessor::numStrings; ++i)
    {
        auto* sc = stringComponents.getUnchecked(i);
        g.setColour(stringColour(i).withAlpha(0.08f));
        g.fillRect(sc->getX(), sc->getY(), sc->getWidth(), sc->getHeight());
    }
}

void StringUIdemoAudioProcessorEditor::SetSeparationFret(juce::Graphics& g)
{
    auto* sc = stringComponents.getUnchecked(0);

    float startX = (float)sc->getX();
    float width = (float)sc->getWidth();
    float fretW = width / (float)numFret;

    g.setColour(juce::Colour(0xFFA88CEC));

    for (int i = 0; i <= numFret; ++i)
    {
        float x = startX + i * fretW;
        g.fillRect((int)x, sc->getY(), 3, sc->getHeight() * (numCorde + 1));
    }
#pragma endregion
}

#pragma region Funzione per preset
void StringUIdemoAudioProcessorEditor::applicaPreset(int presetId)
{
    // Creiamo una funzione Lambda interna per cambiare i parametri in una riga
    auto setParam = [this](const juce::String& id, float veroValore)
        {
            if (auto* param = audioProcessor.apvts.getParameter(id))
            {
                // Convertiamo il valore reale in un valore 0-1 e lo inviamo
                param->setValueNotifyingHost(param->convertTo0to1(veroValore));
            }
        };

    // Cambia l'accordatura di tutte le 6 corde
    // Prende un array di 6 note MIDI (dalla più bassa alla più alta)
    auto setTuning = [this](int n0, int n1, int n2, int n3, int n4, int n5)
        {
            audioProcessor.setStringMidiNote(0, n0);
            audioProcessor.setStringMidiNote(1, n1);
            audioProcessor.setStringMidiNote(2, n2);
            audioProcessor.setStringMidiNote(3, n3);
            audioProcessor.setStringMidiNote(4, n4);
            audioProcessor.setStringMidiNote(5, n5);
            updateAllTuningLabels(); // Aggiorna graficamente i bottoni
        };

    // Usiamo lo switch per applicare il preset scelto
    switch (presetId)
    {
    case 1: // Default (Tutto spento o neutro)
        setParam("drive", 1.0f); setParam("gain", 0.5f);
        setParam("delayTime", 0.4f); setParam("delayFb", 0.0f);
        setParam("revMix", 0.0f); setParam("revSize", 50.0f);
        setParam("hardness", 0.5f); setParam("damping", 100.0f); setParam("sustain", 100.0f);
        setParam("delayOn", 0.0f); setParam("distOn", 0.0f); setParam("revOn", 0.0f);
        setParam("phaserRate", 1.0f); setParam("phaserDepth", 0.5f); setParam("phaserMix", 50.0f); setParam("phaserOn", 0.0f);
        setParam("attack", 0.01f); setParam("decay", 0.5f); setParam("adsrSustain", 100.0f); setParam("release", 1.0f); setParam("adsrOn", 1.0f);

		// Accordatura Standard Chitarra: E2, A2, D3, G3, B3, E4 (invertita perchè le corde sono ordinate dalla più grave alla più acuta)
        setTuning(64, 59, 55, 50, 45, 40);
        break;
    case 2: // Dream Harp
        setParam("drive", 1.0f); setParam("gain", 0.10f);
        setParam("delayTime", 0.11f); setParam("delayFb", 36.0f);
        setParam("revMix", 100.0f); setParam("revSize", 100.0f);
        setParam("hardness",0.01f); setParam("damping", 100.0f); setParam("sustain", 100.0f);
        setParam("delayOn", 1.0f); setParam("distOn", 1.0f); setParam("revOn", 1.0f);
        setParam("phaserRate", 1.0f); setParam("phaserDepth", 0.5f); setParam("phaserMix", 50.0f); setParam("phaserOn", 0.0f);
        setParam("attack", 0.20f); setParam("decay", 1.0f); setParam("adsrSustain", 80.0f); setParam("release", 2.5f); setParam("adsrOn", 1.0f);

        setTuning(60, 57, 55, 52, 50, 48);
        break;
    case 3: // Electric
        setParam("drive", 4.84f); setParam("gain", 0.50f);
        setParam("delayTime", 0.06f); setParam("delayFb", 6.0f);
        setParam("revMix", 15.0f); setParam("revSize", 18.0f);
        setParam("hardness", 0.80f); setParam("damping", 100.0f); setParam("sustain", 100.0f);
        setParam("delayOn", 1.0f); setParam("distOn", 1.0f); setParam("revOn", 1.0f);
        setParam("phaserRate", 1.0f); setParam("phaserDepth", 0.5f); setParam("phaserMix", 50.0f); setParam("phaserOn", 0.0f);
        setParam("attack", 0.002f); setParam("decay", 0.3f); setParam("adsrSustain", 100.0f); setParam("release", 1.2f); setParam("adsrOn", 1.0f);

        setTuning(76, 71, 67, 62, 57, 52);
        break;
    case 4: // Bass
        setParam("drive", 1.44f); setParam("gain", 0.45f);
        setParam("delayTime", 0.06f); setParam("delayFb", 5.0f);
        setParam("revMix", 0.0f); setParam("revSize", 0.0f);
        setParam("hardness", 0.20f); setParam("damping", 90.0f); setParam("sustain", 80.0f);
        setParam("delayOn", 1.0f); setParam("distOn", 1.0f); setParam("revOn", 0.0f);
        setParam("phaserRate", 1.0f); setParam("phaserDepth", 0.5f); setParam("phaserMix", 50.0f); setParam("phaserOn", 0.0f);
        setParam("attack", 0.005f); setParam("decay", 0.6f); setParam("adsrSustain", 85.0f); setParam("release", 0.8f); setParam("adsrOn", 1.0f);

        setTuning(60, 55, 50, 45, 40, 47);
        break;
    default: break;
    }
}
#pragma endregion