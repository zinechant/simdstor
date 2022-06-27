localparam half_cycle = 10;
// typedef logic[7:0] byte_t;

module tb_streambuffer();
    logic ready;

    logic clk;
    logic rst;

    logic ivalid;
    byte_t idata[IBYTES];

    logic ovalid[OOPT];
    logic oready[OOPT];
    byte_t odata[64];

    streambuffer sinst(
        .clk(clk),
        .rst(rst),

        .ivalid(ivalid),
        .idata(idata),
        .iready(),

        .ovalid(ovalid),
        .oready(oready),
        .odata(odata)
    );


    assign oready[0] = 1'b0;
    assign oready[1] = ready;
    assign oready[2] = 1'b0;
    assign oready[3] = 1'b0;


    genvar i;
    generate
        for (i = 0; i < IBYTES; i++) begin
            assign idata[i] = i[7:0];
        end
    endgenerate


    always begin
        #half_cycle
        clk = ~clk;
    end

    always @(posedge clk) begin
        ready <= ~ready;
    end

    initial begin
        clk = 0;
        ready = 0;
        rst = 1'b0;
        ivalid = 1'b0;
        #half_cycle
        #half_cycle
        rst = 1'b1;
        #half_cycle
        #half_cycle
        rst = 1'b0;
        #half_cycle
        #half_cycle
        ivalid = 1'b1;
    end

endmodule