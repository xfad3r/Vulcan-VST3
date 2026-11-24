#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "BinaryData.h"
#include <array>

namespace
{
    constexpr std::array<const char*, 9> kAllParamIDs = {
        "input_gain", "eq_low", "eq_mid", "eq_high",
        "mono_freq", "haas_amount", "amount", "limiter_drive", "output_gain"
    };

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
                   .withNativeFunction ("selectPreset", [this] (const juce::Array<juce::var>& args,
                                                                juce::WebBrowserComponent::NativeFunctionCompletion completion)
                   {
                       const int idx = args.size() > 0 ? static_cast<int> (args[0]) : -1;
                       if (idx >= 0 && idx < audioProcessor.getNumPrograms())
                       {
                           audioProcessor.setCurrentProgram (idx);
                           pushFullSyncToWeb();
                       }
                       completion ({});
                   })
                   .withEventListener ("selectPreset", [this] (const juce::var& payload)
                   {
                       const int idx = payload.isObject()
                                           ? static_cast<int> (payload.getProperty ("index", -1))
                                           : static_cast<int> (payload);
                       if (idx >= 0 && idx < audioProcessor.getNumPrograms())
                       {
                           audioProcessor.setCurrentProgram (idx);
                           pushFullSyncToWeb();
                       }
                   })
                   .withNativeFunction ("setEditorSize", [this] (const juce::Array<juce::var>& args,
                                                                 juce::WebBrowserComponent::NativeFunctionCompletion completion)
                   {
                       const int w = juce::jlimit (640, 1880, args.size() > 0 ? static_cast<int> (args[0]) : 940);
                       const int h = juce::jlimit (320, 940,  args.size() > 1 ? static_cast<int> (args[1]) : 470);
                       if (w != getWidth() || h != getHeight())
                           setSize (w, h);
                       completion ({});
                   })
                   .withEventListener ("setEditorSize", [this] (const juce::var& payload)
                   {
                       const int w = juce::jlimit (640, 1880, static_cast<int> (payload.getProperty ("width", 940)));
                       const int h = juce::jlimit (320, 940,  static_cast<int> (payload.getProperty ("height", 470)));
                       if (w != getWidth() || h != getHeight())
                           setSize (w, h);
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

    // Enable smooth DAW & UI resizing with locked 2:1 aspect ratio
    setResizable (true, false);
    setResizeLimits (640, 320, 1880, 940);
    if (auto* constrainer = getConstrainer())
        constrainer->setFixedAspectRatio (2.0);

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

    auto* paramValues = new juce::DynamicObject();
    for (const auto* id : kAllParamIDs)
    {
        if (auto* param = audioProcessor.apvts.getParameter (id))
            paramValues->setProperty (id, static_cast<double> (param->getValue()));
    }

    lastSyncedPreset = audioProcessor.getCurrentProgram();
    lastSyncedModified = audioProcessor.isPresetModified();

    auto* obj = new juce::DynamicObject();
    obj->setProperty ("presets", presetList);
    obj->setProperty ("currentPreset", lastSyncedPreset);
    obj->setProperty ("presetModified", lastSyncedModified);
    obj->setProperty ("paramValues", juce::var (paramValues));
    obj->setProperty ("sampleRate", audioProcessor.getSampleRate() > 0.0 ? audioProcessor.getSampleRate() : 44100.0);
    obj->setProperty ("editorWidth", getWidth());
    obj->setProperty ("editorHeight", getHeight());

    webView.emitEventIfBrowserIsVisible ("fullSync", juce::var (obj));
}

void VulcanEditor::timerCallback()
{
    const int curProg = audioProcessor.getCurrentProgram();
    const bool curMod = audioProcessor.isPresetModified();

    // If host (e.g. FL Studio wrapper arrows) changed preset externally, push full parameter sync
    if (curProg != lastSyncedPreset || curMod != lastSyncedModified)
        pushFullSyncToWeb();

    auto* obj = new juce::DynamicObject();
    obj->setProperty ("inL", audioProcessor.inPeakL.load());
    obj->setProperty ("inR", audioProcessor.inPeakR.load());
    obj->setProperty ("outL", audioProcessor.peakL.load());
    obj->setProperty ("outR", audioProcessor.peakR.load());
    obj->setProperty ("sampleRate", audioProcessor.getSampleRate() > 0.0 ? audioProcessor.getSampleRate() : 44100.0);
    obj->setProperty ("currentPreset", curProg);
    obj->setProperty ("presetModified", curMod);

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
