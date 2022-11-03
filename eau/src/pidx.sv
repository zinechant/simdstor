module pidx (
    inum, ilen, ipos, psum, index
);
    parameter VLEN = 256, BSW = 5;
    localparam BS = 1 << BSW;
    localparam BLEN = VLEN / BS;
    localparam WW = 8 - BSW + 1;

    input wire [BSW : 0] inum;
    input wire [WW - 1 : 0] ilen[BS];
    input wire [BSW - 1 : 0] ipos[BS];
    input wire [BSW : 0] psum[BS];
    output wire [BSW : 0] index[BS];

    logic [BS - 1: 0] boundary[BS];
    logic [0:0] arr[BS];
    logic [BSW - 1 : 0] idx[BS];
    logic [BSW - 1 : 0] prev[BS];


    pps #(.NW(BSW), .IW(1), .OW(BSW)) pps_inst (arr, idx);

    assign arr[0] = 1'b0;
    genvar i, j, k;
    generate
        for (i = 1; i < BS; i++) begin
            for (j = 0; j < BS; j++) begin
                assign boundary[i][j] = (j < inum) && (psum[j] == i);
            end
            assign arr[i][0] = |boundary[i];
        end
        for (k = 0; k < BS; k++) begin
            assign prev[k] = (k == 0) ? '0 : psum[k - 1];
            assign index[k] = (k - prev[idx[k]] < ilen[idx[k]]) ?
                ipos[idx[k]] + k - prev[idx[k]] : '1;
        end
    endgenerate
endmodule
