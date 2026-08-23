/* SPDX-License-Identifier: (GPL-2.0-only WITH Linux-syscall-note) OR MIT */
#ifndef _UAPI_LINUX_RP1_GPCLK_H
#define _UAPI_LINUX_RP1_GPCLK_H

#include <linux/ioctl.h>
#include <linux/types.h>

#define RP1_GPCLK_UAPI_ABI_V1 1U
#define RP1_GPCLK_UAPI_ABI_V2 2U
#define RP1_GPCLK_IOC_MAGIC 0xb8

#define RP1_GPCLK_MODULE_ID_MAX 64U
#define RP1_GPCLK_BUILD_ID_MAX 64U
#define RP1_GPCLK_COMPAT_ID_MAX 64U

#define RP1_GPCLK_MAX_TONES 4U
#define RP1_GPCLK_WSPR_SYMBOLS 162U
#define RP1_GPCLK_MAX_EVENTS 512U
#define RP1_GPCLK_WSPR_WRITES_PER_SYMBOL_MAX 66792U
#define RP1_GPCLK_DITHER_PERIOD_MAX 66792U
#define RP1_GPCLK_FRACTIONAL_BITS 16U
#define RP1_GPCLK_TICK_DIVIDER 511U
#define RP1_GPCLK_EVENT_DURATION_NS_MIN 1ULL
#define RP1_GPCLK_EVENT_DURATION_NS_MAX 120000000000ULL
#define RP1_GPCLK_REQUEST_DURATION_NS_MAX 120000000000ULL
#define RP1_GPCLK_TONE_DURATION_NS_MIN 1000000ULL
#define RP1_GPCLK_TONE_DURATION_NS_MAX 120000000000ULL
#define RP1_GPCLK_TONE_CONTINUOUS_CHUNK_NS 1000000000ULL

struct rp1_gpclk_uapi_header {
    __u16 size;
    __u16 version;
    __u32 flags;
};

enum rp1_gpclk_route {
    RP1_GPCLK_ROUTE_INVALID = 0,
    RP1_GPCLK_ROUTE_GPIO4 = 1,
    RP1_GPCLK_ROUTE_GPIO20 = 2,
};

enum rp1_gpclk_mode {
    RP1_GPCLK_MODE_INVALID = 0,
    RP1_GPCLK_MODE_WSPR = 1,
    RP1_GPCLK_MODE_QRSS = 2,
    RP1_GPCLK_MODE_FSKCW = 3,
    RP1_GPCLK_MODE_DFCW = 4,
    RP1_GPCLK_MODE_TONE = 5,
};

enum rp1_gpclk_tone_operation {
    RP1_GPCLK_TONE_OPERATION_INVALID = 0,
    RP1_GPCLK_TONE_OPERATION_CONTINUOUS = 1,
    RP1_GPCLK_TONE_OPERATION_FINITE = 2,
};

enum rp1_gpclk_compatibility_state {
    RP1_GPCLK_COMPAT_INVALID = 0,
    RP1_GPCLK_COMPAT_QUALIFIED = 1,
    RP1_GPCLK_COMPAT_EXPERIMENTAL = 2,
    RP1_GPCLK_COMPAT_COMPATIBLE_UNQUALIFIED = 3,
    RP1_GPCLK_COMPAT_UNAVAILABLE = 4,
    RP1_GPCLK_COMPAT_REJECTED = 5,
};

enum rp1_gpclk_compatibility_reason {
    RP1_GPCLK_COMPAT_REASON_NONE = 0,
    RP1_GPCLK_COMPAT_REASON_MANIFEST_MISSING = 1,
    RP1_GPCLK_COMPAT_REASON_IDENTITY_UNKNOWN = 2,
    RP1_GPCLK_COMPAT_REASON_IDENTITY_MISMATCH = 3,
    RP1_GPCLK_COMPAT_REASON_BUILD_UNSUPPORTED = 4,
    RP1_GPCLK_COMPAT_REASON_SIGNATURE_REJECTED = 5,
    RP1_GPCLK_COMPAT_REASON_RESOURCE_UNAVAILABLE = 6,
    RP1_GPCLK_COMPAT_REASON_RESOURCE_CONFLICT = 7,
    RP1_GPCLK_COMPAT_REASON_SELF_TEST_FAILED = 8,
    RP1_GPCLK_COMPAT_REASON_CLEANUP_LATCHED = 9,
    RP1_GPCLK_COMPAT_REASON_ADMIN_ENROLLMENT_REQUIRED = 10,
};

enum rp1_gpclk_state {
    RP1_GPCLK_STATE_IDLE = 0,
    RP1_GPCLK_STATE_RUNNING = 1,
    RP1_GPCLK_STATE_DRAINING = 2,
    RP1_GPCLK_STATE_COMPLETE = 3,
    RP1_GPCLK_STATE_FAILED = 4,
    RP1_GPCLK_STATE_DEAD = 5,
};

