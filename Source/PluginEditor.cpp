#include "PluginEditor.h"

namespace col
{
    const juce::Colour bg      { 0xff17140f };
    const juce::Colour panel   { 0xff211d17 };
    const juce::Colour slot    { 0xff2b2620 };
    const juce::Colour slotHi  { 0xff353029 };
    const juce::Colour amber   { 0xffe0932e };
    const juce::Colour green   { 0xff3fbf6f };
    const juce::Colour text    { 0xffcdbda2 };
    const juce::Colour dim     { 0xff7d7261 };
}

static constexpr int kRowHeight = 74;
static constexpr int kInfoW = 168;

//==============================================================================
RackLookAndFeel::RackLookAndFeel()
{
    setColour (juce::TextButton::buttonColourId,   col::slot);
    setColour (juce::TextButton::buttonOnColourId, col::amber);
    setColour (juce::TextButton::textColourOffId,  col::text);
    setColour (juce::TextButton::textColourOnId,   juce::Colours::black);
    setColour (juce::Slider::rotarySliderFillColourId, col::amber);
}

void RackLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h,
                                        float pos, float startAngle, float endAngle,
                                        juce::Slider&)
{
    auto b = juce::Rectangle<int> (x, y, w, h).toFloat().reduced (6.0f);
    const auto radius = juce::jmin (b.getWidth(), b.getHeight()) / 2.0f;
    const auto cx = b.getCentreX(), cy = b.getCentreY();
    const auto angle = startAngle + pos * (endAngle - startAngle);

    g.setColour (juce::Colour (0xff100d0a));
    g.fillEllipse (cx - radius, cy - radius, radius * 2.0f, radius * 2.0f);
    g.setColour (col::slotHi);
    g.drawEllipse (cx - radius, cy - radius, radius * 2.0f, radius * 2.0f, 2.0f);

    juce::Path p;
    p.startNewSubPath (cx, cy);
    p.lineTo (cx + std::sin (angle) * radius * 0.85f, cy - std::cos (angle) * radius * 0.85f);
    g.setColour (col::amber);
    g.strokePath (p, juce::PathStrokeType (2.5f, juce::PathStrokeType::curved,
                                           juce::PathStrokeType::rounded));
}

//==============================================================================
PluginWindow::PluginWindow (const juce::String& name, juce::AudioProcessorEditor* editor)
    : DocumentWindow (name, juce::Colours::black,
                      DocumentWindow::minimiseButton | DocumentWindow::closeButton)
{
    setUsingNativeTitleBar (true);
    setContentOwned (editor, true);
    setResizable (editor->isResizable(), false);
    centreWithSize (getWidth(), getHeight());
    setVisible (true);
    setAlwaysOnTop (true);
}

void PluginWindow::closeButtonPressed() { if (onClose) onClose(); }

//==============================================================================
LevelMeter::LevelMeter (std::function<float()> src) : getLevel (std::move (src))
{
    startTimerHz (30);
}

void LevelMeter::timerCallback()
{
    const float l = getLevel();
    display = l > display ? l : display * 0.8f;   // fast attack, slow release
    repaint();
}

void LevelMeter::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    const int segments = 20;
    const float gap = 2.0f;
    const float segW = (r.getWidth() - gap * (segments - 1)) / segments;
    const float lvlDb = juce::Decibels::gainToDecibels (display, -60.0f);
    const float norm = juce::jlimit (0.0f, 1.0f, (lvlDb + 60.0f) / 60.0f);
    const int lit = juce::roundToInt (norm * segments);

    for (int i = 0; i < segments; ++i)
    {
        auto seg = juce::Rectangle<float> (r.getX() + i * (segW + gap), r.getY(), segW, r.getHeight());
        juce::Colour c = col::green;
        if (i > segments * 0.75f) c = juce::Colour (0xffd94f2e);
        else if (i > segments * 0.55f) c = col::amber;
        g.setColour (i < lit ? c : c.withAlpha (0.12f));
        g.fillRoundedRectangle (seg, 1.5f);
    }
}

//==============================================================================
SlotRow::SlotRow (RackABEditor& owner, int index, bool empty)
    : slotIndex (index), isEmpty (empty), editor (owner)
{
    if (! isEmpty)
    {
        addAndMakeVisible (muteBtn);
        muteBtn.setClickingTogglesState (true);
        muteBtn.setToggleState (editor.proc.getChain().isMuted (index), juce::dontSendNotification);
        muteBtn.onClick = [this] { editor.toggleMute (slotIndex); };

        addAndMakeVisible (removeBtn);
        removeBtn.setColour (juce::TextButton::textColourOffId, juce::Colour (0xffd9695a));
        removeBtn.onClick = [this] { editor.removeSlot (slotIndex); };
    }
}

