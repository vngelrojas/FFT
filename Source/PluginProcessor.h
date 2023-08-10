/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "SpectrogramExample/SpectrogramComponent.h"
#include "SpectrumAnalyzer/SpectrumAnalyzer.h"
#include "PitchMPM.h"
#include <vector>
#include "src/Gist.h"
#include <unordered_map>

//==============================================================================
/**
*/
class FFTAudioProcessor  : public juce::AudioProcessor
                            #if JucePlugin_Enable_ARA
                             , public juce::AudioProcessorARAExtension
                            #endif
{
public:
    //==============================================================================
    FFTAudioProcessor();
    ~FFTAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;
    SpectrogramComponent fft;
    SpectrumAnalyzer fft2;
    
private:
    PitchMPM mpm;
    juce::dsp::Oscillator<float> osc{ [](float x) {return std::sin(x); }};
    juce::dsp::Gain<float> gain;
    std::vector<float> notes   {  
                                    16.35, 17.32, 18.35, 19.45, 20.60, 21.83, 23.12, 24.50, 25.96, 27.50, 29.14, 30.87,
                                    32.70, 34.65, 36.71, 38.89, 41.20, 43.65, 46.25, 49.00, 51.91, 55.00, 58.27, 61.74,
                                    65.41, 69.30, 73.42, 77.78, 82.41, 87.31, 92.50, 98.00, 103.8, 110.0, 116.5, 123.5,
                                    130.8, 138.6, 146.8, 155.6, 164.8, 174.6, 185.0, 196.0, 207.7, 220.0, 233.1, 246.9,
                                    261.6, 277.2, 293.7, 311.1, 329.6, 349.2, 370.0, 392.0, 415.3, 440.0, 466.2, 493.9,
                                    523.3, 554.4, 587.3, 622.3, 659.3, 698.5, 740.0, 784.0, 830.6, 880.0, 932.3, 987.8,
                                    1047, 1109, 1175, 1245, 1319, 1397, 1480, 1568, 1661, 1760, 1865, 1976,
                                    2093, 2217, 2349, 2489, 2637, 2794, 2960, 3136, 3322, 3520, 3729, 3951,
                                    4186, 4435, 4699, 4978, 5274, 5588, 5920, 6272, 6645, 7040, 7459, 7902
                                };

    float findNote(float noteEstimate)
    {
        int low = 0;
        int high = notes.size() - 1;
        int mid = 0;
        float minDifference = 9999;
        float close = 0;

        while (low <= high)
        {
            mid = low + (high - low) / 2;

     /*       int difference = std::abs(noteEstimate - notes[mid]);
            if (difference < minDifference)
            {
                minDifference = difference;
                close = notes[mid];
            }*/

            if (notes[mid] == noteEstimate)
                return noteEstimate;
            else if (notes[mid] < noteEstimate)
                low = mid + 1;
            else
                high = mid - 1;
        }


        double diff_low = std::abs(notes[low] - noteEstimate);
        double diff_high = std::abs(notes[high] - noteEstimate);
        return (diff_low < diff_high) ? notes[low] : notes[high];
    }



    Gist<float> gist;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FFTAudioProcessor)
};
