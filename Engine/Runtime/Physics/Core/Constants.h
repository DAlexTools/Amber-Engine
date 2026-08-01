#ifndef CONSTANTS_H
#define CONSTANTS_H

namespace AE::Physics
{

constexpr int FPS = 60;
constexpr int MILLISECS_PER_FRAME = 1000 / FPS;
constexpr int PIXELS_PER_METER = 50;
constexpr int ZERO = 0;
constexpr int NEGATIVE = -1;
constexpr int POSITIVE = 1;

constexpr float ZERO_FLOAT = 0.0f;
constexpr float NEGATIVE_FLOAT = -1.0f;
constexpr float POSITIVE_FLOAT = 1.0f;
constexpr float DotPerTimeSeconds = 1000.0f;
} // namespace AE::Physics

#endif // ! CONSTANTS_H