enum rp1_gpclk_terminal_reason {
    RP1_GPCLK_REASON_NONE = 0,
    RP1_GPCLK_REASON_COMPLETE = 1,
    RP1_GPCLK_REASON_STOPPED = 2,
    RP1_GPCLK_REASON_OWNER_CLOSED = 3,
    RP1_GPCLK_REASON_PROVIDER_REMOVED = 4,
    RP1_GPCLK_REASON_DEADLINE_MISSED = 5,
    RP1_GPCLK_REASON_INVALID_REQUEST = 6,
    RP1_GPCLK_REASON_RESOURCE_UNAVAILABLE = 7,
    RP1_GPCLK_REASON_STARTUP_CONFLICT = 8,
    RP1_GPCLK_REASON_DMA_FAILED = 9,
    RP1_GPCLK_REASON_CLOCK_FAILED = 10,
    RP1_GPCLK_REASON_PINCTRL_FAILED = 11,
    RP1_GPCLK_REASON_READBACK_FAILED = 12,
    RP1_GPCLK_REASON_CLEANUP_FAILED = 13,
    RP1_GPCLK_REASON_COMPATIBILITY_REJECTED = 14,
    RP1_GPCLK_REASON_INTERNAL_ERROR = 15,
};

#define RP1_GPCLK_CAP_SUBMIT_WSPR (1ULL << 0)
#define RP1_GPCLK_CAP_SUBMIT_EVENTS (1ULL << 1)
#define RP1_GPCLK_CAP_STOP_DRAIN (1ULL << 2)
#define RP1_GPCLK_CAP_STABLE_STATE (1ULL << 3)
#define RP1_GPCLK_CAP_ROUTE_IDENTITY (1ULL << 4)
#define RP1_GPCLK_CAP_COMPAT_IDENTITY (1ULL << 5)
#define RP1_GPCLK_CAP_CLEANUP_FAULT_LATCH (1ULL << 6)
#define RP1_GPCLK_CAP_LIVE_ELIGIBLE (1ULL << 7)
#define RP1_GPCLK_CAP_TONE_CONTINUOUS (1ULL << 8)
#define RP1_GPCLK_CAP_TONE_FINITE (1ULL << 9)

#define RP1_GPCLK_DRIVE_MA_2 2U
#define RP1_GPCLK_DRIVE_MA_4 4U
#define RP1_GPCLK_DRIVE_MA_8 8U
#define RP1_GPCLK_DRIVE_MA_12 12U
#define RP1_GPCLK_DRIVE_SUPPORT_2_MA (1U << 0)
#define RP1_GPCLK_DRIVE_SUPPORT_4_MA (1U << 1)
#define RP1_GPCLK_DRIVE_SUPPORT_8_MA (1U << 2)
#define RP1_GPCLK_DRIVE_SUPPORT_12_MA (1U << 3)
#define RP1_GPCLK_DRIVE_SUPPORT_ALLOWED_MASK \
    (RP1_GPCLK_DRIVE_SUPPORT_2_MA | RP1_GPCLK_DRIVE_SUPPORT_4_MA | \
     RP1_GPCLK_DRIVE_SUPPORT_8_MA | RP1_GPCLK_DRIVE_SUPPORT_12_MA)

struct rp1_gpclk_query_v1 {
    struct rp1_gpclk_uapi_header header;
    __u16 abi_min;
    __u16 abi_max;
    __u32 route;
    __u32 compatibility_state;
    __u32 compatibility_reason;
    __u32 reserved0;
    __aligned_u64 capabilities;
    __u32 max_tones;
    __u32 wspr_symbols;
    __u32 max_events;
    __u32 max_dither_period;
    __u32 supported_drive_ma_mask;
    __u32 reserved1;
    __aligned_u64 max_event_duration_ns;
    __aligned_u64 max_request_duration_ns;
    char module_id[RP1_GPCLK_MODULE_ID_MAX];
    char build_id[RP1_GPCLK_BUILD_ID_MAX];
    char compatibility_id[RP1_GPCLK_COMPAT_ID_MAX];
    __aligned_u64 reserved[4];
};

struct rp1_gpclk_query_v2 {
    struct rp1_gpclk_uapi_header header;
    __u16 abi_min;
    __u16 abi_max;
    __u32 route;
    __u32 compatibility_state;
    __u32 compatibility_reason;
    __u32 reserved0;
    __aligned_u64 capabilities;
    __u32 max_tones;
    __u32 wspr_symbols;
    __u32 max_events;
    __u32 max_dither_period;
    __u32 supported_drive_ma_mask;
    __u32 reserved1;
    __aligned_u64 max_event_duration_ns;
    __aligned_u64 max_request_duration_ns;
    __aligned_u64 min_tone_duration_ns;
    __aligned_u64 max_tone_duration_ns;
    char module_id[RP1_GPCLK_MODULE_ID_MAX];
    char build_id[RP1_GPCLK_BUILD_ID_MAX];
    char compatibility_id[RP1_GPCLK_COMPAT_ID_MAX];
    __aligned_u64 reserved[4];
};

struct rp1_gpclk_acquire_v1 {
    struct rp1_gpclk_uapi_header header;
    __u32 expected_route;
    __u32 reserved0;
    __aligned_u64 required_capabilities;
    __aligned_u64 lease_id;
    __aligned_u64 reserved[4];
};

