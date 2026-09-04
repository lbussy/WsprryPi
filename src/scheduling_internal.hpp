/**
 * @file scheduling_internal.hpp
 * @brief Private cross-unit scheduler state and helper contracts.
 */

#ifndef SCHEDULING_INTERNAL_HPP
#define SCHEDULING_INTERNAL_HPP

#include "scheduling.hpp"
#include "band_gpio_selector.hpp"
#include "system_clock_frequency_estimate.hpp"

struct BandGPIOResolution
{
    HamBand band = HamBand::BAND_2200M;
    BandGPIOConfig config{};
    bool selector_enabled = false;
    bool from_band_config = false;
    const char *selector_source = "none";
    bool band_known = true;
};

enum class BandGPIOPrepareStatus
{
    Inactive,
    Prepared,
    Failed
};

extern BandGPIOSelector bandGPIOSelector;
extern std::mutex set_config_mtx;
extern int freq_iterator;
extern double current_dial_frequency;
extern WsprFrequencyEntry current_frequency_entry;
extern TransmissionRequest current_transmission_request;
extern bool suppress_scheduler_execution_for_test;
extern bool selector_gpio_control_enabled;
extern bool selector_gpio_drive_enabled;
extern std::atomic<bool> shutdown_after_wspr_plan;
extern std::atomic<std::uint64_t> non_wspr_schedule_generation;
extern PreparedWsprTransmission active_wspr_plan;
extern std::size_t active_wspr_frame_index;
extern double active_wspr_plan_dial_frequency;
extern WsprFrequencyEntry active_wspr_plan_frequency_entry;
extern bool active_wspr_plan_in_progress;

BandGPIOPrepareStatus prepare_band_gpio_for_frequency_or_log(
    double, const WsprFrequencyEntry &, const ArgParserConfig &, int = -1,
    BandGPIOResolution * = nullptr);
bool sync_configured_selector_gpio_idle_state(
    const ArgParserConfig &, bool, std::string * = nullptr);
void commit_band_gpio_snapshot_to_request(
    TransmissionRequest &, const BandGPIOResolution &,
    BandGPIOPrepareStatus) noexcept;
bool stop_active_transmission_selectors(
    const ArgParserConfig * = nullptr, bool = false,
    std::string * = nullptr) noexcept;
void release_idle_selector_gpio_reservations() noexcept;
bool has_configured_selector_gpios(const ArgParserConfig &) noexcept;
void deassert_transmit_gpio_outputs(
    const ArgParserConfig *, bool, const char *) noexcept;
void reset_active_wspr_plan_state();
bool is_managed_persistent_mode() noexcept;
bool is_non_wspr_runtime_mode(ModeType) noexcept;
bool has_non_wspr_cli_startup_request(ModeType) noexcept;
void log_scheduler_path_selection(ModeType);
void log_transmit_disabled_skip(const ArgParserConfig &);
bool runtime_transmit_requested(const ArgParserConfig &) noexcept;
bool runtime_transmit_enabled(const ArgParserConfig &) noexcept;
bool runtime_transmit_preparation_enabled(const ArgParserConfig &) noexcept;
bool managed_reload_generation_changed(std::uint64_t) noexcept;
void set_managed_reload_tx_inhibited(bool, std::string_view = {});
WsprFrequencyEntry next_frequency_entry_from(
    const std::vector<WsprFrequencyEntry> &, int &, bool);
double maybe_apply_wspr_random_offset(double, const ArgParserConfig &);
void schedule_next_non_wspr_launch(const ArgParserConfig &);
TransmissionRequest make_skip_window_request(
    const ArgParserConfig &, double, double, const WsprFrequencyEntry &);
bool configure_current_wspr_transmission(
    const ArgParserConfig &, double, double, const WsprFrequencyEntry &,
    PreparedWsprTransmission &, std::size_t &, double &,
    WsprFrequencyEntry &, bool &, double, TransmissionRequest &);
void refresh_frequency_estimate_for_config();
GpioFrequencyCorrection select_and_publish_gpio_correction_for_config(
    const ArgParserConfig &);
void commit_execution_request(const TransmissionRequest &);

#endif // SCHEDULING_INTERNAL_HPP