juce::Rectangle<int> SlotRow::handleBounds() const
{
    return getLocalBounds().removeFromRight (44);
}

void SlotRow::resized()
{
    if (! isEmpty)
    {
        muteBtn.setBounds (14, getHeight() / 2 - 13, 66, 26);
        removeBtn.setBounds (getWidth() - 44 - 34, getHeight() / 2 - 13, 28, 26);
    }
}

void SlotRow::mouseDown (const juce::MouseEvent& e)
{
    if (! isEmpty && handleBounds().contains (e.getPosition()))
    {
        draggingHandle = true;
        editor.beginDrag (this, e);
        return;
    }
    if (isEmpty) { editor.loadDialog (slotIndex); return; }
    editor.selectSlot (slotIndex);
}

void SlotRow::mouseDrag (const juce::MouseEvent& e)
{
    if (draggingHandle)
        editor.dragTo (this, e);
}

void SlotRow::mouseUp (const juce::MouseEvent& e)
{
    if (draggingHandle)
    {
        draggingHandle = false;
        editor.endDrag (this);
        return;
    }
    if (! isEmpty && e.getDistanceFromDragStart() < 6 && ! handleBounds().contains (e.getPosition()))
        editor.openPluginGui (slotIndex);
}

void SlotRow::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat().reduced (3.0f);
    if (beingDragged)
        r = r.reduced (-1.0f, 1.0f);          // lift slightly

    auto bg = col::slot;
    if (selected)        bg = col::slotHi;
    if (activeInCompare) bg = col::green.withAlpha (0.22f);
    if (beingDragged)    bg = col::slotHi.brighter (0.15f);

    if (beingDragged)
    {
        juce::DropShadow (juce::Colours::black.withAlpha (0.6f), 18, {}).drawForRectangle (g, r.toNearestInt());
    }

    g.setColour (bg);
    g.fillRoundedRectangle (r, 6.0f);
    g.setColour (beingDragged ? col::amber
                              : (activeInCompare ? col::green : juce::Colour (0xff0d0b08)));
    g.drawRoundedRectangle (r, 6.0f, (beingDragged || activeInCompare) ? 2.0f : 1.0f);

    // number badge
    g.setColour (col::amber.withAlpha (0.85f));
    g.setFont (juce::Font (juce::FontOptions (22.0f)).boldened());
    g.drawText (juce::String (slotIndex + 1), 90, 0, 40, getHeight(),
                juce::Justification::centredLeft);

    if (isEmpty)
    {
        auto box = getLocalBounds().reduced (140, 16).toFloat();
        box.removeFromRight (44);
        juce::Path dash;
        dash.addRoundedRectangle (box, 6.0f);
        juce::Path dashed;
        const float dl[] = { 6.0f, 4.0f };
        juce::PathStrokeType (1.5f).createDashedStroke (dashed, dash, dl, 2);
        g.setColour (col::dim.withAlpha (0.7f));
        g.fillPath (dashed);
        g.setColour (col::dim);
        g.setFont (juce::Font (juce::FontOptions (15.0f)));
        g.drawText ("+  LOAD PLUGIN", box.toNearestInt(), juce::Justification::centred);
    }
    else
    {
        g.setColour (col::text);
        g.setFont (juce::Font (juce::FontOptions (17.0f)).boldened());
        g.drawText (pluginName, 140, 0, getWidth() - 200, getHeight(),
                    juce::Justification::centredLeft, true);
    }

    // drag handle (three lines)
    auto h = handleBounds().toFloat();
    g.setColour (col::dim);
    for (int i = 0; i < 3; ++i)
        g.fillRoundedRectangle (h.getCentreX() - 8, h.getCentreY() - 6 + i * 6.0f, 16, 2, 1.0f);
}

