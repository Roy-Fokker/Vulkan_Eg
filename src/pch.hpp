#pragma once

#include <version>
#include <cstdint>
#include <iostream>
#include <fstream>
#include <format>
#include <string_view>
#include <memory>
#include <functional>
#include <algorithm>
#include <ranges>
#include <concepts>
#include <optional>
#include <tuple>
#include <utility>
#include <filesystem>
#include <stdexcept>
#include <exception>

#pragma warning(push)
#pragma warning(disable : 5105)
#include <Windows.h>
#pragma warning(pop)

#include <vulkan/vulkan.hpp>
#include <glm/glm.hpp>

#include <range/v3/all.hpp>