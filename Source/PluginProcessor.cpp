#include "PluginProcessor.h"
#include "PluginEditor.h"

// Accordatura standard: E2=64, A2=59, D3=55, G3=50, B3=45, E4=40
const int StringUIdemoAudioProcessor::defaultMidiNotes[StringUIdemoAudioProcessor::numStrings]
= { 64, 59, 55, 50, 45, 40 };

//==============================================================================
StringUIdemoAudioProcessor::StringUIdemoAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
    : AudioProcessor(BusesProperties()
#if ! JucePlugin_IsMidiEffect
#if ! JucePlugin_IsSynth
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
#endif
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)
#endif
    )
#endif
    , apvts(*this, nullptr, "PARAMETERS", createParameters())
{
    // Inizializzazioni utili...
    for (int i = 0; i < numStrings; ++i) 
    {
        // Inizializzo il tuning corrente con i valori di default
        currentMidiNotes[i] = defaultMidiNotes[i];

        // Inizializzo gli atomic per la UI
        uiStringWasPlucked[i].store(false);
        uiPluckPosition[i].store(0.0f);
    }

#pragma region APVTS parameters
    // Prendo i riferimenti ai parametri di controllo dell'effettistica dalla APVTS
    // (da usare in processBlock) (e quindi da dereferenziare)
    driveParameter = apvts.getRawParameterValue("drive");
    gainParameter = apvts.getRawParameterValue("gain");
    hardnessParameter = apvts.getRawParameterValue("hardness");
    dampingParameter = apvts.getRawParameterValue("damping");
    sustainParameter = apvts.getRawParameterValue("sustain");
    revMixParameter = apvts.getRawParameterValue("revMix");
    revSizeParameter = apvts.getRawParameterValue("revSize");
    delayTimeParameter = apvts.getRawParameterValue("delayTime");
    delayFbParameter = apvts.getRawParameterValue("delayFb");
    masterVolumeParameter = apvts.getRawParameterValue("masterVolume");
	delayOnParameter = apvts.getRawParameterValue("delayOn");
	distOnParameter = apvts.getRawParameterValue("distOn");
	revOnParameter = apvts.getRawParameterValue("revOn");
    phaserRateParameter = apvts.getRawParameterValue("phaserRate");
    phaserDepthParameter = apvts.getRawParameterValue("phaserDepth");
    phaserMixParameter = apvts.getRawParameterValue("phaserMix");
    phaserOnParameter = apvts.getRawParameterValue("phaserOn");

    // Parametri ADSR
    adsrAttackParameter = apvts.getRawParameterValue("attack");
    adsrDecayParameter = apvts.getRawParameterValue("decay");
    adsrSustainParameter = apvts.getRawParameterValue("adsrSustain");
    adsrReleaseParameter = apvts.getRawParameterValue("release");
    adsrOnParameter = apvts.getRawParameterValue("adsrOn");

#pragma endregion
}

StringUIdemoAudioProcessor::~StringUIdemoAudioProcessor() {}


//==============================================================================

