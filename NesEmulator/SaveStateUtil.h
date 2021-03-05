#pragma once
#include <vector>
#include <cstdint>
#include "EmuFileException.h"

typedef std::vector<uint8_t> Snapshot;

inline void AppendVector(Snapshot& vec, const Snapshot& app)
{
	vec.insert(vec.end(), app.begin(), app.end());
}

template <class T>
static void SaveBytes(Snapshot& vec, const T& val)
{
	vec.insert(vec.end(), reinterpret_cast<const uint8_t*>(&val), reinterpret_cast<const uint8_t*>(&val + 1));
}

template <class T>
static void SaveBytes(Snapshot& vec, const T* start, size_t len)
{
	vec.insert(vec.end(), reinterpret_cast<const uint8_t*>(start), reinterpret_cast<const uint8_t*>(start + len));
}

template <class T>
static void LoadBytes(Snapshot& vec, T& val)
{
	if (vec.size() < sizeof(val))
		throw EmuFileException("invalid file, expected more bytes");
	std::memcpy(&val, vec.data(), sizeof(val));
	vec.erase(vec.begin(), vec.begin() + sizeof(val));
}

template <class T>
static void LoadBytes(Snapshot& vec, T* start, size_t len)
{
	if (vec.size() < len)
		throw EmuFileException("invalid file, expected more bytes");
	std::memcpy(start, vec.data(), len * sizeof(T));
	vec.erase(vec.begin(), vec.begin() + len * sizeof(T));
}
