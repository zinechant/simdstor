module trimux (
    inum, ilen, ipos, psum, index
);
    parameter VLEN = 256, BSW = 5;
    localparam BS = 1 << BSW;
    localparam BLEN = VLEN / BS;
    localparam WW = 8 - BSW + 1;

    input wire [BSW : 0] inum;
    input wire [WW - 1 : 0] ilen[BS];
    input wire [BSW - 1 : 0] ipos[BS];
    input wire [BSW - 1 : 0] psum[BS];
    output tri [BSW : 0] index[BS];

    logic [BSW - 1 : 0] prev[BS];

    genvar i, j, k;
    generate
        for (k = 0; k < BS; k++) begin
            assign prev[k] = (k == 0) ? '0 : psum[k - 1];
        end
        for (i = 0; i < BS; i++) begin
            for (j = 0; j < BS; j++) begin
                assign index[i] = (j >= inum || prev[j] > i || psum[j] <= i) ?
                    'z : ((i - prev[j] < ilen[j]) ? ipos[j] + i - prev[j] : '1);
            end
        end
    endgenerate
endmodule
