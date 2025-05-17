#include <assert.h>
#include <windowing/frame_pacer.h>
#include <containers/spsc_u64.h>

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

ULONG WINAPI BufferCallback(EVENT_TRACE_LOGFILEW *__attribute__((unused)
) _unused) {
  return TRUE;
}

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

  /* flip event timestamp */
  const UINT64 qpc          = pEvent->EventHeader.TimeStamp.QuadPart;
  static u32   flip_info_id = -1ul;

  if (flip_info_id == -1ul) {
    /* flip event index not yet found */
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
      flip_info_id = pEvent->EventHeader.EventDescriptor.Id;
      GAME_LOGF("flip event has index: %d", flip_info_id);
    } else {
      return;
    }
  }
  /* ...yes i know, but it changes between statements */
  if (flip_info_id != -1ul) {
    if (pEvent->EventHeader.EventDescriptor.Id != flip_info_id) {
      free(pInfo);
      return;
    }
    for (USHORT i = 0; i < pInfo->TopLevelPropertyCount; ++i) {
      EVENT_PROPERTY_INFO *pPropInfo = &pInfo->EventPropertyInfoArray[i];
      LPCWSTR pname = (LPCWSTR)((PBYTE)pInfo + pPropInfo->NameOffset);

      if (wcscmp(pname, L"VidPnSourceId") == 0) {
        /*
        we know the id already so we note this event;
        */
        PROPERTY_DATA_DESCRIPTOR dataDescriptor;
        ZeroMemory(&dataDescriptor, sizeof(PROPERTY_DATA_DESCRIPTOR));
        dataDescriptor.PropertyName = (ULONGLONG)pname;
        dataDescriptor.ArrayIndex   = ULONG_MAX;

        ULONG propertySize = 0;
        status             = TdhGetPropertySize(
          pEvent,
          0,
          NULL,
          1,
          &dataDescriptor,
          &propertySize
        );

        if (status != ERROR_SUCCESS) {
          wprintf(
            L"  %s: <ETW: Error retrieving size (0x%x)>\n",
            pname,
            status
          );
          free(pInfo);
          return;
        }
        assert(propertySize == sizeof(u32));

        /* making this a u64 even though it should be a u32 just to be safe and
         * future proof */
        u32 monitor_index = -1;

        status = TdhGetProperty(
          pEvent,
          0,
          NULL,
          1,
          &dataDescriptor,
          propertySize,
          (PBYTE)&monitor_index
        );

        if (status != ERROR_SUCCESS) {
          wprintf(L"  %s: <Error retrieving value (0x%x)>\n", pname, status);
          free(pInfo);
          return;
        }

        u64 last;
        if (monitor_index == 0) {
          spsc_u64_16_clear(flip_queue);
          // GAME_LOGF("recieved Flip event from kernel: timestamp: %llu",
          // qpc);
          spsc_u64_16_enqueue(flip_queue, qpc);
        }
        break;
      }
    }
  }
  free(pInfo);
}
