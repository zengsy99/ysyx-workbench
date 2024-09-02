// DESCRIPTION: Verilator: Verilog example module
//
// This file ONLY is placed under the Creative Commons Public Domain, for
// any use, without warranty, 2017 by Wilson Snyder.
// SPDX-License-Identifier: CC0-1.0

// See also https://verilator.org/guide/latest/examples.html"

module top(
   input a,
   input b,
   output f
);

   assign f = a ^ b;

   // Only simulate one time -> gotFinish() will return true
   initial begin
      $display("Finish test!");
      $finish;
   end

endmodule
