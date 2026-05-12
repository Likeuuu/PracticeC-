module leaf(a, y);
  input a;
  output y;
  wire leaf_wire;
  assign leaf_wire = a;
  assign y = leaf_wire;
endmodule

module mid(in1, out1);
  input in1;
  output out1;
  wire mid_wire;
  assign mid_wire = in1;
  leaf u_leaf(.a(mid_wire), .y(out1));
endmodule

module top(in1, in2, in3, out1);
  input in1;
  input in2;
  input in3;
  output out1;
  wire and_out;
  wire mid_out;
  assign and_out = (in1 & in2) ^ ~in3;
  mid u_mid(.in1(and_out), .out1(mid_out));
  assign out1 = mid_out | in3;
endmodule
