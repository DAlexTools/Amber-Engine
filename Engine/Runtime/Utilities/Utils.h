#ifndef UTILS_H
#define UTILS_H

#include "../Logging/Logger.h"
#include <cmath>
#include <algorithm>
#include <stdexcept>

namespace GEngine
{
namespace Math
{
/**
 * Random interger get
 * @param min
 * @param max
 */
inline int randomInt(int min, int max);

/**
 * @brief Smoothly approaches a target value with a maximum step limit.
 *
 * This function adjusts the given value towards the target value by at most
 * the specified maximum step. If the difference between the value and the
 * target is smaller than the step size, the function directly sets the value to the target.
 *
 * @param target The desired target value to approach.
 * @param value The current value to be adjusted.
 * @param maxStep The maximum allowed step to move towards the target per call.
 *                Must be positive.
 * @return The adjusted value, closer to the target.
 *
 * @note If `maxStep` is negative, the function will behave incorrectly.
 */
inline float Approach(float target, float value, float maxStep)
{
	float delta = target - value;

	if (delta > maxStep)
		return value + maxStep;
	if (delta < -maxStep)
		return value - maxStep;

	return value;
}

/**
 * @brief Remaps a value from one range to another.
 *
 * This function takes an input value and maps it from the source range [srcMin, srcMax]
 * to the target range [destMin, destMax] using linear interpolation. The input value is
 * normalized to a value between 0 and 1, then mapped to the target range.
 *
 * @param inputValue The input value to be remapped.
 * @param srcMin The lower bound of the source range.
 * @param srcMax The upper bound of the source range.
 * @param destMin The lower bound of the target range.
 * @param destMax The upper bound of the target range.
 * @return The remapped value within the target range.
 *
 * @throws std::invalid_argument if the source range has zero width (srcMin == srcMax).
 *
 * @note The function ensures that the output value remains within the bounds of the target range
 *       by using `std::clamp` to restrict the result.
 */
inline float Remap(float inputValue, float srcMin, float srcMax, float destMin, float destMax)
{
	if (srcMax == srcMin)
	{
		throw std::invalid_argument("Source range cannot have zero width (srcMin == srcMax).");
	}

	float normalizedValue = (inputValue - srcMin) / (srcMax - srcMin);
	float mappedValue = normalizedValue * (destMax - destMin) + destMin;

	return std::clamp(mappedValue, destMin, destMax);
}

/**
 * @brief Remaps a value from one range to another, with clamping.
 *
 * This function remaps the input value `value` from the source range [rangeStart, rangeEnd]
 * to the target range [targetStart, targetEnd]. If the input value is outside the source range,
 * it is clamped to the nearest bound of the target range.
 *
 * @param value The input value to be remapped.
 * @param rangeStart The lower bound of the source range.
 * @param rangeEnd The upper bound of the source range.
 * @param targetStart The lower bound of the target range.
 * @param targetEnd The upper bound of the target range.
 * @return The remapped value, clamped within the target range.
 *
 * @note If the input value is smaller than `rangeStart`, the function returns `targetStart`.
 *       If the input value is larger than `rangeEnd`, the function returns `targetEnd`.
 *       If the input value is within the source range, the function performs linear interpolation
 *       and remaps it to the target range.
 */
inline float RemapClamp(float value, float rangeStart, float rangeEnd, float targetStart, float targetEnd)
{
	// If value is less than the minimum of the source range, return the minimum of the target range
	if (value < rangeStart)
		return targetStart;

	// If value is greater than the maximum of the source range, return the maximum of the target range
	if (value > rangeEnd)
		return targetEnd;

	// If value is within the source range, perform linear remapping to the target range
	return Remap(value, rangeStart, rangeEnd, targetStart, targetEnd);
}
} // namespace Math

/**
 * Tools class
 */
class Tools
{
public:
	static std::string getFileNameExt(std::string fileName);
};
} // namespace GEngine

#endif