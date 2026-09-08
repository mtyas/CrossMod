#include "MidiLearnManager.h"
#include "MidiChainProcessor.h"
#include <algorithm>

namespace MidiFlux
{

MidiLearnManager::MidiLearnManager()
{
}

void MidiLearnManager::startLearning(const juce::String& paramID)
{
    const juce::SpinLock::ScopedLockType sl(lock);
    learningTargetParam = paramID;
    learningActive.store(true, std::memory_order_release);
    notifyChanged();
}

void MidiLearnManager::cancelLearning()
{
    const juce::SpinLock::ScopedLockType sl(lock);
    learningTargetParam.clear();
    learningActive.store(false, std::memory_order_release);
    notifyChanged();
}

bool MidiLearnManager::isLearningParam(const juce::String& paramID) const
{
    if (!learningActive.load(std::memory_order_acquire))
        return false;

    const juce::SpinLock::ScopedLockType sl(lock);
    return learningTargetParam == paramID;
}

juce::String MidiLearnManager::getLearningParamID() const
{
    const juce::SpinLock::ScopedLockType sl(lock);
    return learningTargetParam;
}

void MidiLearnManager::setMapping(const juce::String& paramID, int cc, int channel)
{
    const juce::SpinLock::ScopedLockType sl(lock);

    mappings.erase(std::remove_if(mappings.begin(), mappings.end(),
        [&](const MidiMapping& m) {
            return m.paramID == paramID || (m.cc == cc && (m.channel == channel || m.channel == 0 || channel == 0));
        }), mappings.end());

    MidiMapping newMap;
    newMap.paramID = paramID;
    newMap.cc = juce::jlimit(0, 127, cc);
    newMap.channel = juce::jlimit(0, 16, channel);
    mappings.push_back(newMap);

    notifyChanged();
}

void MidiLearnManager::removeMappingForParam(const juce::String& paramID)
{
    const juce::SpinLock::ScopedLockType sl(lock);
    mappings.erase(std::remove_if(mappings.begin(), mappings.end(),
        [&](const MidiMapping& m) { return m.paramID == paramID; }), mappings.end());
    notifyChanged();
}

void MidiLearnManager::removeMappingForCC(int cc, int channel)
{
    const juce::SpinLock::ScopedLockType sl(lock);
    mappings.erase(std::remove_if(mappings.begin(), mappings.end(),
        [&](const MidiMapping& m) {
            return m.cc == cc && (m.channel == channel || channel == 0 || m.channel == 0);
        }), mappings.end());
    notifyChanged();
}

void MidiLearnManager::clearAllMappings()
{
    const juce::SpinLock::ScopedLockType sl(lock);
    mappings.clear();
    learningActive.store(false, std::memory_order_release);
    learningTargetParam.clear();
    notifyChanged();
}

bool MidiLearnManager::getMappingForParam(const juce::String& paramID, int& outCC, int& outChannel) const
{
    const juce::SpinLock::ScopedLockType sl(lock);
    for (const auto& m : mappings)
    {
        if (m.paramID == paramID)
        {
            outCC = m.cc;
            outChannel = m.channel;
            return true;
        }
    }
    return false;
}

std::vector<MidiMapping> MidiLearnManager::getAllMappings() const
{
    const juce::SpinLock::ScopedLockType sl(lock);
    return mappings;
}

void MidiLearnManager::remapBlockMoved(int fromIndex, int toIndex)
{
    if (fromIndex == toIndex)
        return;

    const juce::SpinLock::ScopedLockType sl(lock);
    for (auto& m : mappings)
    {
        if (m.paramID.startsWith("block:"))
        {
            auto rest = m.paramID.fromFirstOccurrenceOf("block:", false, false);
            int blockIdx = rest.upToFirstOccurrenceOf(":", false, false).getIntValue();
            auto suffix = rest.fromFirstOccurrenceOf(":", true, false);

            if (blockIdx == fromIndex)
            {
                m.paramID = "block:" + juce::String(toIndex) + suffix;
            }
            else if (fromIndex < toIndex && blockIdx > fromIndex && blockIdx <= toIndex)
            {
                m.paramID = "block:" + juce::String(blockIdx - 1) + suffix;
            }
            else if (fromIndex > toIndex && blockIdx >= toIndex && blockIdx < fromIndex)
            {
                m.paramID = "block:" + juce::String(blockIdx + 1) + suffix;
            }
        }
    }
    notifyChanged();
}

void MidiLearnManager::remapBlockRemoved(int index)
{
    const juce::SpinLock::ScopedLockType sl(lock);
    std::vector<MidiMapping> updated;
    for (const auto& m : mappings)
    {
        if (m.paramID.startsWith("block:"))
        {
            auto rest = m.paramID.fromFirstOccurrenceOf("block:", false, false);
            int blockIdx = rest.upToFirstOccurrenceOf(":", false, false).getIntValue();
            auto suffix = rest.fromFirstOccurrenceOf(":", true, false);

            if (blockIdx == index)
            {
                // Discard removed block mapping
                continue;
            }
            else if (blockIdx > index)
            {
                MidiMapping mCopy = m;
                mCopy.paramID = "block:" + juce::String(blockIdx - 1) + suffix;
                updated.push_back(mCopy);
            }
            else
            {
                updated.push_back(m);
            }
        }
        else
        {
            updated.push_back(m);
        }
    }
    mappings = std::move(updated);
    notifyChanged();
}

static void applyParameterChange(const juce::String& paramID, int ccVal, MidiChainProcessor& chain)
{
    float normVal = ccVal / 127.0f;

    if (paramID.startsWith("block:"))
    {
        auto rest = paramID.fromFirstOccurrenceOf("block:", false, false);
        int blockIdx = rest.upToFirstOccurrenceOf(":", false, false).getIntValue();
        auto sub = rest.fromFirstOccurrenceOf(":", false, false);

        auto* block = chain.getBlock(blockIdx);
        if (!block)
            return;

        if (sub == "power")
        {
            block->setBypassed(ccVal < 64);
        }
        else if (sub == "routing")
        {
            block->setRoutingMode(ccVal >= 64 ? RoutingMode::Parallel : RoutingMode::Series);
        }
        else if (sub.startsWith("param:"))
        {
            int pIdx = sub.fromFirstOccurrenceOf("param:", false, false).getIntValue();
            if (pIdx >= 0 && pIdx < block->getNumParameters())
            {
                const auto& def = block->getParameterDef(pIdx);
                float val = def.minValue + normVal * (def.maxValue - def.minValue);
                if (def.step > 0.0f)
                    val = def.minValue + std::round((val - def.minValue) / def.step) * def.step;
                block->setParameterValue(pIdx, val);
            }
        }
    }
    else if (paramID == "global:rootKey")
    {
        int key = juce::jlimit(0, 11, static_cast<int>(normVal * 12.0f));
        chain.setGlobalRootKey(key);
    }
    else if (paramID == "global:scaleType")
    {
        int scale = juce::jlimit(0, static_cast<int>(NumScales) - 1, static_cast<int>(normVal * static_cast<float>(NumScales)));
        chain.setGlobalScaleType(scale);
    }
    else if (paramID == "global:scaleSeqEnable")
    {
        chain.getScaleProgression().setEnabled(ccVal >= 64);
    }
}

void MidiLearnManager::processMidiController(const juce::MidiMessage& msg, MidiChainProcessor& chain)
{
    if (!msg.isController())
        return;

    int cc = msg.getControllerNumber();
    int channel = msg.getChannel();
    int ccVal = msg.getControllerValue();

    // Check if learning mode is active
    if (learningActive.load(std::memory_order_acquire))
    {
        juce::String targetParam;
        {
            const juce::SpinLock::ScopedLockType sl(lock);
            targetParam = learningTargetParam;
            learningTargetParam.clear();
            learningActive.store(false, std::memory_order_release);

            if (targetParam.isNotEmpty())
            {
                // Remove old mappings for this param or CC
                mappings.erase(std::remove_if(mappings.begin(), mappings.end(),
                    [&](const MidiMapping& m) {
                        return m.paramID == targetParam || (m.cc == cc && (m.channel == channel || m.channel == 0 || channel == 0));
                    }), mappings.end());

                MidiMapping newMap;
                newMap.paramID = targetParam;
                newMap.cc = cc;
                newMap.channel = 0; // Omni by default for learned CC
                mappings.push_back(newMap);
            }
        }

        if (targetParam.isNotEmpty())
        {
            applyParameterChange(targetParam, ccVal, chain);
            notifyChanged();
        }
        return;
    }

    // Normal controller processing
    std::vector<juce::String> matchingParams;
    {
        const juce::SpinLock::ScopedLockType sl(lock);
        for (const auto& m : mappings)
        {
            if (m.cc == cc && (m.channel == 0 || m.channel == channel))
            {
                matchingParams.push_back(m.paramID);
            }
        }
    }

    for (const auto& pid : matchingParams)
    {
        applyParameterChange(pid, ccVal, chain);
    }

    if (!matchingParams.empty())
        notifyChanged();
}

juce::ValueTree MidiLearnManager::getState() const
{
    const juce::SpinLock::ScopedLockType sl(lock);
    juce::ValueTree tree("MidiMappings");
    for (const auto& m : mappings)
    {
        juce::ValueTree child("Mapping");
        child.setProperty("paramID", m.paramID, nullptr);
        child.setProperty("cc", m.cc, nullptr);
        child.setProperty("channel", m.channel, nullptr);
        tree.addChild(child, -1, nullptr);
    }
    return tree;
}

void MidiLearnManager::setState(const juce::ValueTree& vt)
{
    if (!vt.isValid())
        return;

    juce::ValueTree root = vt;
    if (root.getType() != juce::Identifier("MidiMappings"))
    {
        root = vt.getChildWithName("MidiMappings");
        if (!root.isValid())
            return;
    }

    const juce::SpinLock::ScopedLockType sl(lock);
    mappings.clear();

    for (int i = 0; i < root.getNumChildren(); ++i)
    {
        auto child = root.getChild(i);
        if (child.getType() == juce::Identifier("Mapping"))
        {
            MidiMapping m;
            m.paramID = child.getProperty("paramID", "").toString();
            m.cc = static_cast<int>(child.getProperty("cc", 0));
            m.channel = static_cast<int>(child.getProperty("channel", 0));

            if (m.paramID.isNotEmpty() && m.cc >= 0 && m.cc <= 127)
                mappings.push_back(m);
        }
    }

    notifyChanged();
}

juce::File MidiLearnManager::getDefaultMappingsFile()
{
    return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
        .getChildFile("mtyas")
        .getChildFile("MidiFlux")
        .getChildFile("midi_mappings.xml");
}

void MidiLearnManager::saveDefaultMapToFile()
{
    auto file = getDefaultMappingsFile();
    file.getParentDirectory().createDirectory();

    auto vt = getState();
    std::unique_ptr<juce::XmlElement> xml(vt.createXml());
    if (xml != nullptr)
        xml->writeTo(file);
}

void MidiLearnManager::loadDefaultMapFromFile()
{
    auto file = getDefaultMappingsFile();
    if (!file.existsAsFile())
        return;

    std::unique_ptr<juce::XmlElement> xml = juce::parseXML(file);
    if (xml != nullptr)
    {
        auto vt = juce::ValueTree::fromXml(*xml);
        if (vt.isValid())
            setState(vt);
    }
}

void MidiLearnManager::notifyChanged()
{
    if (onMappingsChanged)
    {
        if (juce::MessageManager::getInstanceWithoutCreating() != nullptr)
        {
            juce::MessageManager::callAsync([this]() {
                if (onMappingsChanged)
                    onMappingsChanged();
            });
        }
    }
}

} // namespace MidiFlux
