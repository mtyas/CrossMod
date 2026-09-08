#pragma once

#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>
#include <vector>
#include <cmath>
#include <algorithm>
#include "ScaleTheory.h"

namespace MidiFlux
{

struct ScaleSeqBlock
{
    int rootKey = 0;           // 0 = C, 1 = C#, ... 11 = B
    int scaleType = 0;         // ScaleType enum
    float bars = 1.0f;         // 0.5 (1/2 bar), 1.0, 2.0, 3.0, 4.0, 6.0, 8.0, 12.0, 16.0
    juce::String label = "";   // Optional display label
};

class ScaleProgression
{
public:
    struct PlaybackState
    {
        int activeBlockIndex = -1;
        float blockProgress = 0.0f; // 0.0 to 1.0 within the active block
        int activeRootKey = 0;
        int activeScaleType = 0;
        double currentPpqInSequence = 0.0;
        double totalPpq = 0.0;
        double currentBar = 0.0;
        bool isPlaying = false;
    };

    ScaleProgression()
    {
        loadDefaultProgression();
    }

    void loadDefaultProgression()
    {
        blocks.clear();
        // Default: classic 4-chord progression, 2 bars each (8 bars total)
        blocks.push_back({ 0, Scale_Major,        2.0f, "I - C Maj" });
        blocks.push_back({ 7, Scale_Major,        2.0f, "V - G Maj" });
        blocks.push_back({ 9, Scale_NaturalMinor, 2.0f, "vi - A Min" });
        blocks.push_back({ 5, Scale_Major,        2.0f, "IV - F Maj" });
        enabled = false;
        loop = true;
    }

    bool isEnabled() const { return enabled; }
    void setEnabled(bool e) { enabled = e; }

    bool isLoop() const { return loop; }
    void setLoop(bool l) { loop = l; }

    int getNumBlocks() const { return static_cast<int>(blocks.size()); }

    ScaleSeqBlock getBlock(int index) const
    {
        if (index >= 0 && index < static_cast<int>(blocks.size()))
            return blocks[index];
        return { 0, 0, 1.0f, "" };
    }

    void setBlock(int index, const ScaleSeqBlock& b)
    {
        if (index >= 0 && index < static_cast<int>(blocks.size()))
            blocks[index] = b;
    }

    void addBlock(const ScaleSeqBlock& b)
    {
        blocks.push_back(b);
    }

    void insertBlock(int index, const ScaleSeqBlock& b)
    {
        if (index >= 0 && index <= static_cast<int>(blocks.size()))
            blocks.insert(blocks.begin() + index, b);
        else
            blocks.push_back(b);
    }

    void removeBlock(int index)
    {
        if (index >= 0 && index < static_cast<int>(blocks.size()) && blocks.size() > 1)
            blocks.erase(blocks.begin() + index);
    }

    void duplicateBlock(int index)
    {
        if (index >= 0 && index < static_cast<int>(blocks.size()))
        {
            auto b = blocks[index];
            blocks.insert(blocks.begin() + index + 1, b);
        }
    }

    void moveBlock(int fromIndex, int toIndex)
    {
        if (fromIndex >= 0 && fromIndex < static_cast<int>(blocks.size())
            && toIndex >= 0 && toIndex < static_cast<int>(blocks.size())
            && fromIndex != toIndex)
        {
            auto b = blocks[fromIndex];
            blocks.erase(blocks.begin() + fromIndex);
            blocks.insert(blocks.begin() + toIndex, b);
        }
    }

    void clear()
    {
        blocks.clear();
        blocks.push_back({ 0, Scale_Major, 2.0f, "C Maj" });
    }

    float getTotalBars() const
    {
        float total = 0.0f;
        for (const auto& b : blocks)
            total += b.bars;
        return total;
    }

    PlaybackState getPlaybackState(double ppqPosition, int timeSigNumerator, int timeSigDenominator) const
    {
        PlaybackState state;
        if (blocks.empty())
            return state;

        int num = (timeSigNumerator > 0) ? timeSigNumerator : 4;
        int den = (timeSigDenominator > 0) ? timeSigDenominator : 4;
        double beatsPerBar = static_cast<double>(num) * (4.0 / static_cast<double>(den));

        double totalPpq = 0.0;
        for (const auto& b : blocks)
            totalPpq += static_cast<double>(b.bars) * beatsPerBar;

        state.totalPpq = totalPpq;
        if (totalPpq <= 0.0001)
        {
            state.activeBlockIndex = 0;
            state.activeRootKey = blocks[0].rootKey;
            state.activeScaleType = blocks[0].scaleType;
            return state;
        }

        double pos = ppqPosition;
        if (loop)
        {
            pos = std::fmod(pos, totalPpq);
            if (pos < 0.0)
                pos += totalPpq;
        }
        else
        {
            pos = juce::jlimit(0.0, totalPpq - 0.0001, pos);
        }

        state.currentPpqInSequence = pos;
        state.currentBar = pos / beatsPerBar;

        double accumPpq = 0.0;
        for (size_t i = 0; i < blocks.size(); ++i)
        {
            double blockDurPpq = static_cast<double>(blocks[i].bars) * beatsPerBar;
            if (pos >= accumPpq && (pos < accumPpq + blockDurPpq || i == blocks.size() - 1))
            {
                state.activeBlockIndex = static_cast<int>(i);
                double intraBlockPpq = pos - accumPpq;
                state.blockProgress = static_cast<float>(juce::jlimit(0.0, 1.0, blockDurPpq > 0.0 ? (intraBlockPpq / blockDurPpq) : 0.0));
                state.activeRootKey = blocks[i].rootKey;
                state.activeScaleType = blocks[i].scaleType;
                return state;
            }
            accumPpq += blockDurPpq;
        }

        state.activeBlockIndex = static_cast<int>(blocks.size()) - 1;
        state.activeRootKey = blocks.back().rootKey;
        state.activeScaleType = blocks.back().scaleType;
        state.blockProgress = 1.0f;
        return state;
    }

