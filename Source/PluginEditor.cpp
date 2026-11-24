#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "BinaryData.h"

namespace
{
    juce::File getWebViewUserDataFolder()
    {
        // Crucial on Windows: prevents WebView2 from trying to write next to FL64.exe / Ableton.exe
        // inside Program Files (which causes Access Denied & blank white screen in DAWs).
        auto dir = juce::File::getSpecialLocation (juce::File::tempDirectory)
                       .getChildFile ("VulcanByDJSmartie_WebView2");
        dir.createDirectory();
        return dir;
    }
}

//==============================================================================
// VulcanEditor Implementation
//==============================================================================
VulcanEditor::VulcanEditor (VulcanProcessor& p)
    : AudioProcessorEditor (&p),
      audioProcessor (p),
      webView (juce::WebBrowserComponent::Options{}
                   .withBackend (juce::WebBrowserComponent::Options::Backend::webview2)
                   .withWinWebView2Options (
                       juce::WebBrowserComponent::Options::WinWebView2{}
                           .withUserDataFolder (getWebViewUserDataFolder())
                           .withBackgroundColour (juce::Colour (0xff040911)))
                   .withNativeIntegrationEnabled()
                   .withResourceProvider ([this] (const auto& url) { return handleResourceRequest (url); })
                   .withOptionsFrom (inGainRelay)
                   .withOptionsFrom (eqLowRelay)
                   .withOptionsFrom (eqMidRelay)
                   .withOptionsFrom (eqHighRelay)
                   .withOptionsFrom (monoFreqRelay)
                   .withOptionsFrom (stereoWidthRelay)
                   .withOptionsFrom (characterRelay)
                   .withOptionsFrom (limiterDriveRelay)
                   .withOptionsFrom (outGainRelay)
                   .withEventListener ("selectPreset", [this] (const juce::var& payload)
                   {
                       const int idx = static_cast<int> (payload.getProperty ("index", -1));
                       if (idx >= 0 && idx < audioProcessor.getNumPrograms())
                       {
                           audioProcessor.setCurrentProgram (idx);
                           pushFullSyncToWeb();
                       }
                   })
                   .withEventListener ("uiReady", [this] (const juce::var&)
                   {
                       pushFullSyncToWeb();
                   })),
      inGainAttachment       (*audioProcessor.apvts.getParameter ("input_gain"),    inGainRelay,       nullptr),
      eqLowAttachment        (*audioProcessor.apvts.getParameter ("eq_low"),        eqLowRelay,        nullptr),
      eqMidAttachment        (*audioProcessor.apvts.getParameter ("eq_mid"),        eqMidRelay,        nullptr),
      eqHighAttachment       (*audioProcessor.apvts.getParameter ("eq_high"),       eqHighRelay,       nullptr),
      monoFreqAttachment     (*audioProcessor.apvts.getParameter ("mono_freq"),     monoFreqRelay,     nullptr),
      stereoWidthAttachment  (*audioProcessor.apvts.getParameter ("haas_amount"),   stereoWidthRelay,  nullptr),
      characterAttachment    (*audioProcessor.apvts.getParameter ("amount"),        characterRelay,    nullptr),
      limiterDriveAttachment (*audioProcessor.apvts.getParameter ("limiter_drive"), limiterDriveRelay, nullptr),
      outGainAttachment      (*audioProcessor.apvts.getParameter ("output_gain"),   outGainRelay,      nullptr)
{
    setOpaque (true);
    addAndMakeVisible (webView);

    webView.goToURL (juce::WebBrowserComponent::getResourceProviderRoot());

    setSize (940, 470);
    startTimerHz (30);
}

VulcanEditor::~VulcanEditor()
{
    stopTimer();
}

std::optional<juce::WebBrowserComponent::Resource> VulcanEditor::handleResourceRequest (const juce::String& url)
{
    const auto cleanPath = url.upToFirstOccurrenceOf ("?", false, false);

    if (cleanPath == "/" || cleanPath.isEmpty() || cleanPath.endsWithIgnoreCase ("index.html"))
    {
        const auto* data = reinterpret_cast<const std::byte*> (BinaryData::index_html);
        std::vector<std::byte> bytes (data, data + BinaryData::index_htmlSize);
        return juce::WebBrowserComponent::Resource { std::move (bytes), "text/html; charset=utf-8" };
    }

    return std::nullopt;
}

void VulcanEditor::pushFullSyncToWeb()
{
    juce::Array<juce::var> presetList;
    for (int i = 0; i < audioProcessor.getNumPrograms(); ++i)
        presetList.add (audioProcessor.getProgramName (i));

    auto* obj = new juce::DynamicObject();
    obj->setProperty ("presets", presetList);
    obj->setProperty ("currentPreset", audioProcessor.getCurrentProgram());
    obj->setProperty ("presetModified", audioProcessor.isPresetModified());
    obj->setProperty ("sampleRate", audioProcessor.getSampleRate() > 0.0 ? audioProcessor.getSampleRate() : 44100.0);

    webView.emitEventIfBrowserIsVisible ("fullSync", juce::var (obj));
}

void VulcanEditor::timerCallback()
{
    auto* obj = new juce::DynamicObject();
    obj->setProperty ("inL", audioProcessor.inPeakL.load());
    obj->setProperty ("inR", audioProcessor.inPeakR.load());
    obj->setProperty ("outL", audioProcessor.peakL.load());
    obj->setProperty ("outR", audioProcessor.peakR.load());
    obj->setProperty ("sampleRate", audioProcessor.getSampleRate() > 0.0 ? audioProcessor.getSampleRate() : 44100.0);
    obj->setProperty ("currentPreset", audioProcessor.getCurrentProgram());
    obj->setProperty ("presetModified", audioProcessor.isPresetModified());

    webView.emitEventIfBrowserIsVisible ("telemetryFrame", juce::var (obj));
}

void VulcanEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff040911));
}

void VulcanEditor::resized()
{
    webView.setBounds (getLocalBounds());
}
