#include "HostChain.h"

HostChain::HostChain() : juce::Thread ("RackAB-Scan")
{
    formatManager.addFormat (new juce::VST3PluginFormat());
    loadKnownList();
}

HostChain::~HostChain()
{
    stopThread (4000);
}

//==============================================================================
// Prepare / process
//==============================================================================
void HostChain::prepare (double sampleRate, int blockSize)
{
    const juce::ScopedLock sl (lock);
    currentSampleRate = sampleRate;
    currentBlockSize  = blockSize;
    dryBuffer.setSize (2, blockSize, false, false, true);
    for (auto* n : nodes)
        prepareInstance (n->plugin.get());
}

void HostChain::releaseResources()
{
    const juce::ScopedLock sl (lock);
    for (auto* n : nodes)
        n->plugin->releaseResources();
}

void HostChain::prepareInstance (juce::AudioPluginInstance* p)
{
    p->setPlayConfigDetails (2, 2, currentSampleRate, currentBlockSize);
    p->prepareToPlay (currentSampleRate, currentBlockSize);
}

void HostChain::process (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    const juce::ScopedLock sl (lock);

    if (bypassAll.load())
        return;

    const float wet = dryWet.load();
    const bool blend = wet < 0.999f;
    if (blend)
    {
        dryBuffer.makeCopyOf (buffer, true);
    }

    const bool cmp = compareMode.load();
    const int  sel = selectedIndex.load();

    for (int i = 0; i < nodes.size(); ++i)
    {
        auto* n = nodes.getUnchecked (i);

        // Bypass rule: in compare mode only the selected slot is active
        // (fair A/B on the dry input); otherwise a manually-muted slot bypasses.
        const bool bypassed = cmp ? (i != sel) : n->muted;
        if (bypassed)
            continue;

        n->plugin->processBlock (buffer, midi);
    }

    if (blend)
    {
        const int ch = buffer.getNumChannels();
        const int ns = buffer.getNumSamples();
        for (int c = 0; c < ch; ++c)
        {
            auto* out = buffer.getWritePointer (c);
            const auto* dry = dryBuffer.getReadPointer (juce::jmin (c, dryBuffer.getNumChannels() - 1));
            for (int s = 0; s < ns; ++s)
                out[s] = dry[s] * (1.0f - wet) + out[s] * wet;
        }
    }
}

//==============================================================================
// Scanning
//==============================================================================
juce::File HostChain::getKnownListFile() const
{
    return juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
             .getChildFile ("RackAB").getChildFile ("known_plugins.xml");
}

void HostChain::loadKnownList()
{
    if (auto xml = juce::XmlDocument::parse (getKnownListFile()))
        knownList.recreateFromXml (*xml);
}

void HostChain::saveKnownList()
{
    auto f = getKnownListFile();
    f.getParentDirectory().createDirectory();
    if (auto xml = std::unique_ptr<juce::XmlElement> (knownList.createXml()))
        xml->writeTo (f);
}

void HostChain::startScan()
{
    if (! isThreadRunning())
        startThread();
}

void HostChain::run()
{
    juce::VST3PluginFormat vst3;
    auto paths = vst3.getDefaultLocationsToSearch();
    auto deadMans = getKnownListFile().getSiblingFile ("scan_crash.tmp");

    juce::PluginDirectoryScanner scanner (knownList, vst3, paths, true, deadMans);
    juce::String nameBeingScanned;
    while (! threadShouldExit() && scanner.scanNextFile (true, nameBeingScanned)) {}

    juce::MessageManager::callAsync ([this]
    {
        saveKnownList();
        if (onScanFinished) onScanFinished();
    });
}

//==============================================================================
// Chain editing
//==============================================================================
juce::String HostChain::addPluginFromDescription (const juce::PluginDescription& desc)
{
    juce::String err;
    std::unique_ptr<juce::AudioPluginInstance> inst (
        formatManager.createPluginInstance (desc, currentSampleRate, currentBlockSize, err));

    if (inst == nullptr)
        return "No se pudo instanciar el plugin:\n" + err;

    auto node = std::make_unique<Node>();
    {
        const juce::ScopedLock sl (lock);
        prepareInstance (inst.get());
        node->plugin = std::move (inst);
        nodes.add (node.release());
    }
    return {};
}

juce::String HostChain::addPluginFromFile (const juce::File& vst3File)
{
    juce::OwnedArray<juce::PluginDescription> found;
    juce::VST3PluginFormat vst3;
    juce::KnownPluginList tmp;
    tmp.scanAndAddFile (vst3File.getFullPathName(), true, found, vst3);
    if (found.isEmpty())
        return "No se encontro ningun VST3 en:\n" + vst3File.getFullPathName();
    return addPluginFromDescription (*found.getFirst());
}

