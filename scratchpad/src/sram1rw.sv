// Read/Write Aligned 1B, 2B, 4B, 8B
typedef logic [7:0] byte_t;

module sram1rw #(
    parameter W_WIDTH = 3,
    parameter H_WIDTH = 8
)  (
    input logic clk,
    input logic [H_WIDTH + W_WIDTH - 1 : 0] addr,
    input logic [W_WIDTH : 0] web,
    input byte_t ibyte[1 << W_WIDTH],
    output byte_t obyte[1 << W_WIDTH]
);
    localparam N = 1 << W_WIDTH;
    localparam NWW = W_WIDTH + 1;
    byte_t rbyte[1 << W_WIDTH];
    byte_t wbyte[1 << W_WIDTH];
    logic [W_WIDTH - 1 : 0] mask;

    logic [W_WIDTH - 1 : 0] waddr_r;
    always_ff @(posedge clk) begin
        waddr_r <= addr[0 +: W_WIDTH];
    end

    always_comb begin
        mask = '1;
        for (int k = 0; k < W_WIDTH; k++) begin
            if (!web[k]) begin
                mask = (1 << k) - 1;
            end
        end
    end

    logic nwe[N];
    logic [W_WIDTH:0] we[N];

    genvar i, j, k, l;
    generate
        assign obyte[0] = rbyte[waddr_r];
        for (l = 1; l < W_WIDTH; l++) begin
            for (k = (1 << (l - 1)); k < (1 << l); k++) begin
                assign obyte[k] = rbyte[{waddr_r[W_WIDTH - 1: l], k[0 +: l]}];
            end
        end
        for (k = (1 << (W_WIDTH - 1)); k < (1 << W_WIDTH); k++) begin
            assign obyte[k] = rbyte[k[0 +: W_WIDTH]];
        end

        for (i = 0; i < N; i++) begin
            for (j = 0; j < W_WIDTH; j++) begin
                assign we[i][j] = (i[W_WIDTH - 1 : j] == addr[W_WIDTH - 1 : j]) && !web[j];
            end
            assign we[i][W_WIDTH] = !web[W_WIDTH];
            assign nwe[i] = !(|we[i[W_WIDTH - 1 : 0]]);

            assign wbyte[i] = ibyte[i[W_WIDTH - 1 : 0] & mask];

            byteram #(.AddrW(H_WIDTH)) byteram_inst (
                .clk(clk),
                .addr(addr[W_WIDTH +: H_WIDTH]),
                .web(nwe[i]),
                .ibyte(wbyte[i]),
                .obyte(rbyte[i])
            );

        end

    endgenerate

endmodule
