module top(clk, d, q);
  input clk;
  input d;
  output q;
  reg state;
  assign q = state;
  always @(posedge clk) begin
    state <= d;
  end
endmodule