//==============================================================================
RackABEditor::RackABEditor (RackABProcessor& p)
    : AudioProcessorEditor (&p), proc (p)
{
    setLookAndFeel (&lnf);

    title.setText ("ANALOG", juce::dontSendNotification);
    title.setFont (juce::Font (juce::FontOptions (15.0f)).boldened());
    title.setColour (juce::Label::textColourId, col::dim);
    addAndMakeVisible (title);

    subtitle.setText ("RACK", juce::dontSendNotification);
    subtitle.setFont (juce::Font (juce::FontOptions (26.0f)).boldened());
    subtitle.setColour (juce::Label::textColourId, col::text);
    addAndMakeVisible (subtitle);

    bypassAllBtn.setClickingTogglesState (true);
    bypassAllBtn.onClick = [this] { proc.getChain().setBypassAll (bypassAllBtn.getToggleState()); };
    addAndMakeVisible (bypassAllBtn);

    compareBtn.setClickingTogglesState (true);
    compareBtn.onClick = [this]
    {
        proc.getChain().setCompare (compareBtn.getToggleState());
        grabKeyboardFocus();
    };
    addAndMakeVisible (compareBtn);

    rescanBtn.onClick = [this]
    {
        proc.getChain().startScan();
        status.setText ("Escaneando plugins...", juce::dontSendNotification);
    };
    addAndMakeVisible (rescanBtn);

    status.setFont (juce::Font (juce::FontOptions (12.0f)));
    status.setColour (juce::Label::textColourId, col::dim);
    status.setJustificationType (juce::Justification::centredRight);
    addAndMakeVisible (status);

    addAndMakeVisible (viewport);
    viewport.setViewedComponent (&rowsHolder, false);
    viewport.setScrollBarsShown (true, false);

    dryWet.setRange (0.0, 1.0, 0.001);
    dryWet.setValue (proc.getChain().getDryWet(), juce::dontSendNotification);
    dryWet.onValueChange = [this] { proc.getChain().setDryWet ((float) dryWet.getValue()); };
    addAndMakeVisible (dryWet);
    dryWetLabel.setJustificationType (juce::Justification::centred);
    dryWetLabel.setColour (juce::Label::textColourId, col::dim);
    dryWetLabel.setFont (juce::Font (juce::FontOptions (11.0f)));
    addAndMakeVisible (dryWetLabel);

    inMeter  = std::make_unique<LevelMeter> ([this] { return proc.getInLevel(); });
    outMeter = std::make_unique<LevelMeter> ([this] { return proc.getOutLevel(); });
    addAndMakeVisible (*inMeter);
    addAndMakeVisible (*outMeter);
    for (auto* l : { &inLabel, &outLabel })
    {
        l->setColour (juce::Label::textColourId, col::dim);
        l->setFont (juce::Font (juce::FontOptions (11.0f)));
        addAndMakeVisible (l);
    }

    juce::Component::SafePointer<RackABEditor> safe (this);
    proc.getChain().onScanFinished = [safe]
    {
        if (safe != nullptr)
            safe->status.setText ("Listo: "
                + juce::String (safe->proc.getChain().getKnownList().getNumTypes())
                + " plugins", juce::dontSendNotification);
    };

    if (proc.getChain().getKnownList().getNumTypes() == 0)
        proc.getChain().startScan();
    else
        status.setText (juce::String (proc.getChain().getKnownList().getNumTypes())
                        + " plugins", juce::dontSendNotification);

    setWantsKeyboardFocus (true);
    setResizable (true, true);
    setSize (940, 620);

    rebuildRows();
    startTimerHz (15);
}

RackABEditor::~RackABEditor()
{
    proc.getChain().onScanFinished = nullptr;
    closeAllPluginWindows();
    setLookAndFeel (nullptr);
}

void RackABEditor::paint (juce::Graphics& g)
{
    g.fillAll (col::bg);

    auto b = getLocalBounds();
    b.removeFromTop (66);
    b.removeFromBottom (96);
    auto info = b.removeFromRight (kInfoW).reduced (8);

    g.setColour (col::panel);
    g.fillRoundedRectangle (info.toFloat(), 8.0f);
    g.setColour (col::amber);
    g.setFont (juce::Font (juce::FontOptions (15.0f)).boldened());
    auto ir = info.reduced (14);
    g.drawText ("COMPARE MODE", ir.removeFromTop (24), juce::Justification::topLeft);
    ir.removeFromTop (6);
    g.setColour (col::text);
    g.setFont (juce::Font (juce::FontOptions (12.5f)));
    g.drawFittedText ("Pulsa COMPARE y usa las flechas Arriba/Abajo para pasar de "
                      "plugin en plugin. Solo el seleccionado suena; el resto se "
                      "silencia para comparar A/B.",
                      ir.removeFromTop (150), juce::Justification::topLeft, 8);

    // chain of numbers
    const int n = proc.getChain().getNumPlugins();
    const int sel = proc.getChain().getSelected();
    const bool cmp = proc.getChain().isCompare();
    auto chainArea = ir;
    int y = chainArea.getY() + 6;
    for (int i = 0; i < juce::jmin (n, 8); ++i)
    {
        juce::Rectangle<int> box (chainArea.getX(), y, 34, 26);
        const bool active = cmp && i == sel;
        g.setColour (active ? col::green : col::slot);
        g.fillRoundedRectangle (box.toFloat(), 4.0f);
        g.setColour (active ? juce::Colours::black : col::text);
        g.setFont (juce::Font (juce::FontOptions (13.0f)).boldened());
        g.drawText (juce::String (i + 1), box, juce::Justification::centred);
        y += 30;
    }
}

