# Examples

Each example is a self-contained C++ program with comments explaining what it
does and how to run it.

| Example                                                                            | Description                                                              |
| ---------------------------------------------------------------------------------- | ------------------------------------------------------------------------ |
| [adder.cc](adder.cc)                                                               | Prove a gate-level ripple-carry adder matches intended semantics         |
| [multiplier.cc](multiplier.cc)                                                     | Prove a gate-level array multiplier matches intended semantics           |
| [barrel_shifter.cc](barrel_shifter.cc)                                             | Prove a gate-level barrel shifter matches intended semantics             |
| [midpoint_overflow.cc](midpoint_overflow.cc)                                       | Prove the classic binary search midpoint bug and verify the bit-hack fix |
| [packet_gen.cc](packet_gen.cc)                                                     | Generate valid IPv4/IPv6 packets using symbolic constraints              |
| [packet_view.cc](packet_view.cc)                                                   | Structured view over a packet buffer with nested header sub-views        |
| [symbolic_execution/exact_match_table.cc](symbolic_execution/exact_match_table.cc) | Enumerate paths through an exact-match table with a symbolic input       |
