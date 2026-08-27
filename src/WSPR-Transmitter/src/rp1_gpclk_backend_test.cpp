#include "rp1_gpclk_backend.hpp"
#include "rp1_gpclk_uapi.h"

#include <iostream>
#include <string>
#include <vector>

namespace {
int failures = 0;
void expect(bool value, const char* message) { if (!value) { std::cerr << "FAIL: " << message << '\n'; ++failures; } }

class Provider final : public wsprrypi::Rp1GpclkProvider {
public:
 bool query(std::uint32_t, std::uint64_t, bool, wsprrypi::Rp1GpclkProviderIdentity&, std::string&) override { return true; }
 bool acquire(std::uint32_t route, std::uint64_t caps,
     const std::array<std::uint8_t,32>&, std::string&) override { routes.push_back(route); capabilities.push_back(caps); next_generation=1; return acquire_ok; }
 bool submit(wsprrypi::Rp1GpclkProviderProgram& p, std::string&) override { p.generation=next_generation++; programs.push_back(p); current = wsprrypi::Rp1GpclkCompletionState::running; return submit_ok; }
 bool submitEvents(wsprrypi::Rp1GpclkProviderEventProgram& p, std::string&) override { p.generation=next_generation++; event_programs.push_back(p); current = wsprrypi::Rp1GpclkCompletionState::running; return submit_ok; }
 bool submitTone(wsprrypi::Rp1GpclkProviderToneProgram& p, std::string&) override { p.generation=next_generation++; tone_programs.push_back(p); current = wsprrypi::Rp1GpclkCompletionState::running; return submit_ok; }
 bool requestFiniteStop(std::uint64_t g, std::string&) override { stops.push_back(g); current = wsprrypi::Rp1GpclkCompletionState::draining; return true; }
 wsprrypi::Rp1GpclkCompletionState state(std::uint64_t) const noexcept override { return current; }
 wsprrypi::Rp1GpclkProviderEventState eventState(std::uint64_t) const noexcept override { return {current,0,0}; }
 bool release(std::string&) noexcept override { ++releases; return release_ok; }
 bool acquire_ok{true}, submit_ok{true}, release_ok{true}; int releases{0};
 std::uint64_t next_generation{1};
 wsprrypi::Rp1GpclkCompletionState current{wsprrypi::Rp1GpclkCompletionState::idle};
 std::vector<std::uint32_t> routes; std::vector<std::uint64_t> capabilities; std::vector<std::uint64_t> stops;
 std::vector<wsprrypi::Rp1GpclkProviderProgram> programs;
 std::vector<wsprrypi::Rp1GpclkProviderEventProgram> event_programs;
 std::vector<wsprrypi::Rp1GpclkProviderToneProgram> tone_programs;
};

bool prepare(wsprrypi::Rp1GpclkBackend& backend, std::uint32_t drive, std::string& error)
{ std::array<std::uint8_t,32> digest{}; digest[0]=1; return backend.prepare(drive, RP1_GPCLK_ROUTE_GPIO4, RP1_GPCLK_CAP_LIVE_ELIGIBLE, digest, error); }

wsprrypi::Rp1GpclkPlan plan() {
 wsprrypi::Rp1GpclkPlannerInput in; in.center_frequency_hz=14097100; in.tone_spacing_hz=1.46484375; in.parent_frequency_hz=50000000; in.dither_sequence_length=66792;
 return wsprrypi::planRp1GpclkWspr(in).plan;
}

void test_drive_profiles() {
 for (auto drive : {2u,4u,8u,12u}) { Provider p; wsprrypi::Rp1GpclkBackend b(p); std::string e; expect(prepare(b,drive,e), "supported drive must prepare"); expect(b.cleanup(e), "idle backend must clean up"); }
 for (auto drive : {0u,6u,10u,16u}) { Provider p; wsprrypi::Rp1GpclkBackend b(p); std::string e; expect(!prepare(b,drive,e) && p.routes.empty(), "unsupported drive must be rejected before provider"); }
 expect(wsprrypi::Rp1GpclkBackend::kDefaultDriveMa==2, "default drive must be 2 mA");
}

void test_program_and_finite_stop() {
 Provider p; wsprrypi::Rp1GpclkBackend b(p); std::string e; auto planned=plan();
 expect(prepare(b,2,e), "prepare must acquire provider");
 std::array<std::uint8_t,162> symbols{}; for (std::size_t i=0;i<symbols.size();++i) symbols[i]=i%4;
 {
  expect(b.emitFrame(planned,symbols,e), "complete frame must submit"); auto program=p.programs.back();
  expect(program.writes_per_symbol==66792 && program.tick_divider==511, "production timing constants must be preserved");
  expect(program.symbols.size()==162 && program.fractional_bits==16, "provider must receive exactly 162 symbols");
  for (std::size_t i=0;i<symbols.size();++i) expect(program.symbols[i]==symbols[i], "provider must preserve symbol order");
  for (std::size_t tone=0;tone<4;++tone) expect(program.tones[tone].lower_divider_word==planned.tones[tone].lower_divider_word && program.tones[tone].upper_divider_word==planned.tones[tone].upper_divider_word, "provider must preserve unpacked tone words");
  expect(b.cancel(e), "cancel must request finite stop");
  expect(!b.cleanup(e), "cleanup must not release a draining descriptor");
  p.current=wsprrypi::Rp1GpclkCompletionState::complete;
  expect(b.cleanup(e), "cleanup must release after finite completion");
 }
 expect(p.releases==1 && p.stops.size()==1, "frame must stop and release exactly once");
}

void test_timeout_and_generation() {
 Provider p; wsprrypi::Rp1GpclkBackend b(p); std::string e; auto planned=plan(); std::array<std::uint8_t,162> symbols{}; prepare(b,2,e); b.emitFrame(planned,symbols,e);
 const auto first=b.generation(); expect(b.timedOut(e), "timeout must use finite-stop path"); expect(p.stops.back()==first, "timeout must identify active generation");
 p.current=wsprrypi::Rp1GpclkCompletionState::complete; b.cleanup(e); prepare(b,2,e); symbols.fill(1); b.emitFrame(planned,symbols,e);
 expect(b.generation()==first, "a new lease must accept its provider-owned generation sequence"); p.current=wsprrypi::Rp1GpclkCompletionState::failed; expect(b.cleanup(e), "failed provider generation must still release ownership");
}

void test_event_program_submission() {
 Provider p; wsprrypi::Rp1GpclkBackend b(p); std::string e;
 wsprrypi::Rp1GpclkProviderEventProgram program; program.fractional_bits=16; program.tick_divider=511; program.total_duration_ns=10;
 program.tones.push_back({1,2,1,1}); program.events.push_back({10,0,true});
 expect(prepare(b,2,e) && b.emitEvents(program,e), "finite event program must submit");
 expect(p.event_programs.size()==1 && p.event_programs[0].generation==1 &&
  p.event_programs[0].drive_ma==2,
  "event submission must bind generation and prepared drive");
 p.current=wsprrypi::Rp1GpclkCompletionState::complete;
 expect(b.cleanup(e), "completed event program must release");
}

void test_release_failure_is_not_hidden() {
 Provider p; p.release_ok=false; wsprrypi::Rp1GpclkBackend b(p); std::string e;
 expect(prepare(b,2,e), "release-failure fixture must acquire");
 expect(!b.cleanup(e), "release failure must keep cleanup failed closed");
}

void test_tone_submission() {
 Provider p; wsprrypi::Rp1GpclkBackend b(p); std::string e;
 wsprrypi::Rp1GpclkProviderToneProgram tone; tone.tone={1,2,1,1};
 tone.operation=RP1_GPCLK_TONE_OPERATION_FINITE; tone.duration_ns=1000000000ULL;
 expect(prepare(b,2,e) && b.emitTone(tone,e), "ABI v2 TONE must submit");
 expect(p.tone_programs.size()==1 && p.tone_programs[0].generation==1 &&
  p.tone_programs[0].drive_ma==2, "TONE submission must bind generation and drive");
 p.current=wsprrypi::Rp1GpclkCompletionState::complete;
 expect(b.cleanup(e), "completed TONE must release");
}
}

int main() { test_drive_profiles(); test_program_and_finite_stop(); test_timeout_and_generation(); test_event_program_submission(); test_release_failure_is_not_hidden(); test_tone_submission(); if (failures) return 1; std::cout << "RP1 GPCLK production backend contract tests passed\n"; }
