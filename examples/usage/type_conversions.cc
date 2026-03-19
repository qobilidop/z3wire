// Snippets demonstrating Z3Wire's type conversion APIs.
//
// Shows casting (safe, checked, unsafe), signedness reinterpretation,
// sort conversion, and concrete/symbolic conversion.
//
// Run: ./dev.sh bazel run //examples/usage:type_conversions

#include <z3++.h>

#include "z3wire/bit_vec.h"
#include "z3wire/bool.h"
#include "z3wire/sym_bit_vec.h"
#include "z3wire/sym_bool.h"

// --- Casting ---

void demo_safe_cast() {
  z3::context ctx;
  z3w::SymUInt<8> x(ctx, "x");
  auto wide = z3w::safe_cast<z3w::SymUInt<16>>(x);  // OK: widening

  // Compile error!
  // auto bad = z3w::safe_cast<z3w::SymUInt<4>>(x);
}

void demo_checked_cast() {
  z3::context ctx;
  z3::solver solver(ctx);
  z3w::SymUInt<16> x(ctx, "x");
  auto [narrow, preserved] = z3w::checked_cast<z3w::SymUInt<8>>(x);

  // Assert in the solver: this cast must never lose data.
  solver.add(preserved.expr());
}

void demo_unsafe_cast() {
  z3::context ctx;
  z3w::SymUInt<16> x(ctx, "x");
  auto narrow = z3w::unsafe_cast<z3w::SymUInt<8>>(x);  // Truncate to 8 bits
  auto wide = z3w::unsafe_cast<z3w::SymUInt<32>>(x);   // Zero-extend to 32 bits
  auto reint = z3w::unsafe_cast<z3w::SymSInt<16>>(x);  // Reinterpret as signed
}

// --- Signedness reinterpretation ---

void demo_signedness() {
  z3::context ctx;
  z3w::SymSInt<8> x(ctx, "x");
  z3w::SymUInt<8> y = z3w::as_unsigned(x);

  z3w::SymUInt<8> a(ctx, "a");
  z3w::SymSInt<8> b = z3w::as_signed(a);
}

// --- Sort conversion ---

void demo_sort_conversion() {
  z3::context ctx;

  // SymUInt<1> to SymBool
  z3w::SymUInt<1> x(ctx, "x");
  z3w::SymBool b = z3w::as_bool(x);

  // SymBool to SymUInt<1>
  z3w::SymBool c(ctx, "c");
  z3w::SymUInt<1> d = z3w::as_uint1(c);
}

// --- Concrete / symbolic conversion ---

void demo_to_symbolic() {
  z3::context ctx;
  z3w::SymUInt<8> x(ctx, "x");
  auto y = z3w::UInt<8>::Literal<42>();

  auto sum = x + y;            // SymUInt<9>
  z3w::SymBool eq = (x == y);  // Symbolic equality check

  // Manual conversion
  z3w::SymUInt<8> sym = z3w::to_symbolic(y, ctx);
  z3w::SymBool sym_flag = z3w::to_symbolic(z3w::Bool(true), ctx);
}

void demo_to_concrete() {
  z3::context ctx;
  z3::solver solver(ctx);
  z3w::SymUInt<8> x(ctx, "x");
  solver.add(x.expr() == 42);
  solver.check();

  z3w::UInt<8> result = z3w::to_concrete(x, solver.get_model());
  // result.value() == 42
}

int main() {
  demo_safe_cast();
  demo_checked_cast();
  demo_unsafe_cast();
  demo_signedness();
  demo_sort_conversion();
  demo_to_symbolic();
  demo_to_concrete();
  return 0;
}
