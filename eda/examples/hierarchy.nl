module leaf(a, y);
  input a;
  output y;
endmodule

module top(in1, out1);
  input in1;
  output out1;
  leaf u1(.a(in1), .y(out1));
endmodule
