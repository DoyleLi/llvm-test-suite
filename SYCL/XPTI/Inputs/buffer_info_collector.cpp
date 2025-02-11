#include "xpti/xpti_trace_framework.hpp"

#include <iostream>
#include <mutex>
#include <string_view>

std::mutex GMutex;

XPTI_CALLBACK_API void memCallback(uint16_t, xpti::trace_event_data_t *,
                                   xpti::trace_event_data_t *, uint64_t,
                                   const void *);

XPTI_CALLBACK_API void syclBufferCallback(uint16_t, xpti::trace_event_data_t *,
                                          xpti::trace_event_data_t *, uint64_t,
                                          const void *);

XPTI_CALLBACK_API void xptiTraceInit(unsigned int MajorVersion,
                                     unsigned int MinorVersion,
                                     const char *VersionStr,
                                     const char *StreamName) {
  std::cout << "xptiTraceInit: Stream Name = " << StreamName << "\n";
  std::string_view NameView{StreamName};

  if (NameView == "sycl.experimental.mem_alloc") {
    uint8_t StreamID = xptiRegisterStream(StreamName);
    xptiRegisterCallback(
        StreamID,
        static_cast<uint16_t>(xpti::trace_point_type_t::mem_alloc_begin),
        memCallback);
    xptiRegisterCallback(
        StreamID,
        static_cast<uint16_t>(xpti::trace_point_type_t::mem_alloc_end),
        memCallback);
    xptiRegisterCallback(
        StreamID,
        static_cast<uint16_t>(xpti::trace_point_type_t::mem_release_begin),
        memCallback);
    xptiRegisterCallback(
        StreamID,
        static_cast<uint16_t>(xpti::trace_point_type_t::mem_release_end),
        memCallback);
  }

  if (NameView == "sycl.experimental.buffer") {
    uint8_t StreamID = xptiRegisterStream(StreamName);
    xptiRegisterCallback(StreamID,
                         static_cast<uint16_t>(
                             xpti::trace_point_type_t::offload_alloc_construct),
                         syclBufferCallback);
    xptiRegisterCallback(StreamID,
                         static_cast<uint16_t>(
                             xpti::trace_point_type_t::offload_alloc_associate),
                         syclBufferCallback);
    xptiRegisterCallback(
        StreamID,
        static_cast<uint16_t>(xpti::trace_point_type_t::offload_alloc_release),
        syclBufferCallback);
    xptiRegisterCallback(
        StreamID,
        static_cast<uint16_t>(xpti::trace_point_type_t::offload_alloc_destruct),
        syclBufferCallback);
  }
}

XPTI_CALLBACK_API void xptiTraceFinish(const char *streamName) {
  std::cout << "xptiTraceFinish: Stream Name = " << streamName << "\n";
}

XPTI_CALLBACK_API void memCallback(uint16_t TraceType,
                                   xpti::trace_event_data_t *,
                                   xpti::trace_event_data_t *, uint64_t,
                                   const void *UserData) {
  std::lock_guard Lock{GMutex};
  auto Type = static_cast<xpti::trace_point_type_t>(TraceType);
  auto *Data = static_cast<const xpti::mem_alloc_data_t *>(UserData);
  if (Type == xpti::trace_point_type_t::mem_alloc_begin) {
    std::cout << "Mem Alloc Begin : ";
  } else if (Type == xpti::trace_point_type_t::mem_alloc_end) {
    std::cout << "Mem Alloc End : ";
  } else if (Type == xpti::trace_point_type_t::mem_release_begin) {
    std::cout << "Mem Release Begin : ";
  } else if (Type == xpti::trace_point_type_t::mem_release_end) {
    std::cout << "Mem Release End : ";
  }
  std::cout << "  mem_obj_handle: " << Data->mem_object_handle << "\n";
  std::cout << "  alloc_pointer : " << Data->alloc_pointer << "\n";
  std::cout << "  alloc_size    : " << Data->alloc_size << "\n";
}

XPTI_CALLBACK_API void syclBufferCallback(uint16_t TraceType,
                                          xpti::trace_event_data_t *Parent,
                                          xpti::trace_event_data_t *Event,
                                          uint64_t IId, const void *UserData) {
  std::lock_guard Lock{GMutex};
  auto Type = static_cast<xpti::trace_point_type_t>(TraceType);
  switch (Type) {
  case xpti::trace_point_type_t::offload_alloc_construct: {
    auto BufConstr = (xpti::offload_buffer_data_t *)UserData;
    std::cout << IId << "|Create buffer|" << BufConstr->user_object_handle
              << "|" << Event->reserved.payload->name << "|"
              << Event->reserved.payload->source_file << ":"
              << Event->reserved.payload->line_no << ":"
              << Event->reserved.payload->column_no << "\n";

    break;
  }
  case xpti::trace_point_type_t::offload_alloc_associate: {
    auto BufAssoc = (xpti::offload_buffer_association_data_t *)UserData;
    std::cout << IId << "|Associate buffer|" << BufAssoc->user_object_handle
              << "|" << BufAssoc->mem_object_handle << std::endl;
    break;
  }
  case xpti::trace_point_type_t::offload_alloc_release: {
    auto BufRelease = (xpti::offload_buffer_association_data_t *)UserData;
    std::cout << IId << "|Release buffer|" << BufRelease->user_object_handle
              << "|" << BufRelease->mem_object_handle << std::endl;
    break;
  }
  case xpti::trace_point_type_t::offload_alloc_destruct: {
    auto BufDestr = (xpti::offload_buffer_data_t *)UserData;
    std::cout << IId << "|Destruct buffer|" << BufDestr->user_object_handle
              << "\n";
    break;
  }
  default:
    std::cout << "Unknown tracepoint\n";
  }
}
