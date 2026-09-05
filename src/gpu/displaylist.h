#ifndef displaylist_h
#define displaylist_h

#include <vulkan/vulkan.h>

#define VMA_VULKAN_VERSION 1004000 // Vulkan 1.4
#include <vk_mem_alloc.h>

struct mesh {
  unsigned int buffer; // Which buffer the vertices are in
  unsigned int bufferoffset; // Where in the buffer
  unsigned int size; // How large it is
  
};

struct displaylist {
  float camx,camy,camz;

};

void renderDisplaylist(VkCommandBuffer buffer, VmaAllocator allocator, struct displaylist *render);

#endif