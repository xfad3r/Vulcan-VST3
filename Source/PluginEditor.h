#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class VulcanEditor : public juce::AudioProcessorEditor, private juce::ComboBox::Listener
{
public:
    VulcanEditor (VulcanProcessor&);
    ~VulcanEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void comboBoxChanged (juce::ComboBox* comboBoxThatHasChanged) override;

private:
    VulcanProcessor& audioProcessor;

    juce::ComboBox presetMenu;

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;

    juce::Slider inGainSlider, eqLowSlider, eqMidSlider, eqHighSlider, amountSlider, limDriveSlider, outGainSlider;
    std::unique_ptr<SliderAttachment> inGainAtt, eqLowAtt, eqMidAtt, eqHighAtt, amountAtt, limDriveAtt, outGainAtt;

    void setupSlider (juce::Slider& slider, std::unique_ptr<SliderAttachment>& att, const juce::String& paramID);
    
    juce::Font customFont;
    juce::Font titleFont;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VulcanEditor)
};
