/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
FilterAudioProcessor::FilterAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
{
}

FilterAudioProcessor::~FilterAudioProcessor()
{
}

//==============================================================================
const juce::String FilterAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool FilterAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool FilterAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool FilterAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double FilterAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int FilterAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int FilterAudioProcessor::getCurrentProgram()
{
    return 0;
}

void FilterAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String FilterAudioProcessor::getProgramName (int index)
{
    return {};
}

void FilterAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void FilterAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::dsp::ProcessSpec spec;
    spec.maximumBlockSize =samplesPerBlock;
    spec.numChannels = 1;
    spec.sampleRate = sampleRate;
    
    leftChain.prepare(spec);
    rightChain.prepare(spec);
    
    auto chainSettings = getChainSettings(apvts);
  
    auto highPassCoefficients = juce::dsp::FilterDesign<float>::designIIRHighpassHighOrderButterworthMethod(chainSettings.freq,
                                                                                                        sampleRate,
                                                                                                        (chainSettings.slope + 1));

    auto lowPassCoefficients = juce::dsp::FilterDesign<float>::designIIRLowpassHighOrderButterworthMethod(chainSettings.freq,
                                                                                                            sampleRate,
                                                                                                            (chainSettings.slope + 1));

    auto& leftFilter = leftChain.get<ChainPositions::MyFilter>();
    auto& rightFilter = rightChain.get<ChainPositions::MyFilter>();

    if (chainSettings.type == 0)
    {
        updateFilter(leftFilter, highPassCoefficients, chainSettings.slope);
        updateFilter(rightFilter, highPassCoefficients, chainSettings.slope);

    }

    else if (chainSettings.type == 1)
    {
        updateFilter(leftFilter, lowPassCoefficients, chainSettings.slope);
        updateFilter(rightFilter, lowPassCoefficients, chainSettings.slope);

    }
}
void FilterAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool FilterAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void FilterAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // In case we have more outputs than inputs, this code clears any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    // This is here to avoid people getting screaming feedback
    // when they first compile a plugin, but obviously you don't need to keep
    // this code if your algorithm always overwrites all the output channels.
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());
    
    auto chainSettings = getChainSettings(apvts);

    auto highPassCoefficients =juce::dsp::FilterDesign<float>::designIIRHighpassHighOrderButterworthMethod(chainSettings.freq,
                                                                                                        getSampleRate(),
                                                                                                        (chainSettings.slope + 1));

    auto lowPassCoefficients = juce::dsp::FilterDesign<float>::designIIRLowpassHighOrderButterworthMethod(chainSettings.freq,
                                                                                                            getSampleRate(),
                                                                                                            (chainSettings.slope + 1));

    auto& leftFilter = leftChain.get<ChainPositions::MyFilter>();
    auto& rightFilter = rightChain.get<ChainPositions::MyFilter>();
    
    if(chainSettings.type == 0)
    {
        updateFilter(leftFilter, highPassCoefficients, chainSettings.slope);
        updateFilter(rightFilter, highPassCoefficients, chainSettings.slope);
    }
    else if (chainSettings.type == 1)
    {
        updateFilter(leftFilter, lowPassCoefficients, chainSettings.slope);
        updateFilter(rightFilter, lowPassCoefficients, chainSettings.slope);
    }
    
    juce::dsp::AudioBlock<float> block(buffer);
    
    auto leftBlock = block.getSingleChannelBlock(0);
    auto rightBlock = block.getSingleChannelBlock(1);
    
    juce::dsp::ProcessContextReplacing<float> leftContext(leftBlock);
    juce::dsp::ProcessContextReplacing<float> rightContext(rightBlock);
    
    leftChain.process(leftContext);
    rightChain.process(rightContext);

}

//==============================================================================
bool FilterAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* FilterAudioProcessor::createEditor()
{
//    return new FilterAudioProcessorEditor (*this);
    return new juce::GenericAudioProcessorEditor(*this);
}

//==============================================================================
void FilterAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void FilterAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

ChainSettings getChainSettings(juce::AudioProcessorValueTreeState& apvts)
{
    ChainSettings settings;
    
   settings.freq = apvts.getRawParameterValue("Freq") ->load();
   settings.type = apvts.getRawParameterValue("HP/LP") ->load();
   settings.slope = static_cast<Slope>(apvts.getRawParameterValue("Filter Slope") ->load());
    settings.fType = apvts.getRawParameterValue("Filter Id") ->load();
    return settings;
}


void FilterAudioProcessor::updateCoefficients(Coefficients &old, const Coefficients &replacements)

{
    *old =*replacements;
}
juce::AudioProcessorValueTreeState::ParameterLayout
    FilterAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    
    layout.add(std::make_unique<juce::AudioParameterFloat>("Freq",
                                                           "Freq",
                                                           juce::NormalisableRange<float>(20.f,20000.f,1.f,0.25f),
                                                           1000.f));// Fiter Frequency Parameter
    
    juce::StringArray stringArray4;
    stringArray4.add("Butterworth");
    stringArray4.add("Linkwitz-Riley");
    
    layout.add(std::make_unique<juce::AudioParameterChoice>("Filter Id", "Filter Id", stringArray4, 0));
    
    
    juce::StringArray stringArray1;
    stringArray1.add("High Pass");
    stringArray1.add("Low Pass");
   
    layout.add(std::make_unique<juce::AudioParameterChoice>("HP/LP","HP/LP",stringArray1,0)); // Filter Type parameter

    
    juce::StringArray stringArray3;
    
    
        for (int i = 0; i < 8 ; ++i)
        {
            juce::String str;
            str << (6 + i*6);
            str <<"dB/Oct";
            stringArray3.add(str);
        }
    
    juce::StringArray stringArray5;
    for (int i = 0; i < 8 ; i+=2)
    {
        juce::String str;
        str << (12 + i*6);
        str <<"dB/Oct";
        stringArray5.add(str);
    }
    
        layout.add(std::make_unique<juce::AudioParameterChoice>("Filter Slope","Filter Slope",stringArray3,0));
       
    
    return layout;
    
    
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new FilterAudioProcessor();
}
