/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

enum Slope
{
   Slope_6,
    Slope_12,
    Slope_18,
    Slope_24,
    Slope_30,
    Slope_36,
    Slope_42,
    Slope_48
};

struct ChainSettings
{
    float freq {0};
    Slope slope{Slope::Slope_6};
    int type {0};
    int fType{0};
};

ChainSettings getChainSettings(juce::AudioProcessorValueTreeState& apvts);

//==============================================================================
/**
*/
class FilterAudioProcessor  : public juce::AudioProcessor
{
public:
    //==============================================================================
    FilterAudioProcessor();
    ~FilterAudioProcessor() override;

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
    
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    
    
    juce::AudioProcessorValueTreeState apvts {*this,nullptr,"Parameters",createParameterLayout()};

private:
    
    using Filter = juce::dsp::IIR::Filter<float>;
    
    using PassFilter = juce::dsp::ProcessorChain<Filter,Filter,Filter,Filter>;
    
    using MonoChain = juce::dsp::ProcessorChain<PassFilter>;
    
    MonoChain leftChain, rightChain;
    
    enum ChainPositions
    {
        MyFilter
    };
    
    
    using Coefficients = Filter::CoefficientsPtr;
    static void updateCoefficients(Coefficients& old, const Coefficients& replacements);
    
    template <int Index, typename ChainType, typename CoefficientType>
    void update(ChainType& chain, const CoefficientType& coefficients)
    {
        updateCoefficients(chain.template get<Index>().coefficients,coefficients[Index]);
        chain.template setBypassed<Index>(false);
    }
    template <typename ChainType, typename CoefficientType>
    void updateFilter(ChainType& leftFilter, const CoefficientType& highPassCoefficients, const Slope& slope)
    {
        leftFilter.template setBypassed<0>(true);
        leftFilter.template setBypassed<1>(true);
        leftFilter.template setBypassed<2>(true);
        leftFilter.template setBypassed<3>(true);

        switch (slope)
        {
            case Slope_48:
            {
                update<3>(leftFilter, highPassCoefficients);
                
            }
            case Slope_36:
            {
                update<2>(leftFilter, highPassCoefficients);
                
            }
            case Slope_24:
            {
                update<1>(leftFilter, highPassCoefficients);
                
            }
            case Slope_12:
            {
                update<0>(leftFilter, highPassCoefficients);
                break;
            }
            case Slope_42:
            {
                update<3>(leftFilter, highPassCoefficients);
                
            }
            case Slope_30:
            {
                update<2>(leftFilter, highPassCoefficients);
                
            }
            case Slope_18:
            {
                update<1>(leftFilter, highPassCoefficients);
                
            }
            case Slope_6:
            {
                update<0>(leftFilter, highPassCoefficients);
                break;
            }
                
                
    }
}
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FilterAudioProcessor)
};
