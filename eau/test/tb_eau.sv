module tb_eau();

parameter CP = 10;

parameter VLEN = 256, BSW = 5;
localparam BS = 1 << BSW;
localparam BLEN = VLEN / BS;
localparam WW = 8 - BSW + 1;

logic clk;
logic [BSW : 0] inum1, inum2;
logic [WW - 1 : 0] ilen1[BS], ilen2[BS];
logic [BSW - 1 : 0] ipos1[BS], ipos2[BS];
logic [BLEN - 1 : 0] idata1[BS], idata2[BS];
logic [BSW : 0] onum;
logic [BLEN - 1 : 0] odata1[BS], odata2[BS];

always begin
    #CP
    #CP
    if (clk == 1'b1) begin
        clk <= 1'b0;
    end else begin
        clk <= 1'b1;
    end
end


eau #(.VLEN(VLEN), .BSW(BSW)) inst_eau (
    clk, inum1, inum2, ilen1, ilen2, ipos1, ipos2, idata1, idata2, onum, odata1, odata2
);

initial begin
    inum1 = 0;
    inum2 = 0;
    ilen1 = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    ipos1 = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    ilen2 = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    ipos2 = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    idata1 = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18,
                19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31};
    idata2 = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18,
                19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31};
    #CP
    #CP
    #CP
    #CP

    inum1 = 5'd7;
    inum2 = 5'd9;
    ilen1 = {4, 6, 3, 2, 2, 3, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    ipos1 = {0, 5, 11, 16, 18, 21, 24, 29, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    ilen2 = {2, 3, 3, 2, 4, 3, 3, 3, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    ipos2 = {0, 2, 6, 9, 11, 15, 19, 22, 25, 29, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    #CP
    #CP
    #CP
    #CP
    #CP
    #CP
    #CP
    #CP
    inum1 = 5'd7;
end

endmodule
