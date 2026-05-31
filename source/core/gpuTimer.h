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

  void init(VulkanContext& ctx, uint32_t frames_in_flight,
            uint32_t max_passes_per_frame) {
    device_ = ctx.device;
    framesInFlight_ = frames_in_flight;
    maxPassesPerFrame_ = max_passes_per_frame;

    VkPhysicalDeviceProperties2 props{
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2};
    vkGetPhysicalDeviceProperties2(ctx.physicalDevice, &props);
    period_ = props.properties.limits.timestampPeriod;

    const VkQueryPoolCreateInfo poolCI{
      .sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO,
      .queryType = VK_QUERY_TYPE_TIMESTAMP,
      .queryCount = framesInFlight_ * maxPassesPerFrame_ * 2};
    vkCheck(vkCreateQueryPool(device_, &poolCI, nullptr, &pool_));

    namesByFrame_.assign(framesInFlight_, {});
    passCountByFrame_.assign(framesInFlight_, 0);
    submittedByFrame_.assign(framesInFlight_, false);
  }

  void destroy() {
    if (pool_) {
      vkDestroyQueryPool(device_, pool_, nullptr);
      pool_ = VK_NULL_HANDLE;
    }
  }

  // Call once per frame, after the frame fence has been waited on (so the
  // pool slots for this frameIndex are safe to read and reset).
  void beginFrame(VkCommandBuffer cb, uint32_t frame_index) {
    frameIndex_ = frame_index;

    // Readback any timestamps we wrote the last time this slot was used.
    if (submittedByFrame_[frameIndex_]) {
      const uint32_t passCount = passCountByFrame_[frameIndex_];
      if (passCount > 0) {
        std::vector<uint64_t> raw(passCount * 2);
        const uint32_t firstQuery = frameIndex_ * maxPassesPerFrame_ * 2;
        vkGetQueryPoolResults(device_, pool_, firstQuery, passCount * 2,
                              raw.size() * sizeof(uint64_t), raw.data(),
                              sizeof(uint64_t), VK_QUERY_RESULT_64_BIT);
        lastResults_.clear();
        for (uint32_t i = 0; i < passCount; ++i) {
          const uint64_t ticks = raw[i * 2 + 1] - raw[i * 2];
          lastResults_.push_back(
            {namesByFrame_[frameIndex_][i], ticks * period_ / 1.0e6f});
        }
      }
    }

    // Reset this frame's full slice (must be outside any render scope).
    vkCmdResetQueryPool(cb, pool_, frameIndex_ * maxPassesPerFrame_ * 2,
                        maxPassesPerFrame_ * 2);

    passCount_ = 0;
    namesByFrame_[frameIndex_].clear();
  }

  void beginPass(VkCommandBuffer cb, const char* name) {
    if (passCount_ >= maxPassesPerFrame_)
      return;
    const uint32_t slot = frameIndex_ * maxPassesPerFrame_ * 2 + passCount_ * 2;
    vkCmdWriteTimestamp2(cb, VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT, pool_, slot);
    namesByFrame_[frameIndex_].emplace_back(name ? name : "");
  }

  void endPass(VkCommandBuffer cb) {
    if (passCount_ >= maxPassesPerFrame_)
      return;
    const uint32_t slot =
      frameIndex_ * maxPassesPerFrame_ * 2 + passCount_ * 2 + 1;
    vkCmdWriteTimestamp2(cb, VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT, pool_, slot);
    ++passCount_;
  }

  // Call once per frame, after the command buffer is submitted (or at least
  // after all passes were recorded). Marks the slot as having data to read
  // the next time this frameIndex comes around.
  void endFrame() {
    passCountByFrame_[frameIndex_] = passCount_;
    submittedByFrame_[frameIndex_] = true;
  }

  const std::vector<PassResult>& getLastResults() const { return lastResults_; }

 private:
  VkDevice device_{VK_NULL_HANDLE};
  VkQueryPool pool_{VK_NULL_HANDLE};
  uint32_t framesInFlight_{0};
  uint32_t maxPassesPerFrame_{0};
  float period_{1.0f};  // nanoseconds per tick

  uint32_t frameIndex_{0};
  uint32_t passCount_{0};
  std::vector<std::vector<std::string>> namesByFrame_;
  std::vector<uint32_t> passCountByFrame_;
  std::vector<bool> submittedByFrame_;

  std::vector<PassResult> lastResults_;
};

}  // namespace caldera
