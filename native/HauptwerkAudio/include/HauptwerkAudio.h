//
// HauptwerkAudio.h
//
// Public C-compatible API for the HauptwerkAudio framework.
// Consumed via P/Invoke from HauptwerkVoicing.Audio.Mac on the .NET side,
// and implemented by @_cdecl Swift functions that wrap Core Audio /
// AVAudioEngine.
//
// Threading model:
//   - hw_enumerate_devices, hw_open_capture, hw_close_capture may block and
//     must be called from a non-realtime thread.
//   - hw_start_capture invokes `callback` on a Core Audio HAL realtime
//     thread. That callback MUST NOT allocate, lock, log, or otherwise
//     block. Copy out of the buffer and signal a worker.
//
// Audio format:
//   Phase 1 is locked to 96 kHz / 24-bit per CLAUDE.md §4. Callers must
//   pass sample_rate = 96000.0 and bit_depth = 24 to hw_open_capture; any
//   other value returns HW_ERROR_UNSUPPORTED_FORMAT. The sample buffer is
//   signed 24-bit PCM packed into int32_t with sign extension into the
//   high 8 bits. Do not cast to int24 or reinterpret.
//
// Memory ownership:
//   - hw_enumerate_devices allocates an hw_device_info_t array that the
//     caller MUST release via hw_free_device_list. Do not free() directly.
//   - hw_open_capture returns an opaque hw_capture_t* that the caller MUST
//     release via hw_close_capture.
//

#ifndef HAUPTWERK_AUDIO_H
#define HAUPTWERK_AUDIO_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// ---------------------------------------------------------------------------
// Result codes
// ---------------------------------------------------------------------------

typedef enum hw_result {
    HW_OK                       = 0,
    HW_ERROR_INVALID_ARGUMENT   = -1,
    HW_ERROR_DEVICE_NOT_FOUND   = -2,
    HW_ERROR_UNSUPPORTED_FORMAT = -3,
    HW_ERROR_CORE_AUDIO         = -4,
    HW_ERROR_ALREADY_RUNNING    = -5,
    HW_ERROR_NOT_RUNNING        = -6,
    HW_ERROR_NOT_IMPLEMENTED    = -99
} hw_result_t;

// ---------------------------------------------------------------------------
// Device enumeration
// ---------------------------------------------------------------------------

#define HW_DEVICE_ID_MAX_LEN   128  // Core Audio HAL UID (UTF-8, null-terminated)
#define HW_DEVICE_NAME_MAX_LEN 128  // Human-readable name (UTF-8, null-terminated)

typedef struct hw_device_info {
    char    device_id[HW_DEVICE_ID_MAX_LEN];
    char    display_name[HW_DEVICE_NAME_MAX_LEN];
    int32_t input_channel_count;
    int32_t output_channel_count;
    double  native_sample_rate;
} hw_device_info_t;

// Enumerates all audio devices visible to Core Audio. On success, writes a
// framework-allocated array to *out_devices and its length to *out_count.
// Release the array with hw_free_device_list.
//
// Returns HW_OK, HW_ERROR_INVALID_ARGUMENT, or HW_ERROR_CORE_AUDIO.
hw_result_t hw_enumerate_devices(
    hw_device_info_t** out_devices,
    int32_t*           out_count
);

// Releases memory allocated by hw_enumerate_devices. No-op on NULL.
void hw_free_device_list(hw_device_info_t* devices);

// ---------------------------------------------------------------------------
// Capture
// ---------------------------------------------------------------------------

// Opaque capture handle. Returned by hw_open_capture; NULL indicates failure.
typedef struct hw_capture hw_capture_t;

// Realtime callback invoked from the Core Audio HAL thread.
//
//   samples         Interleaved signed 24-bit PCM, sign-extended into int32_t.
//                   Length = frame_count * channel_count int32_t values.
//                   Buffer is valid only for the duration of this call.
//   frame_count     Number of frames (samples per channel) in this buffer.
//                   Targets ~10 ms @ 96 kHz = 960 frames, but may vary.
//   channel_count   Number of interleaved channels in this buffer.
//   host_time_ns    AudioTimeStamp.mHostTime converted to nanoseconds.
//                   Use for cross-buffer alignment in Phase 2 sweep capture.
//   user_data       Opaque pointer passed to hw_start_capture.
//
// This function MUST NOT allocate, lock, log, or otherwise block.
typedef void (*hw_capture_callback_t)(
    const int32_t* samples,
    int32_t        frame_count,
    int32_t        channel_count,
    uint64_t       host_time_ns,
    void*          user_data
);

// Opens a capture stream on `device_id`, binding to `channel` (1-based).
// Phase 1 validates sample_rate == 96000.0 and bit_depth == 24.
//
// Returns a non-NULL handle on success. On failure returns NULL; if
// out_error is non-NULL it receives the hw_result_t error code.
hw_capture_t* hw_open_capture(
    const char*  device_id,
    int32_t      channel,
    double       sample_rate,
    int32_t      bit_depth,
    hw_result_t* out_error
);

// Begins delivering audio to `callback`. Returns HW_OK on success, or
// HW_ERROR_ALREADY_RUNNING if the stream is already started.
hw_result_t hw_start_capture(
    hw_capture_t*         handle,
    hw_capture_callback_t callback,
    void*                 user_data
);

// Stops delivery. Returns HW_OK, or HW_ERROR_NOT_RUNNING if not started.
hw_result_t hw_stop_capture(hw_capture_t* handle);

// Stops (if running) and releases the capture handle. No-op on NULL.
void hw_close_capture(hw_capture_t* handle);

// ---------------------------------------------------------------------------
// Output (Phase 2 surface; Phase 1 ships only the calibration tone)
// ---------------------------------------------------------------------------

// Plays a steady sine tone on the default output device for playback-chain
// verification during calibration. Amplitude is a linear dBFS scalar in
// [0.0, 1.0]. Call again with amplitude=0.0 to stop.
//
// Phase 2 will add full output-side functions (swept sine, pink noise) on
// the same device handle as capture — see CLAUDE.md §9 "Same-device sync
// requirement".
hw_result_t hw_generate_calibration_tone(double frequency_hz, double amplitude);

#ifdef __cplusplus
}
#endif

#endif // HAUPTWERK_AUDIO_H
