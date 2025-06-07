#include <math.h>
#ifdef _WIN64
#include <assert.h>
#include <containers/spsc_u64.h>
#include <windowing/frame_pacer.h>

const char *event_id_to_name(unsigned id) {
  switch (id) {
  case 0x0023: return "AdapterAllocation_DCStart";
  case 0x0021: return "AdapterAllocation_Start";
  case 0x0022: return "AdapterAllocation_Stop";
  case 0x01f5: return "BlitCancel_Info";
  case 0x00a6: return "Blit_Info";
  case 0x0020: return "Context_DCStart";
  case 0x001e: return "Context_Start";
  case 0x001f: return "Context_Stop";
  case 0x001d: return "Device_DCStart";
  case 0x001b: return "Device_Start";
  case 0x001c: return "Device_Stop";
  case 0x00b1: return "DmaPacket_Info";
  case 0x01c2: return "DmaPacket_Info_2";
  case 0x01c3: return "DmaPacket_Info_3";
  case 0x00af: return "DmaPacket_Start";
  case 0x00fc: return "FlipMultiPlaneOverlay_Info";
  case 0x00a8: return "Flip_Info";
  case 0x017e: return "HSyncDPCMultiPlane_Info";
  case 0x01a8: return "HwQueue_DCStart";
  case 0x01a6: return "HwQueue_Start";
  case 0x010a: return "IndependentFlip_Info";
  case 0x0182: return "MMIOFlipMultiPlaneOverlay3_Info";
  case 0x0103: return "MMIOFlipMultiPlaneOverlay_Info";
  case 0x0074: return "MMIOFlip_Info";
  case 0x00fa: return "NodeMetadata_Info";
  case 0x00d7: return "PresentHistoryDetailed_Start";
  case 0x00ac: return "PresentHistory_Info";
  case 0x00ab: return "PresentHistory_Start";
  case 0x00b8: return "Present_Info";
  case 0x00b2: return "QueuePacket_Start";
  case 0x00f4: return "QueuePacket_Start_2";
  case 0x00f5: return "QueuePacket_Start_3";
  case 0x00b4: return "QueuePacket_Stop";
  case 0x0111: return "VSyncDPCMultiPlane_Info";
  case 0x0011: return "VSyncDPC_Info";
  default:     return "invalid";
  }
}

/*
TODO: actually implement...
*/

ULONG WINAPI
BufferCallback(EVENT_TRACE_LOGFILEW *__attribute__((unused)) _unused) {
  return TRUE;
}

static u32 identify_flip_event(EVENT_RECORD *pEvent, TRACE_EVENT_INFO *pInfo) {
  bool pDmaBuffer_found       = false;
  bool VidPnSourceId_found    = false;
  bool FlipToAllocation_found = false;
  bool FlipInterval_found     = false;
  bool FlipWithNoWait_found   = false;
  bool MMIOFlip_found         = false;

  for (USHORT i = 0; i < pInfo->TopLevelPropertyCount; ++i) {
    EVENT_PROPERTY_INFO *pPropInfo = &pInfo->EventPropertyInfoArray[i];
    LPCWSTR pname = (LPCWSTR)((PBYTE)pInfo + pPropInfo->NameOffset);

    if (wcscmp(pname, L"pDmaBuffer") == 0) {
      pDmaBuffer_found = true;
    } else if (wcscmp(pname, L"VidPnSourceId") == 0) {
      VidPnSourceId_found = true;
    } else if (wcscmp(pname, L"FlipToAllocation") == 0) {
      FlipToAllocation_found = true;
    } else if (wcscmp(pname, L"FlipInterval") == 0) {
      FlipInterval_found = true;
    } else if (wcscmp(pname, L"FlipWithNoWait") == 0) {
      FlipWithNoWait_found = true;
    } else if (wcscmp(pname, L"MMIOFlip") == 0) {
      MMIOFlip_found = true;
    }
  }

  if (pDmaBuffer_found && VidPnSourceId_found && FlipToAllocation_found &&
      FlipInterval_found && FlipWithNoWait_found && MMIOFlip_found) {
    GAME_LOGF(
      "flip event has index: %d",
      pEvent->EventHeader.EventDescriptor.Id
    );
    return pEvent->EventHeader.EventDescriptor.Id;
  } else {
    return -1ul;
  }
}

static u32
identify_present_event(EVENT_RECORD *pEvent, TRACE_EVENT_INFO *pInfo) {
  bool hContext_found        = false;
  bool hWindow_found         = false;
  bool VidPnSourceId_found   = false;
  bool FlipInterval_found    = false;
  bool Flags_found           = false;
  bool hSrcAllocHandle_found = false;
  bool hDstAllocHandle_found = false;

  for (USHORT i = 0; i < pInfo->TopLevelPropertyCount; ++i) {
    EVENT_PROPERTY_INFO *pPropInfo = &pInfo->EventPropertyInfoArray[i];
    LPCWSTR pname = (LPCWSTR)((PBYTE)pInfo + pPropInfo->NameOffset);

    if (wcscmp(pname, L"hContext") == 0) {
      hContext_found = true;
    } else if (wcscmp(pname, L"hWindow") == 0) {
      hWindow_found = true;
    } else if (wcscmp(pname, L"VidPnSourceId") == 0) {
      VidPnSourceId_found = true;
    } else if (wcscmp(pname, L"FlipInterval") == 0) {
      FlipInterval_found = true;
    } else if (wcscmp(pname, L"Flags") == 0) {
      Flags_found = true;
    } else if (wcscmp(pname, L"hSrcAllocHandle") == 0) {
      hSrcAllocHandle_found = true;
    } else if (wcscmp(pname, L"hDstAllocHandle") == 0) {
      hDstAllocHandle_found = true;
    }
  }

  if (hContext_found && hWindow_found && VidPnSourceId_found &&
      FlipInterval_found && Flags_found && hSrcAllocHandle_found &&
      hDstAllocHandle_found) {
    GAME_LOGF(
      "present event has index: %d",
      pEvent->EventHeader.EventDescriptor.Id
    );
    return pEvent->EventHeader.EventDescriptor.Id;
  } else {
    return -1ul;
  }
}

