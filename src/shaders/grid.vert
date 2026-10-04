#version 430 core

struct Cell
{
    uint bitIndex;
    uint shift;
    uint partX;
    uint partY;
};

layout(location = 0) uniform ivec2 screenSize;

layout(location = 1) uniform uint gridType;

layout(std430, binding = 2) buffer atlasSSBO
{
    uint atlasSize; // in bytes
    uint options;
    uint data[];
};

layout(std430, binding = 3) buffer gridSSBO
{
    uint gridSize; // 2 packed uint16_t
    uint cellSize; // 2 packed uint16_t
    uint gridPosi; // 2 packed uint16_t
    uint cells[];
};

out vec2 pixel;

out flat uint bitIndex;

out flat ivec2 bitmapSize;

out flat uint bitsPerChannel; // TODO process in CPU

out flat uint channels; // TODO process in CPU

out flat uint colorArraySize; // TODO process in CPU

out flat uint colorArrayPos;

out flat vec4 shift;

uint bswap(uint x) {
    return (x >> 24) | ((x >> 8) & 0x0000FF00u) | ((x << 8) & 0x00FF0000u) | (x << 24);
}

uint readBits(uint fromBit, uint amount) { // TODO account for 32 bit shifts
    if (amount == 0) return 0;

    // basic data
    uint bitOffset = fromBit % 32;
    uint byteIndex = fromBit / 32;
    uint data1 = bswap(data[byteIndex]);

    if (amount + bitOffset > 32) {
        // 2 byte data
        uint amount1 = 32 - bitOffset;
        uint amount2 = amount - amount1;
        data1 &= ~(0xFFFFFFFF << amount1);
        data1 <<= amount2;

        // 2nd byte
        uint data2 = bswap(data[byteIndex + 1]);
        data2 >>= 32 - amount2;
        return data1 | data2;
    } else {
        // only using 1 byte
        data1 >>= 32 - (fromBit % 32) - amount;
        data1 &= ~(0xFFFFFFFF << amount);
        return data1;
    }
}

uint cellSizeArray[4] = uint[](1, 2, 2, 3);

Cell readCell(uint cellIndex) {
    // get basic cell info
    Cell cell = {0, 0xFFFFFFFF, 0, 0};
    uint bytesPerCell = cellSizeArray[gridType];
    cell.bitIndex = cells[bytesPerCell * cellIndex];

    // get cell shift
    if (bool(gridType & 2)) {
        cell.shift = cells[bytesPerCell * cellIndex + 1];
    }

    // get cell part
    if (bool(gridType & 2)) {
        uint part = cells[bytesPerCell * (cellIndex + 1) - 1];
        cell.partX = part & 0xFFFF;
        cell.partY = part >> 16;
    }

    return cell;
}

ivec2 corner[4] = ivec2[](ivec2(0, 0), ivec2(1, 0), ivec2(0, 1), ivec2(1, 1));

uint bitWidth(uint a) {// TODO see if array lookup is more efficient
    if (a == 0) return 0;
    return uint(ceil(log2(a) + 1));
}

void main() {
    // expand buffer input
    uint gridW = gridSize & 0xFFFF;
    uint gridH = gridSize >> 16;
    uint cellW = cellSize & 0xFFFF;
    uint cellH = cellSize >> 16;
    uint posiX = gridPosi & 0xFFFF;
    uint posiY = gridPosi >> 16;

    // get pixel position
    ivec2 cellCoord = ivec2(gl_InstanceID % gridW, gl_InstanceID / gridW);
    ivec2 pixelPos = ivec2(posiX, posiY);
    pixelPos += cellCoord * ivec2(cellW, cellH);
    pixelPos += corner[gl_VertexID] * ivec2(cellW, cellH);

    // get cell info
    Cell cell = readCell(gl_InstanceID);
    shift = vec4(
        (cell.shift >> 24) & 0xFF,
        (cell.shift >> 16) & 0xFF,
        (cell.shift >> 8 ) & 0xFF,
         cell.shift        & 0xFF
    );
    shift /= 255;
    bitIndex = cell.bitIndex;

    // get bitmap size TODO calculate options in CPU
    uint sizeArraySize = options & 0xFF;
    uint sizeX = ((options >> 16) & 0x1F) + 1;
    uint sizeY = ((options >> 21) & 0x1F) + 1;
    if (bool(sizeArraySize)) {

        // if indexed read index
        uint sizeIndex = readBits(bitIndex, bitWidth(sizeArraySize - 1));
        uint sizeSize = sizeX + sizeY;

        // read size
        bitmapSize.x = int(readBits(sizeIndex * sizeSize, sizeX)) + 1;
        bitmapSize.y = int(readBits(sizeIndex * sizeSize + sizeX, sizeY)) + 1;
        bitIndex += bitWidth(sizeArraySize - 1);

        colorArrayPos = sizeSize * sizeArraySize;
    } else {

        // if non-indexed read size
        bitmapSize.x = int(readBits(bitIndex, bitWidth(sizeX))) + 1;
        bitmapSize.x = int(readBits(bitIndex + sizeX, bitWidth(sizeY))) + 1;
        bitIndex += sizeX + sizeY;

        colorArrayPos = 0;
    }

    // set color setting TODO process in CPU and delete this step
    bitsPerChannel = ((options >> 28) & 0xF) + 1;
    channels =       ((options >> 26) & 0x3) + 1;
    colorArraySize = ((options >> 8 ) & 0xFF);

    // get pixel
    pixel = corner[gl_VertexID] * ivec2(cellW, cellH);
    pixel += ivec2(cell.partX, cell.partY) * ivec2(cellW, cellH);

    // output position
    vec2 position = (vec2(pixelPos) / vec2(screenSize) * 2) - 1;
    gl_Position = vec4(position, 0.0, 1.0);
}