void HostChain::removeAt (int index)
{
    const juce::ScopedLock sl (lock);
    if (juce::isPositiveAndBelow (index, nodes.size()))
    {
        nodes.getUnchecked (index)->plugin->releaseResources();
        nodes.remove (index);
    }
    if (selectedIndex.load() >= nodes.size())
        selectedIndex = juce::jmax (0, nodes.size() - 1);
}

void HostChain::move (int fromIndex, int toIndex)
{
    const juce::ScopedLock sl (lock);
    if (juce::isPositiveAndBelow (fromIndex, nodes.size())
        && juce::isPositiveAndBelow (toIndex, nodes.size()))
        nodes.move (fromIndex, toIndex);
}

void HostChain::clear()
{
    const juce::ScopedLock sl (lock);
    for (auto* n : nodes)
        n->plugin->releaseResources();
    nodes.clear();
    selectedIndex = 0;
}

//==============================================================================
// Mute
//==============================================================================
void HostChain::setMuted (int index, bool shouldBeMuted)
{
    const juce::ScopedLock sl (lock);
    if (juce::isPositiveAndBelow (index, nodes.size()))
        nodes.getUnchecked (index)->muted = shouldBeMuted;
}

bool HostChain::isMuted (int index) const
{
    const juce::ScopedLock sl (lock);
    if (juce::isPositiveAndBelow (index, nodes.size()))
        return nodes.getUnchecked (index)->muted;
    return false;
}

void HostChain::toggleMuted (int index)
{
    const juce::ScopedLock sl (lock);
    if (juce::isPositiveAndBelow (index, nodes.size()))
    {
        auto* n = nodes.getUnchecked (index);
        n->muted = ! n->muted;
    }
}

//==============================================================================
// Compare / selection
//==============================================================================
void HostChain::setCompare (bool shouldBeOn) { compareMode = shouldBeOn; }

void HostChain::setSelected (int index)
{
    const int n = getNumPlugins();
    if (n > 0) selectedIndex = juce::jlimit (0, n - 1, index);
}

void HostChain::moveSelection (int delta)
{
    const int n = getNumPlugins();
    if (n > 0) selectedIndex = juce::jlimit (0, n - 1, selectedIndex.load() + delta);
}

int HostChain::getNumPlugins() const
{
    const juce::ScopedLock sl (lock);
    return nodes.size();
}

juce::String HostChain::getPluginName (int index) const
{
    const juce::ScopedLock sl (lock);
    if (juce::isPositiveAndBelow (index, nodes.size()))
        return nodes.getUnchecked (index)->plugin->getName();
    return {};
}

juce::AudioProcessorEditor* HostChain::createEditorFor (int index)
{
    const juce::ScopedLock sl (lock);
    if (juce::isPositiveAndBelow (index, nodes.size()))
    {
        auto* p = nodes.getUnchecked (index)->plugin.get();
        if (p->hasEditor())
            return p->createEditorIfNeeded();
    }
    return nullptr;
}

//==============================================================================
// State
//==============================================================================
void HostChain::getState (juce::MemoryBlock& dest)
{
    const juce::ScopedLock sl (lock);
    juce::MemoryOutputStream out (dest, false);
    out.writeInt (nodes.size());
    for (auto* n : nodes)
    {
        auto desc = n->plugin->getPluginDescription();
        out.writeString (desc.fileOrIdentifier);
        out.writeBool (n->muted);
        juce::MemoryBlock ps;
        n->plugin->getStateInformation (ps);
        out.writeInt64 ((juce::int64) ps.getSize());
        out.write (ps.getData(), ps.getSize());
    }
}

void HostChain::setState (const void* data, int size)
{
    clear();
    juce::MemoryInputStream in (data, (size_t) size, false);
    const int n = in.readInt();
    for (int i = 0; i < n; ++i)
    {
        const juce::String path = in.readString();
        const bool muted = in.readBool();
        const auto stateSize = (size_t) in.readInt64();
        juce::MemoryBlock ps (stateSize);
        in.read (ps.getData(), (int) stateSize);

        if (addPluginFromFile (juce::File (path)).isEmpty())
        {
            const juce::ScopedLock sl (lock);
            if (auto* node = nodes.getLast())
            {
                node->muted = muted;
                node->plugin->setStateInformation (ps.getData(), (int) ps.getSize());
            }
        }
    }
}
