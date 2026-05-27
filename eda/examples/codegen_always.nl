module top(in1, out1, out2);
  input in1;
  output out1;
  output out2;
  reg state;
  assign out2 = state;
  always begin
    state <= in1;
    out1 = state;
  end
endmodule
