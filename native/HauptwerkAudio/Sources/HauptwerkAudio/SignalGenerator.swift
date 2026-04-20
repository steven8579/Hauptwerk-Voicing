import Foundation

// Phase 1: sine tone for playback-chain calibration verification
// (hw_generate_calibration_tone).
//
// Phase 2: swept sine, pink noise, and reference tones for room
// characterization. These must share an AVAudioEngine with CaptureEngine
// so sweep output and mic capture run on the same device clock — see
// CLAUDE.md §9 "Same-device sync requirement for Phase 2".
internal final class SignalGenerator {
    internal init() {}
}