/// <summary>
/// Passa all'APVTS i parametri che voglio gestire (di tipo RangedAudioParameter) attraverso un vector di unique_ptr
/// a tali parametri.s
/// </summary>
/// <remarks>
/// Il metodo ".push_back(...)" aggiunge semplicemente una nuova entrata alla fine del vector.
/// (Simile ad un ".Add(...)" nelle List<T> di C#)
/// Tecnicamente la funzione ritorna una tupla di puntatori all'inizio e alla fine del vector.
/// Sostanzialmete equivale a paasare il vector stesso.
/// </remarks>
juce::AudioProcessorValueTreeState::ParameterLayout StringUIdemoAudioProcessor::createParameters() 
{
	// Salvo in un vector i puntatori ai parametri che voglio gestire con l'APVTS (da usare in processBlock)
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

	// Attraverso il vector, creo i parametri di controllo dell'effettistica e li aggiungo alla APVTS
    // (ID, nome, min, max, default)
    // IMPORTANTE: è qui che si manipolano i parametri della manopola associata, NON dall'editor!
    // (è una conseguenza dell'impiego dell'APVTS)
	params.push_back(std::make_unique<juce::AudioParameterFloat>("drive", "Drive", 1.0f, 10.0f, 1.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("gain", "Gain", 0.1f, 1.0f, 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("hardness", "Hardness", 0.01f, 1.0f, 0.5f)); //non min = 0 perchè altrimenti si muta l'audio
    
    params.push_back(std::make_unique<juce::AudioParameterInt>("damping", "Damping", 0, 100, 100));
    params.push_back(std::make_unique<juce::AudioParameterInt>("sustain", "Sustain", 0, 100, 100));
    params.push_back(std::make_unique<juce::AudioParameterInt>("revMix", "Rev Mix", 0, 100, 50));
    params.push_back(std::make_unique<juce::AudioParameterInt>("revSize", "Rev Size", 0, 100, 50));

    params.push_back(std::make_unique<juce::AudioParameterFloat>("delayTime", "Time", 0.01f, 1.5f, 0.4f)); // Time va da 0.01 secondi (slapback) a 1.5 secondi (eco lungo)
    
    params.push_back(std::make_unique<juce::AudioParameterInt>("delayFb", "Feedback", 0, 95, 50)); // Il feedback arriva massimo a 0.95 per evitare fischi infiniti
    params.push_back(std::make_unique<juce::AudioParameterInt>("masterVolume", "Master Volume", 0, 100, 50));

    params.push_back(std::make_unique<juce::AudioParameterFloat>("phaserRate", "Phaser Rate", 0.1f, 10.0f, 1.0f)); // Hertz
    params.push_back(std::make_unique<juce::AudioParameterFloat>("phaserDepth", "Phaser Depth", 0.0f, 1.0f, 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterInt>("phaserMix", "Phaser Mix", 0, 100, 50));

    // Aggiungi questi insieme agli altri params.push_back(...)
    params.push_back(std::make_unique<juce::AudioParameterBool>("delayOn", "Delay On", true));
    params.push_back(std::make_unique<juce::AudioParameterBool>("distOn", "Distortion On", true));
    params.push_back(std::make_unique<juce::AudioParameterBool>("revOn", "Reverb On", true));
    params.push_back(std::make_unique<juce::AudioParameterBool>("phaserOn", "Phaser On", true));

    // Parametri Inviluppo ADSR
    params.push_back(std::make_unique<juce::AudioParameterFloat>("attack", "Attack", 0.001f, 2.0f, 0.01f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("decay", "Decay", 0.01f, 3.0f, 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterInt>("adsrSustain", "Sustain", 0, 100, 100));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("release", "Release", 0.01f, 3.0f, 1.0f));
    params.push_back(std::make_unique<juce::AudioParameterBool>("adsrOn", "ADSR On", true));

	return { params.begin(), params.end() };
}

//==============================================================================
/// <summary>
/// Suono la corda
/// </summary>
/// <param name="stringIndex"></param>
/// <param name="position"></param>
void StringUIdemoAudioProcessor::pluckString(int stringIndex, float position)
{
    if (stringIndex < 0 || stringIndex >= stringSynths.size())
        return;

    const int numFrets = 12;
    int fret = juce::jlimit(0, numFrets, (int)(position * (float)numFrets));
    int midiNote = currentMidiNotes[stringIndex] + fret;

    double freqHz = juce::MidiMessage::getMidiNoteInHertz(midiNote);

    auto* synth = stringSynths.getUnchecked(stringIndex);
    synth->setFrequency(freqHz);
    synth->stringPlucked(position);
}

//==============================================================================

/// <summary>
/// Imposto la stringa accordata a fino a un max e min di 12.
/// Fa tre cose:
/// 1) Controllo indice valido.
/// 2) Limito il tuning.
/// 3) Aggiorno frequenze nel StringSynthesiser.h
/// </summary>
/// <param name="stringIndex"></param>
/// <param name="newMidiNote"></param>
void StringUIdemoAudioProcessor::setStringMidiNote(int stringIndex, int newMidiNote)
{
    if (stringIndex < 0 || stringIndex >= numStrings)
        return;

    // Limita il tuning a ±tuningRangeSemitones rispetto al default
    int minNote = defaultMidiNotes[stringIndex] - tuningRangeSemitones;
    int maxNote = defaultMidiNotes[stringIndex] + tuningRangeSemitones;
    currentMidiNotes[stringIndex] = juce::jlimit(minNote, maxNote, newMidiNote);

    // Aggiorna subito la frequenza base del synth (senza pizzicare)
    if (stringIndex < stringSynths.size())
    {
        double freqHz = juce::MidiMessage::getMidiNoteInHertz(currentMidiNotes[stringIndex]);
        stringSynths.getUnchecked(stringIndex)->setFrequency(freqHz);
    }
}

int StringUIdemoAudioProcessor::getStringMidiNote(int stringIndex) const
{
    if (stringIndex >= 0 && stringIndex < numStrings)
        return currentMidiNotes[stringIndex];
    return 0;
}

void StringUIdemoAudioProcessor::resetTuning()
{
    for (int i = 0; i < numStrings; ++i)
        setStringMidiNote(i, defaultMidiNotes[i]);
}

void StringUIdemoAudioProcessor::releaseAllStrings()
{
    for (int i = 0; i < stringSynths.size(); ++i)
        stringSynths.getUnchecked(i)->noteOff();
}

void StringUIdemoAudioProcessor::noteOffString(int stringIndex)
{
    if (stringIndex >= 0 && stringIndex < stringSynths.size())
        stringSynths.getUnchecked(stringIndex)->noteOff();
}

juce::File StringUIdemoAudioProcessor::getCustomTuningsFile()
{
    auto dir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                    .getChildFile("stringUIdemo");
    if (!dir.exists())
        dir.createDirectory();
    return dir.getChildFile("custom_tunings.xml");
}

std::vector<StringUIdemoAudioProcessor::CustomTuning> StringUIdemoAudioProcessor::loadCustomTunings()
{
    std::vector<CustomTuning> tunings;
    auto file = getCustomTuningsFile();
    if (!file.existsAsFile())
        return tunings;

    auto xml = juce::XmlDocument::parse(file);
    if (xml != nullptr && xml->hasTagName("CUSTOM_TUNINGS"))
    {
        for (auto* el : xml->getChildIterator())
        {
            if (el->hasTagName("TUNING"))
            {
                CustomTuning t;
                t.name = el->getStringAttribute("name", "Custom");
                auto notesStr = el->getStringAttribute("notes");
                auto tokens = juce::StringArray::fromTokens(notesStr, " ", "");
                if (tokens.size() == numStrings)
                {
                    for (int i = 0; i < numStrings; ++i)
                        t.notes[i] = tokens[i].getIntValue();
                    tunings.push_back(t);
                }
            }
        }
    }
    return tunings;
}

void StringUIdemoAudioProcessor::saveCustomTuning(const juce::String& name, const std::array<int, numStrings>& notes)
{
    auto tunings = loadCustomTunings();
    bool updated = false;
    for (auto& t : tunings)
    {
        if (t.name.equalsIgnoreCase(name))
        {
            t.notes = notes;
            updated = true;
            break;
        }
    }
    if (!updated)
    {
        CustomTuning newTuning;
        newTuning.name = name;
        newTuning.notes = notes;
        tunings.push_back(newTuning);
    }

    juce::XmlElement root("CUSTOM_TUNINGS");
    for (const auto& t : tunings)
    {
        auto* el = root.createNewChildElement("TUNING");
        el->setAttribute("name", t.name);
        juce::String notesStr;
        for (int i = 0; i < numStrings; ++i)
        {
            notesStr += juce::String(t.notes[i]);
            if (i < numStrings - 1)
                notesStr += " ";
        }
        el->setAttribute("notes", notesStr);
    }
    root.writeTo(getCustomTuningsFile());
}

void StringUIdemoAudioProcessor::deleteCustomTuning(const juce::String& name)
{
    auto tunings = loadCustomTunings();
    juce::XmlElement root("CUSTOM_TUNINGS");
    for (const auto& t : tunings)
    {
        if (!t.name.equalsIgnoreCase(name))
        {
            auto* el = root.createNewChildElement("TUNING");
            el->setAttribute("name", t.name);
            juce::String notesStr;
            for (int i = 0; i < numStrings; ++i)
            {
                notesStr += juce::String(t.notes[i]);
                if (i < numStrings - 1)
                    notesStr += " ";
            }
            el->setAttribute("notes", notesStr);
        }
    }
    root.writeTo(getCustomTuningsFile());
}

void StringUIdemoAudioProcessor::applyTuning(const std::array<int, numStrings>& notes)
{
    for (int i = 0; i < numStrings; ++i)
        setStringMidiNote(i, notes[i]);
}

//==============================================================================
const juce::String StringUIdemoAudioProcessor::getName() const { return JucePlugin_Name; }

bool StringUIdemoAudioProcessor::acceptsMidi() const
{
#if JucePlugin_WantsMidiInput
    return true;
#else
    return false;
#endif
}

bool StringUIdemoAudioProcessor::producesMidi() const
{
#if JucePlugin_ProducesMidiOutput
    return true;
#else
    return false;
#endif
}

bool StringUIdemoAudioProcessor::isMidiEffect() const
{
#if JucePlugin_IsMidiEffect
    return true;
#else
    return false;
#endif
}

double StringUIdemoAudioProcessor::getTailLengthSeconds() const { return 2.0; }

int StringUIdemoAudioProcessor::getNumPrograms() { return 1; }
int StringUIdemoAudioProcessor::getCurrentProgram() { return 0; }
void StringUIdemoAudioProcessor::setCurrentProgram(int) {}
const juce::String StringUIdemoAudioProcessor::getProgramName(int) { return {}; }
void StringUIdemoAudioProcessor::changeProgramName(int, const juce::String&) {}

//==============================================================================
void StringUIdemoAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    stringSynths.clear();

    // Inizializzazione Delay
    currentSampleRate = sampleRate;

    // Creiamo un buffer lungo 2 secondi (il massimo tempo possibile + margine)
    int delayBufferSize = (int)(sampleRate * 2.0);
    delayBuffer.setSize(getTotalNumOutputChannels(), delayBufferSize);
    delayBuffer.clear(); // Svuotiamo la memoria dai "rumori" fantasma

    delayWritePosition = 0;

    for (int i = 0; i < numStrings; ++i)
    {
        double freq = juce::MidiMessage::getMidiNoteInHertz(currentMidiNotes[i]);
        stringSynths.add(new StringSynthesiser(sampleRate, freq, hardnessParameter->load()));
    }
    // Inizializza il Sample Rate del riverbero
    reverb.setSampleRate(sampleRate);

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = samplesPerBlock;
    spec.numChannels = getTotalNumOutputChannels();

    phaser.prepare(spec);
}

void StringUIdemoAudioProcessor::releaseResources()
{
    stringSynths.clear();
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool StringUIdemoAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
#if JucePlugin_IsMidiEffect
    juce::ignoreUnused(layouts);
    return true;
#else
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
        && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;
#if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
#endif
    return true;
#endif
}
#endif

void StringUIdemoAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer,
    juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    #pragma region Gestione MIDI

    // Gestione dei messaggi MIDI in arrivo
    for (const auto metadata : midiMessages)
    {
        auto message = metadata.getMessage();

        if (message.isNoteOn())
        {
            int midiNote = message.getNoteNumber();
            float velocity = message.getFloatVelocity(); // Forza della plettrata

            // Troviamo quale corda deve suonare questa nota.
            // La prima corda che può coprire questa nota entro 12 tasti.
            for (int i = 0; i < numStrings; ++i)
            {
                int openStringNote = currentMidiNotes[i];
                int fret = midiNote - openStringNote;

                if (fret >= 0 && fret <= 12) // Se la nota è suonabile su questa corda
                {
                    // Calcoliamo la posizione "virtuale" del tasto (0.0 a 1.0)
                    // per farla processare alla tua funzione pluckString
                    float position = (float)fret / 12.0f;

                    pluckString(i, position);
                    activeMidiNoteForString[i] = midiNote;

                    // Comunico tramite le variabili atomic (thread-safe) alla UI
					// che la corda i è stata pizzicata e qual è la posizione del tasto.
                    uiPluckPosition[i].store(position);
					uiStringWasPlucked[i].store(true);

                    break;
                }
            }
        }
        else if (message.isNoteOff())
        {
            int midiNote = message.getNoteNumber();
            for (int i = 0; i < numStrings; ++i)
            {
                if (activeMidiNoteForString[i] == midiNote)
                {
                    noteOffString(i);
                    activeMidiNoteForString[i] = -1;
                    break;
                }
            }
        }
    }

    #pragma endregion

    #pragma region Generazione Audio & Copia sui Canali

    buffer.clear();

    float* channelData = buffer.getWritePointer(0);
    
    float att = adsrAttackParameter->load();
    float dec = adsrDecayParameter->load();
    float sus = adsrSustainParameter->load() / 100.0f;
    float rel = adsrReleaseParameter->load();
    bool adsrOn = (adsrOnParameter->load() >= 0.5f);

    for (int i = 0; i < stringSynths.size(); ++i) {
        // Assegno l'hardness corrente su tutte le corde
        stringSynths.getUnchecked(i)->SetHardness(hardnessParameter->load());
        // Assegno i valori attuali di damp e sustain
        stringSynths.getUnchecked(i)->SetDamping(dampingParameter->load() / 100.0f);
        stringSynths.getUnchecked(i)->SetSustain(sustainParameter->load() / 100.0f);
        // Assegno parametri ADSR
        stringSynths.getUnchecked(i)->setAdsrParameters(att, dec, sus, rel);
        stringSynths.getUnchecked(i)->setAdsrEnabled(adsrOn);
    }

	// Genero l'audio per ogni corda e lo sommo al buffer del canale 0 (mono)
    for (int i = 0; i < stringSynths.size(); ++i)
        stringSynths.getUnchecked(i)->generateAndAddData(channelData, buffer.getNumSamples());

	// Essendo la chitarra virtuale monofonica, copio il canale 0 (mono) anche sul canale 1 (destro) per avere un output stereo
    for (int ch = 1; ch < buffer.getNumChannels(); ++ch)
        buffer.copyFrom(ch, 0, buffer, 0, 0, buffer.getNumSamples());

    #pragma endregion

    #pragma region Aggiunta Distorsione

        // Se il parametro "Distortion On" è disattivato, salto tutta la sezione di distorsione
        if (distOnParameter->load() >= 0.5f)
        {
            #pragma region Variabili di distorsione

                // serve a decidere la ripidità della tangente iperbolica (più alto = più distorto)
                float currentDrive = driveParameter->load();

                // serve a compensare l'aumento di volume intrinseco dell'operazione di distorsione
                float currentGain = gainParameter->load();

                /* Nota:
                    Per accedere ai valori dei parametri non passiamo per l'APVTS (relativamente lento), bensi' ci affidiamo
                    ai puntatori atomici definiti nel "PluginProcessor.h" e inizializzati nel costruttore.
                    Ciò è possibile poiché nel costruttore abbiamo passato i puntatori raw (dei parametri della APVTS)
                    ai nostri puntatori atomici.
                */

                // (Amplifico al quadrato per ottenere un effetto di drive più marcato)
                float appliedDrive = currentDrive * currentDrive;

            #pragma endregion

            // Ciclo per ogni canale...
            for (int ch = 0; ch < buffer.getNumChannels(); ++ch) 
            {
                // Accedo tramite puntatore al buffer del canale
                auto* channelData = buffer.getWritePointer(ch);

                // Quindi applico la distorsione sample per sample nel buffer del canale
                for (int numSample = 0; numSample < buffer.getNumSamples(); ++numSample) 
                {
                    // Soft Clipping via tangente iperbolica.
                    channelData[numSample] = std::tanh(channelData[numSample] * appliedDrive) * currentGain;
                }
            }
        }
    #pragma endregion

    #pragma region Aggiunta Phaser

            if (phaserOnParameter->load() >= 0.5f)
            {
                // Aggiorniamo i parametri del phaser
                phaser.setRate(phaserRateParameter->load());
                phaser.setDepth(phaserDepthParameter->load());
                // Il mix va normalizzato da 0-100 a 0.0-1.0
                phaser.setMix(phaserMixParameter->load() / 100.0f);

                // Prepariamo il blocco audio per il modulo DSP di JUCE
                // Il modulo dsp non accetta direttamente juce::AudioBuffer, ma un AudioBlock
                juce::dsp::AudioBlock<float> audioBlock(buffer);
                juce::dsp::ProcessContextReplacing<float> context(audioBlock);

                // Applichiamo l'effetto
                phaser.process(context);
            }

    #pragma endregion

    #pragma region Aggiunta Delay

        if (delayOnParameter->load() >= 0.5f)
        {
            // Leggiamo i parametri dalla UI
            float timeInSeconds = delayTimeParameter->load();
            float feedback = delayFbParameter->load() / 100.0f;

            // Calcoliamo a quanti "campioni" corrisponde il ritardo in secondi
            int delayLengthInSamples = (int)(timeInSeconds * currentSampleRate);
            int delayBufferLength = delayBuffer.getNumSamples();

            // Ciclo sui canali (L e R)
            for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            {
                auto* channelData = buffer.getWritePointer(ch);
                // Assicuriamoci di leggere il canale giusto anche nel delayBuffer
                auto* delayData = delayBuffer.getWritePointer(ch % delayBuffer.getNumChannels());

                // Copiamo la posizione di scrittura per questo specifico canale
                int localWritePosition = delayWritePosition;

                for (int i = 0; i < buffer.getNumSamples(); ++i)
                {
                    // Calcoliamo la testina di lettura (indietro nel tempo rispetto alla scrittura)
                    int readPosition = localWritePosition - delayLengthInSamples;
                    if (readPosition < 0)
                        readPosition += delayBufferLength; // Se andiamo sotto zero, facciamo il giro del ring buffer

                    // Prendiamo il campione ritardato (l'eco)
                    float delayedSample = delayData[readPosition];

                    // Scriviamo nel "nastro" il suono attuale + l'eco attenuato (Feedback)
                    delayData[localWritePosition] = channelData[i] + (delayedSample * feedback);

                    // Aggiungiamo l'eco al suono originale in uscita (Mix al 50%)
                    channelData[i] += delayedSample * 0.5f;

                    // Avanziamo la testina di scrittura locale di 1 step
                    localWritePosition++;
                    if (localWritePosition >= delayBufferLength)
                        localWritePosition = 0;
                }
            }

            // Finito il blocco audio, aggiorniamo la posizione di scrittura globale per il prossimo giro
            delayWritePosition += buffer.getNumSamples();
            delayWritePosition %= delayBufferLength;
        }
        else
        {
		    // Se il delay è spento, assicuriamoci di avanzare comunque la posizione di scrittura per mantenere la coerenza del buffer
		    delayWritePosition += buffer.getNumSamples();
		    delayWritePosition %= delayBuffer.getNumSamples();
        }

    #pragma endregion

    #pragma region Aggiunta Reverb

        if (revOnParameter->load() >= 0.5f)
        {
            // Aggiorniamo i parametri del riverbero leggendo i valori dalle manopole
            float mix = revMixParameter->load() / 100.0f;

            reverbParams.roomSize = revSizeParameter->load() / 100.0f; // Da 0.0 (stanza piccola) a 1.0 (stanza grande)
            reverbParams.damping = 0.5f;
            reverbParams.width = 1.0f;   // Massima ampiezza stereo

            // Calcolo Dry/Wet: se Mix è 0, si sente solo la chitarra; se Mix è 1, si sente solo il riverbero
            reverbParams.dryLevel = 1.0f - mix;
            reverbParams.wetLevel = mix;

            reverb.setParameters(reverbParams);

            // Applichiamo il riverbero
            if (buffer.getNumChannels() >= 2)
            {
                float* leftChannel = buffer.getWritePointer(0);
                float* rightChannel = buffer.getWritePointer(1);

				// funzione standard di JUCE per processare un buffer stereo con il riverbero
                reverb.processStereo(leftChannel, rightChannel, buffer.getNumSamples());
            }
        }

    #pragma endregion

    #pragma region Applicazione Master Volume
        float masterVolume = masterVolumeParameter->load() / 100.0f;
        //Metodo per applicare il volume a tutto il buffer
        buffer.applyGain(masterVolume); 
    #pragma endregion

    #pragma region Calcolo livello Meter Volume
        // Calcoliamo l'RMS per il canale sinistro (0) e lo convertiamo in decibel
        float rmsLeft = juce::Decibels::gainToDecibels(buffer.getRMSLevel(0, 0, buffer.getNumSamples()));
        masterRmsLeft.store(rmsLeft);

		// Facciamo lo stesso con il canale destro (1), se esiste
        if (buffer.getNumChannels() > 1) 
        {
            float rmsRight = juce::Decibels::gainToDecibels(buffer.getRMSLevel(1, 0, buffer.getNumSamples()));
            masterRmsRight.store(rmsRight);
        }
    #pragma endregion
    

    // Se è aperta l'interfaccia grafica, passo il buffer all'oscilloscopio per visualizzare le onde sonore
    if (puntatoreOscilloscopio != nullptr)
    {
        // pushBuffer prende l'intero blocco audio e lo disegna
        puntatoreOscilloscopio->pushBuffer(buffer);
    }
}

//==============================================================================
bool StringUIdemoAudioProcessor::hasEditor() const { return true; }

juce::AudioProcessorEditor* StringUIdemoAudioProcessor::createEditor()
{
    return new StringUIdemoAudioProcessorEditor(*this);
}

void StringUIdemoAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    if (xml != nullptr)
    {
        auto* tuningXml = xml->createNewChildElement("CURRENT_TUNING");
        for (int i = 0; i < numStrings; ++i)
            tuningXml->setAttribute("string" + juce::String(i), currentMidiNotes[i]);

        copyXmlToBinary(*xml, destData);
    }
}

void StringUIdemoAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState != nullptr && xmlState->hasTagName(apvts.state.getType()))
    {
        apvts.replaceState(juce::ValueTree::fromXml(*xmlState));

        if (auto* tuningXml = xmlState->getChildByName("CURRENT_TUNING"))
        {
            for (int i = 0; i < numStrings; ++i)
            {
                int note = tuningXml->getIntAttribute("string" + juce::String(i), defaultMidiNotes[i]);
                setStringMidiNote(i, note);
            }
        }
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new StringUIdemoAudioProcessor();
}