    void loadPresetProgression(int presetIndex)
    {
        blocks.clear();
        switch (presetIndex)
        {
            case 0: // Pop 4-Chords (C - G - Am - F)
                blocks.push_back({ 0, Scale_Major,        2.0f, "I - C Maj" });
                blocks.push_back({ 7, Scale_Major,        2.0f, "V - G Maj" });
                blocks.push_back({ 9, Scale_NaturalMinor, 2.0f, "vi - A Min" });
                blocks.push_back({ 5, Scale_Major,        2.0f, "IV - F Maj" });
                break;

            case 1: // Jazz Turnaround ii - V - I - VI
                blocks.push_back({ 2, Scale_Dorian,       1.0f, "ii - D Dor" });
                blocks.push_back({ 7, Scale_Mixolydian,   1.0f, "V - G Mixo" });
                blocks.push_back({ 0, Scale_Major,        1.0f, "I - C Maj" });
                blocks.push_back({ 9, Scale_HarmonicMinor,1.0f, "VI - A Harm" });
                break;

            case 2: // Cinematic Dark Hero (Am - F - C - G)
                blocks.push_back({ 9, Scale_NaturalMinor, 4.0f, "i - A Min" });
                blocks.push_back({ 5, Scale_Lydian,       4.0f, "VI - F Lyd" });
                blocks.push_back({ 0, Scale_Major,        4.0f, "III - C Maj" });
                blocks.push_back({ 7, Scale_Mixolydian,   4.0f, "VII - G Mix" });
                break;

            case 3: // 12-Bar Blues in E
                blocks.push_back({ 4, Scale_Blues, 4.0f, "I - E Blues (4b)" });
                blocks.push_back({ 9, Scale_Blues, 2.0f, "IV - A Blues (2b)" });
                blocks.push_back({ 4, Scale_Blues, 2.0f, "I - E Blues (2b)" });
                blocks.push_back({ 11,Scale_Blues, 1.0f, "V - B Blues (1b)" });
                blocks.push_back({ 9, Scale_Blues, 1.0f, "IV - A Blues (1b)" });
                blocks.push_back({ 4, Scale_Blues, 2.0f, "I - E Blues (2b)" });
                break;

            case 4: // Modal Odyssey
                blocks.push_back({ 2, Scale_Dorian,       2.0f, "D Dorian" });
                blocks.push_back({ 7, Scale_Mixolydian,   2.0f, "G Mixolydian" });
                blocks.push_back({ 0, Scale_Lydian,       2.0f, "C Lydian" });
                blocks.push_back({ 4, Scale_Phrygian,     2.0f, "E Phrygian" });
                break;

            case 5: // Neo-Soul Journey
                blocks.push_back({ 5, Scale_Major,        2.0f, "F Maj" });
                blocks.push_back({ 4, Scale_NaturalMinor, 2.0f, "E Min" });
                blocks.push_back({ 2, Scale_NaturalMinor, 2.0f, "D Min" });
                blocks.push_back({ 0, Scale_Major,        2.0f, "C Maj" });
                break;

            default:
                loadDefaultProgression();
                break;
        }
    }

    juce::ValueTree getState() const
    {
        juce::ValueTree tree("ScaleProgression");
        tree.setProperty("enabled", enabled, nullptr);
        tree.setProperty("loop", loop, nullptr);

        for (const auto& b : blocks)
        {
            juce::ValueTree bTree("Block");
            bTree.setProperty("rootKey", b.rootKey, nullptr);
            bTree.setProperty("scaleType", b.scaleType, nullptr);
            bTree.setProperty("bars", b.bars, nullptr);
            bTree.setProperty("label", b.label, nullptr);
            tree.addChild(bTree, -1, nullptr);
        }
        return tree;
    }

