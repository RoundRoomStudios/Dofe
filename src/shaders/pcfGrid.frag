#version 430 core

out vec4 FragColor;

flat in int instanceID; // left-right down-up

uniform ivec2 screenDim;

layout(std430, binding = 2) buffer gridSSBO
{
    uint size;    // 2 packed uint16_t
    uint padding; // 2 packed uint16_t
    uint offsets[];
};

layout(std430, binding = 1) buffer bitSSBO
{
    uint sizeBit;
    uint options;
    uint ints[];
};

bool getBit(uint bit) {
    uint byte   = bit / 8;
    uint bitR   = 7u - (bit & 7u);

    uint byteR  = byte % 4;

    uint val = (ints[bit/32] >> (byteR * 8u)) & 0xFFu;

    return bool((val >> bitR) & 1u);
}

void main() {
    // get size as packed
    uint gridWidth  = size & 0xFFFFu;          // lower bits
    uint gridHeight = (size >> 16u) & 0xFFFFu; // upper bits
    ivec2 gridSize  = ivec2(gridWidth, gridHeight);

    // get padding as packed
    uint paddingX = padding & 0xFFFFu;          // lower bits
    uint paddingY = (padding >> 16u) & 0xFFFFu; // upper bits
    ivec2 paddingVec = ivec2(paddingX, paddingY);

    // get cell size in pixels
    ivec2 cellSize = screenDim;
    cellSize -= paddingVec * 2;
    cellSize /= gridSize;

    // convert pixel to position
    ivec2 pixelCoord = ivec2(gl_FragCoord.xy);
    pixelCoord -= paddingVec;
    ivec2 coordInTri = pixelCoord % cellSize;

    // check if left or right
    bool right = instanceID % 2 == 1;

    // check if 1 or 2 sizes
    uint sizeAmount = options & 0xFF;
    bool multipleSizes = sizeAmount == 2;

    // check size
    bool sizeSquare = ((options>>16)&0x1F) == 4;

    // goto bitmap data
    bool data;
    if (multipleSizes) {
        // get size
        bool is1Large = getBit(0);
        bool isLarge = is1Large ^^ getBit(offsets[instanceID]);
        if (isLarge) {
            uint i = uint(float(coordInTri.x)/float(cellSize.x)*8.);
            i += (15-uint(float(coordInTri.y)/float(cellSize.y)*16.)) * 16;
            if (right)
                i += 8;
            data = getBit(offsets[instanceID]+i+1);
        } else {
            uint i = uint(float(coordInTri.x)/float(cellSize.x)*8.);
            i += (15-uint(float(coordInTri.y)/float(cellSize.y)*16.)) * 8;
            data = getBit(offsets[instanceID]+i+1);
        }
    } else {
        // TODO make 1 size (usually ascii) work
        FragColor = vec4(0);
        return;
    }

    vec3 col = vec3(data);
    //col += vec3((vec2(coordInTri)/vec2(8,16)).xy, 0);
    FragColor = vec4(col, 1.0);
}
