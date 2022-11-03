module tb_trimux();

parameter CP = 10;

parameter VLEN = 256, BSW = 5;
localparam BS = 1 << BSW;
localparam BLEN = VLEN / BS;
localparam WW = 8 - BSW + 1;


logic [BSW : 0] inum;
logic [WW - 1 : 0] ilen[BS];
logic [BSW - 1 : 0] ipos[BS];
logic [BSW : 0] psum[BS];
logic [BSW : 0] index[BS];

trimux #(.VLEN(VLEN), .BSW(BSW)) inst_trimux (
    inum, ilen, ipos, psum, index
);

initial begin
    inum = 5'd7;
    psum = {4, 10, 13, 15, 19, 22, 27, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    ilen = {4, 6, 3, 2, 2, 3, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    ipos = {0, 5, 11, 16, 18, 21, 24, 29, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    #CP
    #CP
    #CP
    #CP
    ilen = {2, 3, 3, 2, 4, 3, 3, 3, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    ipos = {0, 2, 6, 9, 11, 15, 19, 22, 25, 29, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    #CP
    #CP
    #CP
    #CP
    inum = 5'd7;
end

endmodule