struct rp1_gpclk_tone_v1 {
    __aligned_u64 lower_divider_q16;
    __aligned_u64 upper_divider_q16;
    __u32 lower_count;
    __u32 upper_count;
};

struct rp1_gpclk_submit_wspr_v1 {
    struct rp1_gpclk_uapi_header header;
    __aligned_u64 lease_id;
    __aligned_u64 generation;
    __aligned_u64 tones_ptr;
    __aligned_u64 symbols_ptr;
    __u32 fractional_bits;
    __u32 tick_divider;
    __u32 writes_per_symbol;
    __u32 tone_count;
    __u32 symbol_count;
    __u32 drive_ma;
    __u32 reserved0;
    __u32 reserved1;
    __aligned_u64 expected_frame_duration_ns;
    __aligned_u64 reserved[4];
};

#define RP1_GPCLK_EVENT_F_OUTPUT_ENABLED (1U << 0)

struct rp1_gpclk_event_v1 {
    __aligned_u64 duration_ns;
    __u16 tone_index;
    __u16 flags;
    __u32 reserved0;
};

struct rp1_gpclk_submit_events_v1 {
    struct rp1_gpclk_uapi_header header;
    __aligned_u64 lease_id;
    __aligned_u64 generation;
    __aligned_u64 tones_ptr;
    __aligned_u64 events_ptr;
    __u32 mode;
    __u32 fractional_bits;
    __u32 tick_divider;
    __u32 tone_count;
    __u32 event_count;
    __u32 drive_ma;
    __u32 reserved0;
    __u32 reserved1;
    __aligned_u64 total_duration_ns;
    __aligned_u64 reserved[4];
};

struct rp1_gpclk_submit_tone_v2 {
    struct rp1_gpclk_uapi_header header;
    __aligned_u64 lease_id;
    __aligned_u64 generation;
    struct rp1_gpclk_tone_v1 tone;
    __aligned_u64 duration_ns;
    __u32 operation;
    __u32 expected_route;
    __u32 fractional_bits;
    __u32 tick_divider;
    __u32 drive_ma;
    __u32 reserved0;
    __aligned_u64 reserved[4];
};

struct rp1_gpclk_stop_v1 {
    struct rp1_gpclk_uapi_header header;
    __aligned_u64 lease_id;
    __aligned_u64 generation;
    __aligned_u64 reserved[4];
};

struct rp1_gpclk_state_v1 {
    struct rp1_gpclk_uapi_header header;
    __aligned_u64 lease_id;
    __aligned_u64 generation;
    __u32 state;
    __u32 terminal_reason;
    __u32 current_event;
    __u32 cleanup_fault;
    __aligned_u64 elapsed_ns;
    __aligned_u64 remaining_ns;
    __aligned_u64 reserved[4];
};

struct rp1_gpclk_release_v2 {
    struct rp1_gpclk_uapi_header header;
    __aligned_u64 lease_id;
    __aligned_u64 generation;
    __aligned_u64 reserved[4];
};

struct rp1_gpclk_release_v1 {
    struct rp1_gpclk_uapi_header header;
    __aligned_u64 lease_id;
    __aligned_u64 reserved[4];
};

#define RP1_GPCLK_IOC_QUERY \
    _IOWR(RP1_GPCLK_IOC_MAGIC, 0x20, struct rp1_gpclk_query_v1)
#define RP1_GPCLK_IOC_ACQUIRE \
    _IOWR(RP1_GPCLK_IOC_MAGIC, 0x21, struct rp1_gpclk_acquire_v1)
#define RP1_GPCLK_IOC_SUBMIT_WSPR \
    _IOWR(RP1_GPCLK_IOC_MAGIC, 0x22, struct rp1_gpclk_submit_wspr_v1)
#define RP1_GPCLK_IOC_SUBMIT_EVENTS \
    _IOWR(RP1_GPCLK_IOC_MAGIC, 0x23, struct rp1_gpclk_submit_events_v1)
#define RP1_GPCLK_IOC_STOP \
    _IOW(RP1_GPCLK_IOC_MAGIC, 0x24, struct rp1_gpclk_stop_v1)
#define RP1_GPCLK_IOC_GET_STATE \
    _IOWR(RP1_GPCLK_IOC_MAGIC, 0x25, struct rp1_gpclk_state_v1)
#define RP1_GPCLK_IOC_RELEASE \
    _IOW(RP1_GPCLK_IOC_MAGIC, 0x26, struct rp1_gpclk_release_v1)
#define RP1_GPCLK_IOC_QUERY_V2 \
    _IOWR(RP1_GPCLK_IOC_MAGIC, 0x27, struct rp1_gpclk_query_v2)
#define RP1_GPCLK_IOC_SUBMIT_TONE_V2 \
    _IOWR(RP1_GPCLK_IOC_MAGIC, 0x28, struct rp1_gpclk_submit_tone_v2)
#define RP1_GPCLK_IOC_RELEASE_V2 \
    _IOW(RP1_GPCLK_IOC_MAGIC, 0x29, struct rp1_gpclk_release_v2)

#endif /* _UAPI_LINUX_RP1_GPCLK_H */
