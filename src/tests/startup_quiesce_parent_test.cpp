#include "config_handler.hpp"
#include "gpio_output.hpp"
#include "scheduling.hpp"
#include <cstdlib>
#include <iostream>

namespace { void require(bool v, const char *m) { if (!v) { std::cerr << m << '\n'; std::exit(1); } }
ArgParserConfig cfg_for(EnableOnBootBehavior policy) { init_default_config(); auto cfg = config; cfg.use_ini = true; cfg.transmit = true; cfg.enable_on_boot = policy; cfg.use_led = true; cfg.led_pin = 5; config = cfg; return cfg; }
void reset() { reset_startup_quiesce_for_test(); GPIOOutput::setTestMode(true); set_scheduler_execution_suppressed_for_test(true); reset_tx_led_request_counts_for_test(); }
}
int main() {
 for (auto policy : {EnableOnBootBehavior::Never, EnableOnBootBehavior::Follow, EnableOnBootBehavior::Always}) { reset(); int calls=0; set_startup_quiesce_invoker_for_test([&calls]{ ++calls; return wsprrypi::StartupQuiesceResult{true,{}}; }); auto cfg=cfg_for(policy); require(run_startup_quiesce_gate_for_test(cfg), "success gate"); require(calls==1, "one quiesce per policy"); require(!startup_quiesce_inhibited_for_test(), "success not inhibited"); require(tx_led_deassert_request_count_for_test()>0, "outputs deasserted"); }
 reset(); int calls=0; set_startup_quiesce_invoker_for_test([&calls]{ ++calls; return wsprrypi::StartupQuiesceResult{false,"fake quiesce failure"}; }); auto cfg=cfg_for(EnableOnBootBehavior::Follow); const bool tx=cfg.transmit; const auto policy=cfg.enable_on_boot; require(!run_startup_quiesce_gate_for_test(cfg), "failure gate"); require(calls==1, "one failed quiesce"); require(startup_quiesce_inhibited_for_test(), "failure inhibited"); require(startup_quiesce_error_for_test()=="fake quiesce failure", "error retained"); require(cfg.transmit==tx && cfg.enable_on_boot==policy, "policy preserved"); require(start_non_wspr_transmission_now_for_test(cfg), "blocked launch returns safely"); require(!current_controller_request_for_test().has_value(), "manual launch must not configure transmitter"); require(startup_quiesce_inhibited_for_test(), "latch retained"); reset_startup_quiesce_for_test(); GPIOOutput::setTestMode(false); set_scheduler_execution_suppressed_for_test(false); std::cout << "startup_quiesce_parent_test passed\n"; }
