module pps (
    data, psum
);

    parameter NW = 5, IW = 4, OW = 6;

    localparam N = 1 << NW;

    input logic [IW - 1 : 0] data[N];
    output logic [OW - 1 : 0] psum[N];

    logic [OW - 1 : 0] tree[NW + 1][N];

    genvar i, j, k;
    generate
        for (k = 0; k < N; k++) begin
            assign tree[0][k] = data[k];
            assign psum[k] = tree[NW][k];
        end
        for (j = 0; j < NW; j++) begin
            for (i = 0; i < N; i++) begin
                assign tree[j + 1][i] = (i < (1 << j)) ? tree[j][i] :
                    tree[j][i] + tree[j][i - (1 << j)];
            end
        end
    endgenerate

endmodule
