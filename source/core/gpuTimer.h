#pragma once
#include <volk/volk.h>
#include <cstdint>
#include <string>
#include <vector>

#include "core/vulkanContext.h"

namespace caldera {

// Double-buffered GPU timestamp profiler. Allocates one VkQueryPool with
// `framesInFlight * maxPassesPerFrame * 2` timestamps and hands out a pair
// of slots per pass per frame. Readback for slot N happens lazily on the
// next time slot N is reused — by then the frame fence has already been
// waited on, so vkGetQueryPoolResults never blocks.
class GpuTimer {
 public:
  struct PassResult {
    std::string name;
    float ms;
  };

  void init(VulkanContext& ctx, uint32_t framesInFlight,
            uint32_t maxPassesPerFrame) {
    device_ = ctx.device;
    framesInFlight = framesInFlight;
    maxPassesPerFrame = maxPassesPerFrame;

    VkPhysicalDeviceProperties2 props{
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2};
    vkGetPhysicalDeviceProperties2(ctx.physicalDevice, &props);
    period = props.properties.limits.timestampPeriod;

    const VkQueryPoolCreateInfo poolCI{
      .sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO,
      .queryType = VK_QUERY_TYPE_TIMESTAMP,
      .queryCount = framesInFlight * maxPassesPerFrame * 2};
    vkCheck(vkCreateQueryPool(device_, &poolCI, nullptr, &pool_));

    namesByFrame.assign(framesInFlight, {});
    passCountByFrame.assign(framesInFlight, 0);
    submittedByFrame.assign(framesInFlight, false);
  }

  void destroy() {
    if (pool_) {
      vkDestroyQueryPool(device_, pool_, nullptr);
      pool_ = VK_NULL_HANDLE;
    }
  }

  // Call once per frame, after the frame fence has been waited on (so the
  // pool slots for this frameIndex are safe to read and reset).
  void beginFrame(VkCommandBuffer cb, uint32_t frameIndex) {
    frameIndex = frameIndex;

    // Readback any timestamps we wrote the last time this slot was used.
    if (submittedByFrame[frameIndex]) {
      const uint32_t passCount = passCountByFrame[frameIndex];
      if (passCount > 0) {
        std::vector<uint64_t> raw(passCount * 2);
        const uint32_t firstQuery = frameIndex * maxPassesPerFrame * 2;
        vkGetQueryPoolResults(device_, pool_, firstQuery, passCount * 2,
                              raw.size() * sizeof(uint64_t), raw.data(),
                              sizeof(uint64_t), VK_QUERY_RESULT_64_BIT);
        lastResults.clear();
        for (uint32_t i = 0; i < passCount; ++i) {
          const uint64_t ticks = raw[i * 2 + 1] - raw[i * 2];
          lastResults.push_back(
            {namesByFrame[frameIndex][i], ticks * period / 1.0e6f});
        }
      }
    }

    // Reset this frame's full slice (must be outside any render scope).
    vkCmdResetQueryPool(cb, pool_, frameIndex * maxPassesPerFrame * 2,
                        maxPassesPerFrame * 2);

    passCount = 0;
    namesByFrame[frameIndex].clear();
  }

  void beginPass(VkCommandBuffer cb, const char* name) {
    if (passCount >= maxPassesPerFrame)
      return;
    const uint32_t slot = frameIndex * maxPassesPerFrame * 2 + passCount * 2;
    vkCmdWriteTimestamp2(cb, VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT, pool_, slot);
    namesByFrame[frameIndex].emplace_back(name ? name : "");
  }

  void endPass(VkCommandBuffer cb) {
    if (passCount >= maxPassesPerFrame)
      return;
    const uint32_t slot =
      frameIndex * maxPassesPerFrame * 2 + passCount * 2 + 1;
    vkCmdWriteTimestamp2(cb, VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT, pool_, slot);
    ++passCount;
  }

  // Call once per frame, after the command buffer is submitted (or at least
  // after all passes were recorded). Marks the slot as having data to read
  // the next time this frameIndex comes around.
  void endFrame() {
    passCountByFrame[frameIndex] = passCount;
    submittedByFrame[frameIndex] = true;
  }

  const std::vector<PassResult>& getLastResults() const { return lastResults; }

 private:
  VkDevice device_{VK_NULL_HANDLE};
  VkQueryPool pool_{VK_NULL_HANDLE};
  uint32_t framesInFlight{0};
  uint32_t maxPassesPerFrame{0};
  float period{1.0f};  // nanoseconds per tick

  uint32_t frameIndex{0};
  uint32_t passCount{0};
  std::vector<std::vector<std::string>> namesByFrame;
  std::vector<uint32_t> passCountByFrame;
  std::vector<bool> submittedByFrame;

  std::vector<PassResult> lastResults;
};

}  // namespace caldera
