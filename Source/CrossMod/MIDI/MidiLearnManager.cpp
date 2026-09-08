#include "MidiLearnManager.h"

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

    // Remove any existing mapping for this parameter or this CC
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

void MidiLearnManager::processMidiController(const juce::MidiMessage& msg, juce::AudioProcessorValueTreeState& apvts)
{
    if (!msg.isController())
        return;

    int cc = msg.getControllerNumber();
    int channel = msg.getChannel();
    float normalizedVal = msg.getControllerValue() / 127.0f;

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
                        return m.paramID == targetParam || m.cc == cc;
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
            if (auto* param = apvts.getParameter(targetParam))
            {
                param->setValueNotifyingHost(normalizedVal);
            }
            notifyChanged();
        }
        return;
    }

    // Normal controller processing: search mappings
    const juce::SpinLock::ScopedLockType sl(lock);
    for (const auto& m : mappings)
    {
        if (m.cc == cc && (m.channel == 0 || m.channel == channel))
        {
            if (auto* param = apvts.getParameter(m.paramID))
            {
                param->setValueNotifyingHost(normalizedVal);
            }
        }
    }
}

void MidiLearnManager::saveToXml(juce::XmlElement& xml) const
{
    const juce::SpinLock::ScopedLockType sl(lock);
    auto* mappingsElem = xml.createNewChildElement("MidiMappings");
    for (const auto& m : mappings)
    {
        auto* child = mappingsElem->createNewChildElement("Mapping");
        child->setAttribute("paramID", m.paramID);
        child->setAttribute("cc", m.cc);
        child->setAttribute("channel", m.channel);
    }
}

void MidiLearnManager::loadFromXml(const juce::XmlElement& xml)
{
    const auto* mappingsElem = xml.getChildByName("MidiMappings");
    if (mappingsElem == nullptr)
        return;

    const juce::SpinLock::ScopedLockType sl(lock);
    mappings.clear();

    for (auto* child : mappingsElem->getChildIterator())
    {
        if (child->hasTagName("Mapping"))
        {
            MidiMapping m;
            m.paramID = child->getStringAttribute("paramID");
            m.cc = child->getIntAttribute("cc", 0);
            m.channel = child->getIntAttribute("channel", 0);

            if (m.paramID.isNotEmpty() && m.cc >= 0 && m.cc <= 127)
            {
                mappings.push_back(m);
            }
        }
    }

    notifyChanged();
}

juce::File MidiLearnManager::getDefaultMappingsFile()
{
    return juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
        .getChildFile("mtyas")
        .getChildFile("CrossMod")
        .getChildFile("midi_mappings.xml");
}

void MidiLearnManager::saveDefaultMapToFile()
{
    auto file = getDefaultMappingsFile();
    file.getParentDirectory().createDirectory();

    juce::XmlElement xml("CrossModMidiSettings");
    saveToXml(xml);
    xml.writeTo(file);
}

void MidiLearnManager::loadDefaultMapFromFile()
{
    auto file = getDefaultMappingsFile();
    if (!file.existsAsFile())
        return;

    std::unique_ptr<juce::XmlElement> xml(juce::XmlDocument::parse(file));
    if (xml != nullptr)
    {
        loadFromXml(*xml);
    }
}

void MidiLearnManager::notifyChanged()
{
    if (onMappingsChanged)
    {
        // Safe dispatch to message thread if called from audio thread
        if (juce::MessageManager::getInstanceWithoutCreating() != nullptr)
        {
            juce::MessageManager::callAsync([this]() {
                if (onMappingsChanged)
                    onMappingsChanged();
            });
        }
    }
}
