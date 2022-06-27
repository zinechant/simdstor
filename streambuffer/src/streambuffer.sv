typedef logic[7:0] byte_t;

localparam BYTES_WIDTH = 8;
localparam OOPT_W = 2;
localparam OOPT = 1 << OOPT_W;
localparam BN = 4;
localparam logic [BYTES_WIDTH - 1 : 0] BW[BN] = {2, 3, 4, 6};
localparam logic [BYTES_WIDTH - 1 : 0] BS[BN] = {4, 8, 16, 64};
localparam BYTES = 1 << BYTES_WIDTH;
localparam HALFBYTES = BYTES >> 1;
localparam IBYTES = HALFBYTES;

module streambuffer (
    input clk,
    input rst,

    input logic ivalid,
    input byte_t idata[IBYTES],
    output logic iready,

    output logic ovalid[OOPT],
    input logic oready[OOPT],
    output byte_t odata[64]
);

logic shift;
logic [OOPT_W - 1 : 0] bid;
logic ovalid_r[OOPT];
byte_t data_r[BYTES], data_n[BYTES];
logic [BYTES_WIDTH:0] num_r, num_n;


byte_t data_zext[BYTES + BYTES];

genvar o, d, b;
generate
    for (o = 0; o < BS[BN - 1]; o++) begin
        assign odata[o] = data_r[o];
    end
    for (d = 0; d < BYTES; d++) begin
        assign data_zext[d] = data_r[d];
    end
    for (d = BYTES; d < BYTES + BYTES; d++) begin
        assign data_zext[d] = '0;
    end

    for (b = 0; b < OOPT; b++) begin
        assign ovalid[b] = ovalid_r[b];
        always_ff @(posedge clk) begin
            if (rst) begin
                ovalid_r[b] <= 1'b0;
            end else begin
                ovalid_r[b] <= (num_n >= BS[b]);
            end
        end
    end


endgenerate


always_comb begin
    shift = 1'b0;
    bid = 2'b00;
    for (int j = 0; j < OOPT; j++) begin
        if (ovalid[j] && oready[j]) begin
            shift = 1'b1;
            bid = j[0 +: OOPT_W];
        end
    end
end

// always_comb begin
//     if (ovalid[3] && oready[3]) begin
//         bid = 2'b11;
//         shift = 1'b1;
//     end else if (ovalid[2] && oready[2]) begin
//         bid = 2'b10;
//         shift = 1'b1;
//     end else if (ovalid[1] && oready[1]) begin
//         bid = 2'b01;
//         shift = 1'b1;
//     end else if (ovalid[0] && oready[0]) begin
//         bid = 2'b00;
//         shift = 1'b1;
//     end else begin
//         bid = 2'b00;
//         shift = 1'b0;
//     end
// end

assign iready = !num_r[BYTES_WIDTH - 1];
always_comb begin
    if (shift) begin
        if (ivalid && iready) begin
            num_n = num_r + IBYTES - BS[bid];
        end else begin
            num_n = num_r - BS[bid];
        end
    end else begin
        if (ivalid && iready) begin
            num_n = num_r + IBYTES;
        end else begin
            num_n = num_r;
        end
    end
end

genvar i;
generate
    for (i = 0; i < BYTES; i++) begin
        always_comb begin
            if (ivalid && ((num_r == 0 && !i[BYTES_WIDTH - 1]) ||
                           (i[BYTES_WIDTH - 1] && !num_r[BYTES_WIDTH - 1]))) begin
                data_n[i] = idata[i[BYTES_WIDTH - 2 : 0]];
            end else if (shift) begin
                data_n[i] = data_zext[i + BS[0]];
                for (int l = 0; l < OOPT; l++) begin
                    if (bid == l[0 +: OOPT_W]) begin
                        data_n[i] = data_zext[i + BS[l[0 +: OOPT_W]]];
                    end
                end

                // if (bid == 2'b00) begin
                //     data_n[i] = data_zext[i + 4];
                // end else if (bid == 2'b01) begin
                //     data_n[i] = data_zext[i + 8];
                // end else if (bid == 2'b10) begin
                //     data_n[i] = data_zext[i + 16];
                // end else begin
                //     data_n[i] = data_zext[i + 64];
                // end
            end else begin
                data_n[i] = data_r[i];
            end
        end
    end
endgenerate

always_ff @(posedge clk) begin
    if (rst) begin
        num_r <= 0;
    end else begin
        num_r <= num_n;
    end
end

genvar k;
generate
    for (k = 0; k < BYTES; k++) begin
        always_ff @(posedge clk) begin
            data_r[k] <= data_n[k];
        end
    end
endgenerate

endmodule