#version 430 core

flat out int instanceID;

uniform ivec2 screenDim;

layout(std430, binding = 2) buffer gridSSBO
{
    uint size;    // 2 packed uint16_t
    uint padding; // 2 packed uint16_t
    uint offsets[];
};

void main() {
    ivec2 positions[4] = ivec2[](
        ivec2(0, 0),
        ivec2(1, 0),
        ivec2(1, 1),
        ivec2(0, 1)
    );

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

    // get current cell
    ivec2 cell;
    cell.x = gl_InstanceID % int(gridWidth);
    cell.y = gl_InstanceID / int(gridWidth);

    // get pixel location
    ivec2 posPixel = paddingVec;
    posPixel += cell * cellSize;
    //cellSize -= ivec2(1); // HACK show borders
    posPixel += positions[gl_VertexID] * cellSize;

    // get location
    vec2 position = posPixel;
    position /= vec2(screenDim);
    position *= 2.;
    position -= 1.;

    instanceID = gl_InstanceID;
    gl_Position = vec4(position, 0, 1.0);
}
