// Public C-compatible entry points for the HauptwerkAudio framework.
// Consumed via P/Invoke from HauptwerkVoicing.Audio.Mac.
//
// The signatures here mirror native/HauptwerkAudio/include/HauptwerkAudio.h
// one-for-one. Every @_cdecl function here is a Phase 1 stub returning
// HW_ERROR_NOT_IMPLEMENTED; real Core Audio / AVAudioEngine wiring lands
// in step 9 of CLAUDE.md §10.

import Foundation

// MARK: - Result codes (mirrors hw_result_t in HauptwerkAudio.h)

private let HW_OK: Int32                       = 0
private let HW_ERROR_INVALID_ARGUMENT: Int32   = -1
private let HW_ERROR_DEVICE_NOT_FOUND: Int32   = -2
private let HW_ERROR_UNSUPPORTED_FORMAT: Int32 = -3
private let HW_ERROR_CORE_AUDIO: Int32         = -4
private let HW_ERROR_ALREADY_RUNNING: Int32    = -5
private let HW_ERROR_NOT_RUNNING: Int32        = -6
private let HW_ERROR_NOT_IMPLEMENTED: Int32    = -99

// Swift-side function-pointer type matching hw_capture_callback_t. The
// Phase 1 stubs don't invoke it; kept here so step 9 already has the type
// nameable when CaptureEngine starts dispatching frames.
public typealias HauptwerkCaptureCallback = @convention(c) (
    _ samples: UnsafePointer<Int32>?,
    _ frameCount: Int32,
    _ channelCount: Int32,
    _ hostTimeNs: UInt64,
    _ userData: UnsafeMutableRawPointer?
) -> Void

// MARK: - Device enumeration

@_cdecl("hw_enumerate_devices")
public func hw_enumerate_devices(
    _ outDevices: UnsafeMutablePointer<UnsafeMutableRawPointer?>?,
    _ outCount: UnsafeMutablePointer<Int32>?
) -> Int32 {
    outCount?.pointee = 0
    outDevices?.pointee = nil
    return HW_ERROR_NOT_IMPLEMENTED
}

@_cdecl("hw_free_device_list")
public func hw_free_device_list(_ devices: UnsafeMutableRawPointer?) {
    // Paired with hw_enumerate_devices' allocation. No-op until step 9.
}

// MARK: - Capture

@_cdecl("hw_open_capture")
public func hw_open_capture(
    _ deviceId: UnsafePointer<CChar>?,
    _ channel: Int32,
    _ sampleRate: Double,
    _ bitDepth: Int32,
    _ outError: UnsafeMutablePointer<Int32>?
) -> OpaquePointer? {
    outError?.pointee = HW_ERROR_NOT_IMPLEMENTED
    return nil
}

@_cdecl("hw_start_capture")
public func hw_start_capture(
    _ handle: OpaquePointer?,
    _ callback: HauptwerkCaptureCallback?,
    _ userData: UnsafeMutableRawPointer?
) -> Int32 {
    return HW_ERROR_NOT_IMPLEMENTED
}

@_cdecl("hw_stop_capture")
public func hw_stop_capture(_ handle: OpaquePointer?) -> Int32 {
    return HW_ERROR_NOT_IMPLEMENTED
}

@_cdecl("hw_close_capture")
public func hw_close_capture(_ handle: OpaquePointer?) {
    // Phase 1 stub: no resources to release yet.
}

// MARK: - Output (Phase 2 surface)

@_cdecl("hw_generate_calibration_tone")
public func hw_generate_calibration_tone(
    _ frequencyHz: Double,
    _ amplitude: Double
) -> Int32 {
    return HW_ERROR_NOT_IMPLEMENTED
}
