#pragma once
#define CUTOFF 0.93 //0.97 is default
#define SMALL_CUTOFF 0.5
#define LOWER_PITCH_CUTOFF 80 //hz
#include "AudioFFT.h"
#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <float.h>
#include <math.h>
#include <complex>
//#include <juce_core/maths/juce_MathsFunctions.h>


class PitchMPM
{

public:

    PitchMPM(size_t bufferSize) : PitchMPM(44100, bufferSize) {}

    PitchMPM(int sampleRate, size_t bufferSize) : bufferSize(bufferSize),
        sampleRate(sampleRate),
        fftSize(2 * bufferSize), // Needs to be a power of 2!
        real(audiofft::AudioFFT::ComplexSize(fftSize)),
        imag(audiofft::AudioFFT::ComplexSize(fftSize)),
        output(fftSize)


    {
        //nsdf.insertMultiple(0, 0.0, bufferSize);

    }


    ~PitchMPM()
    {

        //nsdf.clear();
        maxPositions.clear();
        ampEstimates.clear();
        periodEstimates.clear();

    }

    float getPitch(const float* audioBuffer)
    {
        // Clear old data
        maxPositions.clearQuick();
        periodEstimates.clearQuick();
        ampEstimates.clearQuick();

        // Checking if buffer is empty
        if (audioBuffer == nullptr)
        {
            DBG("audioBuffer NULL");
            return 0.0f;
        }

        // Normalized square difference function 
        std::vector<float> _nsdf = nsdfFrequencyDomain(audioBuffer);
        // Find the highest maximum between every positively sloped zero crossing and negatively sloped zero crossing
        std::vector<int> max_positions = peak_picking(_nsdf);

        std::vector<std::pair<float, float>> estimates;


        float highestAmplitude = -FLT_MAX;

        for (auto tau : max_positions)
        {
            // get the larger of the two values
            highestAmplitude = juce::jmax(highestAmplitude, _nsdf[tau]);

            // SMALL_CUTOFF = 0.5
            if (_nsdf[tau] > SMALL_CUTOFF)
            {
                auto x = parabolic_interpolation(_nsdf, tau);
                estimates.push_back(x);
                // Get highest between prev known highest or .second of pair returened by parabolic_interpolation
                highestAmplitude = std::max(highestAmplitude, std::get<1>(x));
            }
        }

        if (estimates.empty()) 
            return -1;

        float actualCutoff = CUTOFF * highestAmplitude;
        float period = 0;

        for (auto& estimate : estimates)
        {
            if (std::get<1>(estimate) >= actualCutoff)
            {
                period = std::get<0>(estimate);
                break;
            }
        }

        float pitchEstimate = (sampleRate / period);
        return (pitchEstimate > LOWER_PITCH_CUTOFF) ? pitchEstimate : -1;
    }

    void setSampleRate(int newSampleRate)
    {
        sampleRate = newSampleRate;
    }

    void setBufferSize(int newBufferSize)
    {
        bufferSize = newBufferSize;
        input.resize(bufferSize);
        fftSize = 2 * bufferSize;
        real.resize(audiofft::AudioFFT::ComplexSize(fftSize));
        imag.resize(audiofft::AudioFFT::ComplexSize(fftSize));
        output.resize(fftSize);
    }

private:
    size_t bufferSize;

    audiofft::AudioFFT fft;
    size_t fftSize;
    std::vector<float> input;
    std::vector<float> real;
    std::vector<float> imag;
    std::vector<float> output;

    float sampleRate;

    float turningPointX, turningPointY;

    juce::Array<int> maxPositions;
    juce::Array<float> periodEstimates;
    juce::Array<float> ampEstimates;


    inline std::pair<float, float> parabolic_interpolation(std::vector<float> array, float x)
    {
        int x_adjusted;

        if (x < 1) {
            x_adjusted = (array[x] <= array[x + 1]) ? x : x + 1;
        }
        else if (x > signed(array.size()) - 1) {
            x_adjusted = (array[x] <= array[x - 1]) ? x : x - 1;
        }
        else {
            float den = array[x + 1] + array[x - 1] - 2 * array[x];
            float delta = array[x - 1] - array[x + 1];
            return (!den) ? std::make_pair(x, array[x]) : std::make_pair(x + delta / (2 * den), array[x] - delta * delta / (8 * den));
        }
        return std::make_pair(x_adjusted, array[x_adjusted]);
    }

    static std::vector<int> peak_picking(std::vector<float> nsdf)
    {
        std::vector<int> max_positions{};
        int pos = 0;
        int curMaxPos = 0;
        juce::ssize_t size = nsdf.size();

        while (pos < (size - 1) / 3 && nsdf[pos] > 0) pos++;
        while (pos < size - 1 && nsdf[pos] <= 0.0) pos++;

        if (pos == 0) pos = 1;

        while (pos < size - 1) {
            if (nsdf[pos] > nsdf[pos - 1] && nsdf[pos] >= nsdf[pos + 1]) {
                if (curMaxPos == 0) {
                    curMaxPos = pos;
                }
                else if (nsdf[pos] > nsdf[curMaxPos]) {
                    curMaxPos = pos;
                }
            }
            pos++;
            if (pos < size - 1 && nsdf[pos] <= 0) {
                if (curMaxPos > 0) {
                    max_positions.push_back(curMaxPos);
                    curMaxPos = 0;
                }
                while (pos < size - 1 && nsdf[pos] <= 0.0) {
                    pos++;
                }
            }
        }
        if (curMaxPos > 0) {
            max_positions.push_back(curMaxPos);
        }
        return max_positions;
    }

   
    // FFT based methods
    std::vector<float> nsdfFrequencyDomain(const float* audioBuffer)
    {


        real.resize(fftSize);
        imag.resize(fftSize);

        if (audioBuffer == nullptr)
            DBG("audioBuffer NULL: nsdfFrequencyDomain");

        std::vector<float> acf(autoCorrelation(audioBuffer));

    
        return acf;
    }

    std::vector<float> autoCorrelation(const float* audioBuffer)
    {
        if (audioBuffer == nullptr)
            DBG("audioBuffer NULL: autoCorrelation");

        std::vector<float> input(audioBuffer, audioBuffer + bufferSize);
        input.resize(fftSize, 0.0f);

        if (audioBuffer == nullptr)
            DBG("audioBuffer NULL: autoCorrelation post resize");

        if (input.data() == nullptr)
            DBG("input.data() NULL: autoCorrelation post resize");

        fft.init(fftSize);
        fft.fft(input.data(), real.data(), imag.data()); // .data() returns pointer to the underlying element storage 
       

        // Complex Conjugate
        for (int i = 0; i < fftSize; ++i)
        {
            /**
             * std::complex method
             */
            std::complex<float> complex(real[i], imag[i]);
            complex = complex * std::conj(complex); // No need to scale as AudioFFT does this already
            real[i] = complex.real();
            imag[i] = complex.imag();

     
        }

        fft.ifft(output.data(), real.data(), imag.data());
        return output;
    }



};
