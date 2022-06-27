module byteram(clk, addr, web, ibyte, obyte);
    parameter AddrW = 10;
    localparam ByteWidth = 8;

    input logic clk;
    input logic [AddrW - 1 : 0] addr;
    input logic web;
    input logic [ByteWidth - 1 : 0] ibyte;
    output logic [ByteWidth - 1 : 0] obyte;

    genvar Ni;
    generate
        if (AddrW < 6) begin
            SRAMUNNOWN SRAMUNNOWN_inst();
        end else if (AddrW == 6) begin
            SRAM1RW64x8 SRAM1RW64x8_inst(.A(addr), .CE(clk), .WEB(web), .OEB(1'b0), .CSB(1'b0), .I(ibyte), .O(obyte));
        end else if (AddrW == 7) begin
            SRAM1RW128x8 SRAM1RW128x8_inst(.A(addr), .CE(clk), .WEB(web), .OEB(1'b0), .CSB(1'b0), .I(ibyte), .O(obyte));
        end else if (AddrW == 8) begin
            SRAM1RW256x8 SRAM1RW256x8_inst(.A(addr), .CE(clk), .WEB(web), .OEB(1'b0), .CSB(1'b0), .I(ibyte), .O(obyte));
        end else if (AddrW == 9) begin
            SRAM1RW512x8 SRAM1RW512x8_inst(.A(addr), .CE(clk), .WEB(web), .OEB(1'b0), .CSB(1'b0), .I(ibyte), .O(obyte));
        end else begin
            localparam W = 9;
            localparam NW = AddrW - W;
            localparam N = 1 << NW;

            logic [ByteWidth - 1 : 0] obytes[N];
            logic [NW - 1 : 0] naddr_r;

            always_ff @(posedge clk) begin
                naddr_r <= addr[0 +: NW];
            end
            assign obyte = obytes[naddr_r];
            for (Ni = 0; Ni < N; Ni++) begin
                wire nwe;
                assign nwe = web || (Ni[0 +: NW] != addr[0 +: NW]);
                SRAM1RW512x8 SRAM1RW512x8_inst(.A(addr[NW +: W]), .CE(clk), .WEB(nwe), .OEB(1'b0), .CSB(1'b0), .I(ibyte), .O(obytes[Ni]));
            end
        end
    endgenerate

endmodule