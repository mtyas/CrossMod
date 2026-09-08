#include "PresetManager.h"
#include "../Parameters/ParameterIDs.h"

PresetManager::PresetManager(juce::AudioProcessorValueTreeState& state)
    : apvts(state)
{
    refreshPresetList();
}

juce::File PresetManager::getPresetsDirectory() const
{
    auto dir = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
                   .getChildFile("mtyas")
                   .getChildFile("OmniSplit")
                   .getChildFile("Presets");
    if (!dir.exists())
        dir.createDirectory();
    return dir;
}

void PresetManager::refreshPresetList()
{
    presetList.clear();

    // 1. Load Factory Presets
    auto factory = FactoryPresets::getPresets();
    for (const auto& fp : factory)
    {
        PresetItem item;
        item.name = fp.name;
        item.category = fp.category;
        item.isFactory = true;
        item.paramValues = fp.paramValues;
        presetList.push_back(item);
    }

    // 2. Scan User Presets Directory
    auto dir = getPresetsDirectory();
    auto files = dir.findChildFiles(juce::File::findFiles, false, "*.xml");
    for (const auto& file : files)
    {
        std::unique_ptr<juce::XmlElement> xml(juce::XmlDocument::parse(file));
        if (xml != nullptr)
        {
            PresetItem item;
            item.name = xml->getStringAttribute("presetName", file.getFileNameWithoutExtension());
            item.category = xml->getStringAttribute("category", "User");
            item.isFactory = false;
            item.userFile = file;
            presetList.push_back(item);
        }
    }
}

juce::String PresetManager::getPresetName(int index) const
{
    if (index >= 0 && index < static_cast<int>(presetList.size()))
        return presetList[index].name;
    return {};
}

juce::String PresetManager::getPresetCategory(int index) const
{
    if (index >= 0 && index < static_cast<int>(presetList.size()))
        return presetList[index].category;
    return {};
}

bool PresetManager::isPresetFactory(int index) const
{
    if (index >= 0 && index < static_cast<int>(presetList.size()))
        return presetList[index].isFactory;
    return false;
}

void PresetManager::loadPreset(int index)
{
    if (index < 0 || index >= static_cast<int>(presetList.size()))
        return;

    const auto& item = presetList[index];

    if (item.isFactory)
    {
        // Reset all 8 slots across both chains to clean default state first
        for (int i = 0; i < 4; ++i)
        {
            const float tBypassVal = (i > 0) ? 1.0f : 0.0f; // Slot 1 is ON (0.0), Slots 2..4 are OFF (1.0)
            if (auto* p = apvts.getParameter(IDs::getTransSlotSrc(i).getParamID()))
                p->setValueNotifyingHost(p->convertTo0to1(0.0f)); // Stereo In
            if (auto* p = apvts.getParameter(IDs::getTransSlotType(i).getParamID()))
                p->setValueNotifyingHost(p->convertTo0to1(0.0f)); // Bypass
            if (auto* p = apvts.getParameter(IDs::getTransSlotBypass(i).getParamID()))
                p->setValueNotifyingHost(p->convertTo0to1(tBypassVal));
            if (auto* p = apvts.getParameter(IDs::getTransSlotInGain(i).getParamID()))
                p->setValueNotifyingHost(p->convertTo0to1(0.0f)); // 0.0 dB
            if (auto* p = apvts.getParameter(IDs::getTransSlotOutGain(i).getParamID()))
                p->setValueNotifyingHost(p->convertTo0to1(0.0f)); // 0.0 dB
            if (auto* p = apvts.getParameter(IDs::getTransSlotDest(i).getParamID()))
                p->setValueNotifyingHost(p->convertTo0to1(0.0f)); // Main Out

            if (auto* p = apvts.getParameter(IDs::getSustSlotSrc(i).getParamID()))
                p->setValueNotifyingHost(p->convertTo0to1(0.0f)); // Stereo In
            if (auto* p = apvts.getParameter(IDs::getSustSlotType(i).getParamID()))
                p->setValueNotifyingHost(p->convertTo0to1(0.0f)); // Bypass
            if (auto* p = apvts.getParameter(IDs::getSustSlotBypass(i).getParamID()))
                p->setValueNotifyingHost(p->convertTo0to1(1.0f)); // Slots 5..8 OFF (1.0)
            if (auto* p = apvts.getParameter(IDs::getSustSlotInGain(i).getParamID()))
                p->setValueNotifyingHost(p->convertTo0to1(0.0f)); // 0.0 dB
            if (auto* p = apvts.getParameter(IDs::getSustSlotOutGain(i).getParamID()))
                p->setValueNotifyingHost(p->convertTo0to1(0.0f)); // 0.0 dB
            if (auto* p = apvts.getParameter(IDs::getSustSlotDest(i).getParamID()))
                p->setValueNotifyingHost(p->convertTo0to1(0.0f)); // Main Out
        }

        for (const auto& [id, val] : item.paramValues)
        {
            if (auto* param = apvts.getParameter(id))
            {
                const float normVal = param->convertTo0to1(val);
                param->setValueNotifyingHost(normVal);
            }
        }
    }
    else
    {
        loadPresetFromFile(item.userFile);
    }
}

