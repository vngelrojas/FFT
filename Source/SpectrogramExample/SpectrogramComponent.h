/*
  ==============================================================================

    SpectrogramComponent.h
    Created: 6 Aug 2023 11:38:58pm
    Author:  vngel

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <juce_graphics/juce_graphics.h>
class SpectrogramComponent : public juce::AudioAppComponent,
                             private juce::Timer
{
    static constexpr auto fftOrder = 10;
    static constexpr auto fftSize = 1 << fftOrder; // To calculate the corresponding FFT size, we use the left bit shift operator which produces 1024 as binary number 10000000000
private:
    juce::dsp::FFT forwardFFT; //object to perform the forward FFT on.
    juce::Image spectrogramImage;

    std::array<float, fftSize> fifo; // The fifo float array of size 1024 will contain our incoming audio data in samples.
    std::array<float, fftSize * 2> fftData; //The fftData float array of size 2048 will contain the results of our FFT calculations.

    int fifoIndex = 0; // temp index keeps count of the amount of samples in fifo
    bool nextBlockReady = false; //temporary boolean tells us whether the next FFT block is ready to be rendered
public:
    
    SpectrogramComponent() : forwardFFT(fftOrder),
        spectrogramImage(juce::Image::RGB, 512, 512, true){}
    void getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill) override
    {
        if (bufferToFill.buffer->getNumChannels() > 0)
        {
            auto* channelData = bufferToFill.buffer->getReadPointer(0, bufferToFill.startSample);
            for (auto i = 0; i < bufferToFill.numSamples; ++i)
                pushNextSampleIntoFifo(channelData[i]);
        }
    }
    void pushNextSampleIntoFifo(float sample)
    {
        // if the fifo contains enough data, set a flag to say
        // that the next line should now be rendered..
        if (fifoIndex == fftSize)      
        {
            if (!nextBlockReady)    
            {
                std::fill(fftData.begin(), fftData.end(), 0.0f);
                std::copy(fifo.begin(), fifo.end(), fftData.begin());
                nextBlockReady = true;
            }

            fifoIndex = 0;
        }

        fifo[(size_t)fifoIndex++] = sample; // [9]
    }
    void drawNextLineOfSpectrogram()
    {
        auto rightHandEdge = spectrogramImage.getWidth() - 1;
        auto imageHeight = spectrogramImage.getHeight();

        // first, shuffle our image leftwards by 1 pixel..
        spectrogramImage.moveImageSection(0, 0, 1, 0, rightHandEdge, imageHeight);         // [1]

        // then render our FFT data..
        forwardFFT.performFrequencyOnlyForwardTransform(fftData.data());                   // [2]

        // find the range of values produced, so we can scale our rendering to
        // show up the detail clearly
        auto maxLevel = juce::FloatVectorOperations::findMinAndMax(fftData.data(), fftSize / 2); // [3]

        for (auto y = 1; y < imageHeight; ++y)                                              // [4]
        {
            auto skewedProportionY = 1.0f - std::exp(std::log((float)y / (float)imageHeight) * 0.2f);
            auto fftDataIndex = (size_t)juce::jlimit(0, fftSize / 2, (int)(skewedProportionY * fftSize / 2));
            auto level = juce::jmap(fftData[fftDataIndex], 0.0f, juce::jmax(maxLevel.getEnd(), 1e-5f), 0.0f, 1.0f);

            spectrogramImage.setPixelAt(rightHandEdge, y, juce::Colour::fromHSV(level, 1.0f, level, 1.0f)); // [5]
        }
    }
};