    void setState(const juce::ValueTree& tree)
    {
        if (!tree.isValid() || tree.getType() != juce::Identifier("ScaleProgression"))
            return;

        if (tree.hasProperty("enabled"))
            enabled = static_cast<bool>(tree.getProperty("enabled"));
        if (tree.hasProperty("loop"))
            loop = static_cast<bool>(tree.getProperty("loop"));

        blocks.clear();
        for (int i = 0; i < tree.getNumChildren(); ++i)
        {
            auto c = tree.getChild(i);
            if (c.getType() == juce::Identifier("Block"))
            {
                ScaleSeqBlock b;
                b.rootKey = static_cast<int>(c.getProperty("rootKey", 0));
                b.scaleType = static_cast<int>(c.getProperty("scaleType", 0));
                b.bars = static_cast<float>(c.getProperty("bars", 1.0f));
                b.label = c.getProperty("label", "").toString();
                blocks.push_back(b);
            }
        }
    }

    const std::vector<ScaleSeqBlock>& getAllBlocks() const { return blocks; }
    void setAllBlocks(const std::vector<ScaleSeqBlock>& newBlocks)
    {
        if (!newBlocks.empty())
            blocks = newBlocks;
    }

    bool saveToFile(const juce::File& file) const
    {
        auto vt = getState();
        std::unique_ptr<juce::XmlElement> xml(vt.createXml());
        if (xml != nullptr)
            return xml->writeTo(file);
        return false;
    }

    bool loadFromFile(const juce::File& file)
    {
        if (!file.existsAsFile())
            return false;
        std::unique_ptr<juce::XmlElement> xml = juce::parseXML(file);
        if (xml == nullptr)
            return false;
        auto vt = juce::ValueTree::fromXml(*xml);
        if (vt.isValid() && vt.getType() == juce::Identifier("ScaleProgression"))
        {
            setState(vt);
            return true;
        }
        return false;
    }

    struct ProgressionPreset
    {
        juce::String name;
        std::vector<ScaleSeqBlock> blocks;
    };

    static std::vector<ProgressionPreset>& getUserProgressions()
    {
        static std::vector<ProgressionPreset> userPresets;
        static bool loaded = false;
        if (!loaded)
        {
            loaded = true;
            loadUserProgressionsFromDisk(userPresets);
        }
        return userPresets;
    }

    static void addUserProgression(const juce::String& name, const std::vector<ScaleSeqBlock>& b)
    {
        auto& list = getUserProgressions();
        list.push_back({ name, b });
        saveUserProgressionsToDisk(list);
    }

    static void deleteUserProgression(int index)
    {
        auto& list = getUserProgressions();
        if (index >= 0 && index < static_cast<int>(list.size()))
        {
            list.erase(list.begin() + index);
            saveUserProgressionsToDisk(list);
        }
    }

private:
    static juce::File getUserProgressionsFile()
    {
        auto dir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                        .getChildFile("mtyas").getChildFile("MidiFlux");
        dir.createDirectory();
        return dir.getChildFile("UserProgressions.xml");
    }

    static void saveUserProgressionsToDisk(const std::vector<ProgressionPreset>& list)
    {
        juce::ValueTree root("UserProgressions");
        for (const auto& p : list)
        {
            juce::ValueTree pTree("Progression");
            pTree.setProperty("name", p.name, nullptr);
            for (const auto& b : p.blocks)
            {
                juce::ValueTree bTree("Block");
                bTree.setProperty("rootKey", b.rootKey, nullptr);
                bTree.setProperty("scaleType", b.scaleType, nullptr);
                bTree.setProperty("bars", b.bars, nullptr);
                bTree.setProperty("label", b.label, nullptr);
                pTree.addChild(bTree, -1, nullptr);
            }
            root.addChild(pTree, -1, nullptr);
        }
        std::unique_ptr<juce::XmlElement> xml(root.createXml());
        if (xml != nullptr)
            xml->writeTo(getUserProgressionsFile());
    }

    static void loadUserProgressionsFromDisk(std::vector<ProgressionPreset>& list)
    {
        auto file = getUserProgressionsFile();
        if (!file.existsAsFile())
            return;
        std::unique_ptr<juce::XmlElement> xml = juce::parseXML(file);
        if (xml == nullptr)
            return;
        auto root = juce::ValueTree::fromXml(*xml);
        if (!root.isValid() || root.getType() != juce::Identifier("UserProgressions"))
            return;

        list.clear();
        for (int i = 0; i < root.getNumChildren(); ++i)
        {
            auto c = root.getChild(i);
            if (c.getType() == juce::Identifier("Progression"))
            {
                ProgressionPreset p;
                p.name = c.getProperty("name", "Progression").toString();
                for (int j = 0; j < c.getNumChildren(); ++j)
                {
                    auto bc = c.getChild(j);
                    ScaleSeqBlock b;
                    b.rootKey = static_cast<int>(bc.getProperty("rootKey", 0));
                    b.scaleType = static_cast<int>(bc.getProperty("scaleType", 0));
                    b.bars = static_cast<float>(bc.getProperty("bars", 1.0f));
                    b.label = bc.getProperty("label", "").toString();
                    p.blocks.push_back(b);
                }
                list.push_back(p);
            }
        }
    }

    std::vector<ScaleSeqBlock> blocks;
    bool enabled = false;
    bool loop = true;
};

} // namespace MidiFlux
