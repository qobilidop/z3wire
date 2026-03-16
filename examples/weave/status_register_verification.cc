// Can the device reach a bad state?
//
// A common hardware verification question: given the register layout and
// some design rules, can the device end up in an invalid configuration?
// Z3Wire + Weave let you answer this symbolically — checking all possible
// states at once, not just the ones you thought to test.
//
// Scenario: A device has a StatusRegister with a "ready" flag, an operating
// mode, error info, and four counters. The design rules say:
//
//   Rule 1: If mode is SLEEP, all counters must be zero.
//   Rule 2: If any counter is nonzero, ready must be true.
//   Rule 3: If fatal error is set, mode must be IDLE.
//
// We ask Z3: can these rules be satisfied simultaneously? And if a new
// engineer adds a fourth rule, does it create a contradiction?

#include <iostream>

#include <z3++.h>

#include "examples/weave/status_register.h"

using namespace example;

// Helper: assert an implication (p → q).
void imply(z3::solver& solver, const z3w::Bool& p, const z3w::Bool& q) {
  solver.add((!p || q).raw());
}

int main() {
  z3::context ctx;
  z3::solver solver(ctx);

  auto reg = StatusRegisterSymbolic::Create(ctx, "reg");

  // --- Design rules ---

  auto mode_is_sleep =
      z3w::exact_eq(reg.mode, z3w::to_symbolic(OpMode::kSleep, ctx));
  auto mode_is_idle =
      z3w::exact_eq(reg.mode, z3w::to_symbolic(OpMode::kIdle, ctx));

  // Rule 1: SLEEP → all counters zero.
  for (int i = 0; i < 4; ++i) {
    auto counter_zero = z3w::Bool(reg.counters[i].raw() == ctx.bv_val(0, 4));
    imply(solver, mode_is_sleep, counter_zero);
  }

  // Rule 2: counter nonzero → ready.
  for (int i = 0; i < 4; ++i) {
    auto counter_nonzero = z3w::Bool(reg.counters[i].raw() != ctx.bv_val(0, 4));
    imply(solver, counter_nonzero, reg.ready);
  }

  // Rule 3: fatal → IDLE.
  imply(solver, reg.error.fatal, mode_is_idle);

  // --- Query 1: Can mode=ACTIVE with fatal=true? ---
  std::cout << "Query 1: Can mode=ACTIVE with fatal=true?\n";
  {
    solver.push();
    auto mode_is_active =
        z3w::exact_eq(reg.mode, z3w::to_symbolic(OpMode::kActive, ctx));
    solver.add(mode_is_active.raw());
    solver.add(reg.error.fatal.raw());

    if (solver.check() == z3::sat) {
      std::cout << "  Bug: design rules allow this state!\n";
      auto model = solver.get_model();
      auto concrete = reg.ToConcrete(model);
      std::cout << "  Counter-example: ready=" << concrete.ready
                << " mode=" << concrete.mode.value()
                << " fatal=" << concrete.error.fatal << "\n";
    } else {
      std::cout << "  Safe: rules prevent ACTIVE mode with fatal error.\n";
    }
    solver.pop();
  }

  // --- Query 2: Can counters be active while device is not ready? ---
  std::cout << "\nQuery 2: Can a counter be nonzero with ready=false?\n";
  {
    solver.push();
    solver.add((!reg.ready).raw());
    auto any_counter_active =
        z3w::Bool(reg.counters[0].raw() != ctx.bv_val(0, 4) ||
                  reg.counters[1].raw() != ctx.bv_val(0, 4) ||
                  reg.counters[2].raw() != ctx.bv_val(0, 4) ||
                  reg.counters[3].raw() != ctx.bv_val(0, 4));
    solver.add(any_counter_active.raw());

    if (solver.check() == z3::sat) {
      std::cout << "  Bug: active counters with ready=false!\n";
    } else {
      std::cout << "  Safe: rules guarantee ready=true when counters are "
                   "active.\n";
    }
    solver.pop();
  }

  // --- Query 3: Adding a contradictory rule ---
  std::cout << "\nQuery 3: Adding rule 'fatal → SLEEP'. Contradiction?\n";
  {
    solver.push();
    auto mode_is_sleep2 =
        z3w::exact_eq(reg.mode, z3w::to_symbolic(OpMode::kSleep, ctx));
    // Rule 4: fatal → SLEEP.
    imply(solver, reg.error.fatal, mode_is_sleep2);
    // Can fatal ever be true?
    solver.add(reg.error.fatal.raw());

    if (solver.check() == z3::sat) {
      std::cout << "  Possible. Fatal with SLEEP is allowed.\n";
    } else {
      std::cout << "  Contradiction! Rules 3 + 4 make fatal errors "
                   "impossible.\n";
      std::cout << "  (Rule 3: fatal→IDLE, Rule 4: fatal→SLEEP, but "
                   "IDLE!=SLEEP)\n";
    }
    solver.pop();
  }

  return 0;
}
