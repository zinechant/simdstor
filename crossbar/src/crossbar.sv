module crossbar #(
    parameter DW = 16,
    parameter W = 4
) (
    input clk,
    input [DW - 1 : 0] i[1 << W],
    input [W - 1 : 0] s[1 << W],
    output [DW - 1 : 0] o[1 << W]
);

localparam N = 1 << W;

logic [DW - 1 : 0] i_r[1 << W];
logic [W - 1 : 0] s_r[1 << W];
logic [DW - 1 : 0] o_r[1 << W];
genvar j;

generate
    for (j = 0; j < (1 << W); j++) begin
        always_ff @(posedge clk) begin
            i_r[j] <= i[j];
            s_r[j] <= s[j];
            o_r[j] <= i_r[s_r[j]];
        end

        assign o[j] = o_r[j];
    end
endgenerate

endmodule
