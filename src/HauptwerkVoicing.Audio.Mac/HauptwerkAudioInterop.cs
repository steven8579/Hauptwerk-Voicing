using System.Runtime.InteropServices;

namespace HauptwerkVoicing.Audio.Mac;

// P/Invoke surface for the native HauptwerkAudio.framework (Swift, wraps
// Core Audio / AVAudioEngine). Mirrors native/HauptwerkAudio/include/
// HauptwerkAudio.h one-for-one. Managed callers should wrap these via a
// higher-level service; don't call DllImports directly from UI code.
internal static class HauptwerkAudioInterop
{
    // Resolves to libHauptwerkAudio.dylib at runtime. Step 9 wires the
    // build to copy it next to the MAUI app's binary.
    private const string DllName = "HauptwerkAudio";

    // Mirrors hw_result_t.
    internal enum HwResult
    {
        Ok = 0,
        InvalidArgument = -1,
        DeviceNotFound = -2,
        UnsupportedFormat = -3,
        CoreAudio = -4,
        AlreadyRunning = -5,
        NotRunning = -6,
        NotImplemented = -99,
    }

    // Mirrors hw_device_info_t. Fixed-size char buffers keep the ABI
    // stable and avoid string-marshaling allocations on the enumeration
    // callback path.
    [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
    internal struct HwDeviceInfo
    {
        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 128)]
        public string DeviceId;

        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 128)]
        public string DisplayName;

        public int InputChannelCount;
        public int OutputChannelCount;
        public double NativeSampleRate;
    }

    // Mirrors hw_capture_callback_t. Invoked on a Core Audio realtime
    // thread — do not allocate, lock, log, or otherwise block. Copy out
    // of the buffer and signal a worker.
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    internal delegate void HwCaptureCallback(
        IntPtr samples,
        int frameCount,
        int channelCount,
        ulong hostTimeNs,
        IntPtr userData);

    [DllImport(DllName, EntryPoint = "hw_enumerate_devices", CallingConvention = CallingConvention.Cdecl)]
    internal static extern HwResult EnumerateDevices(out IntPtr outDevices, out int outCount);

    [DllImport(DllName, EntryPoint = "hw_free_device_list", CallingConvention = CallingConvention.Cdecl)]
    internal static extern void FreeDeviceList(IntPtr devices);

    [DllImport(DllName, EntryPoint = "hw_open_capture", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
    internal static extern IntPtr OpenCapture(
        [MarshalAs(UnmanagedType.LPStr)] string deviceId,
        int channel,
        double sampleRate,
        int bitDepth,
        out HwResult error);

    [DllImport(DllName, EntryPoint = "hw_start_capture", CallingConvention = CallingConvention.Cdecl)]
    internal static extern HwResult StartCapture(
        IntPtr handle,
        HwCaptureCallback callback,
        IntPtr userData);

    [DllImport(DllName, EntryPoint = "hw_stop_capture", CallingConvention = CallingConvention.Cdecl)]
    internal static extern HwResult StopCapture(IntPtr handle);

    [DllImport(DllName, EntryPoint = "hw_close_capture", CallingConvention = CallingConvention.Cdecl)]
    internal static extern void CloseCapture(IntPtr handle);

    [DllImport(DllName, EntryPoint = "hw_generate_calibration_tone", CallingConvention = CallingConvention.Cdecl)]
    internal static extern HwResult GenerateCalibrationTone(double frequencyHz, double amplitude);
}
