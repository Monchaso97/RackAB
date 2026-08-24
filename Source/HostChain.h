#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>

// Manages a serial chain of hosted VST3 plugin instances plus a scanned list of
// the plugins installed on the system (so the UI can offer them like a DAW does).
// Thread-safe: the audio thread reads under a lock; the message thread mutates.
class HostChain : private juce::Thread
{
public:
    HostChain();
    ~HostChain() override;

    void prepare (double sampleRate, int blockSize);
    void releaseResources();
    void process (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi);

    // --- Installed-plugin scanning ---
    void startScan();                                   // background scan of default VST3 folders
    bool isScanning() const noexcept { return isThreadRunning(); }
    juce::KnownPluginList& getKnownList() { return knownList; }
    std::function<void()> onScanFinished;               // called on message thread

    // --- Chain editing (message thread) ---
    juce::String addPluginFromDescription (const juce::PluginDescription& desc); // "" ok, else error
    juce::String addPluginFromFile (const juce::File& vst3File);
    void removeAt (int index);
    void move (int fromIndex, int toIndex);
    void clear();

    // --- Per-slot mute ---
    void setMuted (int index, bool shouldBeMuted);
    bool isMuted (int index) const;
    void toggleMuted (int index);

    // --- Compare (automute) / selection ---
    void setCompare (bool shouldBeOn);
    bool isCompare() const noexcept { return compareMode; }
    void setSelected (int index);
    int  getSelected() const noexcept { return selectedIndex; }
    void moveSelection (int delta);

    void setBypassAll (bool b) { bypassAll = b; }
    bool isBypassAll() const noexcept { return bypassAll; }

    void setDryWet (float w) { dryWet = juce::jlimit (0.0f, 1.0f, w); }
    float getDryWet() const noexcept { return dryWet; }

    // --- Queries ---
    int getNumPlugins() const;
    juce::String getPluginName (int index) const;
    juce::AudioProcessorEditor* createEditorFor (int index);

    // --- State ---
    void getState (juce::MemoryBlock& dest);
    void setState (const void* data, int size);

private:
    struct Node
    {
        std::unique_ptr<juce::AudioPluginInstance> plugin;
        bool muted = false;
    };

    void run() override;                                // scan thread
    void prepareInstance (juce::AudioPluginInstance* p);
    juce::File getKnownListFile() const;
    void loadKnownList();
    void saveKnownList();

    juce::AudioPluginFormatManager formatManager;
    juce::KnownPluginList knownList;
    juce::OwnedArray<Node> nodes;
    juce::CriticalSection lock;

    double currentSampleRate = 44100.0;
    int    currentBlockSize  = 512;
    juce::AudioBuffer<float> dryBuffer;

    std::atomic<bool> compareMode { false };
    std::atomic<bool> bypassAll   { false };
    std::atomic<int>  selectedIndex { 0 };
    std::atomic<float> dryWet { 1.0f };
};