static bool
event_for_right_monitor(EVENT_RECORD *pEvent, TRACE_EVENT_INFO *pInfo) {
  ULONG status;
  for (USHORT i = 0; i < pInfo->TopLevelPropertyCount; ++i) {
    EVENT_PROPERTY_INFO *pPropInfo = &pInfo->EventPropertyInfoArray[i];
    LPCWSTR pname = (LPCWSTR)((PBYTE)pInfo + pPropInfo->NameOffset);

    if (wcscmp(pname, L"VidPnSourceId") == 0) {
      PROPERTY_DATA_DESCRIPTOR dataDescriptor;
      ZeroMemory(&dataDescriptor, sizeof(PROPERTY_DATA_DESCRIPTOR));
      dataDescriptor.PropertyName = (ULONGLONG)pname;
      dataDescriptor.ArrayIndex   = ULONG_MAX;
      ULONG propertySize          = 0;

      status =
        TdhGetPropertySize(pEvent, 0, NULL, 1, &dataDescriptor, &propertySize);

      if (status != ERROR_SUCCESS) {
        wprintf(L"  %s: <ETW: Error retrieving size (0x%x)>\n", pname, status);
        return false;
      }

      /* its a u32 but we generally just care that its a word sized integer
      that we can != 0 with
      */
      assert(propertySize <= sizeof(u64));
      char *property_data = alloca(propertySize);

      status = TdhGetProperty(
        pEvent,
        0,
        NULL,
        1,
        &dataDescriptor,
        propertySize,
        (PBYTE)property_data
      );

      if (status != ERROR_SUCCESS) {
        wprintf(L"  %s: <Error retrieving value (0x%x)>\n", pname, status);
        return false;
      }

      /* monitor index should be 0 for main monitor */
      if ((*(u32 *)property_data) == 0) {
        return true;
      }
    }
  }

  return false;
}

/*
were not using TRACKED_MALLOC and TRACKED_FREE here
because the allocation logic is trivial and doesnt actually need to be
tracked.
*/
VOID WINAPI EventRecordCallback(EVENT_RECORD *pEvent) {
  ULONG             status;
  ULONG             bufferSize = 0;
  PTRACE_EVENT_INFO pInfo      = NULL;

  spsc_u64_16_ring_t *const flip_queue = pEvent->UserContext;

  status = TdhGetEventInformation(pEvent, 0, NULL, NULL, &bufferSize);
  if (status != ERROR_INSUFFICIENT_BUFFER) {
    wprintf(
      L"TdhGetEventInformation failed to get buffer size: 0x%x\n",
      status
    );
    return;
  }

  pInfo = (TRACE_EVENT_INFO *)malloc(bufferSize);
  if (pInfo == NULL) {
    wprintf(L"Failed to allocate memory for TRACE_EVENT_INFO\n");
    return;
  }

  status = TdhGetEventInformation(pEvent, 0, NULL, pInfo, &bufferSize);
  if (status != ERROR_SUCCESS) {
    wprintf(L"TdhGetEventInformation failed: 0x%x\n", status);
    free(pInfo);
    return;
  }

  if (pEvent->EventHeader.EventDescriptor.Channel != 0x11) {
    free(pInfo);
    return;
  }

  /* event timestamp */
  const UINT64 qpc = pEvent->EventHeader.TimeStamp.QuadPart;

  static LARGE_INTEGER pfc = {0};
  if (pfc.QuadPart == 0) {
    QueryPerformanceFrequency(&pfc);
  }

  f64 time_ns = (f64)qpc / (f64)pfc.QuadPart *
                1e9; /* i assume QPC / frequency gives me time in seconds? */

  // printf(
  //   "---------------- %s (%d) ----------------\n",
  //   event_id_to_name(pEvent->EventHeader.EventDescriptor.Id),
  //   pEvent->EventHeader.EventDescriptor.Id
  // );

  static u32 flip_event_id    = -1ul;
  static u32 present_event_id = -1ul;

  static f64 last_present_ts_ns = 0;
  static f64 last_flip_ts_ns    = 0;

  if (flip_event_id == -1ul) {
    flip_event_id = identify_flip_event(pEvent, pInfo);
  }

  if (present_event_id == -1ul) {
    present_event_id = identify_present_event(pEvent, pInfo);
  }

  if (pEvent->EventHeader.EventDescriptor.Id == present_event_id &&
      event_for_right_monitor(pEvent, pInfo)) {
    printf("PRESENT EVENT fired at: %llu\n", qpc);
    last_present_ts_ns = time_ns;
  }

  if (pEvent->EventHeader.EventDescriptor.Id == flip_event_id &&
      event_for_right_monitor(pEvent, pInfo)) {
    printf("FLIP EVENT fired    at: %llu\n", qpc);
    last_flip_ts_ns = time_ns;

    if (last_flip_ts_ns >= last_present_ts_ns) {
      printf(
        "LAST DRIVER PRESENT-FLIP LATENCY: %lf ms\n",
        (last_flip_ts_ns - last_present_ts_ns) / 1e6
      );
    }
  }

  free(pInfo);
}
#endif
