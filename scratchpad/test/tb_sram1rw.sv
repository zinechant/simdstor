localparam half_cycle = 10;

module tb_sram1rw();
    localparam W_WIDTH = 6;
    localparam H_WIDTH = 10;

    logic clk;
    logic [H_WIDTH + W_WIDTH - 1 : 0] addr;
    logic [W_WIDTH : 0] web;
    byte_t ibyte[1 << W_WIDTH];
    byte_t obyte[1 << W_WIDTH];



    sram1rw #(
        .W_WIDTH(W_WIDTH),
        .H_WIDTH(H_WIDTH)
    ) sram1rw_inst (
        .clk(clk),
        .addr(addr),
        .web(web),
        .ibyte(ibyte),
        .obyte(obyte)
    );

    genvar i;
    generate
        for (i = 0; i < (1 << W_WIDTH); i++) begin
            assign ibyte[i] = i[0 +: W_WIDTH];
        end
    endgenerate


    always begin
        #half_cycle
        clk = ~clk;
    end

    initial begin
        addr = 8'b11111000;
        clk = 0;
        web = '1;
        #half_cycle
        #half_cycle
        #half_cycle
        #half_cycle
        addr = 8'b11111000;
        web[2] = 1'b0;
        #half_cycle
        #half_cycle
        web[2] = 1'b1;
        #half_cycle
        #half_cycle
        #half_cycle
        #half_cycle
        web[6] = 1'b0;
        addr = 8'b11000000;
        #half_cycle
        #half_cycle
        web[6] = 1'b1;
        #half_cycle
        #half_cycle
        #half_cycle
        #half_cycle
        addr = 8'b11101010;
        web[1] = 1'b0;
        #half_cycle
        #half_cycle
        web[1] = 1'b1;
        #half_cycle
        #half_cycle
        #half_cycle
        #half_cycle
        addr = 8'b11000000;
        #half_cycle
        #half_cycle
        #half_cycle
        #half_cycle
        #half_cycle
        #half_cycle
        addr = 16'b1100110011111000;
        web[2] = 1'b0;
        #half_cycle
        #half_cycle
        web[2] = 1'b1;
        #half_cycle
        #half_cycle
        #half_cycle
        #half_cycle
        web[6] = 1'b0;
        addr = 16'b1100110011000000;
        #half_cycle
        #half_cycle
        web[6] = 1'b1;
        #half_cycle
        #half_cycle
        #half_cycle
        #half_cycle
        addr = 16'b1100110011101010;
        web[1] = 1'b0;
        #half_cycle
        #half_cycle
        web[1] = 1'b1;
        #half_cycle
        #half_cycle
        #half_cycle
        #half_cycle
        addr = 16'b1100110011000000;
        #half_cycle
        #half_cycle
        #half_cycle
        #half_cycle
        #half_cycle
        #half_cycle
        #half_cycle
        #half_cycle
        addr = 16'b1100110011000000;
    end

endmodule