#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include "PluginProcessor.h"

class RackABEditor;

// Amber-on-dark "analog gear" look.
class RackLookAndFeel : public juce::LookAndFeel_V4
{
public:
    RackLookAndFeel();
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h,
                           float pos, float startAngle, float endAngle,
                           juce::Slider&) override;
};

// Floating window hosting a nested plugin's own GUI.
class PluginWindow : public juce::DocumentWindow
{
public:
    PluginWindow (const juce::String& name, juce::AudioProcessorEditor* editor);
    void closeButtonPressed() override;
    std::function<void()> onClose;
};

// A simple peak meter fed by an atomic level.
class LevelMeter : public juce::Component, private juce::Timer
{
public:
    LevelMeter (std::function<float()> src);
    void paint (juce::Graphics&) override;
private:
    void timerCallback() override;
    std::function<float()> getLevel;
    float display = 0.0f;
};

// One rack slot: number, MUTE, plugin name / "LOAD PLUGIN", drag handle.
class SlotRow : public juce::Component
{
public:
    SlotRow (RackABEditor& owner, int index, bool empty);
    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp   (const juce::MouseEvent&) override;

    int slotIndex = 0;
    bool isEmpty = false;
    bool selected = false;
    bool activeInCompare = false;
    bool beingDragged = false;
    juce::String pluginName;

    juce::Rectangle<int> handleBounds() const;

private:
    RackABEditor& editor;
    juce::TextButton muteBtn { "MUTE" };
    juce::TextButton removeBtn { "X" };
    bool draggingHandle = false;
};

class RackABEditor : public juce::AudioProcessorEditor,
                     private juce::Timer
{
public:
    explicit RackABEditor (RackABProcessor&);
    ~RackABEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    bool keyPressed (const juce::KeyPress&) override;

    // Row callbacks
    void selectSlot (int index);
    void toggleMute (int index);
    void removeSlot (int index);
    void openPluginGui (int index);
    void loadDialog (int index);              // installed-plugin menu

    // Live drag-reorder
    void beginDrag (SlotRow* row, const juce::MouseEvent& e);
    void dragTo    (SlotRow* row, const juce::MouseEvent& e);
    void endDrag   (SlotRow* row);

    RackABProcessor& proc;

private:
    void timerCallback() override;
    void rebuildRows();
    void showPluginMenu();
    void closeAllPluginWindows();

    RackLookAndFeel lnf;

    juce::TextButton bypassAllBtn { "BYPASS ALL" };
    juce::TextButton compareBtn { "COMPARE" };
    juce::TextButton rescanBtn { "RESCAN" };
    juce::Label title, subtitle, status;

    juce::Viewport viewport;
    juce::Component rowsHolder;
    juce::OwnedArray<SlotRow> rows;

    juce::Slider dryWet { juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::NoTextBox };
    juce::Label dryWetLabel { {}, "DRY / WET" };
    std::unique_ptr<LevelMeter> inMeter, outMeter;
    juce::Label inLabel { {}, "INPUT" }, outLabel { {}, "OUTPUT" };

    juce::OwnedArray<PluginWindow> pluginWindows;

    void layoutRows (int draggedVisualIndex);

    juce::ComponentAnimator animator;
    SlotRow* dragRow = nullptr;
    int dragGrabOffsetY = 0;
    int dragVisualIndex = -1;

    int lastSelected = -1, lastCount = -1;
    bool lastCompare = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RackABEditor)
};
