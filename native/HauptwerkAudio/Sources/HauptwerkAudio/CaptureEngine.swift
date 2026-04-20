import Foundation

// Owns an AVAudioEngine per capture stream. Binds the input node to the
// chosen Fireface channel, configures a 96 kHz / 24-bit format, and
// dispatches frames to the hw_capture_callback_t from the Core Audio
// realtime thread.
//
// Step 9 lands the real implementation.
internal final class CaptureEngine {
    internal init() {}
}
