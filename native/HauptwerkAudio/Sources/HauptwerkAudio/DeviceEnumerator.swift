import Foundation

// Wraps Core Audio HAL device enumeration (AudioObjectGetPropertyData with
// kAudioObjectSystemObject / kAudioHardwarePropertyDevices). Populates the
// hw_device_info_t array that hw_enumerate_devices hands back to the caller.
//
// Step 9 lands the real implementation.
internal final class DeviceEnumerator {
    internal init() {}
}