void RackABEditor::resized()
{
    auto r = getLocalBounds();

    auto header = r.removeFromTop (66).reduced (14, 12);
    auto left = header.removeFromLeft (150);
    title.setBounds (left.removeFromTop (18));
    subtitle.setBounds (left);

    auto rightCtrls = header.removeFromRight (360);
    rescanBtn.setBounds  (rightCtrls.removeFromLeft (90).reduced (4));
    bypassAllBtn.setBounds (rightCtrls.removeFromLeft (120).reduced (4));
    compareBtn.setBounds (rightCtrls.reduced (4));
    status.setBounds (header.reduced (6, 20));

    auto bottom = r.removeFromBottom (96).reduced (14, 12);
    auto dw = bottom.removeFromRight (110);
    dryWetLabel.setBounds (dw.removeFromBottom (16));
    dryWet.setBounds (dw);
    bottom.removeFromRight (20);

    auto outArea = bottom.removeFromRight (280);
    outLabel.setBounds (outArea.removeFromTop (16));
    outMeter->setBounds (outArea.reduced (0, 12));
    bottom.removeFromRight (20);
    auto inArea = bottom.removeFromRight (280);
    inLabel.setBounds (inArea.removeFromTop (16));
    inMeter->setBounds (inArea.reduced (0, 12));

    r.removeFromRight (kInfoW);              // info panel drawn in paint()
    viewport.setBounds (r.reduced (14, 8));

    const int total = rows.size() * kRowHeight;
    rowsHolder.setSize (viewport.getMaximumVisibleWidth(),
                        juce::jmax (viewport.getHeight(), total));
    for (int i = 0; i < rows.size(); ++i)
        rows[i]->setBounds (0, i * kRowHeight, rowsHolder.getWidth(), kRowHeight);
}

bool RackABEditor::keyPressed (const juce::KeyPress& key)
{
    if (! proc.getChain().isCompare())
        return false;
    if (key == juce::KeyPress::downKey) { proc.getChain().moveSelection (+1); return true; }
    if (key == juce::KeyPress::upKey)   { proc.getChain().moveSelection (-1); return true; }
    return false;
}

void RackABEditor::selectSlot (int index)
{
    proc.getChain().setSelected (index);
    grabKeyboardFocus();
}

void RackABEditor::toggleMute (int index)
{
    proc.getChain().toggleMuted (index);
}

void RackABEditor::removeSlot (int index)
{
    proc.getChain().removeAt (index);
    rebuildRows();
}

void RackABEditor::openPluginGui (int index)
{
    if (auto* ed = proc.getChain().createEditorFor (index))
    {
        auto* w = new PluginWindow (proc.getChain().getPluginName (index), ed);
        w->onClose = [this, w] { pluginWindows.removeObject (w); };
        pluginWindows.add (w);
    }
}

void RackABEditor::loadDialog (int)
{
    showPluginMenu();
}

void RackABEditor::showPluginMenu()
{
    auto& kl = proc.getChain().getKnownList();
    juce::PopupMenu menu;

    if (kl.getNumTypes() == 0)
    {
        menu.addItem (1, proc.getChain().isScanning() ? "Escaneando..."
                                                      : "Sin plugins - pulsa RESCAN", false, false);
    }
    else
    {
        kl.addToMenu (menu, juce::KnownPluginList::sortByCategory);
    }

    menu.showMenuAsync (juce::PopupMenu::Options().withMinimumWidth (260),
        [this, &kl] (int result)
        {
            if (result <= 0) return;
            const int idx = kl.getIndexChosenByMenu (result);
            if (idx < 0) return;
            const auto types = kl.getTypes();
            if (juce::isPositiveAndBelow (idx, types.size()))
            {
                const auto err = proc.getChain().addPluginFromDescription (types.getReference (idx));
                if (err.isNotEmpty())
                    juce::AlertWindow::showMessageBoxAsync (juce::AlertWindow::WarningIcon,
                                                            "Error", err);
                rebuildRows();
            }
        });
}

