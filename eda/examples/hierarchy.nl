module leaf(a, y);
  input a;
  output y;
endmodule

module top(in1, out1);
  input in1;
  output out1;
  wire w1;
  leaf u1(.a(in1), .y(out1));
endmodule

module second(a, b, y);
  input a, b;
  output y;

  wire w1;
endmodule
