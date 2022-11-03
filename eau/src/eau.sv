module eau(
    clk, inum1, inum2, ilen1, ilen2, ipos1, ipos2, idata1, idata2, onum, odata1, odata2
);
    parameter VLEN = 256, BSW = 5;
    localparam BS = 1 << BSW;
    localparam BLEN = VLEN / BS;
    localparam WW = 8 - BSW + 1;

    input wire clk;
    input wire [BSW : 0] inum1, inum2;
    input wire [WW - 1 : 0] ilen1[BS], ilen2[BS];
    input wire [BSW - 1 : 0] ipos1[BS], ipos2[BS];
    input wire [BLEN - 1 : 0] idata1[BS], idata2[BS];
    output wire [BSW : 0] onum;
    output wire [BLEN - 1 : 0] odata1[BS], odata2[BS];

    logic [WW - 1 : 0] ilen[BS];
    logic [BSW : 0] inum, onum_r;
    logic [BSW : 0] psum[BS];
    wire [BSW : 0] idx1[BS], idx2[BS];
    logic [BLEN - 1 : 0] odata1_r[BS], odata2_r[BS];

    pps #(.NW(BSW), .IW(WW), .OW(BSW + 1)) pps_inst (ilen, psum);
    trimux #(.VLEN(VLEN), .BSW(BSW)) tmux1 (inum, ilen1, ipos1, psum, idx1);
    trimux #(.VLEN(VLEN), .BSW(BSW)) tmux2 (inum, ilen2, ipos2, psum, idx2);

    assign inum = inum1 < inum2 ? inum1 : inum2;
    assign onum = onum_r;
    always_ff @(posedge clk) begin
        onum_r <= inum;
    end

    genvar i, j, k;
    generate
        for (k = 0; k < BS; k++) begin
            assign ilen[k] = ilen1[k] < ilen2[k] ? ilen2[k] : ilen1[k];
        end
        for (i = 0; i < BS; i++) begin
            always_ff @(posedge clk) begin
                odata1_r[i] <= (idx1[i][BSW] == 0) ? idata1[idx1[i][BSW - 1 : 0]] : '0;
                odata2_r[i] <= (idx2[i][BSW] == 0) ? idata2[idx2[i][BSW - 1 : 0]] : '0;
            end
            assign odata1[i] = odata1_r[i];
            assign odata2[i] = odata2_r[i];
        end
    endgenerate

endmodule
