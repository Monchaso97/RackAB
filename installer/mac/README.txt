RackAB - macOS installer (.dmg)
===============================

This ZIP contains the kit that builds the macOS .dmg for RackAB.

WHY A KIT AND NOT A FINISHED .dmg?
The macOS build of the plugin (RackAB.vst3) can only be compiled on a Mac
(Xcode + CMake). It cannot be produced on Windows, where this plugin was
built. So the finished .dmg is generated the first time you run this on a Mac.

HOW TO BUILD THE .dmg (on a Mac):
  1. Install Xcode command line tools:   xcode-select --install
  2. Install CMake:                       brew install cmake
  3. Have JUCE 9.x available.
  4. From this folder run:
         chmod +x build_dmg.sh
         ./build_dmg.sh /path/to/JUCE
  5. The disk image appears at:
         output/RackAB-1.0.0-macOS.dmg

WHAT THE CLIENT DOES:
  Opens the .dmg and drags RackAB.vst3 onto the "VST3 Plugins" shortcut.
  No installer, no standalone. Works in FL Studio, Pro Tools, etc.

GATEKEEPER (no Apple Developer account = not notarized):
  The client must do ONE first-run step: right-click RackAB.vst3 > Open once
  (or run the xattr command in "READ ME FIRST.txt"). This is unavoidable
  without a paid Apple Developer ID for notarization.
