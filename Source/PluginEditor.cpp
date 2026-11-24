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
                   .withNativeFunction ("paramGesture", [this] (const juce::Array<juce::var>& args,
                                                                juce::WebBrowserComponent::NativeFunctionCompletion completion)
                   {
                       if (args.size() >= 2)
                           handleParamGestureFromWeb (args[0].toString(), static_cast<bool> (args[1]));
                       completion ({});
                   })
                   .withEventListener ("paramGesture", [this] (const juce::var& payload)
                   {
                       if (payload.isObject())
                           handleParamGestureFromWeb (payload.getProperty ("id", "").toString(),
                                                      static_cast<bool> (payload.getProperty ("active", false)));
                   })
                   .withNativeFunction ("setParam", [this] (const juce::Array<juce::var>& args,
                                                            juce::WebBrowserComponent::NativeFunctionCompletion completion)
                   {
                       if (args.size() >= 2)
                           handleSetParamFromWeb (args[0].toString(), static_cast<float> (args[1]));
                       completion ({});
                   })
                   .withEventListener ("setParam", [this] (const juce::var& payload)
                   {
                       if (payload.isObject())
                           handleSetParamFromWeb (payload.getProperty ("id", "").toString(),
                                                  static_cast<float> (payload.getProperty ("value", 0.0f)));
                   })
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
                   }))
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

void VulcanEditor::handleParamGestureFromWeb (const juce::String& paramId, bool isStarting)
{
    if (auto* param = audioProcessor.apvts.getParameter (paramId))
    {
        if (isStarting)
            param->beginChangeGesture();
        else
            param->endChangeGesture();
    }
}

void VulcanEditor::handleSetParamFromWeb (const juce::String& paramId, float normValue)
{
    if (auto* param = audioProcessor.apvts.getParameter (paramId))
        param->setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, normValue));
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

    lastSyncedPreset = audioProcessor.getCurrentProgram();
    lastSyncedModified = audioProcessor.isPresetModified();

    auto* obj = new juce::DynamicObject();
    obj->setProperty ("presets", presetList);
    obj->setProperty ("currentPreset", lastSyncedPreset);
    obj->setProperty ("presetModified", lastSyncedModified);
    obj->setProperty ("sampleRate", audioProcessor.getSampleRate() > 0.0 ? audioProcessor.getSampleRate() : 44100.0);
    obj->setProperty ("editorWidth", getWidth());
    obj->setProperty ("editorHeight", getHeight());

    for (const auto* id : kAllParamIDs)
    {
        if (auto* param = audioProcessor.apvts.getParameter (id))
            obj->setProperty (juce::String ("p_") + id, static_cast<double> (param->getValue()));
    }

    webView.emitEventIfBrowserIsVisible ("fullSync", juce::var (obj));
}

void VulcanEditor::timerCallback()
{
    const int curProg = audioProcessor.getCurrentProgram();
    const bool curMod = audioProcessor.isPresetModified();

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

    for (const auto* id : kAllParamIDs)
    {
        if (auto* param = audioProcessor.apvts.getParameter (id))
            obj->setProperty (juce::String ("p_") + id, static_cast<double> (param->getValue()));
    }

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