bool PresetManager::saveCurrentPresetAs(const juce::String& presetName, const juce::String& category)
{
    if (presetName.trim().isEmpty())
        return false;

    auto dir = getPresetsDirectory();
    auto file = dir.getChildFile(presetName.trim() + ".xml");

    std::unique_ptr<juce::XmlElement> root(new juce::XmlElement("TransientSplitPreset"));
    root->setAttribute("presetName", presetName.trim());
    root->setAttribute("category", category);

    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> stateXml(state.createXml());
    if (stateXml != nullptr)
    {
        root->addChildElement(stateXml.release());
        const bool ok = root->writeTo(file);
        if (ok)
            refreshPresetList();
        return ok;
    }

    return false;
}

bool PresetManager::overwritePreset(int index)
{
    if (index < 0 || index >= static_cast<int>(presetList.size()))
        return false;

    const auto& item = presetList[index];
    juce::File targetFile = item.userFile;

    if (item.isFactory || !targetFile.existsAsFile())
    {
        auto dir = getPresetsDirectory();
        targetFile = dir.getChildFile(item.name + ".xml");
    }

    return savePresetToFile(targetFile);
}

bool PresetManager::deletePreset(int index)
{
    if (index < 0 || index >= static_cast<int>(presetList.size()))
        return false;

    const auto& item = presetList[index];
    if (item.isFactory)
        return false; // Can't delete built-in factory preset

    if (item.userFile.existsAsFile())
    {
        item.userFile.deleteFile();
        refreshPresetList();
        return true;
    }

    return false;
}

bool PresetManager::savePresetToFile(const juce::File& file)
{
    std::unique_ptr<juce::XmlElement> root(new juce::XmlElement("TransientSplitPreset"));
    root->setAttribute("presetName", file.getFileNameWithoutExtension());
    root->setAttribute("category", "User");

    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> stateXml(state.createXml());
    if (stateXml != nullptr)
    {
        root->addChildElement(stateXml.release());
        const bool ok = root->writeTo(file);
        if (ok)
            refreshPresetList();
        return ok;
    }
    return false;
}

bool PresetManager::loadPresetFromFile(const juce::File& file)
{
    if (!file.existsAsFile())
        return false;

    std::unique_ptr<juce::XmlElement> root(juce::XmlDocument::parse(file));
    if (root == nullptr)
        return false;

    juce::XmlElement* paramsElement = nullptr;
    if (root->hasTagName(apvts.state.getType()))
    {
        paramsElement = root.get();
    }
    else if (root->hasTagName("TransientSplitPreset"))
    {
        paramsElement = root->getChildByName(apvts.state.getType().toString());
    }

    if (paramsElement != nullptr)
    {
        apvts.replaceState(juce::ValueTree::fromXml(*paramsElement));
        return true;
    }

    return false;
}
