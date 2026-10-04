#version 430 core

layout(location = 0) uniform ivec2 screenSize;

layout(location = 1) uniform uint gridType;

layout(std430, binding = 2) buffer atlasSSBO
{
    uint atlasSize; // in bytes
    uint options;
    uint data[];
};

in vec2 pixel;

in flat uint bitIndex;

in flat ivec2 bitmapSize;

in flat uint bitsPerChannel; // TODO process in CPU

in flat uint channels; // TODO process in CPU

in flat uint colorArraySize; // TODO process in CPU

in flat uint colorArrayPos;

in flat vec4 shift;

out vec4 FragColor;

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

uint bitWidth(uint a) {// TODO see if array lookup is more efficient
    return uint(ceil(log2(a) + 1));
}

vec4 intToColor(uint i) {
    float maxC = pow(2, bitsPerChannel) - 1;
    switch (channels) {
        case 1:
            float gray = float(i) / maxC;
            return vec4(vec3(gray), 1.0);
        case 2:
            uint mask = (1 << bitsPerChannel) - 1;
            gray  = float(i & mask) / maxC;
            i >>= bitsPerChannel;
            float alpha = float(i & mask) / maxC;
            return vec4(vec3(gray), alpha);
        case 3:
            mask = (1 << bitsPerChannel) - 1;
            float red   = float(i & mask) / maxC;
            i >>= bitsPerChannel;
            float green = float(i & mask) / maxC;
            i >>= bitsPerChannel;
            float blue  = float(i & mask) / maxC;
            return vec4(red, green, blue, 1.0);
        case 4:
            mask = (1 << bitsPerChannel) - 1;
            red   = float(i & mask) / maxC;
            i >>= bitsPerChannel;
            green = float(i & mask) / maxC;
            i >>= bitsPerChannel;
            blue  = float(i & mask) / maxC;
            i >>= bitsPerChannel;
            alpha = float(i & mask) / maxC;
            return vec4(red, green, blue, alpha);
    }
}

void main() {
    // get bit
    uint bit = bitIndex;
    uint pixelIndex = uint(pixel.x) + (bitmapSize.y - uint(pixel.y) - 1) * bitmapSize.x; // account for up-side down
    uint bpp = channels * bitsPerChannel;

    if (bool(colorArraySize)) {
        // indexed colors
        bit += bitWidth(colorArraySize - 1) * pixelIndex;
        uint colorIndex = readBits(bit, bitWidth(colorArraySize - 1));
        uint colorInt = readBits(colorArrayPos + colorIndex * bpp, bpp);
        FragColor = intToColor(colorInt);
    } else {
        // non-indexed colors
        bit += bpp * pixelIndex;
        uint colorInt = readBits(bit, bpp);
        FragColor = intToColor(colorInt);
    }
}
