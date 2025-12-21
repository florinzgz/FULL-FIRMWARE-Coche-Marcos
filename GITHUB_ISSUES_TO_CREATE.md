# GitHub Issues to Create

This document lists GitHub issues that should be created to track pending TODOs found in the codebase.

## Issue 1: Feature - Emergency Lights on Long-Press LIGHTS Button

**Title:** `Feature: Implement emergency lights on long-press LIGHTS button`

**Labels:** `enhancement`, `feature`, `TODO`

**Body:**
```markdown
## Description
Implement emergency/hazard lights functionality when the LIGHTS button is held for a long press (>800ms).

## Current Behavior
- Short press: Toggles regular lights on/off
- Long press: Plays confirmation sound but does not activate emergency lights

## Expected Behavior
- Short press: Toggle regular lights on/off (current behavior - keep unchanged)
- Long press: Activate emergency/hazard lights via LED controller

## Location
- File: `src/input/buttons.cpp`
- Lines: 86-88
- Code reference:
  ```cpp
  // TODO: Long-press should activate emergency/hazard lights via LED controller.
  //       Integration with LED controller required here (future enhancement).
  ```

## Implementation Notes
- Requires integration with LED controller module
- Should implement flashing pattern for all turn signals (hazard lights)
- May need additional state tracking for hazard mode
- Consider audio feedback for activation/deactivation

## Priority
Medium - Enhancement for safety features
```

---

## Issue 2: Feature - Audio Mode Cycling on Long-Press MULTIMEDIA Button

**Title:** `Feature: Implement audio mode cycling on long-press MULTIMEDIA button`

**Labels:** `enhancement`, `feature`, `TODO`

**Body:**
```markdown
## Description
Implement audio mode cycling functionality when the MULTIMEDIA button is held for a long press (>800ms).

## Current Behavior
- Short press: Toggles multimedia mode on/off
- Long press: Logs message but does not cycle audio modes

## Expected Behavior
- Short press: Toggle multimedia mode on/off (current behavior - keep unchanged)
- Long press: Cycle through audio modes: Radio → Bluetooth → AUX → Radio (repeat)

## Location
- File: `src/input/buttons.cpp`
- Lines: 105-110
- Code reference:
  ```cpp
  Logger::info("Buttons: MULTIMEDIA long-press detectado - cambio de modo de audio");
  Alerts::play({Audio::AUDIO_MODULO_OK, Audio::Priority::PRIO_HIGH});
  // TODO: Long-press reserved for cycling audio modes (radio/bluetooth/aux)
  ```

## Implementation Notes
- Requires audio mode state management
- Modes to implement:
  - Radio (FM/AM)
  - Bluetooth audio
  - AUX input
- Should provide audio/visual feedback for mode changes
- Consider storing current mode in config for persistence

## Priority
Low - Quality of life enhancement
```

---

## Instructions for Creating Issues

1. Navigate to the GitHub repository
2. Click on "Issues" tab
3. Click "New Issue"
4. Copy the title and body from above for each issue
5. Add the suggested labels
6. Assign to appropriate milestone if applicable
7. Click "Submit new issue"

## Related Pull Request
These TODOs were documented as part of PR: [Code Cleanup and Bug Fixes]

## Notes
- Both TODOs are marked as future enhancements
- They do not block current functionality
- Implementation requires coordination with hardware modules (LED controller, audio system)