void RackABEditor::beginDrag (SlotRow* row, const juce::MouseEvent& e)
{
    dragRow = row;
    dragGrabOffsetY = e.getPosition().y;
    dragVisualIndex = row->slotIndex;
    row->beingDragged = true;
    row->toFront (false);
    row->repaint();
}

void RackABEditor::dragTo (SlotRow* row, const juce::MouseEvent& e)
{
    if (dragRow != row) return;
    const int n = proc.getChain().getNumPlugins();      // filled rows only
    const auto pos = e.getEventRelativeTo (&rowsHolder).getPosition();

    // Dragged row follows the cursor directly (no animation).
    int y = pos.y - dragGrabOffsetY;
    y = juce::jlimit (0, (n - 1) * kRowHeight, y);
    row->setBounds (0, y, rowsHolder.getWidth(), kRowHeight);

    // Where would it drop?
    const int target = juce::jlimit (0, n - 1,
                                     juce::roundToInt ((float) y / kRowHeight));
    if (target != dragVisualIndex)
    {
        dragVisualIndex = target;
        layoutRows (target);
    }
}

void RackABEditor::endDrag (SlotRow* row)
{
    if (dragRow != row) { return; }
    const int from = row->slotIndex;
    const int to = dragVisualIndex;
    row->beingDragged = false;
    dragRow = nullptr;

    if (from != to)
        proc.getChain().move (from, to);
    rebuildRows();     // snap to committed order
}

// Positions every non-dragged row into its slot, leaving a gap at
// draggedVisualIndex; the shift is animated so rows glide out of the way.
void RackABEditor::layoutRows (int draggedVisualIndex)
{
    const int n = proc.getChain().getNumPlugins();
    const int from = dragRow != nullptr ? dragRow->slotIndex : -1;

    // Build the visual order: all filled indices except the dragged one,
    // with a hole inserted at draggedVisualIndex.
    int slot = 0;
    for (int i = 0; i < n; ++i)
    {
        if (i == from) continue;
        if (slot == draggedVisualIndex) ++slot;   // reserve the hole
        if (auto* r = rows[i])
        {
            const juce::Rectangle<int> target (0, slot * kRowHeight,
                                               rowsHolder.getWidth(), kRowHeight);
            animator.animateComponent (r, target, 1.0f, 130, false, 1.0, 1.0);
        }
        ++slot;
    }
    // Empty "LOAD PLUGIN" row stays at the bottom.
    if (auto* empty = rows[n])
        empty->setBounds (0, n * kRowHeight, rowsHolder.getWidth(), kRowHeight);
}

void RackABEditor::closeAllPluginWindows() { pluginWindows.clear(); }

void RackABEditor::rebuildRows()
{
    rows.clear();
    const int n = proc.getChain().getNumPlugins();
    for (int i = 0; i < n; ++i)
    {
        auto* row = new SlotRow (*this, i, false);
        row->pluginName = proc.getChain().getPluginName (i);
        rowsHolder.addAndMakeVisible (row);
        rows.add (row);
    }
    // trailing empty "LOAD PLUGIN" slot
    auto* add = new SlotRow (*this, n, true);
    rowsHolder.addAndMakeVisible (add);
    rows.add (add);

    lastSelected = -1;
    lastCount = n;
    resized();
}

void RackABEditor::timerCallback()
{
    auto& chain = proc.getChain();
    const int n = chain.getNumPlugins();
    if (n != lastCount) { rebuildRows(); repaint(); return; }

    const int sel = chain.getSelected();
    const bool cmp = chain.isCompare();
    if (sel != lastSelected || cmp != lastCompare)
    {
        lastSelected = sel;
        lastCompare = cmp;
        for (int i = 0; i < rows.size(); ++i)
        {
            rows[i]->selected = (i == sel && ! rows[i]->isEmpty);
            rows[i]->activeInCompare = (cmp && i == sel && ! rows[i]->isEmpty);
            rows[i]->repaint();
        }
        if (juce::isPositiveAndBelow (sel, rows.size()))
            viewport.setViewPosition (0, juce::jmax (0, sel * kRowHeight - viewport.getHeight() / 2));
        repaint();
    }
}
