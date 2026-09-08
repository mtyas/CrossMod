#include "PresetManager.h"
#include "FactoryPresets.h"
#include <algorithm>

PresetManager::PresetManager(juce::AudioProcessorValueTreeState& apvtsToUse)
    : apvts(apvtsToUse)
{
}

juce::File PresetManager::getPresetsDirectory() const
{
    return juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
        .getChildFile("mtyas")
        .getChildFile("CrossMod")
        .getChildFile("Presets");
}

void PresetManager::initialize()
{
    auto dir = getPresetsDirectory();
    if (!dir.exists())
    {
        dir.createDirectory();
    }

    createFactoryPresetsIfEmpty();
    rescanPresets();

    if (!presets.empty())
    {
        loadPreset(0);
    }
}

void PresetManager::createFactoryPresetsIfEmpty()
{
    auto dir = getPresetsDirectory();
    auto existing = dir.findChildFiles(juce::File::findFiles, false, "*.crossmod");

    if (existing.isEmpty())
    {
        restoreFactoryPresets();
    }
}

void PresetManager::restoreFactoryPresets()
{
    auto dir = getPresetsDirectory();
    if (!dir.exists())
        dir.createDirectory();

    auto factoryList = FactoryPresets::getPresets();
    for (size_t i = 0; i < factoryList.size(); ++i)
    {
        const auto& fp = factoryList[i];
        juce::String prefix = (i + 1 < 10) ? ("0" + juce::String(i + 1) + " - ") : (juce::String(i + 1) + " - ");
        juce::String fileName = prefix + fp.name + ".crossmod";
        juce::File file = dir.getChildFile(fileName);

        juce::ValueTree state(apvts.state.getType());
        for (const auto& [paramId, val] : fp.values)
        {
            juce::ValueTree p("PARAM");
            p.setProperty("id", paramId, nullptr);
            p.setProperty("value", val, nullptr);
            state.addChild(p, -1, nullptr);
        }

        // Fill remaining parameters with defaults
        for (auto* param : apvts.processor.getParameters())
        {
            if (auto* rp = dynamic_cast<juce::RangedAudioParameter*>(param))
            {
                juce::String id = rp->getParameterID();
                if (!state.getChildWithProperty("id", id).isValid())
                {
                    juce::ValueTree p("PARAM");
                    p.setProperty("id", id, nullptr);
                    p.setProperty("value", rp->getDefaultValue(), nullptr);
                    state.addChild(p, -1, nullptr);
                }
            }
        }

        std::unique_ptr<juce::XmlElement> xml(state.createXml());
        if (xml != nullptr)
        {
            xml->writeTo(file);
        }
    }
}

void PresetManager::rescanPresets()
{
    presets.clear();
    auto dir = getPresetsDirectory();
    if (!dir.exists())
        return;

    auto files = dir.findChildFiles(juce::File::findFiles, false, "*.crossmod;*.xml");

    std::sort(files.begin(), files.end(), [](const juce::File& a, const juce::File& b) {
        return a.getFileName().compareNatural(b.getFileName()) < 0;
    });

    for (const auto& f : files)
    {
        PresetInfo info;
        info.file = f;
        info.name = f.getFileNameWithoutExtension();
        presets.push_back(info);
    }

    // Restore selection if possible
    currentPresetIndex = 0;
    for (int i = 0; i < (int)presets.size(); ++i)
    {
        if (presets[i].name == currentPresetName)
        {
            currentPresetIndex = i;
            break;
        }
    }
}

juce::String PresetManager::getPresetName(int index) const
{
    if (index >= 0 && index < (int)presets.size())
        return presets[index].name;
    return {};
}

bool PresetManager::loadPreset(int index)
{
    if (index >= 0 && index < (int)presets.size())
    {
        return loadPresetFromFile(presets[index].file, false);
    }
    return false;
}

bool PresetManager::loadPresetFromFile(const juce::File& file, bool copyToPresetsFolder)
{
    if (!file.existsAsFile())
        return false;

    std::unique_ptr<juce::XmlElement> xml(juce::XmlDocument::parse(file));
    if (xml != nullptr && xml->hasTagName(apvts.state.getType()))
    {
        apvts.replaceState(juce::ValueTree::fromXml(*xml));
        currentPresetName = file.getFileNameWithoutExtension();

        if (copyToPresetsFolder)
        {
            auto dest = getPresetsDirectory().getChildFile(file.getFileName());
            if (file != dest)
                file.copyFileTo(dest);
        }

        rescanPresets();
        return true;
    }
    return false;
}

bool PresetManager::saveCurrentPreset(const juce::String& presetName)
{
    if (presetName.trim().isEmpty())
        return false;

    auto file = getPresetsDirectory().getChildFile(presetName.trim() + ".crossmod");
    return saveCurrentPresetToFile(file);
}

bool PresetManager::saveCurrentPresetToFile(const juce::File& file)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    if (xml != nullptr)
    {
        bool success = xml->writeTo(file);
        if (success)
        {
            currentPresetName = file.getFileNameWithoutExtension();
            rescanPresets();
            return true;
        }
    }
    return false;
}

void PresetManager::openPresetsFolderInExplorer()
{
    auto dir = getPresetsDirectory();
    if (!dir.exists())
        dir.createDirectory();

    dir.startAsProcess();
}
