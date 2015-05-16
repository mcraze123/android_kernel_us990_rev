/*
 * Copyright (c) 2012-2014 The Linux Foundation. All rights reserved.
 *
 * Previously licensed under the ISC license by Qualcomm Atheros, Inc.
 *
 *
 * Permission to use, copy, modify, and/or distribute this software for
 * any purpose with or without fee is hereby granted, provided that the
 * above copyright notice and this permission notice appear in all
 * copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
 * WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
 * AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 * PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

/*
 * This file was originally distributed by Qualcomm Atheros, Inc.
 * under proprietary terms before Copyright ownership was assigned
 * to the Linux Foundation.
 */




/*===========================================================================
                       W L A N _ Q C T _ WDA . C
  OVERVIEW:
  This software unit holds the implementation of the WLAN Transport Layer.
  The functions externalized by this module are to be called ONLY by other
  WLAN modules that properly register with the Transport Layer initially.
  DEPENDENCIES:
  Are listed for each API below.

  Copyright (c) 2010-2011 QUALCOMM Incorporated.
  All Rights Reserved.
  Qualcomm Confidential and Proprietary
===========================================================================*/
/*                                                                           
                                           

                                                                       
                                                                

                             

                                         
                                                                         
                                                               
                                                   
                                                          
                                                                           */
#include "vos_mq.h" 
#include "vos_api.h" 
#include "vos_packet.h" 
#include "vos_nvitem.h"
#include "sirApi.h"
#include "wlan_qct_pal_packet.h"
#include "wlan_qct_pal_device.h"
#include "wlan_qct_wda.h"
#include "wlan_qct_wda_msg.h"
#include "wlan_qct_wdi_cfg.h"
#include "wlan_qct_wdi.h"
#include "wlan_qct_wdi_ds.h"
#include "wlan_hal_cfg.h"
/*                    */
#include "wniApi.h"
#include "cfgApi.h"
#include "limApi.h"
#include "wlan_qct_tl.h"
#include "wlan_qct_tli_ba.h"
#include "limUtils.h"
#include "btcApi.h"
#include "vos_sched.h"
#include "pttMsgApi.h"
#include "wlan_qct_sys.h"
/*              */
/*                                  */
#define WDA_2_4_GHZ_MAX_FREQ  3000
#define VOS_GET_WDA_CTXT(a)            vos_get_context(VOS_MODULE_ID_WDA, a)
#define VOS_GET_MAC_CTXT(a)            vos_get_context(VOS_MODULE_ID_PE, a)
#define WDA_BA_TX_FRM_THRESHOLD (5)
#define  CONVERT_WDI2SIR_STATUS(x) \
   ((WDI_STATUS_SUCCESS != (x)) ? eSIR_FAILURE : eSIR_SUCCESS)

#define  IS_WDI_STATUS_FAILURE(status) \
   ((WDI_STATUS_SUCCESS != (status)) && (WDI_STATUS_PENDING != (status)))
#define  CONVERT_WDI2VOS_STATUS(x) \
   ((IS_WDI_STATUS_FAILURE(x)) ? VOS_STATUS_E_FAILURE  : VOS_STATUS_SUCCESS)

/*                                             */
#define WDA_TL_GET_STA_STATE(a, b, c) WLANTL_GetSTAState(a, b, c)
#define WDA_TL_GET_TX_PKTCOUNT(a, b, c, d) WLANTL_GetTxPktCount(a, b, c, d)
#define WDA_GET_BA_TXFLAG(a, b, c)  \
   (((a)->wdaStaInfo[(b)].ucUseBaBitmap) & (1 << (c)))  

#define WDA_SET_BA_TXFLAG(a, b, c)  \
   (((a)->wdaStaInfo[(b)].ucUseBaBitmap) |= (1 << (c))) 

#define WDA_CLEAR_BA_TXFLAG(a, b, c)  \
   (((a)->wdaStaInfo[b].ucUseBaBitmap) &= ~(1 << c))
#define WDA_TL_BA_SESSION_ADD(a, b, c, d, e, f, g) \
   WLANTL_BaSessionAdd(a, b, c, d, e, f, g)
/*                      */
#define WDA_CREATE_TIMER(a, b, c, d, e, f, g) \
   tx_timer_create(a, b, c, d, e, f, g)
#define WDA_START_TIMER(a) tx_timer_activate(a)
#define WDA_STOP_TIMER(a) tx_timer_deactivate(a)
#define WDA_DESTROY_TIMER(a) tx_timer_delete(a)
#define WDA_WDI_START_TIMEOUT (WDI_RESPONSE_TIMEOUT + 5000)

#define WDA_LAST_POLLED_THRESHOLD(a, curSta, tid) \
   ((a)->wdaStaInfo[curSta].framesTxed[tid] + WDA_BA_TX_FRM_THRESHOLD)
#define WDA_BA_MAX_WINSIZE   (64)
#define WDA_INVALID_KEY_INDEX  0xFF
#define WDA_NUM_PWR_SAVE_CFG       11
#define WDA_TX_COMPLETE_TIME_OUT_VALUE 1000
#define WDA_TRAFFIC_STATS_TIME_OUT_VALUE 1000
  
#define WDA_MAX_RETRIES_TILL_RING_EMPTY  1000   /*                                  */

#define WDA_WAIT_MSEC_TILL_RING_EMPTY    10    /*                        */
#define WDA_IS_NULL_MAC_ADDRESS(mac_addr) \
   ((mac_addr[0] == 0x00) && (mac_addr[1] == 0x00) && (mac_addr[2] == 0x00) &&\
    (mac_addr[1] == 0x00) && (mac_addr[2] == 0x00) && (mac_addr[3] == 0x00))

#define WDA_MAC_ADDR_ARRAY(a) (a)[0], (a)[1], (a)[2], (a)[3], (a)[4], (a)[5]
#define WDA_MAC_ADDRESS_STR "%02x:%02x:%02x:%02x:%02x:%02x"
#define WDA_DUMPCMD_WAIT_TIMEOUT 10000

/*                     */
extern void vos_WDAComplete_cback(v_PVOID_t pVosContext);
extern wpt_uint8 WDI_GetActiveSessionsCount (void *pWDICtx, wpt_macAddr macBSSID, wpt_boolean skipBSSID);

/*                      */
void WDA_SendMsg(tWDA_CbContext *pWDA, tANI_U16 msgType, 
                                        void *pBodyptr, tANI_U32 bodyVal) ;
VOS_STATUS WDA_prepareConfigTLV(v_PVOID_t pVosContext, 
                                WDI_StartReqParamsType  *wdiStartParams ) ;
VOS_STATUS WDA_wdiCompleteCB(v_PVOID_t pVosContext) ;
VOS_STATUS WDA_ProcessSetTxPerTrackingReq(tWDA_CbContext *pWDA, tSirTxPerTrackingParam *pTxPerTrackingParams);

extern v_BOOL_t sys_validateStaConfig( void *pImage, unsigned long cbFile,
                               void **ppStaConfig, v_SIZE_t *pcbStaConfig ) ;
void processCfgDownloadReq(tpAniSirGlobal pMac) ;
void WDA_UpdateBSSParams(tWDA_CbContext *pWDA, 
        WDI_ConfigBSSReqInfoType *wdiBssParams, tAddBssParams *wdaBssParams) ;
void WDA_UpdateSTAParams(tWDA_CbContext *pWDA, 
        WDI_ConfigStaReqInfoType *wdiStaParams, tAddStaParams *wdaStaParams) ;
void WDA_lowLevelIndCallback(WDI_LowLevelIndType *wdiLowLevelInd, 
                                                          void* pUserData ) ;
static VOS_STATUS wdaCreateTimers(tWDA_CbContext *pWDA) ;
static VOS_STATUS wdaDestroyTimers(tWDA_CbContext *pWDA);
void WDA_BaCheckActivity(tWDA_CbContext *pWDA) ;
void WDA_TimerTrafficStatsInd(tWDA_CbContext *pWDA);
void WDA_HALDumpCmdCallback(WDI_HALDumpCmdRspParamsType *wdiRspParams, void* pUserData);
#ifdef WLAN_FEATURE_VOWIFI_11R
VOS_STATUS WDA_ProcessAggrAddTSReq(tWDA_CbContext *pWDA, tAggrAddTsParams *pAggrAddTsReqParams);
#endif /*                         */

void WDA_TimerHandler(v_VOID_t *pWDA, tANI_U32 timerInfo) ;
void WDA_ProcessTxCompleteTimeOutInd(tWDA_CbContext* pContext) ;
VOS_STATUS WDA_ResumeDataTx(tWDA_CbContext *pWDA);
#ifdef FEATURE_WLAN_SCAN_PNO
static VOS_STATUS WDA_ProcessSetPrefNetworkReq(tWDA_CbContext *pWDA, tSirPNOScanReq *pPNOScanReqParams);
static VOS_STATUS WDA_ProcessSetRssiFilterReq(tWDA_CbContext *pWDA, tSirSetRSSIFilterReq* pRssiFilterParams);
static VOS_STATUS WDA_ProcessUpdateScanParams(tWDA_CbContext *pWDA, tSirUpdateScanParams *pUpdateScanParams);
#endif //                      
#ifdef WLAN_FEATURE_ROAM_SCAN_OFFLOAD
VOS_STATUS WDA_ProcessRoamScanOffloadReq(tWDA_CbContext *pWDA,tSirRoamOffloadScanReq *pRoamOffloadScanReqParams);
void WDA_RoamOffloadScanReqCallback(WDI_Status status, void* pUserData);
void WDA_ConvertSirAuthToWDIAuth(WDI_AuthType *AuthType, v_U8_t csrAuthType);
void WDA_ConvertSirEncToWDIEnc(WDI_EdType *EncrType, v_U8_t csrEncrType);
#endif
#ifdef WLAN_FEATURE_PACKET_FILTERING
static VOS_STATUS WDA_Process8023MulticastListReq (
                                       tWDA_CbContext *pWDA,
                                       tSirRcvFltMcAddrList *pRcvFltMcAddrLis
                                                  );
static VOS_STATUS WDA_ProcessReceiveFilterSetFilterReq (
                                   tWDA_CbContext *pWDA,
                                   tSirRcvPktFilterCfgType *pRcvPktFilterCfg
                                                       );
static VOS_STATUS WDA_ProcessPacketFilterMatchCountReq (
                                   tWDA_CbContext *pWDA,
                                   tpSirRcvFltPktMatchRsp pRcvFltPktMatchRsp
                                                   );
static VOS_STATUS WDA_ProcessReceiveFilterClearFilterReq (
                               tWDA_CbContext *pWDA,
                               tSirRcvFltPktClearParam *pRcvFltPktClearParam
                                                         );
#endif //                              
VOS_STATUS WDA_ProcessSetPowerParamsReq(tWDA_CbContext *pWDA, tSirSetPowerParamsReq *pPowerParams);
static VOS_STATUS WDA_ProcessTxControlInd(tWDA_CbContext *pWDA,
                                          tpTxControlParams pTxCtrlParam);
VOS_STATUS WDA_GetWepKeysFromCfg( tWDA_CbContext *pWDA, 
                                                      v_U8_t *pDefaultKeyId,
                                                      v_U8_t *pNumKeys,
                                                      WDI_KeysType *pWdiKeys );

#ifdef WLAN_FEATURE_GTK_OFFLOAD
static VOS_STATUS WDA_ProcessGTKOffloadReq(tWDA_CbContext *pWDA, tpSirGtkOffloadParams pGtkOffloadParams);
static VOS_STATUS WDA_ProcessGTKOffloadGetInfoReq(tWDA_CbContext *pWDA, tpSirGtkOffloadGetInfoRspParams pGtkOffloadGetInfoRsp);
#endif //                         

v_VOID_t WDA_ProcessGetBcnMissRateReq(tWDA_CbContext *pWDA,
                                      tSirBcnMissRateReq *pData);

VOS_STATUS WDA_ProcessSetTmLevelReq(tWDA_CbContext *pWDA,
                                    tAniSetTmLevelReq *setTmLevelReq);
#ifdef WLAN_FEATURE_11AC
VOS_STATUS WDA_ProcessUpdateOpMode(tWDA_CbContext *pWDA, 
                                   tUpdateVHTOpMode *pData);
#endif

#ifdef FEATURE_WLAN_LPHB
VOS_STATUS WDA_ProcessLPHBConfReq(tWDA_CbContext *pWDA,
                                  tSirLPHBReq *pData);
#endif /*                   */

#ifdef WLAN_FEATURE_EXTSCAN
VOS_STATUS WDA_ProcessEXTScanStartReq(tWDA_CbContext *pWDA,
                                      tSirEXTScanStartReqParams *wdaRequest);
VOS_STATUS WDA_ProcessEXTScanStopReq(tWDA_CbContext *pWDA,
                                      tSirEXTScanStopReqParams *wdaRequest);
VOS_STATUS WDA_ProcessEXTScanGetCachedResultsReq(tWDA_CbContext *pWDA,
                            tSirEXTScanGetCachedResultsReqParams *wdaRequest);
VOS_STATUS WDA_ProcessEXTScanGetCapabilitiesReq(tWDA_CbContext *pWDA,
                            tSirGetEXTScanCapabilitiesReqParams *wdaRequest);
VOS_STATUS WDA_ProcessEXTScanSetBSSIDHotlistReq(tWDA_CbContext *pWDA,
                            tSirEXTScanSetBssidHotListReqParams *wdaRequest);
VOS_STATUS WDA_ProcessEXTScanResetBSSIDHotlistReq(tWDA_CbContext *pWDA,
                            tSirEXTScanResetBssidHotlistReqParams *wdaRequest);
VOS_STATUS WDA_ProcessEXTScanSetSignfRSSIChangeReq(tWDA_CbContext *pWDA,
                          tSirEXTScanSetSignificantChangeReqParams *wdaRequest);
VOS_STATUS WDA_ProcessEXTScanResetSignfRSSIChangeReq(tWDA_CbContext *pWDA,
                        tSirEXTScanResetSignificantChangeReqParams *wdaRequest);
#endif /*                      */

#ifdef WLAN_FEATURE_LINK_LAYER_STATS
VOS_STATUS WDA_ProcessLLStatsSetReq(tWDA_CbContext *pWDA,
                                      tSirLLStatsSetReq *wdaRequest);

VOS_STATUS WDA_ProcessLLStatsGetReq(tWDA_CbContext *pWDA,
                                      tSirLLStatsGetReq *wdaRequest);

VOS_STATUS WDA_ProcessLLStatsClearReq(tWDA_CbContext *pWDA,
                                      tSirLLStatsClearReq *wdaRequest);
#endif /*                               */
/*
                     
                            
 */ 
VOS_STATUS WDA_open(v_PVOID_t pVosContext, v_PVOID_t devHandle,
                                                tMacOpenParameters *pMacParams )
{
   tWDA_CbContext *wdaContext;
   VOS_STATUS status;
   WDI_DeviceCapabilityType wdiDevCapability = {0} ;
   /*                      */
   status = vos_alloc_context(pVosContext, VOS_MODULE_ID_WDA, 
                           (v_VOID_t **)&wdaContext, sizeof(tWDA_CbContext)) ;
   if(!VOS_IS_STATUS_SUCCESS(status))
   {
      return VOS_STATUS_E_NOMEM;
   }
   /*            */
   vos_mem_zero(wdaContext,sizeof(tWDA_CbContext));
   
   /*                            */
   wdaContext->pVosContext = pVosContext;
   wdaContext->wdaState = WDA_INIT_STATE;
   wdaContext->uTxFlowMask = WDA_TXFLOWMASK;
   
   /*                                          */
   status = vos_event_init(&wdaContext->wdaWdiEvent);
   if(!VOS_IS_STATUS_SUCCESS(status)) 
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                "WDI Sync Event init failed - status = %d", status);
      goto error;
   }
   /*                           */
   status = vos_event_init(&wdaContext->txFrameEvent);
   if(!VOS_IS_STATUS_SUCCESS(status)) 
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                "VOS Mgmt Frame Event init failed - status = %d", status);
      goto error;
   }
   status = vos_event_init(&wdaContext->suspendDataTxEvent);
   if(!VOS_IS_STATUS_SUCCESS(status)) 
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
            "VOS suspend data tx Event init failed - status = %d", status);
      goto error;
   }
   status = vos_event_init(&wdaContext->waitOnWdiIndicationCallBack);
   if(!VOS_IS_STATUS_SUCCESS(status)) 
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
            "VOS wait On Wdi Ind Event init failed - status = %d", status);
      goto error;
   }
   vos_trace_setLevel(VOS_MODULE_ID_WDA,VOS_TRACE_LEVEL_ERROR);
   wdaContext->driverMode = pMacParams->driverType;
   if(WDI_STATUS_SUCCESS != WDI_Init(devHandle, &wdaContext->pWdiContext,
                                     &wdiDevCapability, pMacParams->driverType))
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                                  "WDI Init failed" );
      goto error;
   }
   else 
   {
      pMacParams->maxStation = wdiDevCapability.ucMaxSTASupported ;
      pMacParams->maxBssId =  wdiDevCapability.ucMaxBSSSupported;
      pMacParams->frameTransRequired = wdiDevCapability.bFrameXtlSupported;
      /*                                   */
      wdaContext->wdaMaxSta = pMacParams->maxStation;
      /*                                                                      
                     
       */
      wdaContext->frameTransRequired = wdiDevCapability.bFrameXtlSupported;
   }
   return status;

error:
      vos_free_context(pVosContext, VOS_MODULE_ID_WDA, wdaContext);
      return VOS_STATUS_E_FAILURE;
}

/*
                         
                                        
 */ 
VOS_STATUS WDA_preStart(v_PVOID_t pVosContext)
{   
   VOS_STATUS vosStatus = VOS_STATUS_SUCCESS;
   vos_msg_t wdaMsg = {0} ;
   /*
                                                                 
    */ 
   wdaMsg.type = WNI_CFG_DNLD_REQ ; 
   wdaMsg.bodyptr = NULL;
   wdaMsg.bodyval = 0;
   /*                    */
   vosStatus = vos_mq_post_message( VOS_MQ_ID_WDA, &wdaMsg );
   if ( !VOS_IS_STATUS_SUCCESS(vosStatus) )
   {
      vosStatus = VOS_STATUS_E_BADMSG;
   }
   return( vosStatus );
}
/*
                                 
                                                                       
                                       
 */
void WDA_wdiStartCallback(WDI_StartRspParamsType *wdiRspParams,
                                                            void *pVosContext)
{
   tWDA_CbContext *wdaContext;
   VOS_STATUS status;
   if (NULL == pVosContext)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_FATAL,
                 "%s: Invoked with invalid pVosContext", __func__ );
      return;
   }
   wdaContext = VOS_GET_WDA_CTXT(pVosContext);
   if (NULL == wdaContext)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_FATAL,
                 "%s: Invoked with invalid wdaContext", __func__ );
      return;
   }
   if (WDI_STATUS_SUCCESS != wdiRspParams->wdiStatus)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_FATAL,
                 "%s: WDI_Start() failure reported", __func__ );
   }
   else
   {
      wdaContext->wdaState = WDA_START_STATE;
   }
   /*                                                              */
   wdaContext->wcnssWlanCompiledVersion.major =
      wdiRspParams->wlanCompiledVersion.major;
   wdaContext->wcnssWlanCompiledVersion.minor =
      wdiRspParams->wlanCompiledVersion.minor;
   wdaContext->wcnssWlanCompiledVersion.version =
      wdiRspParams->wlanCompiledVersion.version;
   wdaContext->wcnssWlanCompiledVersion.revision =
      wdiRspParams->wlanCompiledVersion.revision;
   wdaContext->wcnssWlanReportedVersion.major =
      wdiRspParams->wlanReportedVersion.major;
   wdaContext->wcnssWlanReportedVersion.minor =
      wdiRspParams->wlanReportedVersion.minor;
   wdaContext->wcnssWlanReportedVersion.version =
      wdiRspParams->wlanReportedVersion.version;
   wdaContext->wcnssWlanReportedVersion.revision =
      wdiRspParams->wlanReportedVersion.revision;
   wpalMemoryCopy(wdaContext->wcnssSoftwareVersionString,
           wdiRspParams->wcnssSoftwareVersion,
           sizeof(wdaContext->wcnssSoftwareVersionString));
   wpalMemoryCopy(wdaContext->wcnssHardwareVersionString,
           wdiRspParams->wcnssHardwareVersion,
           sizeof(wdaContext->wcnssHardwareVersionString));
   /*                                               */
   status = vos_event_set(&wdaContext->wdaWdiEvent);
   if (VOS_STATUS_SUCCESS != status)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_FATAL,
                 "%s: Unable to unblock WDA_start", __func__ );
   }
   return;
}

/*
                      
                                                
 */
VOS_STATUS WDA_start(v_PVOID_t pVosContext)
{
   tWDA_CbContext *wdaContext;
   VOS_STATUS status;
   WDI_Status wdiStatus;
   WDI_StartReqParamsType wdiStartParam;
   if (NULL == pVosContext)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                 "%s: Invoked with invalid pVosContext", __func__ );
      return VOS_STATUS_E_FAILURE;
   }
   wdaContext = VOS_GET_WDA_CTXT(pVosContext);
   if (NULL == wdaContext)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                 "%s: Invoked with invalid wdaContext", __func__ );
      return VOS_STATUS_E_FAILURE;
   }
   /*                                                
                                                         */
   if ( (WDA_INIT_STATE != wdaContext->wdaState) &&
        (WDA_STOP_STATE != wdaContext->wdaState) )
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                 "%s: Invoked from wrong state %d",
                 __func__, wdaContext->wdaState );
      return VOS_STATUS_E_FAILURE;
   }
   /*                                                               
                                                                  
                */
   vos_mem_set(&wdiStartParam, sizeof(wdiStartParam), 0);
   wdiStartParam.wdiDriverType = wdaContext->driverMode;
   /*                                    */
   status = WDA_prepareConfigTLV(pVosContext, &wdiStartParam);
   if ( !VOS_IS_STATUS_SUCCESS(status) )
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                 "%s: Unable to prepare Config TLV", __func__ );
      return VOS_STATUS_E_FAILURE;
   }
   /*                                                  
                                    */
   wdiStartParam.wdiLowLevelIndCB = WDA_lowLevelIndCallback;
   wdiStartParam.pIndUserData = (v_PVOID_t *)wdaContext;
   wdiStartParam.wdiReqStatusCB = NULL;
   /*                                              */
   vos_event_reset(&wdaContext->wdaWdiEvent);
   /*                */
   wdiStatus = WDI_Start(&wdiStartParam,
                         (WDI_StartRspCb)WDA_wdiStartCallback,
                         (v_VOID_t *)pVosContext);
   if ( IS_WDI_STATUS_FAILURE(wdiStatus) )
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_FATAL,
                 "%s: WDI Start failed", __func__ );
      vos_mem_free(wdiStartParam.pConfigBuffer);
      return VOS_STATUS_E_FAILURE;
   }
   /*                                           */
   status = vos_wait_single_event( &wdaContext->wdaWdiEvent,
                                   WDA_WDI_START_TIMEOUT );
   if ( !VOS_IS_STATUS_SUCCESS(status) )
   {
      if ( VOS_STATUS_E_TIMEOUT == status )
      {
         VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                    "%s: Timeout occurred during WDI_Start", __func__ );
      }
      else
      {
         VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                    "%s: Error %d while waiting for WDI_Start",
                    __func__, status);
      }
      vos_mem_free(wdiStartParam.pConfigBuffer);
      return status;
   }
   /*                                                     */
   /*                                  */
   vos_mem_free(wdiStartParam.pConfigBuffer);
   /*                                                          */
   if (WDA_START_STATE != wdaContext->wdaState)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                 "%s: WDI_Start() failure detected", __func__ );
      return VOS_STATUS_E_FAILURE;
   }
   /*                                               */
   if ( eDRIVER_TYPE_MFG != wdaContext->driverMode )
   {
      status = wdaCreateTimers(wdaContext) ;
      if(VOS_STATUS_SUCCESS == status)
      {
         wdaContext->wdaTimersCreated = VOS_TRUE;
      }
   }
   else
   {
      vos_event_init(&wdaContext->ftmStopDoneEvent);
   }
   return status;
}

/*
                                 
                                       
 */
VOS_STATUS WDA_prepareConfigTLV(v_PVOID_t pVosContext, 
                            WDI_StartReqParamsType  *wdiStartParams )
{
   /*                                 */
   tpAniSirGlobal pMac = (tpAniSirGlobal )VOS_GET_MAC_CTXT(pVosContext);
   tWDA_CbContext *wdaContext= (tWDA_CbContext *)VOS_GET_WDA_CTXT(pVosContext);
   tHalCfg        *tlvStruct = NULL ;
   tANI_U8        *tlvStructStart = NULL ;
   tANI_U32       strLength = WNI_CFG_STA_ID_LEN;
   v_PVOID_t      *configParam;
   tANI_U32       configParamSize;
   tANI_U32       *configDataValue;
   WDI_WlanVersionType wcnssCompiledApiVersion;
   tANI_U8        i;

   if ((NULL == pMac)||(NULL == wdaContext))
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                 "%s: Invoked with invalid wdaContext or pMac", __func__ );
      VOS_ASSERT(0);
      return VOS_STATUS_E_FAILURE;
   }
   configParamSize = (sizeof(tHalCfg) * QWLAN_HAL_CFG_MAX_PARAMS) + 
                           WNI_CFG_STA_ID_LEN +
                           WNI_CFG_EDCA_WME_ACBK_LEN +
                           WNI_CFG_EDCA_WME_ACBE_LEN +
                           WNI_CFG_EDCA_WME_ACVI_LEN +
                           WNI_CFG_EDCA_WME_ACVO_LEN +
                           + (QWLAN_HAL_CFG_INTEGER_PARAM * sizeof(tANI_U32));
   /*                                           */ 
   configParam = vos_mem_malloc(configParamSize);
   
   if(NULL == configParam )
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                           "%s:configParam is NULL", __func__);
      VOS_ASSERT(0) ;
      return VOS_STATUS_E_NOMEM;
   }
   vos_mem_set(configParam, configParamSize, 0);
   wdiStartParams->pConfigBuffer = configParam;
   tlvStruct = (tHalCfg *)configParam;
   tlvStructStart = (tANI_U8 *)configParam;
   /*                    */
   /*                      */
   tlvStruct->type = QWLAN_HAL_CFG_STA_ID;
   configDataValue = (tANI_U32*)((tANI_U8 *) tlvStruct + sizeof(tHalCfg));
   if(wlan_cfgGetStr(pMac, WNI_CFG_STA_ID, (tANI_U8*)configDataValue, &strLength) != 
                                                                eSIR_SUCCESS)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                                   "Failed to get value for WNI_CFG_STA_ID");
      goto handle_failure;
   }
   tlvStruct->length = strLength ;
   /*                                                           */
   tlvStruct->padBytes = ALIGNED_WORD_SIZE - 
                              (tlvStruct->length & (ALIGNED_WORD_SIZE - 1));
   tlvStruct = (tHalCfg *)(( (tANI_U8 *) tlvStruct 
               + sizeof(tHalCfg) + tlvStruct->length + tlvStruct->padBytes)) ;
   /*                                  */
   tlvStruct->type = QWLAN_HAL_CFG_CURRENT_TX_ANTENNA;
   tlvStruct->length = sizeof(tANI_U32);
   configDataValue = (tANI_U32 *)(tlvStruct + 1);
   if(wlan_cfgGetInt(pMac, WNI_CFG_CURRENT_TX_ANTENNA, configDataValue ) 
                                                    != eSIR_SUCCESS)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                          "Failed to get value for WNI_CFG_CURRENT_TX_ANTENNA");
      goto handle_failure;
   }
   tlvStruct = (tHalCfg *)(( (tANI_U8 *) tlvStruct 
                            + sizeof(tHalCfg) + tlvStruct->length)) ; 
   /*                                  */
   tlvStruct->type = QWLAN_HAL_CFG_CURRENT_RX_ANTENNA;
   tlvStruct->length = sizeof(tANI_U32);
   configDataValue = (tANI_U32 *)(tlvStruct + 1);
   if(wlan_cfgGetInt(pMac, WNI_CFG_CURRENT_RX_ANTENNA, configDataValue) != 
                                                                  eSIR_SUCCESS)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                    "Failed to get value for WNI_CFG_CURRENT_RX_ANTENNA");
      goto handle_failure;
   }
   tlvStruct = (tHalCfg *)(( (tANI_U8 *) tlvStruct 
                            + sizeof(tHalCfg) + tlvStruct->length)) ; 
   /*                                 */
   tlvStruct->type = QWLAN_HAL_CFG_LOW_GAIN_OVERRIDE;
   tlvStruct->length = sizeof(tANI_U32);
   configDataValue = (tANI_U32 *)(tlvStruct + 1);
   if(wlan_cfgGetInt(pMac, WNI_CFG_LOW_GAIN_OVERRIDE, configDataValue ) 
                                                 != eSIR_SUCCESS)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                    "Failed to get value for WNI_CFG_LOW_GAIN_OVERRIDE");
      goto handle_failure;
   }
   tlvStruct = (tHalCfg *)(( (tANI_U8 *) tlvStruct 
                            + sizeof(tHalCfg) + tlvStruct->length)) ;
 
   /*                                     */
   tlvStruct->type = QWLAN_HAL_CFG_POWER_STATE_PER_CHAIN;
   tlvStruct->length = sizeof(tANI_U32);
   configDataValue = (tANI_U32 *)(tlvStruct + 1);
   if(wlan_cfgGetInt(pMac, WNI_CFG_POWER_STATE_PER_CHAIN, 
                                            configDataValue ) != eSIR_SUCCESS)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                    "Failed to get value for WNI_CFG_POWER_STATE_PER_CHAIN");
      goto handle_failure;
   }
   tlvStruct = (tHalCfg *)(( (tANI_U8 *) tlvStruct 
                            + sizeof(tHalCfg) + tlvStruct->length)); 
   /*                          */
   tlvStruct->type = QWLAN_HAL_CFG_CAL_PERIOD;
   tlvStruct->length = sizeof(tANI_U32);
   configDataValue = (tANI_U32 *)(tlvStruct + 1);
   if(wlan_cfgGetInt(pMac, WNI_CFG_CAL_PERIOD, configDataValue ) 
                                                  != eSIR_SUCCESS)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                    "Failed to get value for WNI_CFG_CAL_PERIOD");
      goto handle_failure;
   }
   tlvStruct = (tHalCfg *)(( (tANI_U8 *) tlvStruct 
                            + sizeof(tHalCfg) + tlvStruct->length));
   /*                            */
   tlvStruct->type = QWLAN_HAL_CFG_CAL_CONTROL ;
   tlvStruct->length = sizeof(tANI_U32);
   configDataValue = (tANI_U32 *)(tlvStruct + 1);
   if(wlan_cfgGetInt(pMac, WNI_CFG_CAL_CONTROL, configDataValue ) 
                                                   != eSIR_SUCCESS)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                    "Failed to get value for WNI_CFG_CAL_CONTROL");
      goto handle_failure;
   }
   tlvStruct = (tHalCfg *)(( (tANI_U8 *) tlvStruct 
                            + sizeof(tHalCfg) + tlvStruct->length));
   /*                          */
   tlvStruct->type = QWLAN_HAL_CFG_PROXIMITY ;
   tlvStruct->length = sizeof(tANI_U32);
   configDataValue = (tANI_U32 *)(tlvStruct + 1);
   if(wlan_cfgGetInt(pMac, WNI_CFG_PROXIMITY, configDataValue ) 
                                                   != eSIR_SUCCESS)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                    "Failed to get value for WNI_CFG_PROXIMITY");
      goto handle_failure;
   }
   tlvStruct = (tHalCfg *)(( (tANI_U8 *) tlvStruct 
                             + sizeof(tHalCfg) + tlvStruct->length)) ; 
   /*                                */
   tlvStruct->type = QWLAN_HAL_CFG_NETWORK_DENSITY ;
   tlvStruct->length = sizeof(tANI_U32);
   configDataValue = (tANI_U32 *)(tlvStruct + 1);
   if(wlan_cfgGetInt(pMac, WNI_CFG_NETWORK_DENSITY, configDataValue ) 
                                                   != eSIR_SUCCESS)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                    "Failed to get value for WNI_CFG_NETWORK_DENSITY");
      goto handle_failure;
   }
   tlvStruct = (tHalCfg *)(( (tANI_U8 *) tlvStruct 
                            + sizeof(tHalCfg) + tlvStruct->length)); 
   /*                                */
   tlvStruct->type = QWLAN_HAL_CFG_MAX_MEDIUM_TIME ;
   tlvStruct->length = sizeof(tANI_U32);
   configDataValue = (tANI_U32 *)(tlvStruct + 1);
   if(wlan_cfgGetInt(pMac, WNI_CFG_MAX_MEDIUM_TIME, configDataValue ) != 
                                                                 eSIR_SUCCESS)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                    "Failed to get value for WNI_CFG_MAX_MEDIUM_TIME");
      goto handle_failure;
   }
   tlvStruct = (tHalCfg *)(( (tANI_U8 *) tlvStruct 
                            + sizeof(tHalCfg) + tlvStruct->length)); 
   /*                                    */
   tlvStruct->type = QWLAN_HAL_CFG_MAX_MPDUS_IN_AMPDU  ;
   tlvStruct->length = sizeof(tANI_U32);
   configDataValue = (tANI_U32 *)(tlvStruct + 1);
   if(wlan_cfgGetInt(pMac, WNI_CFG_MAX_MPDUS_IN_AMPDU,
                                            configDataValue ) != eSIR_SUCCESS)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                    "Failed to get value for WNI_CFG_MAX_MPDUS_IN_AMPDU");
      goto handle_failure;
   }
   tlvStruct = (tHalCfg *)(( (tANI_U8 *) tlvStruct 
                            + sizeof(tHalCfg) + tlvStruct->length)) ;
   /*                               */
   tlvStruct->type = QWLAN_HAL_CFG_RTS_THRESHOLD  ;
   tlvStruct->length = sizeof(tANI_U32);
   configDataValue = (tANI_U32 *)(tlvStruct + 1);
   if(wlan_cfgGetInt(pMac, WNI_CFG_RTS_THRESHOLD, configDataValue ) != 
                                                                  eSIR_SUCCESS)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                    "Failed to get value for WNI_CFG_RTS_THRESHOLD");
      goto handle_failure;
   }
   tlvStruct = (tHalCfg *)(( (tANI_U8 *) tlvStruct 
                            + sizeof(tHalCfg) + tlvStruct->length));
   /*                                   */
   tlvStruct->type = QWLAN_HAL_CFG_SHORT_RETRY_LIMIT  ;
   tlvStruct->length = sizeof(tANI_U32);
   configDataValue = (tANI_U32 *)(tlvStruct + 1);
   if(wlan_cfgGetInt(pMac, WNI_CFG_SHORT_RETRY_LIMIT, configDataValue ) != 
                                                                 eSIR_SUCCESS)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                    "Failed to get value for WNI_CFG_SHORT_RETRY_LIMIT");
      goto handle_failure;
   }
   tlvStruct = (tHalCfg *)(( (tANI_U8 *) tlvStruct 
                            + sizeof(tHalCfg) + tlvStruct->length)) ; 
   /*                                  */
   tlvStruct->type = QWLAN_HAL_CFG_LONG_RETRY_LIMIT  ;
   tlvStruct->length = sizeof(tANI_U32);
   configDataValue = (tANI_U32 *)(tlvStruct + 1);
   if(wlan_cfgGetInt(pMac, WNI_CFG_LONG_RETRY_LIMIT, configDataValue ) != 
                                                                 eSIR_SUCCESS)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                    "Failed to get value for WNI_CFG_LONG_RETRY_LIMIT");
      goto handle_failure;
   }
   tlvStruct = (tHalCfg *)(( (tANI_U8 *) tlvStruct 
                            + sizeof(tHalCfg) + tlvStruct->length)) ;
   /*                                         */
   tlvStruct->type = QWLAN_HAL_CFG_FRAGMENTATION_THRESHOLD  ;
   tlvStruct->length = sizeof(tANI_U32);
   configDataValue = (tANI_U32 *)(tlvStruct + 1);
   if(wlan_cfgGetInt(pMac, WNI_CFG_FRAGMENTATION_THRESHOLD, 
                                             configDataValue ) != eSIR_SUCCESS)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                    "Failed to get value for WNI_CFG_FRAGMENTATION_THRESHOLD");
      goto handle_failure;
   }
   tlvStruct = (tHalCfg *)(( (tANI_U8 *) tlvStruct 
                            + sizeof(tHalCfg) + tlvStruct->length)) ;
   /*                                        */
   tlvStruct->type = QWLAN_HAL_CFG_DYNAMIC_THRESHOLD_ZERO  ;
   tlvStruct->length = sizeof(tANI_U32);
   configDataValue = (tANI_U32 *)(tlvStruct + 1);
   if(wlan_cfgGetInt(pMac, WNI_CFG_DYNAMIC_THRESHOLD_ZERO,
                                             configDataValue ) != eSIR_SUCCESS)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                    "Failed to get value for WNI_CFG_DYNAMIC_THRESHOLD_ZERO");
      goto handle_failure;
   }
   tlvStruct = (tHalCfg *)(( (tANI_U8 *) tlvStruct 
                            + sizeof(tHalCfg) + tlvStruct->length));
 
   /*                                       */
   tlvStruct->type = QWLAN_HAL_CFG_DYNAMIC_THRESHOLD_ONE  ;
   tlvStruct->length = sizeof(tANI_U32);
   configDataValue = (tANI_U32 *)(tlvStruct + 1);
   if(wlan_cfgGetInt(pMac, WNI_CFG_DYNAMIC_THRESHOLD_ONE, 
                                             configDataValue ) != eSIR_SUCCESS)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                    "Failed to get value for WNI_CFG_DYNAMIC_THRESHOLD_ONE");
      goto handle_failure;
   }
   tlvStruct = (tHalCfg *)(( (tANI_U8 *) tlvStruct 
                            + sizeof(tHalCfg) + tlvStruct->length)); 
   /*                                       */
   tlvStruct->type = QWLAN_HAL_CFG_DYNAMIC_THRESHOLD_TWO  ;
   tlvStruct->length = sizeof(tANI_U32);
   configDataValue = (tANI_U32 *)(tlvStruct + 1);
   if(wlan_cfgGetInt(pMac, WNI_CFG_DYNAMIC_THRESHOLD_TWO, 
                                             configDataValue ) != eSIR_SUCCESS)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                    "Failed to get value for WNI_CFG_DYNAMIC_THRESHOLD_TWO");
      goto handle_failure;
   }
   tlvStruct = (tHalCfg *)(( (tANI_U8 *) tlvStruct 
                            + sizeof(tHalCfg) + tlvStruct->length));
 
   /*                            */
   tlvStruct->type = QWLAN_HAL_CFG_FIXED_RATE  ;
   tlvStruct->length = sizeof(tANI_U32);
   configDataValue = (tANI_U32 *)(tlvStruct + 1);
   if(wlan_cfgGetInt(pMac, WNI_CFG_FIXED_RATE, configDataValue) 
                                                      != eSIR_SUCCESS)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                    "Failed to get value for WNI_CFG_FIXED_RATE");
      goto handle_failure;
   }
   tlvStruct = (tHalCfg *)(( (tANI_U8 *) tlvStruct 
                            + sizeof(tHalCfg) + tlvStruct->length));
 
   /*                                  */
   tlvStruct->type = QWLAN_HAL_CFG_RETRYRATE_POLICY  ;
   tlvStruct->length = sizeof(tANI_U32);
   configDataValue = (tANI_U32 *)(tlvStruct + 1);
   if(wlan_cfgGetInt(pMac, WNI_CFG_RETRYRATE_POLICY, configDataValue ) 
                                                         != eSIR_SUCCESS)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                    "Failed to get value for WNI_CFG_RETRYRATE_POLICY");
      goto handle_failure;
   }
   tlvStruct = (tHalCfg *)(( (tANI_U8 *) tlvStruct 
                            + sizeof(tHalCfg) + tlvStruct->length));
 
   /*                                     */
   tlvStruct->type = QWLAN_HAL_CFG_RETRYRATE_SECONDARY  ;
   tlvStruct->length = sizeof(tANI_U32);
   configDataValue = (tANI_U32 *)(tlvStruct + 1);
   if(wlan_cfgGetInt(pMac, WNI_CFG_RETRYRATE_SECONDARY, 
                                              configDataValue ) != eSIR_SUCCESS)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                    "Failed to get value for WNI_CFG_RETRYRATE_SECONDARY");
      goto handle_failure;
   }
   tlvStruct = (tHalCfg *)(( (tANI_U8 *) tlvStruct 
                            + sizeof(tHalCfg) + tlvStruct->length)) ; 
   /*                                    */
   tlvStruct->type = QWLAN_HAL_CFG_RETRYRATE_TERTIARY  ;
   tlvStruct->length = sizeof(tANI_U32);
   configDataValue = (tANI_U32 *)(tlvStruct + 1);
   if(wlan_cfgGetInt(pMac, WNI_CFG_RETRYRATE_TERTIARY, 
                                              configDataValue ) != eSIR_SUCCESS)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                    "Failed to get value for WNI_CFG_RETRYRATE_TERTIARY");
      goto handle_failure;
   }
   tlvStruct = (tHalCfg *)(( (tANI_U8 *) tlvStruct 
                            + sizeof(tHalCfg) + tlvStruct->length)) ; 
   /*                                         */
   tlvStruct->type = QWLAN_HAL_CFG_FORCE_POLICY_PROTECTION  ;
   tlvStruct->length = sizeof(tANI_U32);
   configDataValue = (tANI_U32 *)(tlvStruct + 1);
   if(wlan_cfgGetInt(pMac, WNI_CFG_FORCE_POLICY_PROTECTION, 
                                              configDataValue ) != eSIR_SUCCESS)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                    "Failed to get value for WNI_CFG_FORCE_POLICY_PROTECTION");
      goto handle_failure;
   }
   tlvStruct = (tHalCfg *)( (tANI_U8 *) tlvStruct 
                            + sizeof(tHalCfg) + tlvStruct->length); 
   /*                                            */
   tlvStruct->type = QWLAN_HAL_CFG_FIXED_RATE_MULTICAST_24GHZ  ;
   tlvStruct->length = sizeof(tANI_U32);
   configDataValue = (tANI_U32 *)(tlvStruct + 1);
   if(wlan_cfgGetInt(pMac, WNI_CFG_FIXED_RATE_MULTICAST_24GHZ, 
                                              configDataValue ) != eSIR_SUCCESS)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                 "Failed to get value for WNI_CFG_FIXED_RATE_MULTICAST_24GHZ");
      goto handle_failure;
   }
   tlvStruct = (tHalCfg *)( (tANI_U8 *) tlvStruct 
                            + sizeof(tHalCfg) + tlvStruct->length); 
   /*                                           */
   tlvStruct->type = QWLAN_HAL_CFG_FIXED_RATE_MULTICAST_5GHZ  ;
   tlvStruct->length = sizeof(tANI_U32);
   configDataValue = (tANI_U32 *)(tlvStruct + 1);
   if(wlan_cfgGetInt(pMac, WNI_CFG_FIXED_RATE_MULTICAST_5GHZ, 
                                              configDataValue ) != eSIR_SUCCESS)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                 "Failed to get value for WNI_CFG_FIXED_RATE_MULTICAST_5GHZ");
      goto handle_failure;
   }
   tlvStruct = (tHalCfg *)( (tANI_U8 *) tlvStruct 
                            + sizeof(tHalCfg) + tlvStruct->length); 
   
   /*                                          */
   tlvStruct->type = QWLAN_HAL_CFG_DEFAULT_RATE_INDEX_24GHZ  ;
   tlvStruct->length = sizeof(tANI_U32);
   configDataValue = (tANI_U32 *)(tlvStruct + 1);
   if(wlan_cfgGetInt(pMac, WNI_CFG_DEFAULT_RATE_INDEX_24GHZ, 
                                              configDataValue ) != eSIR_SUCCESS)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                    "Failed to get value for WNI_CFG_DEFAULT_RATE_INDEX_24GHZ");
      goto handle_failure;
   }
   tlvStruct = (tHalCfg *)( (tANI_U8 *) tlvStruct 
                            + sizeof(tHalCfg) + tlvStruct->length); 
   /*                                         */
   tlvStruct->type = QWLAN_HAL_CFG_DEFAULT_RATE_INDEX_5GHZ  ;
   tlvStruct->length = sizeof(tANI_U32);
   configDataValue = (tANI_U32 *)(tlvStruct + 1);
   if(wlan_cfgGetInt(pMac, WNI_CFG_DEFAULT_RATE_INDEX_5GHZ, 
                                              configDataValue ) != eSIR_SUCCESS)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                    "Failed to get value for WNI_CFG_DEFAULT_RATE_INDEX_5GHZ");
      goto handle_failure;
   }
   tlvStruct = (tHalCfg *)( (tANI_U8 *) tlvStruct 
                             + sizeof(tHalCfg) + tlvStruct->length); 
   /*                                 */
   tlvStruct->type = QWLAN_HAL_CFG_MAX_BA_SESSIONS  ;
   tlvStruct->length = sizeof(tANI_U32);
   configDataValue = (tANI_U32 *)(tlvStruct + 1);
   if(wlan_cfgGetInt(pMac, WNI_CFG_MAX_BA_SESSIONS, configDataValue ) != 
                                                                eSIR_SUCCESS)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                    "Failed to get value for WNI_CFG_MAX_BA_SESSIONS");
      goto handle_failure;
   }
   tlvStruct = (tHalCfg *)( (tANI_U8 *) tlvStruct 
                             + sizeof(tHalCfg) + tlvStruct->length);
 
   /*                                            */
   tlvStruct->type = QWLAN_HAL_CFG_PS_DATA_INACTIVITY_TIMEOUT  ;
   tlvStruct->length = sizeof(tANI_U32);
   configDataValue = (tANI_U32 *)(tlvStruct + 1);
   if(wlan_cfgGetInt(pMac, WNI_CFG_PS_DATA_INACTIVITY_TIMEOUT, 
                                            configDataValue ) != eSIR_SUCCESS)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
               "Failed to get value for WNI_CFG_PS_DATA_INACTIVITY_TIMEOUT");
      goto handle_failure;
   }
   tlvStruct = (tHalCfg *)( (tANI_U8 *) tlvStruct 
                             + sizeof(tHalCfg) + tlvStruct->length); 
   /*                                      */
   tlvStruct->type = QWLAN_HAL_CFG_PS_ENABLE_BCN_FILTER  ;
   tlvStruct->length = sizeof(tANI_U32);
   configDataValue = (tANI_U32 *)(tlvStruct + 1);
   if(wlan_cfgGetInt(pMac, WNI_CFG_PS_ENABLE_BCN_FILTER, 
                                              configDataValue ) != eSIR_SUCCESS)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                    "Failed to get value for WNI_CFG_PS_ENABLE_BCN_FILTER");
      goto handle_failure;
   }
   tlvStruct = (tHalCfg *)( (tANI_U8 *) tlvStruct 
                             + sizeof(tHalCfg) + tlvStruct->length); 
   /*                                        */
   tlvStruct->type = QWLAN_HAL_CFG_PS_ENABLE_RSSI_MONITOR  ;
   tlvStruct->length = sizeof(tANI_U32);
   configDataValue = (tANI_U32 *)(tlvStruct + 1);
   if(wlan_cfgGetInt(pMac, WNI_CFG_PS_ENABLE_RSSI_MONITOR, 
                                              configDataValue ) != eSIR_SUCCESS)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                    "Failed to get value for WNI_CFG_PS_ENABLE_RSSI_MONITOR");
      goto handle_failure;
   }
   tlvStruct = (tHalCfg *)( (tANI_U8 *) tlvStruct 
                              + sizeof(tHalCfg) + tlvStruct->length); 
   /*                                             */
   tlvStruct->type = QWLAN_HAL_CFG_NUM_BEACON_PER_RSSI_AVERAGE  ;
   tlvStruct->length = sizeof(tANI_U32);
   configDataValue = (tANI_U32 *)(tlvStruct + 1);
   if(wlan_cfgGetInt(pMac, WNI_CFG_NUM_BEACON_PER_RSSI_AVERAGE, 
                                            configDataValue ) != eSIR_SUCCESS)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                "Failed to get value for WNI_CFG_NUM_BEACON_PER_RSSI_AVERAGE");
      goto handle_failure;
   }
   tlvStruct = (tHalCfg *)( (tANI_U8 *) tlvStruct 
                              + sizeof(tHalCfg) + tlvStruct->length);
 
   /*                              */
   tlvStruct->type = QWLAN_HAL_CFG_STATS_PERIOD  ;
   tlvStruct->length = sizeof(tANI_U32);
   configDataValue = (tANI_U32 *)(tlvStruct + 1);
   if(wlan_cfgGetInt(pMac, WNI_CFG_STATS_PERIOD, configDataValue ) != 
                                                                eSIR_SUCCESS)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                    "Failed to get value for WNI_CFG_STATS_PERIOD");
      goto handle_failure;
   }
   tlvStruct = (tHalCfg *)( (tANI_U8 *) tlvStruct 
                               + sizeof(tHalCfg) + tlvStruct->length); 
   /*                                  */
   tlvStruct->type = QWLAN_HAL_CFG_CFP_MAX_DURATION  ;
   tlvStruct->length = sizeof(tANI_U32);
   configDataValue = (tANI_U32 *)(tlvStruct + 1);
   if(wlan_cfgGetInt(pMac, WNI_CFG_CFP_MAX_DURATION, configDataValue ) != 
                                                                eSIR_SUCCESS)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                    "Failed to get value for WNI_CFG_CFP_MAX_DURATION");
      goto handle_failure;
   }
   tlvStruct = (tHalCfg *)( (tANI_U8 *) tlvStruct 
                            + sizeof(tHalCfg) + tlvStruct->length) ; 
   /*                                   */
   tlvStruct->type = QWLAN_HAL_CFG_FRAME_TRANS_ENABLED  ;
   tlvStruct->length = sizeof(tANI_U32);
   configDataValue = (tANI_U32 *)(tlvStruct + 1);
   vos_mem_copy(configDataValue, &wdaContext->frameTransRequired, 
                                               sizeof(tANI_U32));
   tlvStruct = (tHalCfg *)( (tANI_U8 *) tlvStruct 
                            + sizeof(tHalCfg) + tlvStruct->length) ; 
   /*                           */
   tlvStruct->type = QWLAN_HAL_CFG_DTIM_PERIOD  ;
   tlvStruct->length = sizeof(tANI_U32);
   configDataValue = (tANI_U32 *)(tlvStruct + 1);
   if(wlan_cfgGetInt(pMac, WNI_CFG_DTIM_PERIOD, configDataValue) 
                                                   != eSIR_SUCCESS)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                    "Failed to get value for WNI_CFG_DTIM_PERIOD");
      goto handle_failure;
   }
   tlvStruct = (tHalCfg *)( (tANI_U8 *) tlvStruct 
                            + sizeof(tHalCfg) + tlvStruct->length) ; 
   /*                             */
   tlvStruct->type = QWLAN_HAL_CFG_EDCA_WMM_ACBK  ;
   strLength = WNI_CFG_EDCA_WME_ACBK_LEN;
   configDataValue = (tANI_U32 *)(tlvStruct + 1);
   if(wlan_cfgGetStr(pMac, WNI_CFG_EDCA_WME_ACBK, (tANI_U8 *)configDataValue,
                                             &strLength) != eSIR_SUCCESS)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                    "Failed to get value for WNI_CFG_EDCA_WME_ACBK");
      goto handle_failure;
   }
   tlvStruct->length = strLength;
   /*                                                           */
   tlvStruct->padBytes = ALIGNED_WORD_SIZE - 
                              (tlvStruct->length & (ALIGNED_WORD_SIZE - 1));
   tlvStruct = (tHalCfg *)( (tANI_U8 *) tlvStruct 
                 + sizeof(tHalCfg) + tlvStruct->length + tlvStruct->padBytes) ; 
   /*                             */
   tlvStruct->type = QWLAN_HAL_CFG_EDCA_WMM_ACBE  ;
   strLength = WNI_CFG_EDCA_WME_ACBE_LEN;
   configDataValue = (tANI_U32 *)(tlvStruct + 1);
   if(wlan_cfgGetStr(pMac, WNI_CFG_EDCA_WME_ACBE, (tANI_U8 *)configDataValue,
                                             &strLength) != eSIR_SUCCESS)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                    "Failed to get value for WNI_CFG_EDCA_WME_ACBE");
      goto handle_failure;
   }
   tlvStruct->length = strLength;
   /*                                                           */
   tlvStruct->padBytes = ALIGNED_WORD_SIZE - 
                              (tlvStruct->length & (ALIGNED_WORD_SIZE - 1));
   tlvStruct = (tHalCfg *)( (tANI_U8 *) tlvStruct 
                 + sizeof(tHalCfg) + tlvStruct->length + tlvStruct->padBytes) ; 
   /*                             */
   tlvStruct->type = QWLAN_HAL_CFG_EDCA_WMM_ACVO  ;
   strLength = WNI_CFG_EDCA_WME_ACVI_LEN;
   configDataValue = (tANI_U32 *)(tlvStruct + 1);
   if(wlan_cfgGetStr(pMac, WNI_CFG_EDCA_WME_ACVO, (tANI_U8 *)configDataValue,
                                             &strLength) != eSIR_SUCCESS)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                    "Failed to get value for WNI_CFG_EDCA_WME_ACVI");
      goto handle_failure;
   }
   tlvStruct->length = strLength;
   /*                                                           */
   tlvStruct->padBytes = ALIGNED_WORD_SIZE - 
                              (tlvStruct->length & (ALIGNED_WORD_SIZE - 1));
   tlvStruct = (tHalCfg *)( (tANI_U8 *) tlvStruct 
                 + sizeof(tHalCfg) + tlvStruct->length + tlvStruct->padBytes) ; 
   /*                             */
   tlvStruct->type = QWLAN_HAL_CFG_EDCA_WMM_ACVI  ;
   strLength = WNI_CFG_EDCA_WME_ACVO_LEN;
   configDataValue = (tANI_U32 *)(tlvStruct + 1);
   if(wlan_cfgGetStr(pMac, WNI_CFG_EDCA_WME_ACVI, (tANI_U8 *)configDataValue,
                                             &strLength) != eSIR_SUCCESS)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                    "Failed to get value for WNI_CFG_EDCA_WME_ACVO");
      goto handle_failure;
   }
   tlvStruct->length = strLength;
   /*                                                           */
   tlvStruct->padBytes = ALIGNED_WORD_SIZE - 
                              (tlvStruct->length & (ALIGNED_WORD_SIZE - 1));
   tlvStruct = (tHalCfg *)( (tANI_U8 *) tlvStruct 
                 + sizeof(tHalCfg) + tlvStruct->length + tlvStruct->padBytes) ; 
   /*                                 */
   tlvStruct->type = QWLAN_HAL_CFG_BA_THRESHOLD_HIGH  ;
   tlvStruct->length = sizeof(tANI_U32);
   configDataValue = (tANI_U32 *)(tlvStruct + 1);
   if(wlan_cfgGetInt(pMac, WNI_CFG_BA_THRESHOLD_HIGH, configDataValue) 
                                                   != eSIR_SUCCESS)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                    "Failed to get value for WNI_CFG_BA_THRESHOLD_HIGH");
      goto handle_failure;
   }
   tlvStruct = (tHalCfg *)( (tANI_U8 *) tlvStruct 
                            + sizeof(tHalCfg) + tlvStruct->length) ; 
   /*                              */
   tlvStruct->type = QWLAN_HAL_CFG_MAX_BA_BUFFERS  ;
   tlvStruct->length = sizeof(tANI_U32);
   configDataValue = (tANI_U32 *)(tlvStruct + 1);
   if(wlan_cfgGetInt(pMac, WNI_CFG_MAX_BA_BUFFERS, configDataValue) 
                                                   != eSIR_SUCCESS)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                    "Failed to get value for WNI_CFG_MAX_BA_BUFFERS");
      goto handle_failure;
   }
   tlvStruct = (tHalCfg *)( (tANI_U8 *) tlvStruct 
                            + sizeof(tHalCfg) + tlvStruct->length) ; 
   /*                                     */
   tlvStruct->type = QWLAN_HAL_CFG_DYNAMIC_PS_POLL_VALUE  ;
   tlvStruct->length = sizeof(tANI_U32);
   configDataValue = (tANI_U32 *)(tlvStruct + 1);
   if(wlan_cfgGetInt(pMac, WNI_CFG_DYNAMIC_PS_POLL_VALUE, configDataValue) 
                                                   != eSIR_SUCCESS)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                    "Failed to get value for WNI_CFG_DYNAMIC_PS_POLL_VALUE");
      goto handle_failure;
   }
   tlvStruct = (tHalCfg *)( (tANI_U8 *) tlvStruct 
                            + sizeof(tHalCfg) + tlvStruct->length) ; 
   /*                                 */
   tlvStruct->type = QWLAN_HAL_CFG_TELE_BCN_TRANS_LI  ;
   tlvStruct->length = sizeof(tANI_U32);
   configDataValue = (tANI_U32 *)(tlvStruct + 1);
   if(wlan_cfgGetInt(pMac, WNI_CFG_TELE_BCN_TRANS_LI, configDataValue) 
      != eSIR_SUCCESS)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                    "Failed to get value for WNI_CFG_TELE_BCN_TRANS_LI");
      goto handle_failure;
   }
   tlvStruct = (tHalCfg *)( (tANI_U8 *) tlvStruct 
                            + sizeof(tHalCfg) + tlvStruct->length) ; 
   /*                                           */
   tlvStruct->type = QWLAN_HAL_CFG_TELE_BCN_TRANS_LI_IDLE_BCNS  ;
   tlvStruct->length = sizeof(tANI_U32);
   configDataValue = (tANI_U32 *)(tlvStruct + 1);
   if(wlan_cfgGetInt(pMac, WNI_CFG_TELE_BCN_TRANS_LI_IDLE_BCNS, configDataValue) 
      != eSIR_SUCCESS)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                    "Failed to get value for WNI_CFG_TELE_BCN_TRANS_LI_IDLE_BCNS");
      goto handle_failure;
   }
   tlvStruct = (tHalCfg *)( (tANI_U8 *) tlvStruct 
                            + sizeof(tHalCfg) + tlvStruct->length) ; 
   /*                               */
   tlvStruct->type = QWLAN_HAL_CFG_TELE_BCN_MAX_LI  ;
   tlvStruct->length = sizeof(tANI_U32);
   configDataValue = (tANI_U32 *)(tlvStruct + 1);
   if(wlan_cfgGetInt(pMac, WNI_CFG_TELE_BCN_MAX_LI, configDataValue) 
      != eSIR_SUCCESS)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                    "Failed to get value for WNI_CFG_TELE_BCN_MAX_LI");
      goto handle_failure;
   }
   tlvStruct = (tHalCfg *)( (tANI_U8 *) tlvStruct 
                            + sizeof(tHalCfg) + tlvStruct->length) ; 
   /*                                         */
   tlvStruct->type = QWLAN_HAL_CFG_TELE_BCN_MAX_LI_IDLE_BCNS  ;
   tlvStruct->length = sizeof(tANI_U32);
   configDataValue = (tANI_U32 *)(tlvStruct + 1);
   if(wlan_cfgGetInt(pMac, WNI_CFG_TELE_BCN_MAX_LI_IDLE_BCNS, configDataValue) 
      != eSIR_SUCCESS)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                    "Failed to get value for WNI_CFG_TELE_BCN_MAX_LI_IDLE_BCNS");
      goto handle_failure;
   }
   tlvStruct = (tHalCfg *)( (tANI_U8 *) tlvStruct 
                            + sizeof(tHalCfg) + tlvStruct->length) ; 
   /*                                  */
   tlvStruct->type = QWLAN_HAL_CFG_TELE_BCN_WAKEUP_EN  ;
   tlvStruct->length = sizeof(tANI_U32);
   configDataValue = (tANI_U32 *)(tlvStruct + 1);
   if(wlan_cfgGetInt(pMac, WNI_CFG_TELE_BCN_WAKEUP_EN, configDataValue) 
      != eSIR_SUCCESS)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                    "Failed to get value for WNI_CFG_TELE_BCN_WAKEUP_EN");
      goto handle_failure;
   }
   tlvStruct = (tHalCfg *)( (tANI_U8 *) tlvStruct 
                            + sizeof(tHalCfg) + tlvStruct->length) ; 
   /*                                           */
   tlvStruct->type = QWLAN_HAL_CFG_INFRA_STA_KEEP_ALIVE_PERIOD  ;
   tlvStruct->length = sizeof(tANI_U32);
   configDataValue = (tANI_U32 *)(tlvStruct + 1);
   if(wlan_cfgGetInt(pMac, WNI_CFG_INFRA_STA_KEEP_ALIVE_PERIOD, configDataValue) 
                                                   != eSIR_SUCCESS)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                    "Failed to get value for WNI_CFG_INFRA_STA_KEEP_ALIVE_PERIOD");
      goto handle_failure;
   }
   tlvStruct = (tHalCfg *)( (tANI_U8 *) tlvStruct 
                            + sizeof(tHalCfg) + tlvStruct->length) ; 
   /*                                */
   tlvStruct->type = QWLAN_HAL_CFG_TX_PWR_CTRL_ENABLE  ;
   tlvStruct->length = sizeof(tANI_U32);
   configDataValue = (tANI_U32 *)(tlvStruct + 1);
   if(wlan_cfgGetInt(pMac, WNI_CFG_TX_PWR_CTRL_ENABLE, configDataValue) 
                                                   != eSIR_SUCCESS)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                    "Failed to get value for WNI_CFG_TX_PWR_CTRL_ENABLE");
      goto handle_failure;
   }
   tlvStruct = (tHalCfg *)( (tANI_U8 *) tlvStruct 
                            + sizeof(tHalCfg) + tlvStruct->length) ;
   /*                                   */
   tlvStruct->type = QWLAN_HAL_CFG_ENABLE_CLOSE_LOOP  ;
   tlvStruct->length = sizeof(tANI_U32);
   configDataValue = (tANI_U32 *)(tlvStruct + 1);
   if(wlan_cfgGetInt(pMac, WNI_CFG_ENABLE_CLOSE_LOOP, configDataValue) 
                                                      != eSIR_SUCCESS)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                    "Failed to get value for WNI_CFG_ENABLE_CLOSE_LOOP");
      goto handle_failure;
   }
   tlvStruct = (tHalCfg *)( (tANI_U8 *) tlvStruct 
                            + sizeof(tHalCfg) + tlvStruct->length) ; 
   /*                                                                                         
                                                                                               
                                               
   */
   /*                                         */
   tlvStruct->type = QWLAN_HAL_CFG_BTC_EXECUTION_MODE  ;
   tlvStruct->length = sizeof(tANI_U32);
   configDataValue = (tANI_U32 *)(tlvStruct + 1);
   *configDataValue = pMac->btc.btcConfig.btcExecutionMode; 
   tlvStruct = (tHalCfg *)( (tANI_U8 *) tlvStruct 
                            + sizeof(tHalCfg) + tlvStruct->length) ; 
   /*                                                 */
   tlvStruct->type = QWLAN_HAL_CFG_BTC_DHCP_BT_SLOTS_TO_BLOCK  ;
   tlvStruct->length = sizeof(tANI_U32);
   configDataValue = (tANI_U32 *)(tlvStruct + 1);
   *configDataValue = pMac->btc.btcConfig.btcConsBtSlotsToBlockDuringDhcp; 
   tlvStruct = (tHalCfg *)( (tANI_U8 *) tlvStruct 
                            + sizeof(tHalCfg) + tlvStruct->length) ; 
   /*                                                     */
   tlvStruct->type = QWLAN_HAL_CFG_BTC_A2DP_DHCP_BT_SUB_INTERVALS  ;
   tlvStruct->length = sizeof(tANI_U32);
   configDataValue = (tANI_U32 *)(tlvStruct + 1);
   *configDataValue = pMac->btc.btcConfig.btcA2DPBtSubIntervalsDuringDhcp; 
   tlvStruct = (tHalCfg *)( (tANI_U8 *) tlvStruct 
                            + sizeof(tHalCfg) + tlvStruct->length) ; 
   /*                                            */
   tlvStruct->type = QWLAN_HAL_CFG_BTC_STATIC_LEN_INQ_BT  ;
   tlvStruct->length = sizeof(tANI_U32);
   configDataValue = (tANI_U32 *)(tlvStruct + 1);
   *configDataValue = pMac->btc.btcConfig.btcStaticLenInqBt; 
   tlvStruct = (tHalCfg *)( (tANI_U8 *) tlvStruct 
                            + sizeof(tHalCfg) + tlvStruct->length) ; 

   /*                                             */
   tlvStruct->type = QWLAN_HAL_CFG_BTC_STATIC_LEN_PAGE_BT  ;
   tlvStruct->length = sizeof(tANI_U32);
   configDataValue = (tANI_U32 *)(tlvStruct + 1);
   *configDataValue = pMac->btc.btcConfig.btcStaticLenPageBt; 
   tlvStruct = (tHalCfg *)( (tANI_U8 *) tlvStruct 
                            + sizeof(tHalCfg) + tlvStruct->length) ; 

   /*                                             */
   tlvStruct->type = QWLAN_HAL_CFG_BTC_STATIC_LEN_CONN_BT  ;
   tlvStruct->length = sizeof(tANI_U32);
   configDataValue = (tANI_U32 *)(tlvStruct + 1);
   *configDataValue = pMac->btc.btcConfig.btcStaticLenConnBt; 
   tlvStruct = (tHalCfg *)( (tANI_U8 *) tlvStruct 
                            + sizeof(tHalCfg) + tlvStruct->length) ; 

   /*                                           */
   tlvStruct->type = QWLAN_HAL_CFG_BTC_STATIC_LEN_LE_BT  ;
   tlvStruct->length = sizeof(tANI_U32);
   configDataValue = (tANI_U32 *)(tlvStruct + 1);
   *configDataValue = pMac->btc.btcConfig.btcStaticLenLeBt; 
   tlvStruct = (tHalCfg *)( (tANI_U8 *) tlvStruct 
                            + sizeof(tHalCfg) + tlvStruct->length) ; 

   /*                                              */
   tlvStruct->type = QWLAN_HAL_CFG_BTC_STATIC_LEN_INQ_WLAN  ;
   tlvStruct->length = sizeof(tANI_U32);
   configDataValue = (tANI_U32 *)(tlvStruct + 1);
   *configDataValue = pMac->btc.btcConfig.btcStaticLenInqWlan; 
   tlvStruct = (tHalCfg *)( (tANI_U8 *) tlvStruct 
                            + sizeof(tHalCfg) + tlvStruct->length) ; 

   /*                                               */
   tlvStruct->type = QWLAN_HAL_CFG_BTC_STATIC_LEN_PAGE_WLAN  ;
   tlvStruct->length = sizeof(tANI_U32);
   configDataValue = (tANI_U32 *)(tlvStruct + 1);
   *configDataValue = pMac->btc.btcConfig.btcStaticLenPageWlan; 
   tlvStruct = (tHalCfg *)( (tANI_U8 *) tlvStruct 
                            + sizeof(tHalCfg) + tlvStruct->length) ; 

   /*                                               */
   tlvStruct->type = QWLAN_HAL_CFG_BTC_STATIC_LEN_CONN_WLAN  ;
   tlvStruct->length = sizeof(tANI_U32);
   configDataValue = (tANI_U32 *)(tlvStruct + 1);
   *configDataValue = pMac->btc.btcConfig.btcStaticLenConnWlan; 
   tlvStruct = (tHalCfg *)( (tANI_U8 *) tlvStruct 
                            + sizeof(tHalCfg) + tlvStruct->length) ; 

   /*                                             */
   tlvStruct->type = QWLAN_HAL_CFG_BTC_STATIC_LEN_LE_WLAN  ;
   tlvStruct->length = sizeof(tANI_U32);
   configDataValue = (tANI_U32 *)(tlvStruct + 1);
   *configDataValue = pMac->btc.btcConfig.btcStaticLenLeWlan; 
   tlvStruct = (tHalCfg *)( (tANI_U8 *) tlvStruct 
                            + sizeof(tHalCfg) + tlvStruct->length) ; 

   /*                                         */
   tlvStruct->type = QWLAN_HAL_CFG_BTC_DYN_MAX_LEN_BT  ;
   tlvStruct->length = sizeof(tANI_U32);
   configDataValue = (tANI_U32 *)(tlvStruct + 1);
   *configDataValue = pMac->btc.btcConfig.btcDynMaxLenBt; 
   tlvStruct = (tHalCfg *)( (tANI_U8 *) tlvStruct 
                            + sizeof(tHalCfg) + tlvStruct->length) ; 

   /*                                           */
   tlvStruct->type = QWLAN_HAL_CFG_BTC_DYN_MAX_LEN_WLAN  ;
   tlvStruct->length = sizeof(tANI_U32);
   configDataValue = (tANI_U32 *)(tlvStruct + 1);
   *configDataValue = pMac->btc.btcConfig.btcDynMaxLenWlan; 
   tlvStruct = (tHalCfg *)( (tANI_U8 *) tlvStruct 
                            + sizeof(tHalCfg) + tlvStruct->length) ; 

   /*                                             */
   tlvStruct->type = QWLAN_HAL_CFG_BTC_MAX_SCO_BLOCK_PERC  ;
   tlvStruct->length = sizeof(tANI_U32);
   configDataValue = (tANI_U32 *)(tlvStruct + 1);
   *configDataValue = pMac->btc.btcConfig.btcMaxScoBlockPerc; 
   tlvStruct = (tHalCfg *)( (tANI_U8 *) tlvStruct 
                            + sizeof(tHalCfg) + tlvStruct->length) ; 

   /*                                            */
   tlvStruct->type = QWLAN_HAL_CFG_BTC_DHCP_PROT_ON_A2DP  ;
   tlvStruct->length = sizeof(tANI_U32);
   configDataValue = (tANI_U32 *)(tlvStruct + 1);
   *configDataValue = pMac->btc.btcConfig.btcDhcpProtOnA2dp; 
   tlvStruct = (tHalCfg *)( (tANI_U8 *) tlvStruct 
                            + sizeof(tHalCfg) + tlvStruct->length) ; 

   /*                                           */
   tlvStruct->type = QWLAN_HAL_CFG_BTC_DHCP_PROT_ON_SCO  ;
   tlvStruct->length = sizeof(tANI_U32);
   configDataValue = (tANI_U32 *)(tlvStruct + 1);
   *configDataValue = pMac->btc.btcConfig.btcDhcpProtOnSco; 
   tlvStruct = (tHalCfg *)( (tANI_U8 *) tlvStruct 
                            + sizeof(tHalCfg) + tlvStruct->length) ; 

   for (i = 0; i < QWLAN_HAL_CFG_MWS_COEX_MAX_VICTIM; i++)
   {
      /*                                           */
      tlvStruct->type = QWLAN_HAL_CFG_MWS_COEX_V1_WAN_FREQ + i*4;
      tlvStruct->length = sizeof(tANI_U32);
      configDataValue = (tANI_U32 *)(tlvStruct + 1);
      *configDataValue = pMac->btc.btcConfig.mwsCoexVictimWANFreq[i];
      tlvStruct = (tHalCfg *)( (tANI_U8 *) tlvStruct
                               + sizeof(tHalCfg) + tlvStruct->length) ;

      /*                                            */
      tlvStruct->type = QWLAN_HAL_CFG_MWS_COEX_V1_WLAN_FREQ + i*4;
      tlvStruct->length = sizeof(tANI_U32);
      configDataValue = (tANI_U32 *)(tlvStruct + 1);
      *configDataValue = pMac->btc.btcConfig.mwsCoexVictimWLANFreq[i];
      tlvStruct = (tHalCfg *)( (tANI_U8 *) tlvStruct
                               + sizeof(tHalCfg) + tlvStruct->length) ;

      /*                                         */
      tlvStruct->type = QWLAN_HAL_CFG_MWS_COEX_V1_CONFIG + i*4;
      tlvStruct->length = sizeof(tANI_U32);
      configDataValue = (tANI_U32 *)(tlvStruct + 1);
      *configDataValue = pMac->btc.btcConfig.mwsCoexVictimConfig[i];
      tlvStruct = (tHalCfg *)( (tANI_U8 *) tlvStruct
                               + sizeof(tHalCfg) + tlvStruct->length) ;

      /*                                          */
      tlvStruct->type = QWLAN_HAL_CFG_MWS_COEX_V1_CONFIG2 + i*4;
      tlvStruct->length = sizeof(tANI_U32);
      configDataValue = (tANI_U32 *)(tlvStruct + 1);
      *configDataValue = pMac->btc.btcConfig.mwsCoexVictimConfig2[i];
      tlvStruct = (tHalCfg *)( (tANI_U8 *) tlvStruct
                               + sizeof(tHalCfg) + tlvStruct->length) ;
   }

   /*                                             */
   tlvStruct->type = QWLAN_HAL_CFG_MWS_COEX_MODEM_BACKOFF  ;
   tlvStruct->length = sizeof(tANI_U32);
   configDataValue = (tANI_U32 *)(tlvStruct + 1);
   *configDataValue = pMac->btc.btcConfig.mwsCoexModemBackoff;
   tlvStruct = (tHalCfg *)( (tANI_U8 *) tlvStruct
                            + sizeof(tHalCfg) + tlvStruct->length) ;

   for (i = 0; i < QWLAN_HAL_CFG_MWS_COEX_MAX_CONFIG; i++)
   {
      /*                                       */
      tlvStruct->type = QWLAN_HAL_CFG_MWS_COEX_CONFIG1 + i;
      tlvStruct->length = sizeof(tANI_U32);
      configDataValue = (tANI_U32 *)(tlvStruct + 1);
      *configDataValue = pMac->btc.btcConfig.mwsCoexConfig[i];
      tlvStruct = (tHalCfg *)( (tANI_U8 *) tlvStruct
                               + sizeof(tHalCfg) + tlvStruct->length) ;
   }

   /*                                        */
   tlvStruct->type = QWLAN_HAL_CFG_SAR_POWER_BACKOFF  ;
   tlvStruct->length = sizeof(tANI_U32);
   configDataValue = (tANI_U32 *)(tlvStruct + 1);
   *configDataValue = pMac->btc.btcConfig.SARPowerBackoff;
   tlvStruct = (tHalCfg *)( (tANI_U8 *) tlvStruct
                            + sizeof(tHalCfg) + tlvStruct->length) ;

   /*                                 */
   tlvStruct->type = QWLAN_HAL_CFG_WCNSS_API_VERSION  ;
   tlvStruct->length = sizeof(tANI_U32);
   configDataValue = (tANI_U32 *)(tlvStruct + 1);
   WDI_GetWcnssCompiledApiVersion(&wcnssCompiledApiVersion);
   *configDataValue = WLAN_HAL_CONSTRUCT_API_VERSION(wcnssCompiledApiVersion.major,
                                      wcnssCompiledApiVersion.minor,
                                      wcnssCompiledApiVersion.version,
                                      wcnssCompiledApiVersion.revision);
   tlvStruct = (tHalCfg *)( (tANI_U8 *) tlvStruct 
                            + sizeof(tHalCfg) + tlvStruct->length) ; 

   /*                                      */
   tlvStruct->type = QWLAN_HAL_CFG_AP_KEEPALIVE_TIMEOUT  ;
   tlvStruct->length = sizeof(tANI_U32);
   configDataValue = (tANI_U32 *)(tlvStruct + 1);
   if(wlan_cfgGetInt(pMac, WNI_CFG_AP_KEEP_ALIVE_TIMEOUT, 
                                            configDataValue ) != eSIR_SUCCESS)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
               "Failed to get value for WNI_CFG_AP_KEEP_ALIVE_TIMEOUT");
      goto handle_failure;
   }

   tlvStruct = (tHalCfg *)( (tANI_U8 *) tlvStruct 
                            + sizeof(tHalCfg) + tlvStruct->length) ; 
   /*                                      */
   tlvStruct->type = QWLAN_HAL_CFG_GO_KEEPALIVE_TIMEOUT  ;
   tlvStruct->length = sizeof(tANI_U32);
   configDataValue = (tANI_U32 *)(tlvStruct + 1);
   if(wlan_cfgGetInt(pMac, WNI_CFG_GO_KEEP_ALIVE_TIMEOUT, 
                                            configDataValue ) != eSIR_SUCCESS)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
               "Failed to get value for WNI_CFG_GO_KEEP_ALIVE_TIMEOUT");
      goto handle_failure;
   }

   tlvStruct = (tHalCfg *)( (tANI_U8 *) tlvStruct 
                            + sizeof(tHalCfg) + tlvStruct->length) ; 

   /*                                   */
   tlvStruct->type = QWLAN_HAL_CFG_ENABLE_MC_ADDR_LIST;
   tlvStruct->length = sizeof(tANI_U32);
   configDataValue = (tANI_U32 *)(tlvStruct + 1);
   if(wlan_cfgGetInt(pMac, WNI_CFG_ENABLE_MC_ADDR_LIST, configDataValue) 
                                                      != eSIR_SUCCESS)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                    "Failed to get value for WNI_CFG_ENABLE_MC_ADDR_LIST");
      goto handle_failure;
   }

   tlvStruct = (tHalCfg *)( (tANI_U8 *) tlvStruct 
                            + sizeof(tHalCfg) + tlvStruct->length) ; 

   /*                                            */
   tlvStruct->type = QWLAN_HAL_CFG_ENABLE_LPWR_IMG_TRANSITION  ;
   tlvStruct->length = sizeof(tANI_U32);
   configDataValue = (tANI_U32 *)(tlvStruct + 1);
   if(wlan_cfgGetInt(pMac, WNI_CFG_ENABLE_LPWR_IMG_TRANSITION, configDataValue) 
                                                     != eSIR_SUCCESS)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                    "Failed to get value for WNI_CFG_ENABLE_LPWR_IMG_TRANSITION");
      goto handle_failure;
   }
      
   tlvStruct = (tHalCfg *)( (tANI_U8 *) tlvStruct 
                            + sizeof(tHalCfg) + tlvStruct->length) ; 
#ifdef WLAN_SOFTAP_VSTA_FEATURE
   tlvStruct->type = QWLAN_HAL_CFG_MAX_ASSOC_LIMIT;
   tlvStruct->length = sizeof(tANI_U32);
   configDataValue = (tANI_U32 *)(tlvStruct + 1);
   if(wlan_cfgGetInt(pMac, WNI_CFG_ASSOC_STA_LIMIT, configDataValue)
                                                     != eSIR_SUCCESS)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                    "Failed to get value for WNI_CFG_ASSOC_STA_LIMIT");
      goto handle_failure;
   }
      
   tlvStruct = (tHalCfg *)( (tANI_U8 *) tlvStruct
                            + sizeof(tHalCfg) + tlvStruct->length) ;
#endif

   /*                                             */
   tlvStruct->type = QWLAN_HAL_CFG_ENABLE_MCC_ADAPTIVE_SCHEDULER;
   tlvStruct->length = sizeof(tANI_U32);
   configDataValue = (tANI_U32 *)(tlvStruct + 1);

   if(wlan_cfgGetInt(pMac, WNI_CFG_ENABLE_MCC_ADAPTIVE_SCHED, configDataValue) 
                                                      != eSIR_SUCCESS)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                    "Failed to get value for WNI_CFG_ENABLE_MCC_ADAPTIVE_SCHED");
      goto handle_failure;
   }

   tlvStruct = (tHalCfg *)( (tANI_U8 *) tlvStruct 
                            + sizeof(tHalCfg) + tlvStruct->length) ;

/*                                         */
   tlvStruct->type = QWLAN_HAL_CFG_AP_LINK_MONITOR_TIMEOUT  ;
   tlvStruct->length = sizeof(tANI_U32);
   configDataValue = (tANI_U32 *)(tlvStruct + 1);
   if(wlan_cfgGetInt(pMac, WNI_CFG_AP_LINK_MONITOR_TIMEOUT,
                                            configDataValue ) != eSIR_SUCCESS)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
               "Failed to get value for WNI_CFG_AP_LINK_MONITOR_TIMEOUT");
      goto handle_failure;
   }

   tlvStruct = (tHalCfg *)( (tANI_U8 *) tlvStruct
                            + sizeof(tHalCfg) + tlvStruct->length) ;
#ifdef FEATURE_WLAN_TDLS
   /*                                 */
   tlvStruct->type = QWLAN_HAL_CFG_TDLS_PUAPSD_MASK;
   tlvStruct->length = sizeof(tANI_U32);
   configDataValue = (tANI_U32 *)(tlvStruct + 1);
   if(wlan_cfgGetInt(pMac, WNI_CFG_TDLS_QOS_WMM_UAPSD_MASK,
                                            configDataValue ) != eSIR_SUCCESS)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
               "Failed to get value for WNI_CFG_TDLS_QOS_WMM_UAPSD_MASK");
      goto handle_failure;
   }
   tlvStruct = (tHalCfg *)( (tANI_U8 *) tlvStruct
                            + sizeof(tHalCfg) + tlvStruct->length) ;

   /*                                               */
   tlvStruct->type = QWLAN_HAL_CFG_TDLS_PUAPSD_BUFFER_STA_CAPABLE;
   tlvStruct->length = sizeof(tANI_U32);
   configDataValue = (tANI_U32 *)(tlvStruct + 1);
   if(wlan_cfgGetInt(pMac, WNI_CFG_TDLS_BUF_STA_ENABLED,
                                            configDataValue ) != eSIR_SUCCESS)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
               "Failed to get value for WNI_CFG_TDLS_BUF_STA_ENABLED");
      goto handle_failure;
   }
   tlvStruct = (tHalCfg *)( (tANI_U8 *) tlvStruct
                            + sizeof(tHalCfg) + tlvStruct->length) ;
   /*                                           */
   tlvStruct->type = QWLAN_HAL_CFG_TDLS_PUAPSD_INACTIVITY_TIME;
   tlvStruct->length = sizeof(tANI_U32);
   configDataValue = (tANI_U32 *)(tlvStruct + 1);
   if(wlan_cfgGetInt(pMac, WNI_CFG_TDLS_PUAPSD_INACT_TIME,
                                            configDataValue ) != eSIR_SUCCESS)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
               "Failed to get value for WNI_CFG_TDLS_PUAPSD_INACT_TIME");
      goto handle_failure;
   }
   tlvStruct = (tHalCfg *)( (tANI_U8 *) tlvStruct
                            + sizeof(tHalCfg) + tlvStruct->length) ;
   /*                                                    */
   tlvStruct->type = QWLAN_HAL_CFG_TDLS_PUAPSD_RX_FRAME_THRESHOLD_IN_SP;
   tlvStruct->length = sizeof(tANI_U32);
   configDataValue = (tANI_U32 *)(tlvStruct + 1);
   if(wlan_cfgGetInt(pMac, WNI_CFG_TDLS_RX_FRAME_THRESHOLD,
                                            configDataValue ) != eSIR_SUCCESS)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
               "Failed to get value for WNI_CFG_TDLS_RX_FRAME_THRESHOLD");
      goto handle_failure;
   }
   tlvStruct = (tHalCfg *)( (tANI_U8 *) tlvStruct
                            + sizeof(tHalCfg) + tlvStruct->length) ;
   /*                                        */
   tlvStruct->type = QWLAN_HAL_CFG_TDLS_OFF_CHANNEL_CAPABLE;
   tlvStruct->length = sizeof(tANI_U32);
   configDataValue = (tANI_U32 *)(tlvStruct + 1);
   if(wlan_cfgGetInt(pMac, WNI_CFG_TDLS_OFF_CHANNEL_ENABLED,
   configDataValue ) != eSIR_SUCCESS)
   {
   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
   "Failed to get value for WNI_CFG_TDLS_BUF_STA_ENABLED");
   goto handle_failure;
   }
   tlvStruct = (tHalCfg *)( (tANI_U8 *) tlvStruct
                            + sizeof(tHalCfg) + tlvStruct->length) ;

#endif

   /*                                         */
   tlvStruct->type = QWLAN_HAL_CFG_ENABLE_ADAPTIVE_RX_DRAIN_FEATURE ;
   tlvStruct->length = sizeof(tANI_U32);
   configDataValue = (tANI_U32 *)(tlvStruct + 1);
   if(wlan_cfgGetInt(pMac, WNI_CFG_ENABLE_ADAPT_RX_DRAIN,
                                            configDataValue ) != eSIR_SUCCESS)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
               "Failed to get value for WNI_CFG_ENABLE_ADAPT_RX_DRAIN");
      goto handle_failure;
   }

   tlvStruct = (tHalCfg *)( (tANI_U8 *) tlvStruct
                            + sizeof(tHalCfg) + tlvStruct->length) ;

   /*                                         */
   tlvStruct->type = QWLAN_HAL_CFG_FLEXCONNECT_POWER_FACTOR;
   tlvStruct->length = sizeof(tANI_U32);
   configDataValue = (tANI_U32 *)(tlvStruct + 1);
   if(wlan_cfgGetInt(pMac, WNI_CFG_FLEX_CONNECT_POWER_FACTOR, configDataValue)
                                                               != eSIR_SUCCESS)
   {
      VOS_TRACE(VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                "Failed to get value for WNI_CFG_FLEX_CONNECT_POWER_FACTOR");
      goto handle_failure;
   }
   tlvStruct = (tHalCfg *)(((tANI_U8 *) tlvStruct
                            + sizeof(tHalCfg) + tlvStruct->length));

   /*                                 */
   tlvStruct->type = QWLAN_HAL_CFG_ANTENNA_DIVERSITY;
   tlvStruct->length = sizeof(tANI_U32);
   configDataValue = (tANI_U32 *)(tlvStruct + 1);
   if (wlan_cfgGetInt(pMac, WNI_CFG_ANTENNA_DIVESITY,
                                            configDataValue ) != eSIR_SUCCESS)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
               "Failed to get value for WNI_CFG_ANTENNA_DIVESITY");
      goto handle_failure;
   }

   tlvStruct = (tHalCfg *)( (tANI_U8 *) tlvStruct
                            + sizeof(tHalCfg) + tlvStruct->length) ;

   /*                                         */
   tlvStruct->type = QWLAN_HAL_CFG_GO_LINK_MONITOR_TIMEOUT  ;
   tlvStruct->length = sizeof(tANI_U32);
   configDataValue = (tANI_U32 *)(tlvStruct + 1);
   if(wlan_cfgGetInt(pMac, WNI_CFG_GO_LINK_MONITOR_TIMEOUT,
                                            configDataValue ) != eSIR_SUCCESS)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
               "Failed to get value for WNI_CFG_GO_LINK_MONITOR_TIMEOUT");
      goto handle_failure;
   }
   tlvStruct = (tHalCfg *)( (tANI_U8 *) tlvStruct
                            + sizeof(tHalCfg) + tlvStruct->length) ;

   /*                           */
   tlvStruct->type = QWLAN_HAL_CFG_ATH_DISABLE ;
   tlvStruct->length = sizeof(tANI_U32);
   configDataValue = (tANI_U32 *)(tlvStruct + 1);
   if(wlan_cfgGetInt(pMac, WNI_CFG_ATH_DISABLE,
                                            configDataValue ) != eSIR_SUCCESS)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
               "Failed to get value for WNI_CFG_ATH_DISABLE");
      goto handle_failure;
   }
   tlvStruct = (tHalCfg *)( (tANI_U8 *) tlvStruct
                            + sizeof(tHalCfg) + tlvStruct->length) ;

   /*                                                   */
   tlvStruct->type = QWLAN_HAL_CFG_BTC_STATIC_OPP_WLAN_ACTIVE_WLAN_LEN ;
   tlvStruct->length = sizeof(tANI_U32);
   configDataValue = (tANI_U32 *)(tlvStruct + 1);
   if (wlan_cfgGetInt(pMac, WNI_CFG_BTC_ACTIVE_WLAN_LEN,
                                            configDataValue ) != eSIR_SUCCESS)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
               "Failed to get value for WNI_CFG_BTC_ACTIVE_WLAN_LEN");
      goto handle_failure;
   }
   tlvStruct = (tHalCfg *)( (tANI_U8 *) tlvStruct
                            + sizeof(tHalCfg) + tlvStruct->length) ;

   /*                                                 */
   tlvStruct->type = QWLAN_HAL_CFG_BTC_STATIC_OPP_WLAN_ACTIVE_BT_LEN ;
   tlvStruct->length = sizeof(tANI_U32);
   configDataValue = (tANI_U32 *)(tlvStruct + 1);
   if (wlan_cfgGetInt(pMac, WNI_CFG_BTC_ACTIVE_BT_LEN,
                                            configDataValue ) != eSIR_SUCCESS)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
               "Failed to get value for WNI_CFG_BTC_ACTIVE_BT_LEN");
      goto handle_failure;
   }
   tlvStruct = (tHalCfg *)( (tANI_U8 *) tlvStruct
                            + sizeof(tHalCfg) + tlvStruct->length) ;

   /*                                                        */
   tlvStruct->type = QWLAN_HAL_CFG_BTC_SAP_STATIC_OPP_WLAN_ACTIVE_WLAN_LEN ;
   tlvStruct->length = sizeof(tANI_U32);
   configDataValue = (tANI_U32 *)(tlvStruct + 1);
   if (wlan_cfgGetInt(pMac,  WNI_CFG_BTC_SAP_ACTIVE_WLAN_LEN,
                                            configDataValue ) != eSIR_SUCCESS)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
               "Failed to get value for WNI_CFG_BTC_SAP_ACTIVE_WLAN_LEN");
      goto handle_failure;
   }
   tlvStruct = (tHalCfg *)( (tANI_U8 *) tlvStruct
                            + sizeof(tHalCfg) + tlvStruct->length) ;

   /*                                                     */
   tlvStruct->type = QWLAN_HAL_CFG_BTC_SAP_STATIC_OPP_WLAN_ACTIVE_BT_LEN ;
   tlvStruct->length = sizeof(tANI_U32);
   configDataValue = (tANI_U32 *)(tlvStruct + 1);
   if (wlan_cfgGetInt(pMac, WNI_CFG_BTC_SAP_ACTIVE_BT_LEN,
                                            configDataValue ) != eSIR_SUCCESS)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
               "Failed to get value for WNI_CFG_BTC_SAP_ACTIVE_BT_LEN");
      goto handle_failure;
   }
   tlvStruct = (tHalCfg *)( (tANI_U8 *) tlvStruct
                           + sizeof(tHalCfg) + tlvStruct->length) ;

  /*                                  */
   tlvStruct->type = QWLAN_HAL_CFG_ASD_PROBE_INTERVAL  ;
   tlvStruct->length = sizeof(tANI_U32);
   configDataValue = (tANI_U32 *)(tlvStruct + 1);
   if(wlan_cfgGetInt(pMac, WNI_CFG_ASD_PROBE_INTERVAL,
                                            configDataValue ) != eSIR_SUCCESS)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
               "Failed to get value for WNI_CFG_ASD_PROBE_INTERVAL");
      goto handle_failure;
   }
   tlvStruct = (tHalCfg *)( (tANI_U8 *) tlvStruct
                            + sizeof(tHalCfg) + tlvStruct->length) ;

   /*                                     */
   tlvStruct->type = QWLAN_HAL_CFG_ASD_TRIGGER_THRESHOLD  ;
   tlvStruct->length = sizeof(tANI_U32);
   configDataValue = (tANI_U32 *)(tlvStruct + 1);
   if(wlan_cfgGetInt(pMac, WNI_CFG_ASD_TRIGGER_THRESHOLD,
                                            configDataValue ) != eSIR_SUCCESS)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
               "Failed to get value for WNI_CFG_ASD_TRIGGER_THRESHOLD");
      goto handle_failure;
   }
   tlvStruct = (tHalCfg *)( (tANI_U8 *) tlvStruct
                            + sizeof(tHalCfg) + tlvStruct->length) ;

   /*                                           */
   tlvStruct->type = QWLAN_HAL_CFG_ASD_RTT_RSSI_HYST_THRESHOLD ;
   tlvStruct->length = sizeof(tANI_U32);
   configDataValue = (tANI_U32 *)(tlvStruct + 1);
   if(wlan_cfgGetInt(pMac, WNI_CFG_ASD_RTT_RSSI_HYST_THRESHOLD,
                                            configDataValue ) != eSIR_SUCCESS)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
               "Failed to get value for WNI_CFG_ASD_RTT_RSSI_HYST_THRESHOLD");
      goto handle_failure;
   }
   tlvStruct = (tHalCfg *)( (tANI_U8 *) tlvStruct
                            + sizeof(tHalCfg) + tlvStruct->length) ;

   /*                                            */
   tlvStruct->type = QWLAN_HAL_CFG_BTC_CTS2S_ON_STA_DURING_SCO ;
   tlvStruct->length = sizeof(tANI_U32);
   configDataValue = (tANI_U32 *)(tlvStruct + 1);
   if (wlan_cfgGetInt(pMac, WNI_CFG_BTC_CTS2S_DURING_SCO,
                                            configDataValue ) != eSIR_SUCCESS)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
               "Failed to get value for WNI_CFG_BTC_CTS2S_DURING_SCO");
      goto handle_failure;
   }
   tlvStruct = (tHalCfg *)( (tANI_U8 *) tlvStruct
                           + sizeof(tHalCfg) + tlvStruct->length) ;

   /*                                 */
   tlvStruct->type = QWLAN_HAL_CFG_RA_FILTER_ENABLE ;
   tlvStruct->length = sizeof(tANI_U32);
   configDataValue = (tANI_U32 *)(tlvStruct + 1);

   if (wlan_cfgGetInt(pMac, WNI_CFG_RA_FILTER_ENABLE,
                                            configDataValue ) != eSIR_SUCCESS)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
               "Failed to get value for WNI_CFG_RA_FILTER_ENABLE");
      goto handle_failure;
   }
   tlvStruct = (tHalCfg *)( (tANI_U8 *) tlvStruct
                           + sizeof(tHalCfg) + tlvStruct->length) ;

   /*                                       */
   tlvStruct->type = QWLAN_HAL_CFG_RA_RATE_LIMIT_INTERVAL ;
   tlvStruct->length = sizeof(tANI_U32);
   configDataValue = (tANI_U32 *)(tlvStruct + 1);

   if (wlan_cfgGetInt(pMac, WNI_CFG_RA_RATE_LIMIT_INTERVAL,
                                            configDataValue ) != eSIR_SUCCESS)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
               "Failed to get value for WNI_CFG_RA_RATE_LIMIT_INTERVAL");
      goto handle_failure;
   }
   tlvStruct = (tHalCfg *)( (tANI_U8 *) tlvStruct
                            + sizeof(tHalCfg) + tlvStruct->length) ;

   /*                                                 */
   tlvStruct->type = QWLAN_HAL_CFG_BTC_FATAL_HID_NSNIFF_BLK_GUIDANCE ;
   tlvStruct->length = sizeof(tANI_U32);
   configDataValue = (tANI_U32 *)(tlvStruct + 1);

   if (wlan_cfgGetInt(pMac, WNI_CFG_BTC_FATAL_HID_NSNIFF_BLK_GUIDANCE,
                                            configDataValue ) != eSIR_SUCCESS)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
               "Failed to get value for WNI_CFG_BTC_FATAL_HID_NSNIFF_BLK_GUIDANCE");
      goto handle_failure;
   }
   tlvStruct = (tHalCfg *)( (tANI_U8 *) tlvStruct
                            + sizeof(tHalCfg) + tlvStruct->length) ;

  /*                                                    */
   tlvStruct->type = QWLAN_HAL_CFG_BTC_CRITICAL_HID_NSNIFF_BLK_GUIDANCE ;
   tlvStruct->length = sizeof(tANI_U32);
   configDataValue = (tANI_U32 *)(tlvStruct + 1);

   if (wlan_cfgGetInt(pMac, WNI_CFG_BTC_CRITICAL_HID_NSNIFF_BLK_GUIDANCE,
                                            configDataValue ) != eSIR_SUCCESS)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
               "Failed to get value for WNI_CFG_BTC_CRITICAL_HID_NSNIFF_BLK_GUIDANCE");
      goto handle_failure;
   }
   tlvStruct = (tHalCfg *)( (tANI_U8 *) tlvStruct
                            + sizeof(tHalCfg) + tlvStruct->length) ;

   /*                                           */
   tlvStruct->type = QWLAN_HAL_CFG_BTC_DYN_A2DP_TX_QUEUE_THOLD ;
   tlvStruct->length = sizeof(tANI_U32);
   configDataValue = (tANI_U32 *)(tlvStruct + 1);

   if (wlan_cfgGetInt(pMac, WNI_CFG_BTC_DYN_A2DP_TX_QUEUE_THOLD,
                                            configDataValue ) != eSIR_SUCCESS)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
               "Failed to get value for WNI_CFG_BTC_DYN_A2DP_TX_QUEUE_THOLD");
      goto handle_failure;
   }
   tlvStruct = (tHalCfg *)( (tANI_U8 *) tlvStruct
                            + sizeof(tHalCfg) + tlvStruct->length) ;

   /*                                          */
   tlvStruct->type = QWLAN_HAL_CFG_BTC_DYN_OPP_TX_QUEUE_THOLD ;
   tlvStruct->length = sizeof(tANI_U32);
   configDataValue = (tANI_U32 *)(tlvStruct + 1);

   if (wlan_cfgGetInt(pMac, WNI_CFG_BTC_DYN_OPP_TX_QUEUE_THOLD,
                                            configDataValue ) != eSIR_SUCCESS)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
               "Failed to get value for WNI_CFG_BTC_DYN_OPP_TX_QUEUE_THOLD");
      goto handle_failure;
   }
   tlvStruct = (tHalCfg *)( (tANI_U8 *) tlvStruct
                           + sizeof(tHalCfg) + tlvStruct->length) ;

   /*                                   */
   tlvStruct->type = QWLAN_HAL_CFG_MAX_UAPSD_CONSEC_SP ;
   tlvStruct->length = sizeof(tANI_U32);
   configDataValue = (tANI_U32 *)(tlvStruct + 1);

   if (wlan_cfgGetInt(pMac, WNI_CFG_MAX_UAPSD_CONSEC_SP,
                                            configDataValue ) != eSIR_SUCCESS)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
               "Failed to get value for WNI_CFG_MAX_UAPSD_CONSEC_SP");
      goto handle_failure;
   }
   tlvStruct = (tHalCfg *)( (tANI_U8 *) tlvStruct
                           + sizeof(tHalCfg) + tlvStruct->length) ;

   /*                                       */
   tlvStruct->type = QWLAN_HAL_CFG_MAX_UAPSD_CONSEC_RX_CNT ;
   tlvStruct->length = sizeof(tANI_U32);
   configDataValue = (tANI_U32 *)(tlvStruct + 1);

   if (wlan_cfgGetInt(pMac, WNI_CFG_MAX_UAPSD_CONSEC_RX_CNT,
                                            configDataValue ) != eSIR_SUCCESS)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
               "Failed to get value for WNI_CFG_MAX_UAPSD_CONSEC_RX_CNT");
      goto handle_failure;
   }
   tlvStruct = (tHalCfg *)( (tANI_U8 *) tlvStruct
                           + sizeof(tHalCfg) + tlvStruct->length) ;

   /*                                       */
   tlvStruct->type = QWLAN_HAL_CFG_MAX_UAPSD_CONSEC_TX_CNT ;
   tlvStruct->length = sizeof(tANI_U32);
   configDataValue = (tANI_U32 *)(tlvStruct + 1);

   if (wlan_cfgGetInt(pMac, WNI_CFG_MAX_UAPSD_CONSEC_TX_CNT,
                                            configDataValue ) != eSIR_SUCCESS)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
               "Failed to get value for WNI_CFG_MAX_UAPSD_CONSEC_TX_CNT");
      goto handle_failure;
   }
   tlvStruct = (tHalCfg *)( (tANI_U8 *) tlvStruct
                           + sizeof(tHalCfg) + tlvStruct->length) ;

   /*                                                   */
   tlvStruct->type = QWLAN_HAL_CFG_MAX_UAPSD_CONSEC_TX_CNT_MEAS_WINDOW ;
   tlvStruct->length = sizeof(tANI_U32);
   configDataValue = (tANI_U32 *)(tlvStruct + 1);

   if (wlan_cfgGetInt(pMac, WNI_CFG_MAX_UAPSD_CONSEC_TX_CNT_MEAS_WINDOW,
                                            configDataValue ) != eSIR_SUCCESS)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
               "Failed to get value for WNI_CFG_MAX_UAPSD_CONSEC_TX_CNT_MEAS_WINDOW");
      goto handle_failure;
   }
   tlvStruct = (tHalCfg *)( (tANI_U8 *) tlvStruct
                           + sizeof(tHalCfg) + tlvStruct->length) ;

   /*                                                   */
   tlvStruct->type = QWLAN_HAL_CFG_MAX_UAPSD_CONSEC_RX_CNT_MEAS_WINDOW ;
   tlvStruct->length = sizeof(tANI_U32);
   configDataValue = (tANI_U32 *)(tlvStruct + 1);

   if (wlan_cfgGetInt(pMac, WNI_CFG_MAX_UAPSD_CONSEC_RX_CNT_MEAS_WINDOW,
                                            configDataValue ) != eSIR_SUCCESS)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
               "Failed to get value for WNI_CFG_MAX_UAPSD_CONSEC_RX_CNT_MEAS_WINDOW");
      goto handle_failure;
   }
   tlvStruct = (tHalCfg *)( (tANI_U8 *) tlvStruct
                           + sizeof(tHalCfg) + tlvStruct->length) ;

   /*                                               */
   tlvStruct->type = QWLAN_HAL_CFG_MAX_PSPOLL_IN_WMM_UAPSD_PS_MODE ;
   tlvStruct->length = sizeof(tANI_U32);
   configDataValue = (tANI_U32 *)(tlvStruct + 1);

   if (wlan_cfgGetInt(pMac, WNI_CFG_MAX_PSPOLL_IN_WMM_UAPSD_PS_MODE,
                                            configDataValue ) != eSIR_SUCCESS)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
               "Failed to get value for WNI_CFG_MAX_PSPOLL_IN_WMM_UAPSD_PS_MODE");
      goto handle_failure;
   }
   tlvStruct = (tHalCfg *)( (tANI_U8 *) tlvStruct
                           + sizeof(tHalCfg) + tlvStruct->length) ;

   /*                                              */
   tlvStruct->type = QWLAN_HAL_CFG_MAX_UAPSD_INACTIVITY_INTERVALS ;
   tlvStruct->length = sizeof(tANI_U32);
   configDataValue = (tANI_U32 *)(tlvStruct + 1);

   if (wlan_cfgGetInt(pMac, WNI_CFG_MAX_UAPSD_INACTIVITY_INTERVALS,
                                            configDataValue ) != eSIR_SUCCESS)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
               "Failed to get value for WNI_CFG_MAX_UAPSD_INACTIVITY_INTERVALS");
      goto handle_failure;
   }
   tlvStruct = (tHalCfg *)( (tANI_U8 *) tlvStruct
                           + sizeof(tHalCfg) + tlvStruct->length) ;

   /*                                    */
   tlvStruct->type = QWLAN_HAL_CFG_ENABLE_DYNAMIC_WMMPS ;
   tlvStruct->length = sizeof(tANI_U32);
   configDataValue = (tANI_U32 *)(tlvStruct + 1);

   if (wlan_cfgGetInt(pMac, WNI_CFG_ENABLE_DYNAMIC_WMMPS,
                                            configDataValue ) != eSIR_SUCCESS)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
               "Failed to get value for WNI_CFG_ENABLE_DYNAMIC_WMMPS");
      goto handle_failure;
   }
   tlvStruct = (tHalCfg *)( (tANI_U8 *) tlvStruct
                           + sizeof(tHalCfg) + tlvStruct->length) ;

   /*                                        */
   tlvStruct->type = QWLAN_HAL_CFG_BURST_MODE_BE_TXOP_VALUE ;
   tlvStruct->length = sizeof(tANI_U32);
   configDataValue = (tANI_U32 *)(tlvStruct + 1);

   if (wlan_cfgGetInt(pMac, WNI_CFG_BURST_MODE_BE_TXOP_VALUE,
                                            configDataValue ) != eSIR_SUCCESS)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
               "Failed to get value for WNI_CFG_BURST_MODE_BE_TXOP_VALUE");
      goto handle_failure;
   }
   tlvStruct = (tHalCfg *)( (tANI_U8 *) tlvStruct
                           + sizeof(tHalCfg) + tlvStruct->length) ;

   /*                                            */
   tlvStruct->type = QWLAN_HAL_CFG_ENABLE_DYNAMIC_RA_START_RATE ;
   tlvStruct->length = sizeof(tANI_U32);
   configDataValue = (tANI_U32 *)(tlvStruct + 1);

   if (wlan_cfgGetInt(pMac, WNI_CFG_ENABLE_DYNAMIC_RA_START_RATE,
                                            configDataValue ) != eSIR_SUCCESS)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
               "Failed to get value for WNI_CFG_ENABLE_DYNAMIC_RA_START_RATE");
      goto handle_failure;
   }
   tlvStruct = (tHalCfg *)( (tANI_U8 *) tlvStruct
                           + sizeof(tHalCfg) + tlvStruct->length) ;


   wdiStartParams->usConfigBufferLen = (tANI_U8 *)tlvStruct - tlvStructStart ;
#ifdef WLAN_DEBUG
   {
      int i;
       VOS_TRACE(VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                    "****** Dumping CFG TLV ***** ");
      for (i=0; (i+7) < wdiStartParams->usConfigBufferLen; i+=8)
      {
         VOS_TRACE(VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                    "%02x %02x %02x %02x %02x %02x %02x %02x", 
                    tlvStructStart[i],
                    tlvStructStart[i+1],
                    tlvStructStart[i+2],
                    tlvStructStart[i+3],
                    tlvStructStart[i+4],
                    tlvStructStart[i+5],
                    tlvStructStart[i+6],
                    tlvStructStart[i+7]);
      }
      /*                                */
      for (; i < wdiStartParams->usConfigBufferLen; i++)
      {
         VOS_TRACE(VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                    "%02x ",tlvStructStart[i]);
      }
      VOS_TRACE(VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                    "**************************** ");
   }
#endif
   return VOS_STATUS_SUCCESS ;
handle_failure:
   vos_mem_free(configParam);
   return VOS_STATUS_E_FAILURE;
}
/*
                              
                                   
 */ 
void WDA_stopCallback(WDI_Status status, void* pUserData)
{
   tWDA_ReqParams *pWdaParams = (tWDA_ReqParams *)pUserData;
   tWDA_CbContext *wdaContext;

   if(NULL == pWdaParams)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                 "%s: pWdaParams received NULL", __func__);
      VOS_ASSERT(0) ;
      return ;
   }
   
   wdaContext = (tWDA_CbContext *)pWdaParams->pWdaContext;

   if (NULL == wdaContext)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                 "%s: Invoked with invalid wdaContext", __func__ );
      return ;
   }

   /*                           */
   if(pWdaParams->wdaWdiApiMsgParam != NULL)
   {
      vos_mem_free(pWdaParams->wdaWdiApiMsgParam);
   }
   vos_mem_free(pWdaParams);

   if(WDI_STATUS_SUCCESS != status)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                               "WDI stop callback returned failure" );
      VOS_ASSERT(0) ;
   }
   else
   {
      wdaContext->wdaState = WDA_STOP_STATE;
   }

   /*                                            
                                                 */
   if (eDRIVER_TYPE_MFG == wdaContext->driverMode)
   {
      if (VOS_STATUS_SUCCESS != vos_event_set(&wdaContext->ftmStopDoneEvent))
      {
         VOS_TRACE(VOS_MODULE_ID_VOSS, VOS_TRACE_LEVEL_ERROR,
                   "%s: FTM Stop Event Set Fail", __func__);
         VOS_ASSERT(0);
      }
   }

   /*                                        */
   vos_WDAComplete_cback(wdaContext->pVosContext);

   return ;
}
/*
                     
                
 */ 
VOS_STATUS WDA_stop(v_PVOID_t pVosContext, tANI_U8 reason)
{
   WDI_Status wdiStatus;
   VOS_STATUS status = VOS_STATUS_SUCCESS;
   WDI_StopReqParamsType *wdiStopReq;
   tWDA_CbContext *pWDA = (tWDA_CbContext *)VOS_GET_WDA_CTXT(pVosContext);
   tWDA_ReqParams *pWdaParams ;

   if (NULL == pWDA)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                 "%s: Invoked with invalid pWDA", __func__ );
      VOS_ASSERT(0);
      return VOS_STATUS_E_FAILURE;
   }
   if (pWDA->wdiFailed == true)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_FATAL,
                 "%s: WDI in failed state", __func__ );
      return VOS_STATUS_E_ALREADY;
   }

   /*                           */
   if( (WDA_READY_STATE != pWDA->wdaState) &&
       (WDA_INIT_STATE != pWDA->wdaState) &&
       (WDA_START_STATE != pWDA->wdaState) )
   {
      VOS_ASSERT(0);
   }
   wdiStopReq = (WDI_StopReqParamsType *)
                            vos_mem_malloc(sizeof(WDI_StopReqParamsType));
   if(NULL == wdiStopReq)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                           "%s: VOS MEM Alloc Failure", __func__);
      VOS_ASSERT(0);
      return VOS_STATUS_E_NOMEM;
   }

   wdiStopReq->wdiStopReason = reason;
   wdiStopReq->wdiReqStatusCB = NULL;
   
   pWdaParams = (tWDA_ReqParams *)vos_mem_malloc(sizeof(tWDA_ReqParams)) ;
   if(NULL == pWdaParams)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                           "%s: VOS MEM Alloc Failure", __func__); 
      VOS_ASSERT(0);
      vos_mem_free(wdiStopReq);
      return VOS_STATUS_E_NOMEM;
   }

   if ( (eDRIVER_TYPE_MFG != pWDA->driverMode) &&
        (VOS_TRUE == pWDA->wdaTimersCreated))
   {
      wdaDestroyTimers(pWDA);
      pWDA->wdaTimersCreated = VOS_FALSE;
   }

   pWdaParams->wdaWdiApiMsgParam = (v_PVOID_t *)wdiStopReq;
   pWdaParams->wdaMsgParam = NULL;
   pWdaParams->pWdaContext = pWDA;

   /*               */
   wdiStatus = WDI_Stop(wdiStopReq,
                           (WDI_StopRspCb)WDA_stopCallback, pWdaParams);

   if (IS_WDI_STATUS_FAILURE(wdiStatus) )
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                                  "error in WDA Stop" );
      vos_mem_free(pWdaParams->wdaWdiApiMsgParam);
      vos_mem_free(pWdaParams);
      status = VOS_STATUS_E_FAILURE;
   }

   /*                                            
                                                 */
   if (eDRIVER_TYPE_MFG == pWDA->driverMode)
   {
      status = vos_wait_single_event(&pWDA->ftmStopDoneEvent,
                                     WDI_RESPONSE_TIMEOUT);
      if (status != VOS_STATUS_SUCCESS)
      {
         VOS_TRACE(VOS_MODULE_ID_VOSS, VOS_TRACE_LEVEL_ERROR,
                   "%s: FTM Stop Timepoout", __func__);
         VOS_ASSERT(0);
      }
      vos_event_destroy(&pWDA->ftmStopDoneEvent);
   }
   return status;
}
/*
                      
                                          
 */ 
VOS_STATUS WDA_close(v_PVOID_t pVosContext)
{
   VOS_STATUS status = VOS_STATUS_SUCCESS;
   WDI_Status wstatus;
   VOS_STATUS vstatus;
   tWDA_CbContext *wdaContext= (tWDA_CbContext *)VOS_GET_WDA_CTXT(pVosContext);
   if (NULL == wdaContext)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                 "%s: Invoked with invalid wdaContext", __func__ );
      return VOS_STATUS_E_FAILURE;
   }
   if((WDA_INIT_STATE != wdaContext->wdaState) && 
                              (WDA_STOP_STATE != wdaContext->wdaState))
   {
      VOS_ASSERT(0);
   }
   /*              */
   wstatus = WDI_Close();
   if ( wstatus != WDI_STATUS_SUCCESS )
   {
      status = VOS_STATUS_E_FAILURE;
   }
   wdaContext->wdaState = WDA_CLOSE_STATE;
   /*                    */
   vstatus = vos_event_destroy(&wdaContext->wdaWdiEvent);
   if(!VOS_IS_STATUS_SUCCESS(vstatus)) 
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                "WDI Sync Event destroy failed - status = %d", status);
      status = VOS_STATUS_E_FAILURE;
   }

   vstatus = vos_event_destroy(&wdaContext->txFrameEvent);
   if(!VOS_IS_STATUS_SUCCESS(vstatus))
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                  "VOS Event destroy failed - status = %d", status);
      status = VOS_STATUS_E_FAILURE;
   }
   vstatus = vos_event_destroy(&wdaContext->suspendDataTxEvent);
   if(!VOS_IS_STATUS_SUCCESS(vstatus))
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                  "VOS Event destroy failed - status = %d", status);
      status = VOS_STATUS_E_FAILURE;
   }
   vstatus = vos_event_destroy(&wdaContext->waitOnWdiIndicationCallBack);
   if(!VOS_IS_STATUS_SUCCESS(vstatus))
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                  "VOS Event destroy failed - status = %d", status);
      status = VOS_STATUS_E_FAILURE;
   }
   /*                  */
   vstatus = vos_free_context(pVosContext, VOS_MODULE_ID_WDA, wdaContext);
   if ( !VOS_IS_STATUS_SUCCESS(vstatus) )
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                                  "error in WDA close " );
      status = VOS_STATUS_E_FAILURE;
   }
   return status;
}
/*
                                                             
                                                                                  
 */ 

uint8 WDA_IsWcnssWlanCompiledVersionGreaterThanOrEqual(uint8 major, uint8 minor, uint8 version, uint8 revision)
{    
    VOS_STATUS status = VOS_STATUS_SUCCESS;
    v_CONTEXT_t vosContext = vos_get_global_context(VOS_MODULE_ID_WDA, NULL);
    tSirVersionType compiledVersion;
    status = WDA_GetWcnssWlanCompiledVersion(vosContext, &compiledVersion);
    if ((compiledVersion.major > major) || ((compiledVersion.major == major)&& (compiledVersion.minor > minor)) ||
        ((compiledVersion.major == major)&& (compiledVersion.minor == minor) &&(compiledVersion.version > version)) ||
        ((compiledVersion.major == major)&& (compiledVersion.minor == minor) &&(compiledVersion.version == version) &&
        (compiledVersion.revision >= revision)))        
        return 1;
    else
        return 0;
}
/*
                                                             
                                                                                  
 */ 
uint8 WDA_IsWcnssWlanReportedVersionGreaterThanOrEqual(uint8 major, uint8 minor, uint8 version, uint8 revision)
{    
    VOS_STATUS status = VOS_STATUS_SUCCESS;
    v_CONTEXT_t vosContext = vos_get_global_context(VOS_MODULE_ID_WDA, NULL);
    tSirVersionType reportedVersion;
    status = WDA_GetWcnssWlanReportedVersion(vosContext, &reportedVersion);
    if ((reportedVersion.major > major) || ((reportedVersion.major == major)&& (reportedVersion.minor > minor)) ||
        ((reportedVersion.major == major)&& (reportedVersion.minor == minor) &&(reportedVersion.version > version)) ||
        ((reportedVersion.major == major)&& (reportedVersion.minor == minor) &&(reportedVersion.version == version) &&
        (reportedVersion.revision >= revision)))        
        return 1;
    else
        return 0;
}
/*
                                            
                                                                
                             
 */ 
VOS_STATUS WDA_GetWcnssWlanCompiledVersion(v_PVOID_t pvosGCtx,
                                           tSirVersionType *pVersion)
{
   tWDA_CbContext *pWDA;
   if ((NULL == pvosGCtx) || (NULL == pVersion))
   {
      VOS_TRACE(VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                "%s: Invoked with invalid parameter", __func__);
      VOS_ASSERT(0);
      return VOS_STATUS_E_FAILURE;
   }
   pWDA = (tWDA_CbContext *)VOS_GET_WDA_CTXT(pvosGCtx);
   if (NULL == pWDA )
   {
      VOS_TRACE(VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                "%s: Invalid WDA context", __func__);
      VOS_ASSERT(0);
      return VOS_STATUS_E_FAILURE;
   }
   *pVersion = pWDA->wcnssWlanCompiledVersion;
   return VOS_STATUS_SUCCESS;
}
/*
                                            
                                                                 
                             
 */ 
VOS_STATUS WDA_GetWcnssWlanReportedVersion(v_PVOID_t pvosGCtx,
                                           tSirVersionType *pVersion)
{
   tWDA_CbContext *pWDA;
   if ((NULL == pvosGCtx) || (NULL == pVersion))
   {
      VOS_TRACE(VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                "%s: Invoked with invalid parameter", __func__);
      VOS_ASSERT(0);
      return VOS_STATUS_E_FAILURE;
   }
   pWDA = (tWDA_CbContext *)VOS_GET_WDA_CTXT(pvosGCtx);
   if (NULL == pWDA )
   {
      VOS_TRACE(VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                "%s: Invalid WDA context", __func__);
      VOS_ASSERT(0);
      return VOS_STATUS_E_FAILURE;
   }
   *pVersion = pWDA->wcnssWlanReportedVersion;
   return VOS_STATUS_SUCCESS;
}
/*
                                        
                                            
 */ 
VOS_STATUS WDA_GetWcnssSoftwareVersion(v_PVOID_t pvosGCtx,
                                       tANI_U8 *pVersion,
                                       tANI_U32 versionBufferSize)
{
   tWDA_CbContext *pWDA;
   VOS_TRACE(VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
             "%s: Entered", __func__);
   if ((NULL == pvosGCtx) || (NULL == pVersion))
   {
      VOS_TRACE(VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                "%s: Invoked with invalid parameter", __func__);
      VOS_ASSERT(0);
      return VOS_STATUS_E_FAILURE;
   }
   pWDA = (tWDA_CbContext *)VOS_GET_WDA_CTXT(pvosGCtx);
   if (NULL == pWDA )
   {
      VOS_TRACE(VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                "%s: Invalid WDA context", __func__);
      VOS_ASSERT(0);
      return VOS_STATUS_E_FAILURE;
   }
   wpalMemoryCopy(pVersion, pWDA->wcnssSoftwareVersionString, versionBufferSize);
   return VOS_STATUS_SUCCESS;
}
/*
                                        
                                            
 */ 
VOS_STATUS WDA_GetWcnssHardwareVersion(v_PVOID_t pvosGCtx,
                                       tANI_U8 *pVersion,
                                       tANI_U32 versionBufferSize)
{
   tWDA_CbContext *pWDA;
   VOS_TRACE(VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
             "%s: Entered", __func__);
   if ((NULL == pvosGCtx) || (NULL == pVersion))
   {
      VOS_TRACE(VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                "%s: Invoked with invalid parameter", __func__);
      VOS_ASSERT(0);
      return VOS_STATUS_E_FAILURE;
   }
   pWDA = (tWDA_CbContext *)VOS_GET_WDA_CTXT(pvosGCtx);
   if (NULL == pWDA )
   {
      VOS_TRACE(VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                "%s: Invalid WDA context", __func__);
      VOS_ASSERT(0);
      return VOS_STATUS_E_FAILURE;
   }
   wpalMemoryCopy(pVersion, pWDA->wcnssHardwareVersionString, versionBufferSize);
   return VOS_STATUS_SUCCESS;
}
/*
                           
                       
 */ 
VOS_STATUS WDA_WniCfgDnld(tWDA_CbContext *pWDA) 
{
   tpAniSirGlobal pMac = (tpAniSirGlobal )VOS_GET_MAC_CTXT(pWDA->pVosContext);
   VOS_STATUS vosStatus = VOS_STATUS_SUCCESS;

   if (NULL == pMac )
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                 "%s: Invoked with invalid MAC context ", __func__ );
      VOS_ASSERT(0);
      return VOS_STATUS_E_FAILURE;
   }
   processCfgDownloadReq(pMac);
   return vosStatus;
}
/*                                                                  
                 
                                                                    
 */
/*
                                      
                                                               
 */ 
VOS_STATUS WDA_SuspendDataTxCallback( v_PVOID_t      pvosGCtx,
                                            v_U8_t*        ucSTAId,
                                            VOS_STATUS     vosStatus)
{
   tWDA_CbContext *pWDA = (tWDA_CbContext *)VOS_GET_WDA_CTXT(pvosGCtx);
   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                      "%s: Entered " ,__func__);
   if (NULL == pWDA )
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                 "%s: Invoked with invalid WDA context ", __func__ );
      VOS_ASSERT(0);
      return VOS_STATUS_E_FAILURE;
   }
   if(VOS_IS_STATUS_SUCCESS(vosStatus)) 
   {
      pWDA->txStatus = WDA_TL_TX_SUSPEND_SUCCESS;
   }
   else
   {
      pWDA->txStatus = WDA_TL_TX_SUSPEND_FAILURE;        
   }
   /*                                                                
                 */
   vosStatus = vos_event_set(&pWDA->suspendDataTxEvent);
   if(!VOS_IS_STATUS_SUCCESS(vosStatus))
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR, 
                      "NEW VOS Event Set failed - status = %d", vosStatus);
   }
   /*                                                                           
        */
   if (pWDA->txSuspendTimedOut) 
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                  "Late TLSuspendCallback, resuming TL back again");
      WDA_ResumeDataTx(pWDA);
      pWDA->txSuspendTimedOut = FALSE;
   }
   return VOS_STATUS_SUCCESS;
}
/*
                              
                                             
 */ 
VOS_STATUS WDA_SuspendDataTx(tWDA_CbContext *pWDA)
{
   VOS_STATUS status = VOS_STATUS_E_FAILURE;
   tANI_U8 eventIdx = 0;

   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                      "%s: Entered " ,__func__);
   pWDA->txStatus = WDA_TL_TX_SUSPEND_FAILURE;
   if (pWDA->txSuspendTimedOut) 
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
         "TL suspend timedout previously, CB not called yet");
        return status;
   }
   /*                                     */
   status = vos_event_reset(&pWDA->suspendDataTxEvent);
   if(!VOS_IS_STATUS_SUCCESS(status))
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR, 
                            "VOS Event reset failed - status = %d",status);
      return VOS_STATUS_E_FAILURE;
   }
   /*                                                   */
   status = WLANTL_SuspendDataTx(pWDA->pVosContext, NULL,
                                                WDA_SuspendDataTxCallback);
   if(status != VOS_STATUS_SUCCESS)
   {
      return status;
   }
   /*                                                                
                                                                         
                           */
   status = vos_wait_events(&pWDA->suspendDataTxEvent, 1, 
                                   WDA_TL_SUSPEND_TIMEOUT, &eventIdx);
   if(!VOS_IS_STATUS_SUCCESS(status))
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR, 
                 "%s: Status %d when waiting for Suspend Data TX Event",
                 __func__, status);
      /*                                                                  
                                                                              
                                                      */
      pWDA->txSuspendTimedOut = TRUE;
   }
   else
   {
      pWDA->txSuspendTimedOut = FALSE;
   }
   if(WDA_TL_TX_SUSPEND_SUCCESS == pWDA->txStatus)
   {
      status = VOS_STATUS_SUCCESS;
   }
   return status;
}
/*
                             
                                            
 */ 
VOS_STATUS WDA_ResumeDataTx(tWDA_CbContext *pWDA)
{
   VOS_STATUS status = VOS_STATUS_SUCCESS;

   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                      "%s: Entered " ,__func__);

   status = WLANTL_ResumeDataTx(pWDA->pVosContext, NULL);
   return status;
}
/*
                                    
                             
 */ 
void WDA_InitScanReqCallback(WDI_Status wdiStatus, void* pUserData)
{
   tWDA_ReqParams *pWdaParams = (tWDA_ReqParams *)pUserData; 
   tWDA_CbContext *pWDA; 
   tInitScanParams *pWDA_ScanParam ;
   VOS_STATUS      status;      
   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                                          "<------ %s " ,__func__);
   if(NULL == pWdaParams)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                 "%s: pWdaParams received NULL", __func__);
      VOS_ASSERT(0) ;
      return ;
   }
   pWDA = (tWDA_CbContext *)pWdaParams->pWdaContext;
   pWDA_ScanParam = (tInitScanParams *)pWdaParams->wdaMsgParam;
   if(NULL == pWDA_ScanParam)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                  "%s: pWDA_ScanParam received NULL", __func__);
      VOS_ASSERT(0);
      vos_mem_free(pWdaParams->wdaWdiApiMsgParam);
      vos_mem_free(pWdaParams);
      return ;
   }
   if(WDI_STATUS_SUCCESS != wdiStatus)
   {
      status = WDA_ResumeDataTx(pWDA) ;
      if(VOS_STATUS_SUCCESS != status)
      {
         VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                                  "%s error in Resume Tx ", __func__ );
      }
   }
   /*                         */
   vos_mem_free(pWdaParams->wdaWdiApiMsgParam);
   vos_mem_free(pWdaParams) ;
   
   
   /*                              */
   /*                                                         
                                  */
   pWDA_ScanParam->status = wdiStatus ;
   /*                                  */
   WDA_SendMsg(pWDA, WDA_INIT_SCAN_RSP, (void *)pWDA_ScanParam, 0) ;
   return ;
}
  
/*
                                   
                           
 */ 
VOS_STATUS  WDA_ProcessInitScanReq(tWDA_CbContext *pWDA, 
                                   tInitScanParams *initScanParams)
{
   WDI_Status status = WDI_STATUS_SUCCESS ;
   WDI_InitScanReqParamsType *wdiInitScanParam = 
                  (WDI_InitScanReqParamsType *)vos_mem_malloc(
                                   sizeof(WDI_InitScanReqParamsType)) ;
   tWDA_ReqParams *pWdaParams;
   tANI_U8 i = 0;
   
   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                                          "------> %s " ,__func__);
   if(NULL == wdiInitScanParam) 
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                           "%s:VOS MEM Alloc Failure", __func__); 
      VOS_ASSERT(0);
      return VOS_STATUS_E_NOMEM;
   }
   pWdaParams = (tWDA_ReqParams *)vos_mem_malloc(sizeof(tWDA_ReqParams)) ;
   if(NULL == pWdaParams)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                           "%s: VOS MEM Alloc Failure", __func__); 
      VOS_ASSERT(0);
      vos_mem_free(wdiInitScanParam);
      return VOS_STATUS_E_NOMEM;
   }
   
   /*                                        */
   wdiInitScanParam->wdiReqInfo.wdiScanMode = initScanParams->scanMode ;
   vos_mem_copy(wdiInitScanParam->wdiReqInfo.macBSSID, initScanParams->bssid,
                                             sizeof(tSirMacAddr)) ;
   wdiInitScanParam->wdiReqInfo.bNotifyBSS = initScanParams->notifyBss ;
   wdiInitScanParam->wdiReqInfo.ucFrameType = initScanParams->frameType ;
   wdiInitScanParam->wdiReqInfo.ucFrameLength = initScanParams->frameLength ;
   wdiInitScanParam->wdiReqInfo.bUseNOA = initScanParams->useNoA;
   wdiInitScanParam->wdiReqInfo.scanDuration = initScanParams->scanDuration;
   wdiInitScanParam->wdiReqInfo.wdiScanEntry.activeBSScnt = 
                                     initScanParams->scanEntry.activeBSScnt ;
   for (i=0; i < initScanParams->scanEntry.activeBSScnt; i++)
   {
       wdiInitScanParam->wdiReqInfo.wdiScanEntry.bssIdx[i] = 
                                        initScanParams->scanEntry.bssIdx[i] ;
   }
    
   /*                                                   */ 
   if(0 != wdiInitScanParam->wdiReqInfo.ucFrameLength)
   {
      vos_mem_copy(&wdiInitScanParam->wdiReqInfo.wdiMACMgmtHdr, 
                       &initScanParams->macMgmtHdr, sizeof(tSirMacMgmtHdr)) ;
   } 
   wdiInitScanParam->wdiReqStatusCB = NULL ;

   /*                                                           */
   pWdaParams->pWdaContext = pWDA;
   pWdaParams->wdaMsgParam = initScanParams;
   pWdaParams->wdaWdiApiMsgParam = wdiInitScanParam;
   /*                         */
   status = WDA_SuspendDataTx(pWDA) ;
   if(WDI_STATUS_SUCCESS != status)
   {
      goto handleWdiFailure;
   }
   /*                                               */
   status = WDI_InitScanReq(wdiInitScanParam, 
                        WDA_InitScanReqCallback, pWdaParams) ;
   if(IS_WDI_STATUS_FAILURE(status))
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                               "error in WDA Init Scan, Resume Tx " );
      status = WDA_ResumeDataTx(pWDA) ;
      VOS_ASSERT(0) ;
  
      goto handleWdiFailure;
   }
   return CONVERT_WDI2VOS_STATUS(status) ;
handleWdiFailure:
   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                      "Failure in WDI Api, free all the memory " );
   /*                         */
   vos_mem_free(pWdaParams->wdaWdiApiMsgParam);
   vos_mem_free(pWdaParams) ;
   /*                    */
   initScanParams->status = eSIR_FAILURE ;
   WDA_SendMsg(pWDA, WDA_INIT_SCAN_RSP, (void *)initScanParams, 0) ;
   return CONVERT_WDI2VOS_STATUS(status) ;
}

/*
                                     
                                 
 */ 
void WDA_StartScanReqCallback(WDI_StartScanRspParamsType *pScanRsp, 
                                                    void* pUserData)
{
   tWDA_ReqParams *pWdaParams = (tWDA_ReqParams *)pUserData; 
   tWDA_CbContext *pWDA; 
   tStartScanParams *pWDA_ScanParam;
   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                                          "<------ %s " ,__func__);
   if(NULL == pWdaParams)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                 "%s: pWdaParams received NULL", __func__);
      VOS_ASSERT(0) ;
      return ;
   }
   pWDA = (tWDA_CbContext *)pWdaParams->pWdaContext;
   pWDA_ScanParam = (tStartScanParams *)pWdaParams->wdaMsgParam;
   if(NULL == pWDA_ScanParam)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                   "%s: pWDA_ScanParam received NULL", __func__);
      VOS_ASSERT(0) ;
      vos_mem_free(pWdaParams);
      return ;
   }
   if(NULL == pWdaParams->wdaWdiApiMsgParam)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                  "%s: wdaWdiApiMsgParam is NULL", __func__);
      VOS_ASSERT(0) ;
      vos_mem_free(pWdaParams);
      return ;
   }
   vos_mem_free(pWdaParams->wdaWdiApiMsgParam);
   vos_mem_free(pWdaParams) ;
   
   
   /*                              */
   pWDA_ScanParam->status = pScanRsp->wdiStatus ;
   /*                                  */
   WDA_SendMsg(pWDA, WDA_START_SCAN_RSP, (void *)pWDA_ScanParam, 0) ;
   return ;
}

/*
                                    
                            
 */ 
VOS_STATUS  WDA_ProcessStartScanReq(tWDA_CbContext *pWDA, 
                                           tStartScanParams *startScanParams)
{
   WDI_Status status = WDI_STATUS_SUCCESS;
   WDI_StartScanReqParamsType *wdiStartScanParams = 
                            (WDI_StartScanReqParamsType *)vos_mem_malloc(
                                          sizeof(WDI_StartScanReqParamsType)) ;
   tWDA_ReqParams *pWdaParams;
   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                                          "------> %s " ,__func__);
   if(NULL == wdiStartScanParams) 
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                           "%s: VOS MEM Alloc Failure", __func__); 
      VOS_ASSERT(0);
      return VOS_STATUS_E_NOMEM;
   }
   pWdaParams = (tWDA_ReqParams *)vos_mem_malloc(sizeof(tWDA_ReqParams)) ;
   if(NULL == pWdaParams)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                           "%s: VOS MEM Alloc Failure", __func__); 
      VOS_ASSERT(0);
      vos_mem_free(wdiStartScanParams);
      return VOS_STATUS_E_NOMEM;
   }
   /*                                        */
   wdiStartScanParams->ucChannel = startScanParams->scanChannel ;
   wdiStartScanParams->wdiReqStatusCB = NULL ;

   /*                                                           */
   /*                             */
   pWdaParams->pWdaContext = pWDA;
   pWdaParams->wdaMsgParam = startScanParams;
   pWdaParams->wdaWdiApiMsgParam = wdiStartScanParams;
   /*                                               */
   status = WDI_StartScanReq(wdiStartScanParams, 
                              WDA_StartScanReqCallback, pWdaParams) ;
   /*                             */
   if(IS_WDI_STATUS_FAILURE(status))
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                     "Failure in Start Scan WDI API, free all the memory "
                     "It should be due to previous abort scan." );
      vos_mem_free(pWdaParams->wdaWdiApiMsgParam);
      vos_mem_free(pWdaParams) ;
      startScanParams->status = eSIR_FAILURE ;
      WDA_SendMsg(pWDA, WDA_START_SCAN_RSP, (void *)startScanParams, 0) ;
   }
   return CONVERT_WDI2VOS_STATUS(status) ;
}
/*
                                   
                    
 */ 
void WDA_EndScanReqCallback(WDI_Status status, void* pUserData)
{
   tWDA_ReqParams *pWdaParams = (tWDA_ReqParams *)pUserData; 
   tWDA_CbContext *pWDA; 
   tEndScanParams *endScanParam;
   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                                             "<------ %s " ,__func__);
   if(NULL == pWdaParams)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                   "%s: pWdaParams received NULL", __func__);
      VOS_ASSERT(0) ;
      return ;
   }
   pWDA = (tWDA_CbContext *)pWdaParams->pWdaContext;
   endScanParam = (tEndScanParams *)pWdaParams->wdaMsgParam;
   if(NULL == endScanParam)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                   "%s: endScanParam received NULL", __func__);
      VOS_ASSERT(0) ;
      vos_mem_free(pWdaParams->wdaWdiApiMsgParam);
      vos_mem_free(pWdaParams);
      return ;
   } 
   
   /*                         */
   vos_mem_free(pWdaParams->wdaWdiApiMsgParam);
   vos_mem_free(pWdaParams) ;
   /*                              */
   endScanParam->status = status ;
   /*                          */
   WDA_SendMsg(pWDA, WDA_END_SCAN_RSP, (void *)endScanParam, 0) ;
   return ;
}

/*
                                  
                          
 */ 
VOS_STATUS  WDA_ProcessEndScanReq(tWDA_CbContext *pWDA, 
                                           tEndScanParams *endScanParams)
{
   WDI_Status status = WDI_STATUS_SUCCESS;
   WDI_EndScanReqParamsType *wdiEndScanParams =
                            (WDI_EndScanReqParamsType *)vos_mem_malloc(
                                          sizeof(WDI_EndScanReqParamsType)) ;
   tWDA_ReqParams *pWdaParams ;
   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                                             "------> %s " ,__func__);
   if(NULL == wdiEndScanParams) 
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                           "%s: VOS MEM Alloc Failure", __func__); 
      VOS_ASSERT(0);
      return VOS_STATUS_E_NOMEM;
   }
   pWdaParams = (tWDA_ReqParams *)vos_mem_malloc(sizeof(tWDA_ReqParams)) ;
   if(NULL == pWdaParams)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                           "%s: VOS MEM Alloc Failure", __func__); 
      VOS_ASSERT(0);
      vos_mem_free(wdiEndScanParams);
      return VOS_STATUS_E_NOMEM;
   }
   /*                                        */
   wdiEndScanParams->ucChannel = endScanParams->scanChannel ;
   wdiEndScanParams->wdiReqStatusCB = NULL ;
   /*                                                           */
   /*                             */
   pWdaParams->pWdaContext = pWDA;
   pWdaParams->wdaMsgParam = endScanParams;
   pWdaParams->wdaWdiApiMsgParam = wdiEndScanParams;
   /*                                               */
   status = WDI_EndScanReq(wdiEndScanParams, 
                              WDA_EndScanReqCallback, pWdaParams) ;
   if(IS_WDI_STATUS_FAILURE(status))
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                     "Failure in End Scan WDI API, free all the memory "
                     "It should be due to previous abort scan." );
      vos_mem_free(pWdaParams->wdaWdiApiMsgParam);
      vos_mem_free(pWdaParams) ;
      endScanParams->status = eSIR_FAILURE ;
      WDA_SendMsg(pWDA, WDA_END_SCAN_RSP, (void *)endScanParams, 0) ;
   }
   return CONVERT_WDI2VOS_STATUS(status) ;
}
/*
                                      
                               
 */ 
void WDA_FinishScanReqCallback(WDI_Status wdiStatus, void* pUserData)
{
   tWDA_ReqParams *pWdaParams = (tWDA_ReqParams *)pUserData; 
   tWDA_CbContext *pWDA; 
   tFinishScanParams *finishScanParam; 
   VOS_STATUS      status;
   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                                          "<------ %s " ,__func__);
   if(NULL == pWdaParams)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                  "%s: pWdaParams received NULL", __func__);
      VOS_ASSERT(0) ;
      return ;
   }
   
   pWDA = (tWDA_CbContext *)pWdaParams->pWdaContext;
   finishScanParam = (tFinishScanParams *)pWdaParams->wdaMsgParam;
   if(NULL == finishScanParam)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                  "%s: finishScanParam is NULL", __func__);
      VOS_ASSERT(0) ;
      vos_mem_free(pWdaParams->wdaWdiApiMsgParam);
      vos_mem_free(pWdaParams);
      return ;
   }
   vos_mem_free(pWdaParams->wdaWdiApiMsgParam);
   vos_mem_free(pWdaParams) ;
   /* 
                                                                           
                                      
    */
   status = WDA_ResumeDataTx(pWDA) ;
       
   if(VOS_STATUS_SUCCESS != status)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                                  "%s error in Resume Tx ", __func__ );
   }
   finishScanParam->status = wdiStatus ;
   WDA_SendMsg(pWDA, WDA_FINISH_SCAN_RSP, (void *)finishScanParam, 0) ;
   return ;
}
/*
                                    
                             
 */ 
VOS_STATUS  WDA_ProcessFinishScanReq(tWDA_CbContext *pWDA, 
                                           tFinishScanParams *finishScanParams)
{
   WDI_Status status = WDI_STATUS_SUCCESS;
   WDI_FinishScanReqParamsType *wdiFinishScanParams =
                            (WDI_FinishScanReqParamsType *)vos_mem_malloc(
                                        sizeof(WDI_FinishScanReqParamsType)) ;
   tWDA_ReqParams *pWdaParams ;
   tANI_U8 i = 0;
   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                                             "------> %s " ,__func__);
   if(NULL == wdiFinishScanParams) 
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                           "%s: VOS MEM Alloc Failure", __func__); 
      VOS_ASSERT(0);
      return VOS_STATUS_E_NOMEM;
   }
   pWdaParams = (tWDA_ReqParams *)vos_mem_malloc(sizeof(tWDA_ReqParams)) ;
   if(NULL == pWdaParams)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                           "%s: VOS MEM Alloc Failure", __func__); 
      VOS_ASSERT(0);
      vos_mem_free(wdiFinishScanParams);
      return VOS_STATUS_E_NOMEM;
   }
   /*                                        */
   wdiFinishScanParams->wdiReqInfo.wdiScanMode = finishScanParams->scanMode ;
   vos_mem_copy(wdiFinishScanParams->wdiReqInfo.macBSSID, 
                              finishScanParams->bssid, sizeof(tSirMacAddr)) ;
   wdiFinishScanParams->wdiReqInfo.bNotifyBSS = finishScanParams->notifyBss ;
   wdiFinishScanParams->wdiReqInfo.ucFrameType = finishScanParams->frameType ;
   wdiFinishScanParams->wdiReqInfo.ucFrameLength = 
                                                finishScanParams->frameLength ;
   wdiFinishScanParams->wdiReqInfo.ucCurrentOperatingChannel = 
                                         finishScanParams->currentOperChannel ;
   wdiFinishScanParams->wdiReqInfo.wdiCBState = finishScanParams->cbState ;
   wdiFinishScanParams->wdiReqInfo.wdiScanEntry.activeBSScnt = 
                                     finishScanParams->scanEntry.activeBSScnt ;
   for (i = 0; i < finishScanParams->scanEntry.activeBSScnt; i++)
   {
       wdiFinishScanParams->wdiReqInfo.wdiScanEntry.bssIdx[i] = 
                                        finishScanParams->scanEntry.bssIdx[i] ;
   }
   
 
   /*                                                   */ 
   if(0 != wdiFinishScanParams->wdiReqInfo.ucFrameLength)
   {
      vos_mem_copy(&wdiFinishScanParams->wdiReqInfo.wdiMACMgmtHdr, 
                                          &finishScanParams->macMgmtHdr, 
                                                     sizeof(WDI_MacMgmtHdr)) ;
   } 
   wdiFinishScanParams->wdiReqStatusCB = NULL ;
   /*                                                           */
   /*                             */
   pWdaParams->pWdaContext = pWDA;
   pWdaParams->wdaMsgParam = finishScanParams;
   pWdaParams->wdaWdiApiMsgParam = wdiFinishScanParams;
   /*                                               */
   status = WDI_FinishScanReq(wdiFinishScanParams, 
                              WDA_FinishScanReqCallback, pWdaParams) ;
   
   
   /* 
                               
    */
   if(IS_WDI_STATUS_FAILURE( status))
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                     "Failure in Finish Scan WDI API, free all the memory " );
      vos_mem_free(pWdaParams->wdaWdiApiMsgParam);
      vos_mem_free(pWdaParams) ;
      finishScanParams->status = eSIR_FAILURE ;
      WDA_SendMsg(pWDA, WDA_FINISH_SCAN_RSP, (void *)finishScanParams, 0) ;
   }
   return CONVERT_WDI2VOS_STATUS(status) ;
}
/*                                                                     
              
                                                                       
 */
/*
                                
                             
 */ 
void WDA_JoinRspCallback(WDI_Status status, void* pUserData)
{
   tWDA_ReqParams *pWdaParams = (tWDA_ReqParams *)pUserData; 
   tWDA_CbContext *pWDA; 
   tSwitchChannelParams *joinReqParam;
   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                                          "<------ %s " ,__func__);
   if(NULL == pWdaParams)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
              "%s: pWdaParams received NULL", __func__);
      VOS_ASSERT(0) ;
      return ;
   }
   pWDA = (tWDA_CbContext *)pWdaParams->pWdaContext;
   joinReqParam = (tSwitchChannelParams *)pWdaParams->wdaMsgParam;
   vos_mem_free(pWdaParams->wdaWdiApiMsgParam) ;
   vos_mem_free(pWdaParams) ;
   /*                */
   vos_mem_set(pWDA->macBSSID, sizeof(pWDA->macBSSID),0 );
   /*                  */
   vos_mem_set(pWDA->macSTASelf, sizeof(pWDA->macSTASelf),0 );
   joinReqParam->status = status ;
   WDA_SendMsg(pWDA, WDA_SWITCH_CHANNEL_RSP, (void *)joinReqParam , 0) ;
   return ;
}

/*
                                
                                                      
                                                                              
 */
void WDA_JoinReqCallback(WDI_Status wdiStatus, void* pUserData)
{
   tWDA_ReqParams *pWdaParams = (tWDA_ReqParams *)pUserData;
   tWDA_CbContext *pWDA;
   tSwitchChannelParams *joinReqParam;

   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
              "<------ %s, wdiStatus: %d", __func__, wdiStatus);

   if(NULL == pWdaParams)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                 "%s: pWdaParams received NULL", __func__);
      VOS_ASSERT(0);
      return;
   }

   pWDA = (tWDA_CbContext *)pWdaParams->pWdaContext;
   joinReqParam = (tSwitchChannelParams *)pWdaParams->wdaMsgParam;
   joinReqParam->status = wdiStatus;

   if(IS_WDI_STATUS_FAILURE(wdiStatus))
   {
      /*                */
      vos_mem_set(pWDA->macBSSID, sizeof(pWDA->macBSSID),0 );
      /*                  */
      vos_mem_set(pWDA->macSTASelf, sizeof(pWDA->macSTASelf),0 );

      vos_mem_free(pWdaParams->wdaWdiApiMsgParam);
      vos_mem_free(pWdaParams);
      WDA_SendMsg(pWDA, WDA_SWITCH_CHANNEL_RSP, (void *)joinReqParam , 0) ;
   }

   return;
}

/*
                               
                          
 */ 
VOS_STATUS WDA_ProcessJoinReq(tWDA_CbContext *pWDA, 
                                            tSwitchChannelParams* joinReqParam)
{
   WDI_Status status = WDI_STATUS_SUCCESS ;
   WDI_JoinReqParamsType *wdiJoinReqParam = 
                             (WDI_JoinReqParamsType *)vos_mem_malloc(
                                   sizeof(WDI_JoinReqParamsType)) ;
   tWDA_ReqParams *pWdaParams ;
   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                                          "------> %s " ,__func__);
   if(NULL == wdiJoinReqParam) 
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                           "%s: VOS MEM Alloc Failure", __func__); 
      VOS_ASSERT(0);
      vos_mem_free(joinReqParam);
      return VOS_STATUS_E_NOMEM;
   }
   pWdaParams = (tWDA_ReqParams *)vos_mem_malloc(sizeof(tWDA_ReqParams)) ;
   if(NULL == pWdaParams)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                           "%s: VOS MEM Alloc Failure", __func__); 
      VOS_ASSERT(0);
      vos_mem_free(wdiJoinReqParam);
      vos_mem_free(joinReqParam);
      return VOS_STATUS_E_NOMEM;
   }

   /*                                                                         */
   if(WDA_IS_NULL_MAC_ADDRESS(pWDA->macBSSID) || 
      WDA_IS_NULL_MAC_ADDRESS(pWDA->macSTASelf))
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_FATAL,
                 "%s: received join request when BSSID or self-STA is NULL "
                 " BSSID:" WDA_MAC_ADDRESS_STR "Self-STA:" WDA_MAC_ADDRESS_STR,
                 __func__, WDA_MAC_ADDR_ARRAY(pWDA->macBSSID),
                 WDA_MAC_ADDR_ARRAY(pWDA->macSTASelf)); 
      VOS_ASSERT(0);
      vos_mem_free(wdiJoinReqParam);
      vos_mem_free(pWdaParams);
      joinReqParam->status = eSIR_FAILURE ;
      WDA_SendMsg(pWDA, WDA_SWITCH_CHANNEL_RSP, (void *)joinReqParam, 0) ;
      return VOS_STATUS_E_INVAL;
   }

   /*                         */
   vos_mem_copy(wdiJoinReqParam->wdiReqInfo.macBSSID, pWDA->macBSSID, 
                                             sizeof(tSirMacAddr)) ;
   vos_mem_copy(wdiJoinReqParam->wdiReqInfo.macSTASelf, 
                pWDA->macSTASelf, sizeof(tSirMacAddr)) ;
   wdiJoinReqParam->wdiReqInfo.wdiChannelInfo.ucChannel = 
                                                 joinReqParam->channelNumber ;
#ifdef WLAN_FEATURE_VOWIFI
   wdiJoinReqParam->wdiReqInfo.wdiChannelInfo.cMaxTxPower =
                                          joinReqParam->maxTxPower ;
#else
   wdiJoinReqParam->wdiReqInfo.wdiChannelInfo.ucLocalPowerConstraint = 
                                          joinReqParam->localPowerConstraint ;
#endif
   wdiJoinReqParam->wdiReqInfo.wdiChannelInfo.wdiSecondaryChannelOffset = 
                                        joinReqParam->secondaryChannelOffset ;
   wdiJoinReqParam->wdiReqInfo.linkState = pWDA->linkState;
   
   wdiJoinReqParam->wdiReqStatusCB = WDA_JoinReqCallback;
   wdiJoinReqParam->pUserData = pWdaParams;

   /*                                                           */
   /*                             */
   pWdaParams->pWdaContext = pWDA;
   pWdaParams->wdaMsgParam = joinReqParam;
   pWdaParams->wdaWdiApiMsgParam = wdiJoinReqParam;
   status = WDI_JoinReq(wdiJoinReqParam, 
                               (WDI_JoinRspCb )WDA_JoinRspCallback, pWdaParams) ;
   if(IS_WDI_STATUS_FAILURE(status))
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                   "Failure in Join WDI API, free all the memory " );
      vos_mem_free(pWdaParams->wdaWdiApiMsgParam);
      vos_mem_free(pWdaParams) ;
      joinReqParam->status = eSIR_FAILURE ;
      WDA_SendMsg(pWDA, WDA_SWITCH_CHANNEL_RSP, (void *)joinReqParam, 0) ;
   }
   return CONVERT_WDI2VOS_STATUS(status) ;
}
/*
                                         
                                     
 */ 
void WDA_SwitchChannelReqCallback(
               WDI_SwitchCHRspParamsType   *wdiSwitchChanRsp, void* pUserData)
{
   tWDA_ReqParams *pWdaParams = (tWDA_ReqParams *)pUserData ; 
   tWDA_CbContext *pWDA; 
   tSwitchChannelParams *pSwitchChanParams;
   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                                          "<------ %s " ,__func__);
   if(NULL == pWdaParams)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                   "%s: pWdaParams received NULL", __func__);
      VOS_ASSERT(0) ;
      return ;
   }
   pWDA = (tWDA_CbContext *)pWdaParams->pWdaContext;
   pSwitchChanParams = (tSwitchChannelParams *)pWdaParams->wdaMsgParam;
   
#ifdef WLAN_FEATURE_VOWIFI
   pSwitchChanParams->txMgmtPower  =  wdiSwitchChanRsp->ucTxMgmtPower;
#endif
   vos_mem_free(pWdaParams->wdaWdiApiMsgParam);
   vos_mem_free(pWdaParams) ;
   pSwitchChanParams->status = 
                          wdiSwitchChanRsp->wdiStatus ;
   WDA_SendMsg(pWDA, WDA_SWITCH_CHANNEL_RSP, (void *)pSwitchChanParams , 0) ;
   return ;
}
/*
                                        
                                               
 */ 
VOS_STATUS WDA_ProcessChannelSwitchReq(tWDA_CbContext *pWDA, 
                                       tSwitchChannelParams *pSwitchChanParams)
{
   WDI_Status status = WDI_STATUS_SUCCESS ;
   WDI_SwitchChReqParamsType *wdiSwitchChanParam = 
                         (WDI_SwitchChReqParamsType *)vos_mem_malloc(
                                         sizeof(WDI_SwitchChReqParamsType)) ;
   tWDA_ReqParams *pWdaParams ;
   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                                          "------> %s " ,__func__);
   if(NULL == wdiSwitchChanParam) 
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                           "%s: VOS MEM Alloc Failure", __func__); 
      VOS_ASSERT(0);
      return VOS_STATUS_E_NOMEM;
   }
   pWdaParams = (tWDA_ReqParams *)vos_mem_malloc(sizeof(tWDA_ReqParams)) ;
   if(NULL == pWdaParams)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                           "%s: VOS MEM Alloc Failure", __func__); 
      VOS_ASSERT(0);
      vos_mem_free(wdiSwitchChanParam);
      return VOS_STATUS_E_NOMEM;
   }
   wdiSwitchChanParam->wdiChInfo.ucChannel = pSwitchChanParams->channelNumber;
#ifndef WLAN_FEATURE_VOWIFI
   wdiSwitchChanParam->wdiChInfo.ucLocalPowerConstraint = 
                                       pSwitchChanParams->localPowerConstraint;
#endif
   wdiSwitchChanParam->wdiChInfo.wdiSecondaryChannelOffset = 
                                     pSwitchChanParams->secondaryChannelOffset;
   wdiSwitchChanParam->wdiReqStatusCB = NULL ;
   /*                                                      */
   /*                             */
   pWdaParams->pWdaContext = pWDA;
   pWdaParams->wdaMsgParam = pSwitchChanParams;
   pWdaParams->wdaWdiApiMsgParam = wdiSwitchChanParam;
#ifdef WLAN_FEATURE_VOWIFI
   wdiSwitchChanParam->wdiChInfo.cMaxTxPower 
                                     = pSwitchChanParams->maxTxPower;
   vos_mem_copy(wdiSwitchChanParam->wdiChInfo.macSelfStaMacAddr, 
                    pSwitchChanParams ->selfStaMacAddr,
                    sizeof(tSirMacAddr));
#endif
   vos_mem_copy(wdiSwitchChanParam->wdiChInfo.macBSSId, 
                     pSwitchChanParams->bssId,
                     sizeof(tSirMacAddr));

   status = WDI_SwitchChReq(wdiSwitchChanParam, 
                     (WDI_SwitchChRspCb)WDA_SwitchChannelReqCallback, pWdaParams);
   if(IS_WDI_STATUS_FAILURE(status))
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
       "Failure in process channel switch Req WDI API, free all the memory " );
      vos_mem_free(pWdaParams->wdaWdiApiMsgParam);
      vos_mem_free(pWdaParams) ;
      pSwitchChanParams->status = eSIR_FAILURE ;
      WDA_SendMsg(pWDA, WDA_SWITCH_CHANNEL_RSP, (void *)pSwitchChanParams, 0) ;
   }
   return CONVERT_WDI2VOS_STATUS(status) ;
}

/*
                                            
                                     
 */
void WDA_SwitchChannelReqCallback_V1(
               WDI_SwitchChRspParamsType_V1 *wdiSwitchChanRsp,
               void* pUserData)
{
   tWDA_ReqParams *pWdaParams = (tWDA_ReqParams *)pUserData ;
   tWDA_CbContext *pWDA;
   tSwitchChannelParams *pSwitchChanParams;
   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                 "<------ %s " ,__func__);

   if (NULL == pWdaParams)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                   "%s: pWdaParams received NULL", __func__);
      VOS_ASSERT(0);
      return ;
   }
   pWDA = (tWDA_CbContext *)pWdaParams->pWdaContext;
   pSwitchChanParams =
        (tSwitchChannelParams *)pWdaParams->wdaMsgParam;
   pSwitchChanParams->channelSwitchSrc =
        wdiSwitchChanRsp->channelSwitchSrc;
#ifdef WLAN_FEATURE_VOWIFI
   pSwitchChanParams->txMgmtPower =
        wdiSwitchChanRsp->ucTxMgmtPower;
#endif
   vos_mem_free(pWdaParams->wdaWdiApiMsgParam);
   vos_mem_free(pWdaParams);
   pSwitchChanParams->status =
                          wdiSwitchChanRsp->wdiStatus ;
   WDA_SendMsg(pWDA, WDA_SWITCH_CHANNEL_RSP,
        (void *)pSwitchChanParams , 0);
   return;
}

/*
                                           
                                               
 */
VOS_STATUS WDA_ProcessChannelSwitchReq_V1(tWDA_CbContext *pWDA,
                        tSwitchChannelParams *pSwitchChanParams)
{
   WDI_Status status = WDI_STATUS_SUCCESS ;
   WDI_SwitchChReqParamsType_V1 *wdiSwitchChanParam =
                    (WDI_SwitchChReqParamsType_V1 *)vos_mem_malloc(
                    sizeof(WDI_SwitchChReqParamsType_V1)) ;
   tWDA_ReqParams *pWdaParams ;
   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                "------> %s " ,__func__);
   if (NULL == wdiSwitchChanParam)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                           "%s: VOS MEM Alloc Failure", __func__);
      VOS_ASSERT(0);
      return VOS_STATUS_E_NOMEM;
   }
   pWdaParams = (tWDA_ReqParams *)vos_mem_malloc(sizeof(tWDA_ReqParams)) ;
   if (NULL == pWdaParams)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                           "%s: VOS MEM Alloc Failure", __func__);
      VOS_ASSERT(0);
      vos_mem_free(wdiSwitchChanParam);
      return VOS_STATUS_E_NOMEM;
   }
   wdiSwitchChanParam->wdiChInfo.channelSwitchSrc =
        pSwitchChanParams->channelSwitchSrc;

   wdiSwitchChanParam->wdiChInfo.ucChannel =
        pSwitchChanParams->channelNumber;
#ifndef WLAN_FEATURE_VOWIFI
   wdiSwitchChanParam->wdiChInfo.ucLocalPowerConstraint =
        pSwitchChanParams->localPowerConstraint;
#endif
   wdiSwitchChanParam->wdiChInfo.wdiSecondaryChannelOffset =
        pSwitchChanParams->secondaryChannelOffset;
   wdiSwitchChanParam->wdiReqStatusCB = NULL ;
   /*                                                      */
   /*                             */
   pWdaParams->pWdaContext = pWDA;
   pWdaParams->wdaMsgParam = pSwitchChanParams;
   pWdaParams->wdaWdiApiMsgParam = wdiSwitchChanParam;
#ifdef WLAN_FEATURE_VOWIFI
   wdiSwitchChanParam->wdiChInfo.cMaxTxPower =
        pSwitchChanParams->maxTxPower;
   vos_mem_copy(wdiSwitchChanParam->wdiChInfo.macSelfStaMacAddr,
                    pSwitchChanParams ->selfStaMacAddr,
                    sizeof(tSirMacAddr));
#endif
   vos_mem_copy(wdiSwitchChanParam->wdiChInfo.macBSSId,
                    pSwitchChanParams->bssId,
                    sizeof(tSirMacAddr));

   status = WDI_SwitchChReq_V1(wdiSwitchChanParam,
            (WDI_SwitchChRspCb_V1)WDA_SwitchChannelReqCallback_V1,
            pWdaParams);
   if (IS_WDI_STATUS_FAILURE(status))
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
           "Failure in process channel switch Req WDI "
           "API, free all the memory " );
      vos_mem_free(pWdaParams->wdaWdiApiMsgParam);
      vos_mem_free(pWdaParams) ;
      pSwitchChanParams->status = eSIR_FAILURE ;
      WDA_SendMsg(pWDA, WDA_SWITCH_CHANNEL_RSP,
            (void *)pSwitchChanParams, 0) ;
   }
   return CONVERT_WDI2VOS_STATUS(status) ;
}

/*
                                     
                                         
 */ 
void WDA_ConfigBssReqCallback(WDI_ConfigBSSRspParamsType *wdiConfigBssRsp  
                                                          ,void* pUserData)
{
   tWDA_ReqParams *pWdaParams = (tWDA_ReqParams *)pUserData; 
   tWDA_CbContext *pWDA; 
   tAddBssParams *configBssReqParam;
   tAddStaParams *staConfigBssParam;
   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                                          "<------ %s " ,__func__);
   if(NULL == pWdaParams)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                  "%s: pWdaParams received NULL", __func__);
      VOS_ASSERT(0) ;
      return ;
   }
   pWDA = (tWDA_CbContext *)pWdaParams->pWdaContext;
   configBssReqParam = (tAddBssParams *)pWdaParams->wdaMsgParam;
   staConfigBssParam = &configBssReqParam->staContext ;
   configBssReqParam->status = 
                     wdiConfigBssRsp->wdiStatus;
   if(WDI_STATUS_SUCCESS == wdiConfigBssRsp->wdiStatus)
   {
      vos_mem_copy(configBssReqParam->bssId, wdiConfigBssRsp->macBSSID, 
                                                    sizeof(tSirMacAddr));
      configBssReqParam->bssIdx = wdiConfigBssRsp->ucBSSIdx;
      configBssReqParam->bcastDpuSignature = wdiConfigBssRsp->ucBcastSig;
      configBssReqParam->mgmtDpuSignature = wdiConfigBssRsp->ucUcastSig;
      
      if (configBssReqParam->operMode == BSS_OPERATIONAL_MODE_STA) 
      {
         if(configBssReqParam->bssType == eSIR_IBSS_MODE) 
         {
            pWDA->wdaGlobalSystemRole = eSYSTEM_STA_IN_IBSS_ROLE;
            staConfigBssParam->staType = STA_ENTRY_BSSID;
         }
         else if ((configBssReqParam->bssType == eSIR_BTAMP_STA_MODE) &&
                 (staConfigBssParam->staType == STA_ENTRY_SELF))
         {
            /*                                               */
            pWDA->wdaGlobalSystemRole = eSYSTEM_BTAMP_STA_ROLE;
            staConfigBssParam->staType = STA_ENTRY_BSSID;
         }
         else if ((configBssReqParam->bssType == eSIR_BTAMP_AP_MODE) &&
                (staConfigBssParam->staType == STA_ENTRY_PEER))
         {
            /*                                             
                                                     
                                          */
            pWDA->wdaGlobalSystemRole = eSYSTEM_BTAMP_STA_ROLE;
        }
         else if ((configBssReqParam->bssType == eSIR_BTAMP_AP_MODE) &&
                      (staConfigBssParam->staType == STA_ENTRY_SELF))
         {
            /*                              
                                                               */
            pWDA->wdaGlobalSystemRole = eSYSTEM_BTAMP_AP_ROLE;
            staConfigBssParam->staType = STA_ENTRY_BSSID;
         } 
         else 
         {
            pWDA->wdaGlobalSystemRole = eSYSTEM_STA_ROLE;
            staConfigBssParam->staType = STA_ENTRY_PEER;
         }
      }
      else if (configBssReqParam->operMode == BSS_OPERATIONAL_MODE_AP) 
      {
         pWDA->wdaGlobalSystemRole = eSYSTEM_AP_ROLE;
         staConfigBssParam->staType = STA_ENTRY_SELF;
      }
      else
      {
         VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                                          "Invalid operation mode specified");
         VOS_ASSERT(0);
      }
      
      staConfigBssParam->staIdx = wdiConfigBssRsp->ucSTAIdx;
      staConfigBssParam->bssIdx = wdiConfigBssRsp->ucBSSIdx;
      staConfigBssParam->ucUcastSig = wdiConfigBssRsp->ucUcastSig;
      staConfigBssParam->ucBcastSig = wdiConfigBssRsp->ucBcastSig;
      vos_mem_copy(staConfigBssParam->staMac, wdiConfigBssRsp->macSTA,
                                                    sizeof(tSirMacAddr));
      staConfigBssParam->txChannelWidthSet = 
                               configBssReqParam->txChannelWidthSet;
      if(staConfigBssParam->staType == STA_ENTRY_PEER && 
                                    staConfigBssParam->htCapable)
      {
         pWDA->wdaStaInfo[staConfigBssParam->staIdx].bssIdx = 
                                                    wdiConfigBssRsp->ucBSSIdx;
         pWDA->wdaStaInfo[staConfigBssParam->staIdx].ucValidStaIndex = 
                                                         WDA_VALID_STA_INDEX ;
         pWDA->wdaStaInfo[staConfigBssParam->staIdx].currentOperChan =
                                        configBssReqParam->currentOperChannel;
      }
      if(WDI_DS_SetStaIdxPerBssIdx(pWDA->pWdiContext,
                                   wdiConfigBssRsp->ucBSSIdx,
                                   wdiConfigBssRsp->ucSTAIdx))
      {
         VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                    "%s: fail to set STA idx associated with BSS index", __func__);
         VOS_ASSERT(0) ;
      }
      if(WDI_DS_AddSTAMemPool(pWDA->pWdiContext, wdiConfigBssRsp->ucSTAIdx))
      {
         VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                    "%s: add BSS into mempool fail", __func__);
         VOS_ASSERT(0) ;
      }
#ifdef WLAN_FEATURE_VOWIFI
      configBssReqParam->txMgmtPower = wdiConfigBssRsp->ucTxMgmtPower;
#endif
   }
   vos_mem_zero(pWdaParams->wdaWdiApiMsgParam,
                 sizeof(WDI_ConfigBSSReqParamsType));
   vos_mem_free(pWdaParams->wdaWdiApiMsgParam);
   vos_mem_free(pWdaParams) ;
   WDA_SendMsg(pWDA, WDA_ADD_BSS_RSP, (void *)configBssReqParam , 0) ;
   return ;
}
/*
                                      
                                             
 */ 
void WDA_UpdateEdcaParamsForAC(tWDA_CbContext *pWDA, 
                         WDI_EdcaParamRecord *wdiEdcaParam, 
                             tSirMacEdcaParamRecord *macEdcaParam)
{
   wdiEdcaParam->wdiACI.aifsn = macEdcaParam->aci.aifsn;
   wdiEdcaParam->wdiACI.acm= macEdcaParam->aci.acm;
   wdiEdcaParam->wdiACI.aci = macEdcaParam->aci.aci;
   wdiEdcaParam->wdiCW.min = macEdcaParam->cw.min;
   wdiEdcaParam->wdiCW.max = macEdcaParam->cw.max;
   wdiEdcaParam->usTXOPLimit = macEdcaParam->txoplimit;
}
/*
                                    
                                              
 */ 
VOS_STATUS WDA_ProcessConfigBssReq(tWDA_CbContext *pWDA, 
                                         tAddBssParams* configBssReqParam)
{
   WDI_Status status = WDI_STATUS_SUCCESS ;
   WDI_ConfigBSSReqParamsType *wdiConfigBssReqParam;
   tWDA_ReqParams *pWdaParams ;
   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                                          "------> %s " ,__func__);
   if (NULL == configBssReqParam)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_FATAL,
                           "%s: configBssReqParam is NULL", __func__);
      return VOS_STATUS_E_INVAL;
   }

   wdiConfigBssReqParam = (WDI_ConfigBSSReqParamsType *)vos_mem_malloc(
                          sizeof(WDI_ConfigBSSReqParamsType)) ;

   if(NULL == wdiConfigBssReqParam) 
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                           "%s: VOS MEM Alloc Failure", __func__); 
      VOS_ASSERT(0);
      return VOS_STATUS_E_NOMEM;
   }
   pWdaParams = (tWDA_ReqParams *)vos_mem_malloc(sizeof(tWDA_ReqParams)) ;
   if(NULL == pWdaParams)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                           "%s: VOS MEM Alloc Failure", __func__); 
      VOS_ASSERT(0);
      vos_mem_free(wdiConfigBssReqParam);
      return VOS_STATUS_E_NOMEM;
   }
   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                           "%s: maxTxPower %d", __func__, configBssReqParam->maxTxPower);
   vos_mem_set(wdiConfigBssReqParam, sizeof(WDI_ConfigBSSReqParamsType), 0);
   WDA_UpdateBSSParams(pWDA, &wdiConfigBssReqParam->wdiReqInfo, 
                       configBssReqParam) ;
   /*                                                           */
   /*                             */
   pWdaParams->pWdaContext = pWDA;
   pWdaParams->wdaMsgParam = configBssReqParam;
   pWdaParams->wdaWdiApiMsgParam = wdiConfigBssReqParam;
   status = WDI_ConfigBSSReq(wdiConfigBssReqParam, 
                        (WDI_ConfigBSSRspCb )WDA_ConfigBssReqCallback, pWdaParams) ;
   if(IS_WDI_STATUS_FAILURE(status))
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                   "Failure in Config BSS WDI API, free all the memory " );
      vos_mem_free(pWdaParams->wdaWdiApiMsgParam);
      vos_mem_free(pWdaParams) ;
      configBssReqParam->status = eSIR_FAILURE ;
      WDA_SendMsg(pWDA, WDA_ADD_BSS_RSP, (void *)configBssReqParam, 0) ;
   }
   return CONVERT_WDI2VOS_STATUS(status) ;
}
#ifdef ENABLE_HAL_COMBINED_MESSAGES
/*
                                     
                                               
 */ 
void WDA_PostAssocReqCallback(WDI_PostAssocRspParamsType *wdiPostAssocRsp,  
                                                          void* pUserData)
{
   tWDA_CbContext *pWDA = (tWDA_CbContext *)pUserData ; 
   tPostAssocParams *postAssocReqParam = 
                             (tPostAssocParams *)pWDA->wdaMsgParam ;
   /*                                 */
   tAddStaParams *staPostAssocParam = 
      &postAssocReqParam->addBssParams.staContext ;
   /*                       */
   tAddStaParams *selfStaPostAssocParam = 
      &postAssocReqParam->addStaParams ;
   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                                          "<------ %s " ,__func__);
   postAssocReqParam->status = 
                   wdiPostAssocRsp->wdiStatus ;
   if(WDI_STATUS_SUCCESS == wdiPostAssocRsp->wdiStatus)
   {
      staPostAssocParam->staIdx = wdiPostAssocRsp->bssParams.ucSTAIdx ;
      vos_mem_copy(staPostAssocParam->staMac, wdiPostAssocRsp->bssParams.macSTA, 
                   sizeof(tSirMacAddr)) ;
      staPostAssocParam->ucUcastSig = wdiPostAssocRsp->bssParams.ucUcastSig ;
      staPostAssocParam->ucBcastSig = wdiPostAssocRsp->bssParams.ucBcastSig ;
      staPostAssocParam->bssIdx = wdiPostAssocRsp->bssParams.ucBSSIdx;
      selfStaPostAssocParam->staIdx = wdiPostAssocRsp->staParams.ucSTAIdx;
   }
   vos_mem_zero(pWDA->wdaWdiApiMsgParam, sizeof(WDI_PostAssocReqParamsType));
   vos_mem_free(pWDA->wdaWdiApiMsgParam) ;
   pWDA->wdaWdiApiMsgParam = NULL;
   pWDA->wdaMsgParam = NULL;
   WDA_SendMsg(pWDA, WDA_POST_ASSOC_RSP, (void *)postAssocReqParam, 0) ;
   return ;
}
/*
                                    
                                       
 */ 
VOS_STATUS WDA_ProcessPostAssocReq(tWDA_CbContext *pWDA, 
                                    tPostAssocParams *postAssocReqParam)
{
   WDI_Status status = WDI_STATUS_SUCCESS ;
   
   WDI_PostAssocReqParamsType *wdiPostAssocReqParam =
                           (WDI_PostAssocReqParamsType *)vos_mem_malloc(
                                   sizeof(WDI_PostAssocReqParamsType)) ;
   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                                          "------> %s " ,__func__);

   if(NULL == wdiPostAssocReqParam) 
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                           "%s: VOS MEM Alloc Failure", __func__); 
      VOS_ASSERT(0);
      return VOS_STATUS_E_NOMEM;
   }
   
   if((NULL != pWDA->wdaMsgParam) || (NULL != pWDA->wdaWdiApiMsgParam))
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
             "%s: wdaMsgParam or wdaWdiApiMsgParam is not NULL", __func__); 
      VOS_ASSERT(0);
      return VOS_STATUS_E_FAILURE;
   }

   /*                                      */
   WDA_UpdateBSSParams(pWDA, &wdiPostAssocReqParam->wdiBSSParams, 
                       &postAssocReqParam->addBssParams) ;
   /*                                      */
   WDA_UpdateSTAParams(pWDA, &wdiPostAssocReqParam->wdiSTAParams, 
                       &postAssocReqParam->addStaParams) ;
   
   wdiPostAssocReqParam->wdiBSSParams.ucEDCAParamsValid = 
                           postAssocReqParam->addBssParams.highPerformance;
   WDA_UpdateEdcaParamsForAC(pWDA, 
                        &wdiPostAssocReqParam->wdiBSSParams.wdiBEEDCAParams,
                        &postAssocReqParam->addBssParams.acbe);
   WDA_UpdateEdcaParamsForAC(pWDA, 
                        &wdiPostAssocReqParam->wdiBSSParams.wdiBKEDCAParams,
                        &postAssocReqParam->addBssParams.acbk);
   WDA_UpdateEdcaParamsForAC(pWDA, 
                        &wdiPostAssocReqParam->wdiBSSParams.wdiVIEDCAParams,
                        &postAssocReqParam->addBssParams.acvi);
   WDA_UpdateEdcaParamsForAC(pWDA, 
                        &wdiPostAssocReqParam->wdiBSSParams.wdiVOEDCAParams,
                        &postAssocReqParam->addBssParams.acvo);
   /*                                                           */
   pWDA->wdaMsgParam = (void *)postAssocReqParam ;
   /*                             */
   pWDA->wdaWdiApiMsgParam = (void *)wdiPostAssocReqParam ;
   status = WDI_PostAssocReq(wdiPostAssocReqParam, 
                        (WDI_PostAssocRspCb )WDA_PostAssocReqCallback, pWDA) ;
   if(IS_WDI_STATUS_FAILURE(status))
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                   "Failure in Post Assoc WDI API, free all the memory " );
      vos_mem_free(pWDA->wdaWdiApiMsgParam) ;
      pWDA->wdaWdiApiMsgParam = NULL;
      pWDA->wdaMsgParam = NULL;
      postAssocReqParam->status = eSIR_FAILURE ;
      WDA_SendMsg(pWDA, WDA_POST_ASSOC_RSP, (void *)postAssocReqParam, 0) ;
   }
   return CONVERT_WDI2VOS_STATUS(status) ;
}
#endif
/*
                                  
                                            
 */ 
void WDA_AddStaReqCallback(WDI_ConfigSTARspParamsType *wdiConfigStaRsp,  
                                                          void* pUserData)
{
   tWDA_ReqParams *pWdaParams = (tWDA_ReqParams *)pUserData; 
   tWDA_CbContext *pWDA; 
   tAddStaParams *addStaReqParam;
   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                                          "<------ %s " ,__func__);
   if(NULL == pWdaParams)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,"%s: pWdaParams is NULL", __func__);
      VOS_ASSERT(0) ;
      return ;
   }
   pWDA = (tWDA_CbContext *)pWdaParams->pWdaContext;
   addStaReqParam = (tAddStaParams *)pWdaParams->wdaMsgParam;
   addStaReqParam->status = 
                   wdiConfigStaRsp->wdiStatus ;
   if(WDI_STATUS_SUCCESS == wdiConfigStaRsp->wdiStatus)
   {
      addStaReqParam->staIdx = wdiConfigStaRsp->ucSTAIdx;
      /*                                              */
      /*                                               
                                                          
                                                             */
      addStaReqParam->ucUcastSig = wdiConfigStaRsp->ucUcastSig;
      addStaReqParam->ucBcastSig = wdiConfigStaRsp->ucBcastSig;
      /*                                                           */
#ifdef FEATURE_WLAN_TDLS 
      if( (addStaReqParam->staType == STA_ENTRY_PEER ||
         addStaReqParam->staType == STA_ENTRY_TDLS_PEER) && addStaReqParam->htCapable)
#else
      if(addStaReqParam->staType == STA_ENTRY_PEER && addStaReqParam->htCapable)
#endif
      {
         pWDA->wdaStaInfo[addStaReqParam->staIdx].bssIdx = 
                                                    wdiConfigStaRsp->ucBssIdx;
         pWDA->wdaStaInfo[addStaReqParam->staIdx].ucValidStaIndex = 
                                                         WDA_VALID_STA_INDEX ;
         pWDA->wdaStaInfo[addStaReqParam->staIdx].currentOperChan =
                                             addStaReqParam->currentOperChan;
      }
      if(WDI_DS_AddSTAMemPool(pWDA->pWdiContext, wdiConfigStaRsp->ucSTAIdx))
      {
         VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                    "%s: add STA into mempool fail", __func__);
         VOS_ASSERT(0) ;
         return ;
      }
   }
   vos_mem_free(pWdaParams->wdaWdiApiMsgParam) ;
   vos_mem_free(pWdaParams) ;
   WDA_SendMsg(pWDA, WDA_ADD_STA_RSP, (void *)addStaReqParam, 0) ;
   return ;
}
/*
                             
                                       
 */ 
VOS_STATUS WDA_ProcessAddStaReq(tWDA_CbContext *pWDA, 
                                    tAddStaParams *addStaReqParam)
{
   WDI_Status status = WDI_STATUS_SUCCESS ;
   WDI_ConfigSTAReqParamsType *wdiConfigStaReqParam =
                           (WDI_ConfigSTAReqParamsType *)vos_mem_malloc(
                                   sizeof(WDI_ConfigSTAReqParamsType)) ;
   tWDA_ReqParams *pWdaParams ;
   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                                          "------> %s " ,__func__);
   if(NULL == wdiConfigStaReqParam) 
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                           "%s: VOS MEM Alloc Failure", __func__); 
      VOS_ASSERT(0);
      return VOS_STATUS_E_NOMEM;
   }
   pWdaParams = (tWDA_ReqParams *)vos_mem_malloc(sizeof(tWDA_ReqParams)) ;
   if(NULL == pWdaParams)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                           "%s: VOS MEM Alloc Failure", __func__); 
      VOS_ASSERT(0);
      vos_mem_free(wdiConfigStaReqParam);
      return VOS_STATUS_E_NOMEM;
   }
   vos_mem_set(wdiConfigStaReqParam, sizeof(WDI_ConfigSTAReqParamsType), 0);
   /*                                      */
   WDA_UpdateSTAParams(pWDA, &wdiConfigStaReqParam->wdiReqInfo, 
                       addStaReqParam) ;
   /*                                                           */
   /*                             */
   pWdaParams->pWdaContext = pWDA;
   pWdaParams->wdaMsgParam = addStaReqParam;
   pWdaParams->wdaWdiApiMsgParam = wdiConfigStaReqParam;
   status = WDI_ConfigSTAReq(wdiConfigStaReqParam, 
                        (WDI_ConfigSTARspCb )WDA_AddStaReqCallback, pWdaParams) ;
   if(IS_WDI_STATUS_FAILURE(status))
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                   "Failure in Config STA WDI API, free all the memory " );
      vos_mem_free(pWdaParams->wdaWdiApiMsgParam);
      vos_mem_free(pWdaParams) ;
      addStaReqParam->status = eSIR_FAILURE ;
      WDA_SendMsg(pWDA, WDA_ADD_STA_RSP, (void *)addStaReqParam, 0) ;
   }
   return CONVERT_WDI2VOS_STATUS(status) ;
}
/*
                                  
                              
 */ 
void WDA_DelBSSReqCallback(WDI_DelBSSRspParamsType *wdiDelBssRsp, 
                                                         void* pUserData)
{
   tWDA_ReqParams *pWdaParams = (tWDA_ReqParams *)pUserData; 
   tWDA_CbContext *pWDA; 
   tDeleteBssParams *delBssReqParam;
   tANI_U8  staIdx,tid;
   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                                          "<------ %s " ,__func__);
   if(NULL == pWdaParams)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
            "%s: pWdaParams received NULL", __func__);
      VOS_ASSERT(0) ;
      return ;
   }
   pWDA = (tWDA_CbContext *)pWdaParams->pWdaContext;
   delBssReqParam = (tDeleteBssParams *)pWdaParams->wdaMsgParam;
   delBssReqParam->status = wdiDelBssRsp->wdiStatus ;
   if(WDI_STATUS_SUCCESS == wdiDelBssRsp->wdiStatus)
   {
      vos_mem_copy(delBssReqParam->bssid, wdiDelBssRsp->macBSSID, 
                                             sizeof(tSirMacAddr)) ;
   }
   if(WDI_DS_GetStaIdxFromBssIdx(pWDA->pWdiContext, delBssReqParam->bssIdx, &staIdx))
   {
     VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                 "%s: Get STA index from BSS index Fail", __func__);
     VOS_ASSERT(0) ;
   }
   if(WDI_DS_DelSTAMemPool(pWDA->pWdiContext, staIdx))
   {
     VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                 "%s: DEL STA from MemPool Fail", __func__);
     VOS_ASSERT(0) ;
   }
   if(WDI_DS_ClearStaIdxPerBssIdx(pWDA->pWdiContext, delBssReqParam->bssIdx, staIdx))
   {
     VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                 "%s: Clear STA index form table Fail", __func__);
     VOS_ASSERT(0) ;
   }

   WLANTL_StartForwarding(staIdx,0,0);

   vos_mem_free(pWdaParams->wdaWdiApiMsgParam);
   vos_mem_free(pWdaParams) ;
   /*                          */
   pWDA->wdaGlobalSystemRole = eSYSTEM_UNKNOWN_ROLE;
   
   /*                                  */
   for(staIdx=0; staIdx < WDA_MAX_STA; staIdx++)
   {
      if( pWDA->wdaStaInfo[staIdx].bssIdx == wdiDelBssRsp->ucBssIdx )
      {
         pWDA->wdaStaInfo[staIdx].ucValidStaIndex = WDA_INVALID_STA_INDEX;
         pWDA->wdaStaInfo[staIdx].ucUseBaBitmap = 0;
         pWDA->wdaStaInfo[staIdx].currentOperChan = 0;
         /*                                */
         for(tid = 0; tid < STACFG_MAX_TC; tid++)
         {
            pWDA->wdaStaInfo[staIdx].framesTxed[tid] = 0;
         }
      }
   }
   WDA_SendMsg(pWDA, WDA_DELETE_BSS_RSP, (void *)delBssReqParam , 0) ;
   return ;
}

/*
                                 
                            
 */ 
VOS_STATUS WDA_ProcessDelBssReq(tWDA_CbContext *pWDA, 
                                        tDeleteBssParams *delBssParam)
{
   WDI_Status status = WDI_STATUS_SUCCESS ;
   WDI_DelBSSReqParamsType *wdiDelBssReqParam = 
                             (WDI_DelBSSReqParamsType *)vos_mem_malloc(
                                   sizeof(WDI_DelBSSReqParamsType)) ;
   tWDA_ReqParams *pWdaParams ;
   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                                          "------> %s " ,__func__);
   if(NULL == wdiDelBssReqParam) 
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                           "%s: VOS MEM Alloc Failure", __func__); 
      VOS_ASSERT(0);
      return VOS_STATUS_E_NOMEM;
   }
   pWdaParams = (tWDA_ReqParams *)vos_mem_malloc(sizeof(tWDA_ReqParams)) ;
   if(NULL == pWdaParams)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                           "%s: VOS MEM Alloc Failure", __func__); 
      VOS_ASSERT(0);
      vos_mem_free(wdiDelBssReqParam);
      return VOS_STATUS_E_NOMEM;
   }
   wdiDelBssReqParam->ucBssIdx = delBssParam->bssIdx;
   wdiDelBssReqParam->wdiReqStatusCB = NULL ;
   
   /*                                                           */
   /*                             */
   pWdaParams->pWdaContext = pWDA;
   pWdaParams->wdaMsgParam = delBssParam;
   pWdaParams->wdaWdiApiMsgParam = wdiDelBssReqParam;
   status = WDI_DelBSSReq(wdiDelBssReqParam, 
                        (WDI_DelBSSRspCb )WDA_DelBSSReqCallback, pWdaParams) ;
   if(IS_WDI_STATUS_FAILURE(status))
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                   "Failure in Del BSS WDI API, free all the memory " );
      vos_mem_free(pWdaParams->wdaWdiApiMsgParam);
      vos_mem_free(pWdaParams) ;
      delBssParam->status = eSIR_FAILURE ;
      WDA_SendMsg(pWDA, WDA_DELETE_BSS_RSP, (void *)delBssParam, 0) ;
   }
   return CONVERT_WDI2VOS_STATUS(status) ;
}
/*
                                  
                              
 */ 
void WDA_DelSTAReqCallback(WDI_DelSTARspParamsType *wdiDelStaRsp, 
                                                         void* pUserData)
{
   tWDA_ReqParams *pWdaParams = (tWDA_ReqParams *)pUserData; 
   tWDA_CbContext *pWDA; 
   tDeleteStaParams *delStaReqParam;
   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                                          "<------ %s " ,__func__);
   if(NULL == pWdaParams)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
               "%s: pWdaParams received NULL", __func__);
      VOS_ASSERT(0) ;
      return ;
   }
   pWDA = (tWDA_CbContext *)pWdaParams->pWdaContext;
   delStaReqParam = (tDeleteStaParams *)pWdaParams->wdaMsgParam;
   delStaReqParam->status = wdiDelStaRsp->wdiStatus ;
   if(WDI_STATUS_SUCCESS == wdiDelStaRsp->wdiStatus)
   {
      if(WDI_DS_DelSTAMemPool(pWDA->pWdiContext, wdiDelStaRsp->ucSTAIdx))
      {
         VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                    "%s: DEL STA from MemPool Fail", __func__);
         VOS_ASSERT(0) ;
      }
      delStaReqParam->staIdx = wdiDelStaRsp->ucSTAIdx ;
      WLANTL_StartForwarding(delStaReqParam->staIdx,0,0);
   }
   vos_mem_free(pWdaParams->wdaWdiApiMsgParam);
   vos_mem_free(pWdaParams) ;
   /*                                                      */
   pWDA->wdaStaInfo[wdiDelStaRsp->ucSTAIdx].ucValidStaIndex = 
                                                      WDA_INVALID_STA_INDEX;
   pWDA->wdaStaInfo[wdiDelStaRsp->ucSTAIdx].ucUseBaBitmap = 0;
   pWDA->wdaStaInfo[wdiDelStaRsp->ucSTAIdx].currentOperChan = 0;
   WDA_SendMsg(pWDA, WDA_DELETE_STA_RSP, (void *)delStaReqParam , 0) ;
   return ;
}
/*
                                 
                            
 */ 
VOS_STATUS WDA_ProcessDelStaReq(tWDA_CbContext *pWDA, 
                                      tDeleteStaParams *delStaParam)
{
   WDI_Status status = WDI_STATUS_SUCCESS ;
   WDI_DelSTAReqParamsType *wdiDelStaReqParam = 
                             (WDI_DelSTAReqParamsType *)vos_mem_malloc(
                                   sizeof(WDI_DelSTAReqParamsType)) ;
   tWDA_ReqParams *pWdaParams ;
   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                                          "------> %s " ,__func__);
   if(NULL == wdiDelStaReqParam) 
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                           "%s: VOS MEM Alloc Failure", __func__); 
      VOS_ASSERT(0);
      return VOS_STATUS_E_NOMEM;
   }
   pWdaParams = (tWDA_ReqParams *)vos_mem_malloc(sizeof(tWDA_ReqParams)) ;
   if(NULL == pWdaParams)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                           "%s: VOS MEM Alloc Failure", __func__); 
      VOS_ASSERT(0);
      vos_mem_free(wdiDelStaReqParam);
      return VOS_STATUS_E_NOMEM;
   }
   wdiDelStaReqParam->ucSTAIdx = delStaParam->staIdx ;
   wdiDelStaReqParam->wdiReqStatusCB = NULL ;
   /*                                                           */
   /*                             */
   pWdaParams->pWdaContext = pWDA;
   pWdaParams->wdaMsgParam = delStaParam;
   pWdaParams->wdaWdiApiMsgParam = wdiDelStaReqParam;
   status = WDI_DelSTAReq(wdiDelStaReqParam, 
                        (WDI_DelSTARspCb )WDA_DelSTAReqCallback, pWdaParams) ;
   if(IS_WDI_STATUS_FAILURE(status))
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
               "Failure in Del STA WDI API, free all the memory status = %d", 
                                                                status );
      vos_mem_free(pWdaParams->wdaWdiApiMsgParam);
      vos_mem_free(pWdaParams) ;
      delStaParam->status = eSIR_FAILURE ;
      WDA_SendMsg(pWDA, WDA_DELETE_STA_RSP, (void *)delStaParam, 0) ;
   }
   return CONVERT_WDI2VOS_STATUS(status) ;
}
void WDA_ProcessAddStaSelfRsp(WDI_AddSTASelfRspParamsType* pwdiAddSTASelfRsp, void* pUserData)
{
   tWDA_ReqParams *pWdaParams = (tWDA_ReqParams *)pUserData; 
   tWDA_CbContext *pWDA; 
   tAddStaSelfParams *pAddStaSelfRsp = NULL;
   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                                          "<------ %s " ,__func__);
   if(NULL == pWdaParams)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
              "%s: pWdaParams received NULL", __func__);
      VOS_ASSERT(0) ;
      return ;
   }
   pWDA = (tWDA_CbContext *)pWdaParams->pWdaContext;
   pAddStaSelfRsp = (tAddStaSelfParams*)pWdaParams->wdaMsgParam;
   vos_mem_free(pWdaParams->wdaWdiApiMsgParam);
   vos_mem_free(pWdaParams);
   pAddStaSelfRsp->status = CONVERT_WDI2SIR_STATUS(pwdiAddSTASelfRsp->wdiStatus);
   vos_mem_copy(pAddStaSelfRsp->selfMacAddr, 
                pwdiAddSTASelfRsp->macSelfSta, 
                sizeof(pAddStaSelfRsp->selfMacAddr));
   pWDA->wdaAddSelfStaParams.ucSTASelfIdx = pwdiAddSTASelfRsp->ucSTASelfIdx;
   pWDA->wdaAddSelfStaParams.wdiAddStaSelfStaRspCounter++;
   if (pAddStaSelfRsp->status == eSIR_FAILURE)
   {
       pWDA->wdaAddSelfStaParams.wdaAddSelfStaFailReason = WDA_ADDSTA_RSP_WDI_FAIL;
       pWDA->wdaAddSelfStaParams.wdiAddStaSelfStaFailCounter++;
   }
   WDA_SendMsg( pWDA, WDA_ADD_STA_SELF_RSP, (void *)pAddStaSelfRsp, 0) ;
   return ;
}

/*
                                     
   
 */ 
VOS_STATUS WDA_ProcessAddStaSelfReq( tWDA_CbContext *pWDA, tpAddStaSelfParams pAddStaSelfReq)
{
   VOS_STATUS status = VOS_STATUS_SUCCESS;
   WDI_Status wstatus;
   WDI_AddSTASelfReqParamsType *wdiAddStaSelfReq = 
      (WDI_AddSTASelfReqParamsType *)vos_mem_malloc(
         sizeof(WDI_AddSTASelfReqParamsType)) ;
   tWDA_ReqParams *pWdaParams; 
   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                                          "------> %s " ,__func__);
   pWDA->wdaAddSelfStaParams.wdiAddStaSelfStaReqCounter++;
   if( NULL == wdiAddStaSelfReq )
   {
      VOS_ASSERT( 0 );
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                                          "%s: Unable to allocate memory " ,__func__);
      pWDA->wdaAddSelfStaParams.wdaAddSelfStaFailReason = WDA_ADDSTA_REQ_NO_MEM;
      pWDA->wdaAddSelfStaParams.wdiAddStaSelfStaFailCounter++;
      return( VOS_STATUS_E_NOMEM );
   }
   pWdaParams = (tWDA_ReqParams *)vos_mem_malloc(sizeof(tWDA_ReqParams)) ;
   if( NULL == pWdaParams )
   {
      VOS_ASSERT( 0 );
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                                          "%s: Unable to allocate memory " ,__func__);
      pWDA->wdaAddSelfStaParams.wdaAddSelfStaFailReason = WDA_ADDSTA_REQ_NO_MEM;
      pWDA->wdaAddSelfStaParams.wdiAddStaSelfStaFailCounter++;
      vos_mem_free(wdiAddStaSelfReq) ;
      return( VOS_STATUS_E_NOMEM );
   }
   wdiAddStaSelfReq->wdiReqStatusCB = NULL;
   vos_mem_copy( wdiAddStaSelfReq->wdiAddSTASelfInfo.selfMacAddr, pAddStaSelfReq->selfMacAddr, 6);
   wdiAddStaSelfReq->wdiAddSTASelfInfo.currDeviceMode = pAddStaSelfReq->currDeviceMode;
   /*                                                           */
   /*                             */
   pWdaParams->pWdaContext = pWDA;
   pWdaParams->wdaMsgParam = pAddStaSelfReq;
   pWdaParams->wdaWdiApiMsgParam = wdiAddStaSelfReq; 
   wstatus = WDI_AddSTASelfReq( wdiAddStaSelfReq, WDA_ProcessAddStaSelfRsp, pWdaParams);

   if(IS_WDI_STATUS_FAILURE(wstatus))
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
               "Failure in Add Self Sta Request API, free all the memory status = %d", 
                                                                wstatus );
      status = CONVERT_WDI2VOS_STATUS(wstatus);
      vos_mem_free(pWdaParams->wdaWdiApiMsgParam);
      vos_mem_free(pWdaParams) ;
      pWDA->wdaAddSelfStaParams.wdaAddSelfStaFailReason = WDA_ADDSTA_REQ_WDI_FAIL;
      pWDA->wdaAddSelfStaParams.wdiAddStaSelfStaFailCounter++;
      pAddStaSelfReq->status = eSIR_FAILURE ;
      WDA_SendMsg( pWDA, WDA_ADD_STA_SELF_RSP, (void *)pAddStaSelfReq, 0) ;
   }
   return status;
}
/*
                                       
   
 */ 
void WDA_DelSTASelfRespCallback(WDI_DelSTASelfRspParamsType *
                                      wdiDelStaSelfRspParams , void* pUserData)
{
   tWDA_ReqParams        *pWdaParams = (tWDA_ReqParams *)pUserData; 
   tWDA_CbContext        *pWDA; 
   tDelStaSelfParams     *delStaSelfParams;
   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                                          "<------ %s " ,__func__);
   if (NULL == pWdaParams)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
              "%s: Invalid pWdaParams pointer", __func__);
      VOS_ASSERT(0);
      return;
   }
   pWDA = (tWDA_CbContext *)pWdaParams->pWdaContext;
   delStaSelfParams = (tDelStaSelfParams *)pWdaParams->wdaMsgParam;
   delStaSelfParams->status = 
               wdiDelStaSelfRspParams->wdiStatus ;
   
   vos_mem_free(pWdaParams->wdaWdiApiMsgParam);
   vos_mem_free(pWdaParams) ;
   
   WDA_SendMsg(pWDA, WDA_DEL_STA_SELF_RSP, (void *)delStaSelfParams , 0) ;
   return ;
}
/*
                                      
   
 */
void WDA_DelSTASelfReqCallback(WDI_Status   wdiStatus,
                                void*        pUserData)
{
   tWDA_ReqParams        *pWdaParams = (tWDA_ReqParams *)pUserData; 
   tWDA_CbContext        *pWDA; 
   tDelStaSelfParams     *delStaSelfParams;
   
   VOS_TRACE(VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
             "<------ %s, wdiStatus: %d pWdaParams: %p",
              __func__, wdiStatus, pWdaParams); 

   if (NULL == pWdaParams)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
              "%s: Invalid pWdaParams pointer", __func__);
      VOS_ASSERT(0);
      return;
   }

   pWDA = (tWDA_CbContext *)pWdaParams->pWdaContext;
   delStaSelfParams = (tDelStaSelfParams *)pWdaParams->wdaMsgParam;

   delStaSelfParams->status = wdiStatus ;

   if(IS_WDI_STATUS_FAILURE(wdiStatus))
   {
         VOS_ASSERT(0);
         vos_mem_free(pWdaParams->wdaWdiApiMsgParam);
         vos_mem_free(pWdaParams) ;
         WDA_SendMsg(pWDA, WDA_DEL_STA_SELF_RSP, (void *)delStaSelfParams , 0) ;
   }

   return ;
}

/*
                              
                                       
 */ 
VOS_STATUS WDA_ProcessDelSTASelfReq(tWDA_CbContext *pWDA, 
                                    tDelStaSelfParams* pDelStaSelfReqParam)
{
   VOS_STATUS status = VOS_STATUS_SUCCESS;
   WDI_Status wstatus;
   tWDA_ReqParams *pWdaParams = NULL; 
   WDI_DelSTASelfReqParamsType *wdiDelStaSelfReq = 
                (WDI_DelSTASelfReqParamsType *)vos_mem_malloc(
                              sizeof(WDI_DelSTASelfReqParamsType)) ;

   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                                          "------> %s " ,__func__);
   if( NULL == wdiDelStaSelfReq )
   {
      VOS_ASSERT( 0 );
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                                          "%s: Unable to allocate memory " ,__func__);
      return( VOS_STATUS_E_NOMEM );
   }
   
   pWdaParams = (tWDA_ReqParams *)vos_mem_malloc(sizeof(tWDA_ReqParams)) ;
   if( NULL == pWdaParams )
   {
      VOS_ASSERT( 0 );
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                               "%s: Unable to allocate memory " ,__func__);
      vos_mem_free(wdiDelStaSelfReq) ;
      return( VOS_STATUS_E_NOMEM );
   }
   pWdaParams->pWdaContext = pWDA;
   /*                                            */
   pWdaParams->wdaMsgParam = pDelStaSelfReqParam;
   /*                             */
   pWdaParams->wdaWdiApiMsgParam = (void *)wdiDelStaSelfReq;
   vos_mem_copy( wdiDelStaSelfReq->wdiDelStaSelfInfo.selfMacAddr, 
                 pDelStaSelfReqParam->selfMacAddr, sizeof(tSirMacAddr));
   
    wdiDelStaSelfReq->wdiReqStatusCB = WDA_DelSTASelfReqCallback;
    wdiDelStaSelfReq->pUserData = pWdaParams;

   wstatus = WDI_DelSTASelfReq(wdiDelStaSelfReq, 
                      (WDI_DelSTASelfRspCb)WDA_DelSTASelfRespCallback, pWdaParams);

   if(IS_WDI_STATUS_FAILURE(wstatus))
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_FATAL,
              "Failure in Del Sta Self REQ WDI API, free all the memory " );
      VOS_ASSERT(0);
      status = CONVERT_WDI2VOS_STATUS(wstatus);
      vos_mem_free(pWdaParams->wdaWdiApiMsgParam);
      vos_mem_free(pWdaParams) ;
      pDelStaSelfReqParam->status = eSIR_FAILURE ;
      WDA_SendMsg(pWDA, WDA_DEL_STA_SELF_RSP, (void *)pDelStaSelfReqParam, 0) ;
   }
   return status;
}

/*
                        
                          
 */ 
void WDA_SendMsg(tWDA_CbContext *pWDA, tANI_U16 msgType, 
                                        void *pBodyptr, tANI_U32 bodyVal)
{
   tSirMsgQ msg = {0} ;
   tANI_U32 status = VOS_STATUS_SUCCESS ;
   tpAniSirGlobal pMac = (tpAniSirGlobal )VOS_GET_MAC_CTXT(pWDA->pVosContext);
   msg.type        = msgType;
   msg.bodyval     = bodyVal;
   msg.bodyptr     = pBodyptr;
   status = limPostMsgApi(pMac, &msg);
   if (VOS_STATUS_SUCCESS != status)
   {
      if(NULL != pBodyptr)
      {
         vos_mem_free(pBodyptr);
      }
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                      "%s: limPostMsgApi is failed " ,__func__);
      VOS_ASSERT(0) ;
   }
   return ;
}
/*
                                
                                                 
 */
void WDA_UpdateBSSParams(tWDA_CbContext *pWDA, 
                         WDI_ConfigBSSReqInfoType *wdiBssParams, 
                         tAddBssParams *wdaBssParams)
{
   v_U8_t keyIndex = 0;
   /*                                     */
   vos_mem_copy(wdiBssParams->macBSSID,
                           wdaBssParams->bssId, sizeof(tSirMacAddr)) ;
   vos_mem_copy(wdiBssParams->macSelfAddr, wdaBssParams->selfMacAddr,
                                                   sizeof(tSirMacAddr)) ;
   wdiBssParams->wdiBSSType = wdaBssParams->bssType ;
   wdiBssParams->ucOperMode = wdaBssParams->operMode ;
   wdiBssParams->wdiNWType   = wdaBssParams->nwType ;
   wdiBssParams->ucShortSlotTimeSupported = 
                                  wdaBssParams->shortSlotTimeSupported ;
   wdiBssParams->ucllaCoexist  = wdaBssParams->llaCoexist ;
   wdiBssParams->ucllbCoexist  = wdaBssParams->llbCoexist ;
   wdiBssParams->ucllgCoexist  = wdaBssParams->llgCoexist ;
   wdiBssParams->ucHT20Coexist = wdaBssParams->ht20Coexist ;
   wdiBssParams->ucObssProtEnabled = wdaBssParams->obssProtEnabled ;

   wdiBssParams->ucllnNonGFCoexist = wdaBssParams->llnNonGFCoexist ;
   wdiBssParams->ucTXOPProtectionFullSupport =
                           wdaBssParams->fLsigTXOPProtectionFullSupport ;
   wdiBssParams->ucRIFSMode = wdaBssParams->fRIFSMode ;
   wdiBssParams->usBeaconInterval = wdaBssParams->beaconInterval ;
   wdiBssParams->ucDTIMPeriod = wdaBssParams->dtimPeriod ;
   wdiBssParams->ucTXChannelWidthSet = wdaBssParams->txChannelWidthSet ;
   wdiBssParams->ucCurrentOperChannel = wdaBssParams->currentOperChannel ;
   wdiBssParams->ucCurrentExtChannel = wdaBssParams->currentExtChannel ;
   wdiBssParams->bHiddenSSIDEn = wdaBssParams->bHiddenSSIDEn ;

   wdiBssParams->ucRMFEnabled = wdaBssParams->rmfEnabled;

   /*                              */
   wdiBssParams->wdiSSID.ucLength = wdaBssParams->ssId.length ;
   vos_mem_copy(wdiBssParams->wdiSSID.sSSID,
                 wdaBssParams->ssId.ssId, wdaBssParams->ssId.length) ;
   WDA_UpdateSTAParams(pWDA, &wdiBssParams->wdiSTAContext, 
                       &wdaBssParams->staContext) ;
   wdiBssParams->wdiAction = wdaBssParams->updateBss;
#ifdef WLAN_FEATURE_VOWIFI
   wdiBssParams->cMaxTxPower = wdaBssParams->maxTxPower;
#endif
   wdiBssParams->ucPersona = wdaBssParams->halPersona;
   wdiBssParams->bSpectrumMgtEn = wdaBssParams->bSpectrumMgtEnabled;
#ifdef WLAN_FEATURE_VOWIFI_11R
   wdiBssParams->bExtSetStaKeyParamValid = wdaBssParams->extSetStaKeyParamValid;
   if(wdiBssParams->bExtSetStaKeyParamValid)
   {
      /*                                          */
      wdiBssParams->wdiExtSetKeyParam.ucSTAIdx = 
         wdaBssParams->extSetStaKeyParam.staIdx;
      wdiBssParams->wdiExtSetKeyParam.wdiEncType = 
         wdaBssParams->extSetStaKeyParam.encType;
      wdiBssParams->wdiExtSetKeyParam.wdiWEPType = 
         wdaBssParams->extSetStaKeyParam.wepType;
      wdiBssParams->wdiExtSetKeyParam.ucDefWEPIdx = 
         wdaBssParams->extSetStaKeyParam.defWEPIdx;
      if(wdaBssParams->extSetStaKeyParam.encType != eSIR_ED_NONE)
      {
         if( (wdiBssParams->wdiExtSetKeyParam.wdiWEPType == WDI_WEP_STATIC) && 
             (WDA_INVALID_KEY_INDEX == wdaBssParams->extSetStaKeyParam.defWEPIdx) &&
             (eSYSTEM_AP_ROLE != pWDA->wdaGlobalSystemRole))
         {
            WDA_GetWepKeysFromCfg( pWDA, 
                                   &wdiBssParams->wdiExtSetKeyParam.ucDefWEPIdx, 
                                   &wdiBssParams->wdiExtSetKeyParam.ucNumKeys,
                                   wdiBssParams->wdiExtSetKeyParam.wdiKey );
         }
         else
         {
            for( keyIndex=0; keyIndex < SIR_MAC_MAX_NUM_OF_DEFAULT_KEYS; 
                                                                  keyIndex++)
            {
               wdiBssParams->wdiExtSetKeyParam.wdiKey[keyIndex].keyId =
                  wdaBssParams->extSetStaKeyParam.key[keyIndex].keyId;
               wdiBssParams->wdiExtSetKeyParam.wdiKey[keyIndex].unicast =
                  wdaBssParams->extSetStaKeyParam.key[keyIndex].unicast;
               wdiBssParams->wdiExtSetKeyParam.wdiKey[keyIndex].keyDirection =
                  wdaBssParams->extSetStaKeyParam.key[keyIndex].keyDirection;
               vos_mem_copy(wdiBssParams->wdiExtSetKeyParam.wdiKey[keyIndex].keyRsc, 
                            wdaBssParams->extSetStaKeyParam.key[keyIndex].keyRsc, WLAN_MAX_KEY_RSC_LEN);
               wdiBssParams->wdiExtSetKeyParam.wdiKey[keyIndex].paeRole =
                  wdaBssParams->extSetStaKeyParam.key[keyIndex].paeRole;
               wdiBssParams->wdiExtSetKeyParam.wdiKey[keyIndex].keyLength =
                  wdaBssParams->extSetStaKeyParam.key[keyIndex].keyLength;
               vos_mem_copy(wdiBssParams->wdiExtSetKeyParam.wdiKey[keyIndex].key, 
                            wdaBssParams->extSetStaKeyParam.key[keyIndex].key, SIR_MAC_MAX_KEY_LENGTH);
            }
            wdiBssParams->wdiExtSetKeyParam.ucNumKeys = 
               SIR_MAC_MAX_NUM_OF_DEFAULT_KEYS;
         }
      }
      wdiBssParams->wdiExtSetKeyParam.ucSingleTidRc = wdaBssParams->extSetStaKeyParam.singleTidRc;
   }
   else /*                                                    */
   {
      vos_mem_zero( &wdaBssParams->extSetStaKeyParam, 
                    sizeof(wdaBssParams->extSetStaKeyParam) );
   }
#endif /*                       */
#ifdef WLAN_FEATURE_11AC
   wdiBssParams->ucVhtCapableSta = wdaBssParams->vhtCapable;
   wdiBssParams->ucVhtTxChannelWidthSet = wdaBssParams->vhtTxChannelWidthSet;
#endif

   return ;
}
/*
                                
                                                 
 */
void WDA_UpdateSTAParams(tWDA_CbContext *pWDA, 
                               WDI_ConfigStaReqInfoType *wdiStaParams, 
                                                tAddStaParams *wdaStaParams)
{
   tANI_U8 i = 0;
   /*                   */
   vos_mem_copy(wdiStaParams->macBSSID, wdaStaParams->bssId, 
                                            sizeof(tSirMacAddr)) ;
   wdiStaParams->usAssocId = wdaStaParams->assocId;
   wdiStaParams->wdiSTAType = wdaStaParams->staType;
   wdiStaParams->staIdx = wdaStaParams->staIdx;

   wdiStaParams->ucShortPreambleSupported = 
                                        wdaStaParams->shortPreambleSupported;
   vos_mem_copy(wdiStaParams->macSTA, wdaStaParams->staMac, 
                                               sizeof(tSirMacAddr)) ;
   wdiStaParams->usListenInterval = wdaStaParams->listenInterval;
   
   wdiStaParams->ucWMMEnabled = wdaStaParams->wmmEnabled;
   
   wdiStaParams->ucHTCapable = wdaStaParams->htCapable;
   wdiStaParams->ucTXChannelWidthSet = wdaStaParams->txChannelWidthSet;
   wdiStaParams->ucRIFSMode = wdaStaParams->rifsMode;
   wdiStaParams->ucLSIGTxopProtection = wdaStaParams->lsigTxopProtection;
   wdiStaParams->ucMaxAmpduSize = wdaStaParams->maxAmpduSize;
   wdiStaParams->ucMaxAmpduDensity = wdaStaParams->maxAmpduDensity;
   wdiStaParams->ucMaxAmsduSize = wdaStaParams->maxAmsduSize;
   
   wdiStaParams->ucShortGI40Mhz = wdaStaParams->fShortGI40Mhz;
   wdiStaParams->ucShortGI20Mhz = wdaStaParams->fShortGI20Mhz;
   wdiStaParams->wdiSupportedRates.opRateMode = 
                                wdaStaParams->supportedRates.opRateMode;
   for(i = 0;i < WDI_NUM_11B_RATES;i++)
   {
     wdiStaParams->wdiSupportedRates.llbRates[i] = 
                               wdaStaParams->supportedRates.llbRates[i];
   }
   for(i = 0;i < WDI_NUM_11A_RATES;i++)
   {
     wdiStaParams->wdiSupportedRates.llaRates[i] = 
                               wdaStaParams->supportedRates.llaRates[i];
   }
   for(i = 0;i < SIR_NUM_POLARIS_RATES;i++)
   {
     wdiStaParams->wdiSupportedRates.aLegacyRates[i] = 
                               wdaStaParams->supportedRates.aniLegacyRates[i];
   }
   wdiStaParams->wdiSupportedRates.uEnhancedRateBitmap = 
                            wdaStaParams->supportedRates.aniEnhancedRateBitmap;
#ifdef WLAN_FEATURE_11AC
   wdiStaParams->wdiSupportedRates.vhtRxMCSMap = wdaStaParams->supportedRates.vhtRxMCSMap;
   wdiStaParams->wdiSupportedRates.vhtRxHighestDataRate = wdaStaParams->supportedRates.vhtRxHighestDataRate;
   wdiStaParams->wdiSupportedRates.vhtTxMCSMap = wdaStaParams->supportedRates.vhtTxMCSMap;
   wdiStaParams->wdiSupportedRates.vhtTxHighestDataRate = wdaStaParams->supportedRates.vhtTxHighestDataRate;
#endif
   for(i = 0;i <SIR_MAC_MAX_SUPPORTED_MCS_SET;i++)
   {
     wdiStaParams->wdiSupportedRates.aSupportedMCSSet[i] = 
                               wdaStaParams->supportedRates.supportedMCSSet[i];
   }
   wdiStaParams->wdiSupportedRates.aRxHighestDataRate = 
                           wdaStaParams->supportedRates.rxHighestDataRate;
   
   wdiStaParams->ucRMFEnabled = wdaStaParams->rmfEnabled;
   
   wdiStaParams->wdiAction = wdaStaParams->updateSta; 
   
   wdiStaParams->ucAPSD = wdaStaParams->uAPSD;
   wdiStaParams->ucMaxSPLen = wdaStaParams->maxSPLen;
   wdiStaParams->ucGreenFieldCapable = wdaStaParams->greenFieldCapable;
   
   wdiStaParams->ucDelayedBASupport = wdaStaParams->delBASupport;
   wdiStaParams->us32MaxAmpduDuratio = wdaStaParams->us32MaxAmpduDuration;
   wdiStaParams->ucDsssCckMode40Mhz = wdaStaParams->fDsssCckMode40Mhz;
   wdiStaParams->ucEncryptType = wdaStaParams->encryptType;
   wdiStaParams->ucP2pCapableSta = wdaStaParams->p2pCapableSta;
#ifdef WLAN_FEATURE_11AC
   wdiStaParams->ucVhtCapableSta = wdaStaParams->vhtCapable;
   wdiStaParams->ucVhtTxChannelWidthSet = wdaStaParams->vhtTxChannelWidthSet;
   wdiStaParams->ucVhtTxBFEnabled = wdaStaParams->vhtTxBFCapable;
   wdiStaParams->vhtTxMUBformeeCapable = wdaStaParams->vhtTxMUBformeeCapable;
   /*                                                          
                             */
   if ( wdiStaParams->vhtTxMUBformeeCapable )
       wdiStaParams->ucVhtTxBFEnabled = wdaStaParams->vhtTxMUBformeeCapable;
#endif
   wdiStaParams->ucHtLdpcEnabled= wdaStaParams->htLdpcCapable;
   wdiStaParams->ucVhtLdpcEnabled = wdaStaParams->vhtLdpcCapable;
   return ;
}
/*
                                                                            
                    
                                                                             
 */
 
 /*
                                          
                                       
 */ 
static inline v_U8_t WDA_ConvertWniCfgIdToHALCfgId(v_U32_t wniCfgId)
{
   switch(wniCfgId)
   {
      case WNI_CFG_STA_ID:
         return QWLAN_HAL_CFG_STA_ID;
      case WNI_CFG_CURRENT_TX_ANTENNA:
         return QWLAN_HAL_CFG_CURRENT_TX_ANTENNA;
      case WNI_CFG_CURRENT_RX_ANTENNA:
         return QWLAN_HAL_CFG_CURRENT_RX_ANTENNA;
      case WNI_CFG_LOW_GAIN_OVERRIDE:
         return QWLAN_HAL_CFG_LOW_GAIN_OVERRIDE;
      case WNI_CFG_POWER_STATE_PER_CHAIN:
         return QWLAN_HAL_CFG_POWER_STATE_PER_CHAIN;
      case WNI_CFG_CAL_PERIOD:
         return QWLAN_HAL_CFG_CAL_PERIOD;
      case WNI_CFG_CAL_CONTROL:
         return QWLAN_HAL_CFG_CAL_CONTROL;
      case WNI_CFG_PROXIMITY:
         return QWLAN_HAL_CFG_PROXIMITY;
      case WNI_CFG_NETWORK_DENSITY:
         return QWLAN_HAL_CFG_NETWORK_DENSITY;
      case WNI_CFG_MAX_MEDIUM_TIME:
         return QWLAN_HAL_CFG_MAX_MEDIUM_TIME;
      case WNI_CFG_MAX_MPDUS_IN_AMPDU:
         return QWLAN_HAL_CFG_MAX_MPDUS_IN_AMPDU;
      case WNI_CFG_RTS_THRESHOLD:
         return QWLAN_HAL_CFG_RTS_THRESHOLD;
      case WNI_CFG_SHORT_RETRY_LIMIT:
         return QWLAN_HAL_CFG_SHORT_RETRY_LIMIT;
      case WNI_CFG_LONG_RETRY_LIMIT:
         return QWLAN_HAL_CFG_LONG_RETRY_LIMIT;
      case WNI_CFG_FRAGMENTATION_THRESHOLD:
         return QWLAN_HAL_CFG_FRAGMENTATION_THRESHOLD;
      case WNI_CFG_DYNAMIC_THRESHOLD_ZERO:
         return QWLAN_HAL_CFG_DYNAMIC_THRESHOLD_ZERO;
      case WNI_CFG_DYNAMIC_THRESHOLD_ONE:
         return QWLAN_HAL_CFG_DYNAMIC_THRESHOLD_ONE;
      case WNI_CFG_DYNAMIC_THRESHOLD_TWO:
         return QWLAN_HAL_CFG_DYNAMIC_THRESHOLD_TWO;
      case WNI_CFG_FIXED_RATE:
         return QWLAN_HAL_CFG_FIXED_RATE;
      case WNI_CFG_RETRYRATE_POLICY:
         return QWLAN_HAL_CFG_RETRYRATE_POLICY;
      case WNI_CFG_RETRYRATE_SECONDARY:
         return QWLAN_HAL_CFG_RETRYRATE_SECONDARY;
      case WNI_CFG_RETRYRATE_TERTIARY:
         return QWLAN_HAL_CFG_RETRYRATE_TERTIARY;
      case WNI_CFG_FORCE_POLICY_PROTECTION:
         return QWLAN_HAL_CFG_FORCE_POLICY_PROTECTION;
      case WNI_CFG_FIXED_RATE_MULTICAST_24GHZ:
         return QWLAN_HAL_CFG_FIXED_RATE_MULTICAST_24GHZ;
      case WNI_CFG_FIXED_RATE_MULTICAST_5GHZ:
         return QWLAN_HAL_CFG_FIXED_RATE_MULTICAST_5GHZ;
      case WNI_CFG_DEFAULT_RATE_INDEX_24GHZ:
         return QWLAN_HAL_CFG_DEFAULT_RATE_INDEX_24GHZ;
      case WNI_CFG_DEFAULT_RATE_INDEX_5GHZ:
         return QWLAN_HAL_CFG_DEFAULT_RATE_INDEX_5GHZ;
      case WNI_CFG_MAX_BA_SESSIONS:
         return QWLAN_HAL_CFG_MAX_BA_SESSIONS;
      case WNI_CFG_PS_DATA_INACTIVITY_TIMEOUT:
         return QWLAN_HAL_CFG_PS_DATA_INACTIVITY_TIMEOUT;
      case WNI_CFG_PS_ENABLE_BCN_FILTER:
         return QWLAN_HAL_CFG_PS_ENABLE_BCN_FILTER;
      case WNI_CFG_PS_ENABLE_RSSI_MONITOR:
         return QWLAN_HAL_CFG_PS_ENABLE_RSSI_MONITOR;
      case WNI_CFG_NUM_BEACON_PER_RSSI_AVERAGE:
         return QWLAN_HAL_CFG_NUM_BEACON_PER_RSSI_AVERAGE;
      case WNI_CFG_STATS_PERIOD:
         return QWLAN_HAL_CFG_STATS_PERIOD;
      case WNI_CFG_CFP_MAX_DURATION:
         return QWLAN_HAL_CFG_CFP_MAX_DURATION;
#if 0 /*                       */
      case WNI_CFG_FRAME_TRANS_ENABLED:
         return QWLAN_HAL_CFG_FRAME_TRANS_ENABLED;
#endif
      case WNI_CFG_DTIM_PERIOD:
         return QWLAN_HAL_CFG_DTIM_PERIOD;
      case WNI_CFG_EDCA_WME_ACBK:
         return QWLAN_HAL_CFG_EDCA_WMM_ACBK;
      case WNI_CFG_EDCA_WME_ACBE:
         return QWLAN_HAL_CFG_EDCA_WMM_ACBE;
      case WNI_CFG_EDCA_WME_ACVI:
         return QWLAN_HAL_CFG_EDCA_WMM_ACVI;
      case WNI_CFG_EDCA_WME_ACVO:
         return QWLAN_HAL_CFG_EDCA_WMM_ACVO;
#if 0
      case WNI_CFG_TELE_BCN_WAKEUP_EN:
         return QWLAN_HAL_CFG_TELE_BCN_WAKEUP_EN;
      case WNI_CFG_TELE_BCN_TRANS_LI:
         return QWLAN_HAL_CFG_TELE_BCN_TRANS_LI;
      case WNI_CFG_TELE_BCN_TRANS_LI_IDLE_BCNS:
         return QWLAN_HAL_CFG_TELE_BCN_TRANS_LI_IDLE_BCNS;
      case WNI_CFG_TELE_BCN_MAX_LI:
         return QWLAN_HAL_CFG_TELE_BCN_MAX_LI;
      case WNI_CFG_TELE_BCN_MAX_LI_IDLE_BCNS:
         return QWLAN_HAL_CFG_TELE_BCN_MAX_LI_IDLE_BCNS;
#endif
      case WNI_CFG_ENABLE_CLOSE_LOOP:
         return QWLAN_HAL_CFG_ENABLE_CLOSE_LOOP;
      case WNI_CFG_ENABLE_LPWR_IMG_TRANSITION:
         return QWLAN_HAL_CFG_ENABLE_LPWR_IMG_TRANSITION;
      default:
      {
         VOS_TRACE(VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
               "There is no HAL CFG Id corresponding to WNI CFG Id: %d",
                       wniCfgId);
         return VOS_STATUS_E_INVAL;
      }
   }
}
/*
                                  
   
 */ 
void WDA_UpdateCfgCallback(WDI_Status   wdiStatus, void* pUserData)
{
   tWDA_CbContext *pWDA = (tWDA_CbContext *)pUserData ; 
   WDI_UpdateCfgReqParamsType *wdiCfgParam = 
                  (WDI_UpdateCfgReqParamsType *)pWDA->wdaWdiCfgApiMsgParam ;
   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                                          "<------ %s " ,__func__);
   /*
                                                                       
                                                        
    */
   if(WDI_STATUS_SUCCESS != wdiStatus)
   {
      VOS_TRACE(VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                  "%s: CFG (%d) config failure", __func__,
              ((tHalCfg *)(wdiCfgParam->pConfigBuffer))->type);
   }
   
   vos_mem_free(wdiCfgParam->pConfigBuffer) ;
   vos_mem_free(pWDA->wdaWdiCfgApiMsgParam) ;
   pWDA->wdaWdiCfgApiMsgParam = NULL;
   return ;
}
/*
                          
   
 */ 
VOS_STATUS WDA_UpdateCfg(tWDA_CbContext *pWDA, tSirMsgQ *cfgParam)
{
   
   WDI_Status status = WDI_STATUS_SUCCESS ;
   tANI_U32 val =0;
   tpAniSirGlobal pMac = (tpAniSirGlobal )VOS_GET_MAC_CTXT(pWDA->pVosContext) ;
   tHalCfg *configData;
   WDI_UpdateCfgReqParamsType *wdiCfgReqParam = NULL ;
   tANI_U8        *configDataValue;
   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                                          "------> %s " ,__func__);
   if (NULL == pMac )
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                 "%s: Invoked with invalid MAC context ", __func__ );
      return VOS_STATUS_E_FAILURE;
   }
   if(WDA_START_STATE != pWDA->wdaState)
   {
      return VOS_STATUS_E_FAILURE;
   }
   
   if(NULL != pWDA->wdaWdiCfgApiMsgParam)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                           "%s:wdaWdiCfgApiMsgParam is not NULL", __func__); 
      VOS_ASSERT(0);
      return VOS_STATUS_E_FAILURE;
   }
   
   wdiCfgReqParam = (WDI_UpdateCfgReqParamsType *)vos_mem_malloc(
                                   sizeof(WDI_UpdateCfgReqParamsType)) ;
   if(NULL == wdiCfgReqParam) 
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                           "%s: VOS MEM Alloc Failure", __func__); 
      VOS_ASSERT(0);
      return VOS_STATUS_E_NOMEM;
   }
   wdiCfgReqParam->pConfigBuffer =  vos_mem_malloc(sizeof(tHalCfg) + 
                                                            sizeof(tANI_U32)) ;
   if(NULL == wdiCfgReqParam->pConfigBuffer)
   {
      VOS_TRACE(VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                             "%s: VOS MEM Alloc Failure", __func__);
      vos_mem_free(wdiCfgReqParam);
      VOS_ASSERT(0);
      return VOS_STATUS_E_NOMEM;
   }
   
   /*                                    */
   ((tHalCfg *)wdiCfgReqParam->pConfigBuffer)->type =
                             WDA_ConvertWniCfgIdToHALCfgId(cfgParam->bodyval);
   
   /*                                                  */
   if (wlan_cfgGetInt(pMac, (tANI_U16) cfgParam->bodyval, 
                                                      &val) != eSIR_SUCCESS)
   {
       VOS_TRACE(VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                              "Failed to cfg get id %d", cfgParam->bodyval);
       vos_mem_free(wdiCfgReqParam->pConfigBuffer);
       vos_mem_free(wdiCfgReqParam);
       return eSIR_FAILURE;
   }
   ((tHalCfg *)wdiCfgReqParam->pConfigBuffer)->length = sizeof(tANI_U32);
   configData =((tHalCfg *)wdiCfgReqParam->pConfigBuffer) ;
   configDataValue = ((tANI_U8 *)configData + sizeof(tHalCfg));
   vos_mem_copy( configDataValue, &val, sizeof(tANI_U32));
   wdiCfgReqParam->wdiReqStatusCB = NULL ;
   
   /*                             */
   pWDA->wdaWdiCfgApiMsgParam = (void *)wdiCfgReqParam ;
#ifdef FEATURE_HAL_SUPPORT_DYNAMIC_UPDATE_CFG
   status = WDI_UpdateCfgReq(wdiCfgReqParam, 
                   (WDI_UpdateCfgRspCb )WDA_UpdateCfgCallback, pWDA) ;
   if(IS_WDI_STATUS_FAILURE(status))
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                   "Failure in Update CFG WDI API, free all the memory " );
      vos_mem_free(wdiCfgReqParam->pConfigBuffer) ;
      vos_mem_free(pWDA->wdaWdiCfgApiMsgParam) ;
      pWDA->wdaWdiCfgApiMsgParam = NULL;
      /*                         */
      VOS_ASSERT(0) ;
   }
#else
   vos_mem_free(wdiCfgReqParam->pConfigBuffer) ;
   vos_mem_free(pWDA->wdaWdiCfgApiMsgParam) ;
   pWDA->wdaWdiCfgApiMsgParam = NULL;
#endif
   return CONVERT_WDI2VOS_STATUS(status) ;
}

VOS_STATUS WDA_GetWepKeysFromCfg( tWDA_CbContext *pWDA, 
                                                      v_U8_t *pDefaultKeyId,
                                                      v_U8_t *pNumKeys,
                                                      WDI_KeysType *pWdiKeys )
{
   v_U32_t i, j, defKeyId = 0;
   v_U32_t val = SIR_MAC_KEY_LENGTH;
   VOS_STATUS status = WDI_STATUS_SUCCESS;
   tpAniSirGlobal pMac = (tpAniSirGlobal )VOS_GET_MAC_CTXT(pWDA->pVosContext) ;
   if (NULL == pMac )
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                 "%s: Invoked with invalid MAC context ", __func__ );
      return VOS_STATUS_E_FAILURE;
   }
   if( eSIR_SUCCESS != wlan_cfgGetInt( pMac, WNI_CFG_WEP_DEFAULT_KEYID,
                                                                    &defKeyId ))
   {
        VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
            "Unable to retrieve defaultKeyId from CFG. Defaulting to 0...");
   }
   
  *pDefaultKeyId = (v_U8_t)defKeyId;
   /*                                                */
   for( i = 0, j = 0; i < SIR_MAC_MAX_NUM_OF_DEFAULT_KEYS; i++ )
   {
      val = SIR_MAC_KEY_LENGTH;
      if( eSIR_SUCCESS != wlan_cfgGetStr( pMac, 
                                     (v_U16_t) (WNI_CFG_WEP_DEFAULT_KEY_1 + i),
                                     pWdiKeys[j].key,
                                     &val ))
      {
         VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                            "WEP Key index [%d] may not configured in CFG",i);
      }
      else
      {
         pWdiKeys[j].keyId = (tANI_U8) i;
         /* 
                                              
                                                  
         */
         pWdiKeys[j].unicast = 0;
         /*
                                   
         */
         pWdiKeys[j].keyDirection = eSIR_TX_RX;
         /*                                         */
         pWdiKeys[j].paeRole = 0;
         /*                                        */
         pWdiKeys[j].keyLength = (tANI_U16) val;
         j++;
         *pNumKeys = (tANI_U8) j;
      }
   }
   return status;
}
/*
                                     
                                  
 */ 
void WDA_SetBssKeyReqCallback(WDI_Status status, void* pUserData)
{
   tWDA_ReqParams *pWdaParams = (tWDA_ReqParams *)pUserData; 
   tWDA_CbContext *pWDA; 
   tSetBssKeyParams *setBssKeyParams;
   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                                          "<------ %s " ,__func__);
   if(NULL == pWdaParams)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                "%s: pWdaParams received NULL", __func__);
      VOS_ASSERT(0) ;
      return ;
   }
   pWDA = (tWDA_CbContext *)pWdaParams->pWdaContext;
   setBssKeyParams = (tSetBssKeyParams *)pWdaParams->wdaMsgParam;
   vos_mem_zero(pWdaParams->wdaWdiApiMsgParam,
                 sizeof(WDI_SetBSSKeyReqParamsType));
   vos_mem_free(pWdaParams->wdaWdiApiMsgParam);
   vos_mem_free(pWdaParams) ;
   setBssKeyParams->status = status ;
   WDA_SendMsg(pWDA, WDA_SET_BSSKEY_RSP, (void *)setBssKeyParams , 0) ;
   return ;
}
/*
                                    
                                                       
                                         
 */ 
VOS_STATUS WDA_ProcessSetBssKeyReq(tWDA_CbContext *pWDA, 
                                          tSetBssKeyParams *setBssKeyParams )
{
   WDI_Status status = WDI_STATUS_SUCCESS ;
   WDI_SetBSSKeyReqParamsType *wdiSetBssKeyParam = 
                  (WDI_SetBSSKeyReqParamsType *)vos_mem_malloc(
                                   sizeof(WDI_SetBSSKeyReqParamsType)) ;
   tWDA_ReqParams *pWdaParams ;
   v_U8_t keyIndex;
   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                                          "------> %s " ,__func__);
   if(NULL == wdiSetBssKeyParam) 
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                           "%s: VOS MEM Alloc Failure", __func__); 
      VOS_ASSERT(0);
      return VOS_STATUS_E_NOMEM;
   }
   pWdaParams = (tWDA_ReqParams *)vos_mem_malloc(sizeof(tWDA_ReqParams)) ;
   if(NULL == pWdaParams)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                           "%s: VOS MEM Alloc Failure", __func__); 
      VOS_ASSERT(0);
      vos_mem_free(wdiSetBssKeyParam);
      return VOS_STATUS_E_NOMEM;
   }
   vos_mem_zero(wdiSetBssKeyParam, sizeof(WDI_SetBSSKeyReqParamsType));
   /*                                      */
   wdiSetBssKeyParam->wdiBSSKeyInfo.ucBssIdx = setBssKeyParams->bssIdx;
   wdiSetBssKeyParam->wdiBSSKeyInfo.wdiEncType = setBssKeyParams->encType;
   wdiSetBssKeyParam->wdiBSSKeyInfo.ucNumKeys = setBssKeyParams->numKeys;
   if(setBssKeyParams->encType != eSIR_ED_NONE)
   {
      if( setBssKeyParams->numKeys == 0 && 
         (( setBssKeyParams->encType == eSIR_ED_WEP40)|| 
                                setBssKeyParams->encType == eSIR_ED_WEP104))
      {
         tANI_U8 defaultKeyId = 0;
         WDA_GetWepKeysFromCfg( pWDA, &defaultKeyId, 
            &wdiSetBssKeyParam->wdiBSSKeyInfo.ucNumKeys,
            wdiSetBssKeyParam->wdiBSSKeyInfo.aKeys );
      }
      else
      {
         for( keyIndex=0; keyIndex < setBssKeyParams->numKeys; keyIndex++)
         {
            wdiSetBssKeyParam->wdiBSSKeyInfo.aKeys[keyIndex].keyId =
                                 setBssKeyParams->key[keyIndex].keyId;
            wdiSetBssKeyParam->wdiBSSKeyInfo.aKeys[keyIndex].unicast =
                                 setBssKeyParams->key[keyIndex].unicast;
            wdiSetBssKeyParam->wdiBSSKeyInfo.aKeys[keyIndex].keyDirection =
                                 setBssKeyParams->key[keyIndex].keyDirection;
            vos_mem_copy(wdiSetBssKeyParam->wdiBSSKeyInfo.aKeys[keyIndex].keyRsc, 
                  setBssKeyParams->key[keyIndex].keyRsc, WLAN_MAX_KEY_RSC_LEN);
            wdiSetBssKeyParam->wdiBSSKeyInfo.aKeys[keyIndex].paeRole =
                                      setBssKeyParams->key[keyIndex].paeRole;
            wdiSetBssKeyParam->wdiBSSKeyInfo.aKeys[keyIndex].keyLength =
                                      setBssKeyParams->key[keyIndex].keyLength;
            vos_mem_copy(wdiSetBssKeyParam->wdiBSSKeyInfo.aKeys[keyIndex].key, 
                                          setBssKeyParams->key[keyIndex].key, 
                                          SIR_MAC_MAX_KEY_LENGTH);
         }
      }
   }
   wdiSetBssKeyParam->wdiBSSKeyInfo.ucSingleTidRc = 
                                      setBssKeyParams->singleTidRc;
   wdiSetBssKeyParam->wdiReqStatusCB = NULL ;
   /*                                                          */
   /*                             */
   pWdaParams->pWdaContext = pWDA;
   pWdaParams->wdaMsgParam = setBssKeyParams;
   pWdaParams->wdaWdiApiMsgParam = wdiSetBssKeyParam;
   status = WDI_SetBSSKeyReq(wdiSetBssKeyParam, 
                           (WDI_SetBSSKeyRspCb)WDA_SetBssKeyReqCallback ,pWdaParams);
   
   if(IS_WDI_STATUS_FAILURE(status))
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                   "Failure in Set BSS Key Req WDI API, free all the memory " );
      vos_mem_free(pWdaParams->wdaWdiApiMsgParam);
      vos_mem_free(pWdaParams) ;
      setBssKeyParams->status = eSIR_FAILURE ;
      WDA_SendMsg(pWDA, WDA_SET_BSSKEY_RSP, (void *)setBssKeyParams, 0) ;
   }
   return CONVERT_WDI2VOS_STATUS(status) ;
}
/*
                                        
                                  
 */ 
void WDA_RemoveBssKeyReqCallback(WDI_Status status, void* pUserData)
{
   tWDA_ReqParams *pWdaParams = (tWDA_ReqParams *)pUserData; 
   tWDA_CbContext *pWDA; 
   tRemoveBssKeyParams *removeBssKeyParams;
   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                                          "<------ %s " ,__func__);
   if(NULL == pWdaParams)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                  "%s: pWdaParams received NULL", __func__);
      VOS_ASSERT(0) ;
      return ;
   }
   pWDA = (tWDA_CbContext *)pWdaParams->pWdaContext;
   removeBssKeyParams = (tRemoveBssKeyParams *)pWdaParams->wdaMsgParam;
   vos_mem_free(pWdaParams->wdaWdiApiMsgParam);
   vos_mem_free(pWdaParams) ;
   
   removeBssKeyParams->status = status ;
   WDA_SendMsg(pWDA, WDA_REMOVE_BSSKEY_RSP, (void *)removeBssKeyParams , 0) ;
   return ;
}

/*
                                        
                                           
 */
void WDA_SpoofMacAddrRspCallback(WDI_SpoofMacAddrRspParamType* wdiRsp, void* pUserData)
{
   tWDA_ReqParams *pWdaParams = (tWDA_ReqParams *)pUserData;
   tWDA_CbContext *pWDA;
   tSirSpoofMacAddrReq *spoofMacAddrReq;
   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                                          "<------ %s " ,__func__);
   if(NULL == pWdaParams)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
              "%s: pWdaParams received NULL", __func__);
      VOS_ASSERT(0) ;
      return ;
   }

   pWDA = (tWDA_CbContext *)pWdaParams->pWdaContext;
   spoofMacAddrReq = (tSirSpoofMacAddrReq *)pWdaParams->wdaMsgParam ;

   if(wdiRsp->wdiStatus != WDI_STATUS_SUCCESS )
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
              "%s: Unable to set Random Mac Addr in FW", __func__);
   }

   vos_mem_free(spoofMacAddrReq);
   vos_mem_free(pWdaParams->wdaWdiApiMsgParam);
   vos_mem_free(pWdaParams);
   return ;
}

/*
                                       
                                                                     
                     
 */ 
VOS_STATUS WDA_ProcessRemoveBssKeyReq(tWDA_CbContext *pWDA, 
                                       tRemoveBssKeyParams *removeBssKeyParams )
{
   WDI_Status status = WDI_STATUS_SUCCESS ;
   WDI_RemoveBSSKeyReqParamsType *wdiRemoveBssKeyParam = 
                  (WDI_RemoveBSSKeyReqParamsType *)vos_mem_malloc(
                                   sizeof(WDI_RemoveBSSKeyReqParamsType)) ;
   tWDA_ReqParams *pWdaParams ;
   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                                          "------> %s " ,__func__);
   if(NULL == wdiRemoveBssKeyParam) 
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                           "%s: VOS MEM Alloc Failure", __func__); 
      VOS_ASSERT(0);
      return VOS_STATUS_E_NOMEM;
   }
   pWdaParams = (tWDA_ReqParams *)vos_mem_malloc(sizeof(tWDA_ReqParams)) ;
   if(NULL == pWdaParams)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                           "%s: VOS MEM Alloc Failure", __func__); 
      VOS_ASSERT(0);
      vos_mem_free(wdiRemoveBssKeyParam);
      return VOS_STATUS_E_NOMEM;
   }
   /*                                            */
   wdiRemoveBssKeyParam->wdiKeyInfo.ucBssIdx = removeBssKeyParams->bssIdx;
   wdiRemoveBssKeyParam->wdiKeyInfo.wdiEncType = removeBssKeyParams->encType;
   wdiRemoveBssKeyParam->wdiKeyInfo.ucKeyId = removeBssKeyParams->keyId;
   wdiRemoveBssKeyParam->wdiKeyInfo.wdiWEPType = removeBssKeyParams->wepType;
   wdiRemoveBssKeyParam->wdiReqStatusCB = NULL ;
   /*                                                             */
   /*                             */
   pWdaParams->pWdaContext = pWDA;
   pWdaParams->wdaMsgParam = removeBssKeyParams;
   pWdaParams->wdaWdiApiMsgParam = wdiRemoveBssKeyParam;
   status = WDI_RemoveBSSKeyReq(wdiRemoveBssKeyParam, 
                     (WDI_RemoveBSSKeyRspCb)WDA_RemoveBssKeyReqCallback, pWdaParams);
   if(IS_WDI_STATUS_FAILURE(status))
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
              "Failure in Remove BSS Key Req WDI API, free all the memory " );
      vos_mem_free(pWdaParams->wdaWdiApiMsgParam);
      vos_mem_free(pWdaParams) ;
      removeBssKeyParams->status = eSIR_FAILURE ;
      WDA_SendMsg(pWDA, WDA_REMOVE_BSSKEY_RSP, (void *)removeBssKeyParams, 0) ;
   }
   return CONVERT_WDI2VOS_STATUS(status) ;
}
/*
                                     
                                  
 */ 
void WDA_SetStaKeyReqCallback(WDI_Status status, void* pUserData)
{
   tWDA_ReqParams *pWdaParams = (tWDA_ReqParams *)pUserData; 
   tWDA_CbContext *pWDA; 
   tSetStaKeyParams *setStaKeyParams;
   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                                          "<------ %s " ,__func__);
   if(NULL == pWdaParams)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR
                ,"%s: pWdaParams received NULL", __func__);
      VOS_ASSERT(0) ;
      return ;
   }
   pWDA = (tWDA_CbContext *)pWdaParams->pWdaContext;
   setStaKeyParams = (tSetStaKeyParams *)pWdaParams->wdaMsgParam;
   vos_mem_zero(pWdaParams->wdaWdiApiMsgParam,
                 sizeof(WDI_SetSTAKeyReqParamsType));
   vos_mem_free(pWdaParams->wdaWdiApiMsgParam);
   vos_mem_free(pWdaParams) ;
   setStaKeyParams->status = status ;
   WDA_SendMsg(pWDA, WDA_SET_STAKEY_RSP, (void *)setStaKeyParams , 0) ;
   return ;
}
/*
                                    
                                                                      
              
 */
VOS_STATUS WDA_ProcessSetStaKeyReq(tWDA_CbContext *pWDA, 
                                          tSetStaKeyParams *setStaKeyParams )
{
   WDI_Status status = WDI_STATUS_SUCCESS ;
   WDI_SetSTAKeyReqParamsType *wdiSetStaKeyParam = 
                  (WDI_SetSTAKeyReqParamsType *)vos_mem_malloc(
                                   sizeof(WDI_SetSTAKeyReqParamsType)) ;
   tWDA_ReqParams *pWdaParams ;
   v_U8_t keyIndex;
   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                                          "------> %s " ,__func__);
   if(NULL == wdiSetStaKeyParam) 
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                           "%s: VOS MEM Alloc Failure", __func__); 
      VOS_ASSERT(0);
      return VOS_STATUS_E_NOMEM;
   }
   pWdaParams = (tWDA_ReqParams *)vos_mem_malloc(sizeof(tWDA_ReqParams)) ;
   if(NULL == pWdaParams)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                           "%s: VOS MEM Alloc Failure", __func__); 
      VOS_ASSERT(0);
      vos_mem_free(wdiSetStaKeyParam);
      return VOS_STATUS_E_NOMEM;
   }
   vos_mem_set(wdiSetStaKeyParam, sizeof(WDI_SetSTAKeyReqParamsType), 0);
   vos_mem_zero(wdiSetStaKeyParam, sizeof(WDI_SetSTAKeyReqParamsType));
   /*                                          */
   wdiSetStaKeyParam->wdiKeyInfo.ucSTAIdx = setStaKeyParams->staIdx;
   wdiSetStaKeyParam->wdiKeyInfo.wdiEncType = setStaKeyParams->encType;
   wdiSetStaKeyParam->wdiKeyInfo.wdiWEPType = setStaKeyParams->wepType;
   wdiSetStaKeyParam->wdiKeyInfo.ucDefWEPIdx = setStaKeyParams->defWEPIdx;
   if(setStaKeyParams->encType != eSIR_ED_NONE)
   {
      if( (wdiSetStaKeyParam->wdiKeyInfo.wdiWEPType == WDI_WEP_STATIC) && 
                    (WDA_INVALID_KEY_INDEX == setStaKeyParams->defWEPIdx) &&
                    (eSYSTEM_AP_ROLE != pWDA->wdaGlobalSystemRole))
      {
         WDA_GetWepKeysFromCfg( pWDA, 
            &wdiSetStaKeyParam->wdiKeyInfo.ucDefWEPIdx, 
            &wdiSetStaKeyParam->wdiKeyInfo.ucNumKeys,
            wdiSetStaKeyParam->wdiKeyInfo.wdiKey );
      }
      else
      {
         for( keyIndex=0; keyIndex < SIR_MAC_MAX_NUM_OF_DEFAULT_KEYS; 
                                                                  keyIndex++)
         {
            wdiSetStaKeyParam->wdiKeyInfo.wdiKey[keyIndex].keyId =
                                  setStaKeyParams->key[keyIndex].keyId;
            wdiSetStaKeyParam->wdiKeyInfo.wdiKey[keyIndex].unicast =
                                  setStaKeyParams->key[keyIndex].unicast;
            wdiSetStaKeyParam->wdiKeyInfo.wdiKey[keyIndex].keyDirection =
                                  setStaKeyParams->key[keyIndex].keyDirection;
            vos_mem_copy(wdiSetStaKeyParam->wdiKeyInfo.wdiKey[keyIndex].keyRsc, 
                  setStaKeyParams->key[keyIndex].keyRsc, WLAN_MAX_KEY_RSC_LEN);
            wdiSetStaKeyParam->wdiKeyInfo.wdiKey[keyIndex].paeRole =
                                   setStaKeyParams->key[keyIndex].paeRole;
            wdiSetStaKeyParam->wdiKeyInfo.wdiKey[keyIndex].keyLength =
                                   setStaKeyParams->key[keyIndex].keyLength;
            vos_mem_copy(wdiSetStaKeyParam->wdiKeyInfo.wdiKey[keyIndex].key, 
                  setStaKeyParams->key[keyIndex].key, SIR_MAC_MAX_KEY_LENGTH);
            /*                                                                        */
            if (WDI_TX_DEFAULT == wdiSetStaKeyParam->wdiKeyInfo.wdiKey[keyIndex].keyDirection)
            {
                wdiSetStaKeyParam->wdiKeyInfo.ucDefWEPIdx = keyIndex;
            }
         }
         wdiSetStaKeyParam->wdiKeyInfo.ucNumKeys = 
                                          SIR_MAC_MAX_NUM_OF_DEFAULT_KEYS;
      }
   }
   wdiSetStaKeyParam->wdiKeyInfo.ucSingleTidRc = setStaKeyParams->singleTidRc;
   wdiSetStaKeyParam->wdiReqStatusCB = NULL ;
   /*                                                          */
   /*                             */
   pWdaParams->pWdaContext = pWDA;
   pWdaParams->wdaMsgParam = setStaKeyParams;
   pWdaParams->wdaWdiApiMsgParam = wdiSetStaKeyParam;
   status = WDI_SetSTAKeyReq(wdiSetStaKeyParam, 
                          (WDI_SetSTAKeyRspCb)WDA_SetStaKeyReqCallback, pWdaParams);
   if(IS_WDI_STATUS_FAILURE(status))
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                   "Failure in set STA Key Req WDI API, free all the memory " );
      vos_mem_free(pWdaParams->wdaWdiApiMsgParam);
      vos_mem_free(pWdaParams) ;
      setStaKeyParams->status = eSIR_FAILURE ;
      WDA_SendMsg(pWDA, WDA_SET_STAKEY_RSP, (void *)setStaKeyParams, 0) ;
   }
   return CONVERT_WDI2VOS_STATUS(status) ;
}
/*
                                          
                                        
 */ 
void WDA_SetBcastStaKeyReqCallback(WDI_Status status, void* pUserData)
{
   tWDA_ReqParams *pWdaParams = (tWDA_ReqParams *)pUserData; 
   tWDA_CbContext *pWDA; 
   tSetStaKeyParams *setStaKeyParams;
   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                                          "<------ %s " ,__func__);
   if(NULL == pWdaParams)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
              "%s: pWdaParams received NULL", __func__);
      VOS_ASSERT(0) ;
      return ;
   }
   pWDA = (tWDA_CbContext *)pWdaParams->pWdaContext;
   setStaKeyParams = (tSetStaKeyParams *)pWdaParams->wdaMsgParam;
   vos_mem_free(pWdaParams->wdaWdiApiMsgParam);
   vos_mem_free(pWdaParams) ;
   setStaKeyParams->status = status ;
   WDA_SendMsg(pWDA, WDA_SET_STA_BCASTKEY_RSP, (void *)setStaKeyParams , 0) ;
   return ;
}

/*
                                         
                                                                              
              
 */
VOS_STATUS WDA_ProcessSetBcastStaKeyReq(tWDA_CbContext *pWDA, 
                                          tSetStaKeyParams *setStaKeyParams )
{
   WDI_Status status = WDI_STATUS_SUCCESS ;
   WDI_SetSTAKeyReqParamsType *wdiSetStaKeyParam = 
                  (WDI_SetSTAKeyReqParamsType *)vos_mem_malloc(
                                   sizeof(WDI_SetSTAKeyReqParamsType)) ;
   tWDA_ReqParams *pWdaParams ;
   v_U8_t keyIndex;
   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                                          "------> %s " ,__func__);
   if(NULL == wdiSetStaKeyParam) 
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                           "%s: VOS MEM Alloc Failure", __func__); 
      VOS_ASSERT(0);
      return VOS_STATUS_E_NOMEM;
   }
   pWdaParams = (tWDA_ReqParams *)vos_mem_malloc(sizeof(tWDA_ReqParams)) ;
   if(NULL == pWdaParams)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                           "%s: VOS MEM Alloc Failure", __func__); 
      VOS_ASSERT(0);
      vos_mem_free(wdiSetStaKeyParam);
      return VOS_STATUS_E_NOMEM;
   }
   vos_mem_set(wdiSetStaKeyParam, sizeof(WDI_SetSTAKeyReqParamsType), 0);
   vos_mem_zero(wdiSetStaKeyParam, sizeof(WDI_SetSTAKeyReqParamsType));
   /*                                          */
   wdiSetStaKeyParam->wdiKeyInfo.ucSTAIdx = setStaKeyParams->staIdx;
   wdiSetStaKeyParam->wdiKeyInfo.wdiEncType = setStaKeyParams->encType;
   wdiSetStaKeyParam->wdiKeyInfo.wdiWEPType = setStaKeyParams->wepType;
   wdiSetStaKeyParam->wdiKeyInfo.ucDefWEPIdx = setStaKeyParams->defWEPIdx;
   if(setStaKeyParams->encType != eSIR_ED_NONE)
   {
      for( keyIndex=0; keyIndex < SIR_MAC_MAX_NUM_OF_DEFAULT_KEYS; 
                                                               keyIndex++)
      {
         wdiSetStaKeyParam->wdiKeyInfo.wdiKey[keyIndex].keyId =
                               setStaKeyParams->key[keyIndex].keyId;
         wdiSetStaKeyParam->wdiKeyInfo.wdiKey[keyIndex].unicast =
                               setStaKeyParams->key[keyIndex].unicast;
         wdiSetStaKeyParam->wdiKeyInfo.wdiKey[keyIndex].keyDirection =
                               setStaKeyParams->key[keyIndex].keyDirection;
         vos_mem_copy(wdiSetStaKeyParam->wdiKeyInfo.wdiKey[keyIndex].keyRsc, 
               setStaKeyParams->key[keyIndex].keyRsc, WLAN_MAX_KEY_RSC_LEN);
         wdiSetStaKeyParam->wdiKeyInfo.wdiKey[keyIndex].paeRole =
                                setStaKeyParams->key[keyIndex].paeRole;
         wdiSetStaKeyParam->wdiKeyInfo.wdiKey[keyIndex].keyLength =
                                setStaKeyParams->key[keyIndex].keyLength;
         vos_mem_copy(wdiSetStaKeyParam->wdiKeyInfo.wdiKey[keyIndex].key, 
               setStaKeyParams->key[keyIndex].key, SIR_MAC_MAX_KEY_LENGTH);
      }
      wdiSetStaKeyParam->wdiKeyInfo.ucNumKeys = 
                                       SIR_MAC_MAX_NUM_OF_DEFAULT_KEYS;
   }
   wdiSetStaKeyParam->wdiKeyInfo.ucSingleTidRc = setStaKeyParams->singleTidRc;
   /*                                                          */
   /*                             */
   pWdaParams->pWdaContext = pWDA;
   pWdaParams->wdaMsgParam = setStaKeyParams;
   pWdaParams->wdaWdiApiMsgParam = wdiSetStaKeyParam;
   status = WDI_SetSTABcastKeyReq(wdiSetStaKeyParam, 
                          (WDI_SetSTAKeyRspCb)WDA_SetBcastStaKeyReqCallback, pWdaParams);
   if(IS_WDI_STATUS_FAILURE(status))
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
          "Failure in set BCAST STA Key Req WDI API, free all the memory " );
      vos_mem_free(pWdaParams->wdaWdiApiMsgParam);
      vos_mem_free(pWdaParams) ;
      setStaKeyParams->status = eSIR_FAILURE ;
      WDA_SendMsg(pWDA, WDA_SET_STA_BCASTKEY_RSP, (void *)setStaKeyParams, 0) ;
   }
   return CONVERT_WDI2VOS_STATUS(status) ;
}
/*
                                        
                                  
 */ 
void WDA_RemoveStaKeyReqCallback(WDI_Status status, void* pUserData)
{
   tWDA_ReqParams *pWdaParams = (tWDA_ReqParams *)pUserData; 
   tWDA_CbContext *pWDA; 
   tRemoveStaKeyParams *removeStaKeyParams;
   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                                          "<------ %s " ,__func__);
   if(NULL == pWdaParams)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                  "%s: pWdaParams received NULL", __func__);
      VOS_ASSERT(0) ;
      return ;
   }
   pWDA = (tWDA_CbContext *)pWdaParams->pWdaContext;
   removeStaKeyParams = (tRemoveStaKeyParams *)pWdaParams->wdaMsgParam;
   vos_mem_free(pWdaParams->wdaWdiApiMsgParam);
   vos_mem_free(pWdaParams) ;
   removeStaKeyParams->status = status ;
   WDA_SendMsg(pWDA, WDA_REMOVE_STAKEY_RSP, (void *)removeStaKeyParams , 0) ;
   return ;
}

/*
                                       
                                                                           
 */ 
VOS_STATUS WDA_ProcessRemoveStaKeyReq(tWDA_CbContext *pWDA, 
                                    tRemoveStaKeyParams *removeStaKeyParams )
{
   WDI_Status status = WDI_STATUS_SUCCESS ;
   WDI_RemoveSTAKeyReqParamsType *wdiRemoveStaKeyParam = 
                  (WDI_RemoveSTAKeyReqParamsType *)vos_mem_malloc(
                                   sizeof(WDI_RemoveSTAKeyReqParamsType)) ;
   tWDA_ReqParams *pWdaParams ;
   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                                          "------> %s " ,__func__);
   if(NULL == wdiRemoveStaKeyParam) 
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                           "%s: VOS MEM Alloc Failure", __func__); 
      VOS_ASSERT(0);
      return VOS_STATUS_E_NOMEM;
   }
   pWdaParams = (tWDA_ReqParams *)vos_mem_malloc(sizeof(tWDA_ReqParams)) ;
   if(NULL == pWdaParams)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                           "%s: VOS MEM Alloc Failure", __func__); 
      VOS_ASSERT(0);
      vos_mem_free(wdiRemoveStaKeyParam);
      return VOS_STATUS_E_NOMEM;
   }
   /*                                            */
   wdiRemoveStaKeyParam->wdiKeyInfo.ucSTAIdx = removeStaKeyParams->staIdx;
   wdiRemoveStaKeyParam->wdiKeyInfo.wdiEncType = removeStaKeyParams->encType;
   wdiRemoveStaKeyParam->wdiKeyInfo.ucKeyId = removeStaKeyParams->keyId;
   wdiRemoveStaKeyParam->wdiKeyInfo.ucUnicast = removeStaKeyParams->unicast;
   wdiRemoveStaKeyParam->wdiReqStatusCB = NULL ;
   /*                                                             */
   /*                             */
   pWdaParams->pWdaContext = pWDA;
   pWdaParams->wdaMsgParam = removeStaKeyParams;
   pWdaParams->wdaWdiApiMsgParam = wdiRemoveStaKeyParam;
   status = WDI_RemoveSTAKeyReq(wdiRemoveStaKeyParam, 
                     (WDI_RemoveSTAKeyRspCb)WDA_RemoveStaKeyReqCallback, pWdaParams);
   if(IS_WDI_STATUS_FAILURE(status))
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
               "Failure in remove STA Key Req WDI API, free all the memory " );
      vos_mem_free(pWdaParams->wdaWdiApiMsgParam);
      vos_mem_free(pWdaParams) ;
      removeStaKeyParams->status = eSIR_FAILURE ;
      WDA_SendMsg(pWDA, WDA_REMOVE_STAKEY_RSP, (void *)removeStaKeyParams, 0) ;
   }
   return CONVERT_WDI2VOS_STATUS(status) ;
}
/*
                                        
                                                                           
 */ 
WDA_processSetLinkStateStatus WDA_IsHandleSetLinkStateReq(
                                          tWDA_CbContext *pWDA,
                                          tLinkStateParams *linkStateParams)
{
   WDA_processSetLinkStateStatus status = WDA_PROCESS_SET_LINK_STATE;
   switch(linkStateParams->state)
   {
      case eSIR_LINK_PREASSOC_STATE:
      case eSIR_LINK_BTAMP_PREASSOC_STATE:
        /* 
                                          
                                                                          
                                            
         */
         if( !WDA_IS_NULL_MAC_ADDRESS(linkStateParams->bssid) )
         {
            vos_mem_copy(pWDA->macBSSID,linkStateParams->bssid, 
                                                   sizeof(tSirMacAddr));
         }
         else
         {
            VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_FATAL,
               "%s: BSSID in set link state is NULL ", __func__);
            VOS_ASSERT(0);
         }

         if( !WDA_IS_NULL_MAC_ADDRESS(linkStateParams->selfMacAddr) )
         {
            vos_mem_copy(pWDA->macSTASelf,linkStateParams->selfMacAddr, 
                                                   sizeof(tSirMacAddr));
         }
         else
         {
            VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_FATAL,
               "%s: self mac address in set link state is NULL ", __func__);
            VOS_ASSERT(0);
         }

         /*                                                                   
                                                                              
                                                        
         */
         if(WDA_PRE_ASSOC_STATE == pWDA->wdaState)
         {
            /*                                         */
            pWDA->wdaState = WDA_READY_STATE;
         }
         else
         {
            pWDA->wdaState = WDA_PRE_ASSOC_STATE;
         }
         //                                    
         pWDA->linkState = linkStateParams->state;
         break;
      default:
         if(pWDA->wdaState != WDA_READY_STATE)
         {
            /*                                                                  
                                                                                    
                                                                               
                                                                              
                                                          */
            if (pWDA->wdaState == WDA_PRE_ASSOC_STATE)
            {
               pWDA->wdaState = WDA_READY_STATE;
               /*                                       */
            }
            else
            {
               VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                        "Set link state called when WDA is not in READY STATE " );
               status = WDA_IGNORE_SET_LINK_STATE;
            }
         }
         break;
   }
   
   return status;
}
/*
                                     
                                                 
 */ 
void WDA_SetLinkStateCallback(WDI_Status status, void* pUserData)
{
   tWDA_CbContext *pWDA;
   tLinkStateParams *linkStateParams;
   tWDA_ReqParams *pWdaParams = (tWDA_ReqParams *)pUserData; 
   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                                          "<------ %s " ,__func__);
   if(NULL == pWdaParams)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
             "%s: pWdaParams received NULL", __func__);
      VOS_ASSERT(0) ;
      return ;
   }
   pWDA = (tWDA_CbContext *)pWdaParams->pWdaContext;
   linkStateParams = (tLinkStateParams *)pWdaParams->wdaMsgParam ;
   /*
                                                                     
                                                                   */
   if( ((linkStateParams->state == eSIR_LINK_POSTASSOC_STATE) &&
         status == WDI_STATUS_SUCCESS) ||  ((status == WDI_STATUS_SUCCESS) &&
       (linkStateParams->state == eSIR_LINK_AP_STATE)) ||
       ((status == WDI_STATUS_SUCCESS) &&
       (linkStateParams->state == eSIR_LINK_IBSS_STATE)))
   {
      WDA_START_TIMER(&pWDA->wdaTimers.baActivityChkTmr);
   }
   WDA_SendMsg(pWDA, WDA_SET_LINK_STATE_RSP, (void *)linkStateParams , 0) ;
   /* 
                                                                     
                
    */
   if( pWdaParams != NULL )
   {
      if( pWdaParams->wdaWdiApiMsgParam != NULL )
      {
         vos_mem_free(pWdaParams->wdaWdiApiMsgParam) ;
      }
      vos_mem_free(pWdaParams);
   }
   return ;
}
/*
                                    
                                         
 */ 
VOS_STATUS WDA_ProcessSetLinkState(tWDA_CbContext *pWDA, 
                                           tLinkStateParams *linkStateParams)
{
   WDI_Status status = WDI_STATUS_SUCCESS ;
   WDI_SetLinkReqParamsType *wdiSetLinkStateParam = 
                  (WDI_SetLinkReqParamsType *)vos_mem_malloc(
                                   sizeof(WDI_SetLinkReqParamsType)) ;
   tWDA_ReqParams *pWdaParams ;
   tpAniSirGlobal pMac;
   pMac = (tpAniSirGlobal )VOS_GET_MAC_CTXT(pWDA->pVosContext);

   if(NULL == pMac)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                           "%s:pMac is NULL", __func__);
      VOS_ASSERT(0);
      vos_mem_free(wdiSetLinkStateParam);
      return VOS_STATUS_E_FAILURE;
   }

   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                                          "------> %s " ,__func__);
   if(NULL == wdiSetLinkStateParam) 
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                           "%s: VOS MEM Alloc Failure", __func__); 
      VOS_ASSERT(0);
      return VOS_STATUS_E_NOMEM;
   }
   pWdaParams = (tWDA_ReqParams *)vos_mem_malloc(sizeof(tWDA_ReqParams)) ;
   if(NULL == pWdaParams)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                           "%s: VOS MEM Alloc Failure", __func__); 
      VOS_ASSERT(0);
      vos_mem_free(wdiSetLinkStateParam);
      return VOS_STATUS_E_NOMEM;
   }
   if(WDA_IGNORE_SET_LINK_STATE == 
                  WDA_IsHandleSetLinkStateReq(pWDA,linkStateParams))
   {
      status = WDI_STATUS_E_FAILURE;
   }
   else
   {
      vos_mem_copy(wdiSetLinkStateParam->wdiLinkInfo.macBSSID, 
                                  linkStateParams->bssid, sizeof(tSirMacAddr));
      vos_mem_copy(wdiSetLinkStateParam->wdiLinkInfo.macSelfStaMacAddr, 
                                  linkStateParams->selfMacAddr, sizeof(tSirMacAddr));
      wdiSetLinkStateParam->wdiLinkInfo.wdiLinkState = linkStateParams->state;
      wdiSetLinkStateParam->wdiReqStatusCB = NULL ;
      pWdaParams->pWdaContext = pWDA;
      /*                                                             */
      pWdaParams->wdaMsgParam = (void *)linkStateParams ;
      /*                             */
      pWdaParams->wdaWdiApiMsgParam = (void *)wdiSetLinkStateParam ;
      /*                                                           */
      if( (linkStateParams->state == eSIR_LINK_IDLE_STATE)
          && (0 == WDI_GetActiveSessionsCount(pWDA->pWdiContext, linkStateParams->bssid, TRUE)) &&
          (wdaGetGlobalSystemRole(pMac) != eSYSTEM_AP_ROLE) )
      {
         WDA_STOP_TIMER(&pWDA->wdaTimers.baActivityChkTmr);
      }
      status = WDI_SetLinkStateReq(wdiSetLinkStateParam, 
                        (WDI_SetLinkStateRspCb)WDA_SetLinkStateCallback, pWdaParams);
      if(IS_WDI_STATUS_FAILURE(status))
      {
         VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
             "Failure in set link state Req WDI API, free all the memory " );
      }
   }
   if(IS_WDI_STATUS_FAILURE(status))
   {
      vos_mem_free(wdiSetLinkStateParam) ;
      WDA_SendMsg(pWDA, WDA_SET_LINK_STATE_RSP, (void *)linkStateParams, 0);
      vos_mem_free(pWdaParams);
   }
   return CONVERT_WDI2VOS_STATUS(status) ;
}
/*
                                          
                                                       
 */ 
void WDA_GetStatsReqParamsCallback(
                              WDI_GetStatsRspParamsType *wdiGetStatsRsp,
                              void* pUserData)
{
   tWDA_CbContext *pWDA = (tWDA_CbContext *)pUserData ;
   tAniGetPEStatsRsp *pGetPEStatsRspParams;

   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                                          "<------ %s " ,__func__);
   pGetPEStatsRspParams = 
       (tAniGetPEStatsRsp *)vos_mem_malloc(sizeof(tAniGetPEStatsRsp) +
       (wdiGetStatsRsp->usMsgLen - sizeof(WDI_GetStatsRspParamsType)));

   if(NULL == pGetPEStatsRspParams)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                           "%s: VOS MEM Alloc Failure", __func__); 
      VOS_ASSERT(0);
      return;
   }
   vos_mem_set(pGetPEStatsRspParams, wdiGetStatsRsp->usMsgLen, 0);
   pGetPEStatsRspParams->msgType = wdiGetStatsRsp->usMsgType;
   pGetPEStatsRspParams->msgLen = sizeof(tAniGetPEStatsRsp) + 
                   (wdiGetStatsRsp->usMsgLen - sizeof(WDI_GetStatsRspParamsType));

  //                                  
   pGetPEStatsRspParams->sessionId = 0;
   pGetPEStatsRspParams->rc = 
                      wdiGetStatsRsp->wdiStatus;
   pGetPEStatsRspParams->staId   = wdiGetStatsRsp->ucSTAIdx;
   pGetPEStatsRspParams->statsMask = wdiGetStatsRsp->uStatsMask;
   vos_mem_copy( pGetPEStatsRspParams + 1,
                  wdiGetStatsRsp + 1,
                  wdiGetStatsRsp->usMsgLen - sizeof(WDI_GetStatsRspParamsType));
  /*                      */
   WDA_SendMsg(pWDA, WDA_GET_STATISTICS_RSP, pGetPEStatsRspParams , 0) ;
   
   return;
}

/*
                                   
                                       
 */ 
VOS_STATUS WDA_ProcessGetStatsReq(tWDA_CbContext *pWDA,
                                    tAniGetPEStatsReq *pGetStatsParams)
{
   WDI_Status status = WDI_STATUS_SUCCESS ;
   WDI_GetStatsReqParamsType wdiGetStatsParam;
   tAniGetPEStatsRsp *pGetPEStatsRspParams;
   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                                          "------> %s " ,__func__);
   wdiGetStatsParam.wdiGetStatsParamsInfo.ucSTAIdx = 
                                          pGetStatsParams->staId;
   wdiGetStatsParam.wdiGetStatsParamsInfo.uStatsMask = 
                                          pGetStatsParams->statsMask;
   wdiGetStatsParam.wdiReqStatusCB = NULL ;
   status = WDI_GetStatsReq(&wdiGetStatsParam, 
       (WDI_GetStatsRspCb)WDA_GetStatsReqParamsCallback, pWDA);
   if(IS_WDI_STATUS_FAILURE(status))
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
          "Failure in Get Stats Req WDI API, free all the memory " );
      pGetPEStatsRspParams = 
         (tAniGetPEStatsRsp *)vos_mem_malloc(sizeof(tAniGetPEStatsRsp));
      if(NULL == pGetPEStatsRspParams) 
      {
          VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                           "%s: VOS MEM Alloc Failure", __func__); 
          VOS_ASSERT(0);
          vos_mem_free(pGetStatsParams);
          return VOS_STATUS_E_NOMEM;
      }
      pGetPEStatsRspParams->msgType = WDA_GET_STATISTICS_RSP;
      pGetPEStatsRspParams->msgLen = sizeof(tAniGetPEStatsRsp);
      pGetPEStatsRspParams->staId = pGetStatsParams->staId;
      pGetPEStatsRspParams->rc    = eSIR_FAILURE;
      WDA_SendMsg(pWDA, WDA_GET_STATISTICS_RSP, 
                                 (void *)pGetPEStatsRspParams, 0) ;
   }
   /*                          */
   vos_mem_free(pGetStatsParams);
   return CONVERT_WDI2VOS_STATUS(status);
}

#if defined WLAN_FEATURE_VOWIFI_11R || defined FEATURE_WLAN_ESE || defined(FEATURE_WLAN_LFR)
/*
                                                       
                                                           
 */
void WDA_GetRoamRssiReqParamsCallback(
                              WDI_GetRoamRssiRspParamsType *wdiGetRoamRssiRsp,
                              void* pUserData)
{
   tWDA_ReqParams *pWdaParams = (tWDA_ReqParams *)pUserData;
   tWDA_CbContext *pWDA = NULL;
   tAniGetRoamRssiRsp *pGetRoamRssiRspParams = NULL;
   tpAniGetRssiReq  pGetRoamRssiReqParams = NULL;
   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                                          "<------ %s " ,__func__);
   if(NULL == pWdaParams)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                 "%s: pWdaParams received NULL", __func__);
      VOS_ASSERT(0) ;
      return ;
   }
   pWDA = (tWDA_CbContext *)pWdaParams->pWdaContext;
   pGetRoamRssiReqParams = (tAniGetRssiReq *)pWdaParams->wdaMsgParam;

   if(NULL == pGetRoamRssiReqParams)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                  "%s: pGetRoamRssiReqParams received NULL", __func__);
      VOS_ASSERT(0);
      vos_mem_free(pWdaParams->wdaWdiApiMsgParam);
      vos_mem_free(pWdaParams);
      return ;
   }
   pGetRoamRssiRspParams =
       (tAniGetRoamRssiRsp *)vos_mem_malloc(sizeof(tAniGetRoamRssiRsp));

   if(NULL == pGetRoamRssiRspParams)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                           "%s: VOS MEM Alloc Failure", __func__);
      VOS_ASSERT(0);
      return;
   }
   vos_mem_set(pGetRoamRssiRspParams, sizeof(tAniGetRoamRssiRsp), 0);
   pGetRoamRssiRspParams->rc =
                      wdiGetRoamRssiRsp->wdiStatus;
   pGetRoamRssiRspParams->staId   = wdiGetRoamRssiRsp->ucSTAIdx;
   pGetRoamRssiRspParams->rssi = wdiGetRoamRssiRsp->rssi;

   /*                                                      */
   pGetRoamRssiRspParams->rssiReq = pGetRoamRssiReqParams;

   /*                         */
   vos_mem_free(pWdaParams->wdaWdiApiMsgParam);
   vos_mem_free(pWdaParams) ;

  /*                      */
   WDA_SendMsg(pWDA, WDA_GET_ROAM_RSSI_RSP, pGetRoamRssiRspParams , 0) ;

   return;
}



/*
                                      
                                       
 */
VOS_STATUS WDA_ProcessGetRoamRssiReq(tWDA_CbContext *pWDA,
                                    tAniGetRssiReq *pGetRoamRssiParams)
{
   WDI_Status status = WDI_STATUS_SUCCESS ;
   WDI_GetRoamRssiReqParamsType wdiGetRoamRssiParam;
   tAniGetRoamRssiRsp *pGetRoamRssiRspParams = NULL;
   tWDA_ReqParams *pWdaParams = NULL;

   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                                          "------> %s " ,__func__);
   wdiGetRoamRssiParam.wdiGetRoamRssiParamsInfo.ucSTAIdx =
                                          pGetRoamRssiParams->staId;
   wdiGetRoamRssiParam.wdiReqStatusCB = NULL ;

   pWdaParams = (tWDA_ReqParams *)vos_mem_malloc(sizeof(tWDA_ReqParams)) ;
   if(NULL == pWdaParams)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                           "%s: VOS MEM Alloc Failure", __func__);
      VOS_ASSERT(0);
      return VOS_STATUS_E_NOMEM;
   }

   /*                                                           */
   pWdaParams->pWdaContext = pWDA;

   /*                                                                           
                             */
   pWdaParams->wdaMsgParam = pGetRoamRssiParams;
   pWdaParams->wdaWdiApiMsgParam = NULL;

   status = WDI_GetRoamRssiReq(&wdiGetRoamRssiParam,
       (WDI_GetRoamRssiRspCb)WDA_GetRoamRssiReqParamsCallback, pWdaParams);
   if(IS_WDI_STATUS_FAILURE(status))
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
          "Failure in Get RoamRssi Req WDI API, free all the memory status=%d", status );
      pGetRoamRssiRspParams =
         (tAniGetRoamRssiRsp *)vos_mem_malloc(sizeof(tAniGetRoamRssiRsp));
      if(NULL == pGetRoamRssiRspParams)
      {
          VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                           "%s: VOS MEM Alloc Failure", __func__);
          VOS_ASSERT(0);
          vos_mem_free(pGetRoamRssiParams);
          vos_mem_free(pWdaParams);
          return VOS_STATUS_E_NOMEM;
      }
      pGetRoamRssiRspParams->staId = pGetRoamRssiParams->staId;
      pGetRoamRssiRspParams->rc    = eSIR_FAILURE;
      pGetRoamRssiRspParams->rssi    = 0;
      pGetRoamRssiRspParams->rssiReq = pGetRoamRssiParams;
      WDA_SendMsg(pWDA, WDA_GET_ROAM_RSSI_RSP,
                                 (void *)pGetRoamRssiRspParams, 0) ;
   }
   return CONVERT_WDI2VOS_STATUS(status);
}
#endif


/*
                                        
                                                     
 */ 
void WDA_UpdateEDCAParamCallback(WDI_Status status, void* pUserData)
{
   tWDA_ReqParams *pWdaParams = (tWDA_ReqParams *)pUserData; 
   tEdcaParams *pEdcaParams; 
   
   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                                          "<------ %s " ,__func__);
   if(NULL == pWdaParams)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                 "%s: pWdaParams received NULL", __func__);
      VOS_ASSERT(0) ;
      return ;
   }
   pEdcaParams = (tEdcaParams *)pWdaParams->wdaMsgParam ;
   vos_mem_free(pWdaParams->wdaWdiApiMsgParam) ;
   vos_mem_free(pWdaParams);
   vos_mem_free(pEdcaParams);
   return ;
}
/*
                                          
                                            
 */ 
VOS_STATUS WDA_ProcessUpdateEDCAParamReq(tWDA_CbContext *pWDA, 
                                                   tEdcaParams *pEdcaParams)
{
   WDI_Status status = WDI_STATUS_SUCCESS ;
   WDI_UpdateEDCAParamsType *wdiEdcaParam = 
                     (WDI_UpdateEDCAParamsType *)vos_mem_malloc(
                                             sizeof(WDI_UpdateEDCAParamsType)) ;
   tWDA_ReqParams *pWdaParams ;
   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                                          "------> %s " ,__func__);
   if(NULL == wdiEdcaParam) 
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                           "%s: VOS MEM Alloc Failure", __func__); 
      VOS_ASSERT(0);
      vos_mem_free(pEdcaParams);
      return VOS_STATUS_E_NOMEM;
   }
   pWdaParams = (tWDA_ReqParams *)vos_mem_malloc(sizeof(tWDA_ReqParams)) ;
   if(NULL == pWdaParams)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                           "%s: VOS MEM Alloc Failure", __func__); 
      VOS_ASSERT(0);
      vos_mem_free(wdiEdcaParam);
      vos_mem_free(pEdcaParams);
      return VOS_STATUS_E_NOMEM;
   }
   wdiEdcaParam->wdiEDCAInfo.ucBssIdx = pEdcaParams->bssIdx;
   /*
                                                                     
                                                                      
                                                                 
                                                                     
                                                           
                                                                      
                                                 
                                               
   */
   WDA_UpdateEdcaParamsForAC(pWDA, &wdiEdcaParam->wdiEDCAInfo.wdiEdcaBEInfo,
                                                           &pEdcaParams->acbe);
   WDA_UpdateEdcaParamsForAC(pWDA, &wdiEdcaParam->wdiEDCAInfo.wdiEdcaBKInfo,
                                                           &pEdcaParams->acbk);
   WDA_UpdateEdcaParamsForAC(pWDA, &wdiEdcaParam->wdiEDCAInfo.wdiEdcaVIInfo,
                                                           &pEdcaParams->acvi);
   WDA_UpdateEdcaParamsForAC(pWDA, &wdiEdcaParam->wdiEDCAInfo.wdiEdcaVOInfo,
                                                           &pEdcaParams->acvo);
   wdiEdcaParam->wdiReqStatusCB = NULL ;
   pWdaParams->pWdaContext = pWDA;
   /*                                                             */
   pWdaParams->wdaMsgParam = (void *)pEdcaParams ;
   /*                             */
   pWdaParams->wdaWdiApiMsgParam = (void *)wdiEdcaParam ;
   status = WDI_UpdateEDCAParams(wdiEdcaParam, 
               (WDI_UpdateEDCAParamsRspCb)WDA_UpdateEDCAParamCallback, pWdaParams);
   if(IS_WDI_STATUS_FAILURE(status))
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
              "Failure in Update EDCA Params WDI API, free all the memory " );
      vos_mem_free(pWdaParams->wdaWdiApiMsgParam) ;
      vos_mem_free(pWdaParams);
      vos_mem_free(pEdcaParams);
   }
   return CONVERT_WDI2VOS_STATUS(status) ;
}
/*
                                 
                             
 */ 
void WDA_AddBAReqCallback(WDI_AddBARspinfoType *pAddBARspParams, 
                                                            void* pUserData)
{
   tWDA_ReqParams *pWdaParams = (tWDA_ReqParams *)pUserData; 
   tWDA_CbContext *pWDA;
   tAddBAParams *pAddBAReqParams;
   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                                          "<------ %s " ,__func__);
   if(NULL == pWdaParams)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                "%s: pWdaParams received NULL", __func__);
      VOS_ASSERT(0) ;
      return ;
   }
   pWDA = (tWDA_CbContext *)pWdaParams->pWdaContext;
   pAddBAReqParams = (tAddBAParams *)pWdaParams->wdaMsgParam;
   vos_mem_free(pWdaParams->wdaWdiApiMsgParam) ;
   vos_mem_free(pWdaParams);
   pAddBAReqParams->status = pAddBARspParams->wdiStatus ;
   WDA_SendMsg(pWDA, WDA_ADDBA_RSP, (void *)pAddBAReqParams , 0) ;
   return ;
}

/*
                                
                                                 
 */ 
VOS_STATUS WDA_ProcessAddBAReq(tWDA_CbContext *pWDA, VOS_STATUS status,
           tANI_U16 baSessionID, tANI_U8 staIdx, tAddBAParams *pAddBAReqParams)
{
   WDI_Status wstatus;
   WDI_AddBAReqParamsType *wdiAddBAReqParam = 
                     (WDI_AddBAReqParamsType *)vos_mem_malloc(
                                             sizeof(WDI_AddBAReqParamsType)) ;
   tWDA_ReqParams *pWdaParams ;
   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                                          "------> %s " ,__func__);
   if(NULL == wdiAddBAReqParam) 
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                           "%s: VOS MEM Alloc Failure", __func__); 
      VOS_ASSERT(0);
      return VOS_STATUS_E_NOMEM;
   }
   pWdaParams = (tWDA_ReqParams *)vos_mem_malloc(sizeof(tWDA_ReqParams)) ;
   if(NULL == pWdaParams)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                           "%s: VOS MEM Alloc Failure", __func__); 
      VOS_ASSERT(0);
      vos_mem_free(wdiAddBAReqParam);
      return VOS_STATUS_E_NOMEM;
   }
   do
   {
      WDI_AddBAReqinfoType *wdiAddBaInfo = &wdiAddBAReqParam->wdiBAInfoType ;
      wdiAddBaInfo->ucSTAIdx = staIdx ;
      wdiAddBaInfo->ucBaSessionID = baSessionID ;
      wdiAddBaInfo->ucWinSize     = WDA_BA_MAX_WINSIZE ;
   } while(0) ;
   wdiAddBAReqParam->wdiReqStatusCB = NULL ;
   pWdaParams->pWdaContext = pWDA;
   /*                             */
   pWdaParams->wdaWdiApiMsgParam = (void *)wdiAddBAReqParam ;
   pWdaParams->wdaMsgParam = pAddBAReqParams;
   wstatus = WDI_AddBAReq(wdiAddBAReqParam, 
                          (WDI_AddBARspCb)WDA_AddBAReqCallback, pWdaParams);

   if(IS_WDI_STATUS_FAILURE(wstatus))
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
              "Failure in ADD BA REQ Params WDI API, free all the memory" );
      status = CONVERT_WDI2VOS_STATUS(wstatus);
      vos_mem_free(pWdaParams->wdaWdiApiMsgParam);
      vos_mem_free(pWdaParams);
      pAddBAReqParams->status = eSIR_FAILURE;
      WDA_SendMsg(pWDA, WDA_ADDBA_RSP, (void *)pAddBAReqParams , 0) ;
   }
   return status;
}
/*
                                        
                                             
 */ 
void WDA_AddBASessionReqCallback(
              WDI_AddBASessionRspParamsType *wdiAddBaSession, void* pUserData)
{
   VOS_STATUS status = VOS_STATUS_SUCCESS ;
   tWDA_ReqParams *pWdaParams = (tWDA_ReqParams *)pUserData; 
   tWDA_CbContext *pWDA; 
   tAddBAParams *pAddBAReqParams ;
   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                                          "<------ %s " ,__func__);
   if(NULL == pWdaParams)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
               "%s: pWdaParams received NULL", __func__);
      VOS_ASSERT(0) ;
      return ;
   }
   pWDA = (tWDA_CbContext *)pWdaParams->pWdaContext;
   pAddBAReqParams = (tAddBAParams *)pWdaParams->wdaMsgParam;
   if( NULL == pAddBAReqParams )
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                                          "%s: pAddBAReqParams received NULL " ,__func__);
      VOS_ASSERT( 0 );
      vos_mem_free(pWdaParams->wdaWdiApiMsgParam) ;
      vos_mem_free(pWdaParams);
      return ;
   }
   vos_mem_free(pWdaParams->wdaWdiApiMsgParam) ;
   vos_mem_free(pWdaParams);
   /* 
                                                                          
                                               
    */
   
   if((VOS_STATUS_SUCCESS == 
                       CONVERT_WDI2VOS_STATUS(wdiAddBaSession->wdiStatus)) && 
                                 (WDA_BA_UPDATE_TL_STATE == pWDA->wdaState))
   {
      /*                                              */
      status =  WDA_TL_BA_SESSION_ADD(pWDA->pVosContext,
                                        wdiAddBaSession->usBaSessionID,
                                        wdiAddBaSession->ucSTAIdx,
                                        wdiAddBaSession->ucBaTID,
                                        wdiAddBaSession->ucBaBufferSize,
                                        wdiAddBaSession->ucWinSize,
                                        wdiAddBaSession->usBaSSN );
      WDA_ProcessAddBAReq(pWDA, status, wdiAddBaSession->usBaSessionID, 
                                      wdiAddBaSession->ucSTAIdx, pAddBAReqParams) ;
   }
   else
   {
      pAddBAReqParams->status = 
            CONVERT_WDI2SIR_STATUS(wdiAddBaSession->wdiStatus) ;
  
      /*                                                 */
      if(WDI_STATUS_SUCCESS == wdiAddBaSession->wdiStatus)
      {
         tANI_U16 curSta = wdiAddBaSession->ucSTAIdx;
         tANI_U8 tid = wdiAddBaSession->ucBaTID;
         WDA_SET_BA_TXFLAG(pWDA, curSta, tid) ;
      }
      WDA_SendMsg(pWDA, WDA_ADDBA_RSP, (void *)pAddBAReqParams , 0) ;
   }
   /*                             */
   pWDA->wdaState = WDA_READY_STATE;
   return ;
}

/*
                                       
                                                 
 */ 
VOS_STATUS WDA_ProcessAddBASessionReq(tWDA_CbContext *pWDA, 
                                         tAddBAParams *pAddBAReqParams)
{
   WDI_Status status = WDI_STATUS_SUCCESS ;
   WDI_AddBASessionReqParamsType *wdiAddBASessionReqParam = 
                     (WDI_AddBASessionReqParamsType *)vos_mem_malloc(
                          sizeof(WDI_AddBASessionReqParamsType)) ;
   tWDA_ReqParams *pWdaParams ;
   WLANTL_STAStateType tlSTAState = 0;

   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                                          "------> %s " ,__func__);
   if(NULL == wdiAddBASessionReqParam) 
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                           "%s: VOS MEM Alloc Failure", __func__); 
      VOS_ASSERT(0);
      return VOS_STATUS_E_NOMEM;
   }
   pWdaParams = (tWDA_ReqParams *)vos_mem_malloc(sizeof(tWDA_ReqParams)) ;
   if(NULL == pWdaParams)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                           "%s: VOS MEM Alloc Failure", __func__); 
      VOS_ASSERT(0);
      vos_mem_free(wdiAddBASessionReqParam);
      return VOS_STATUS_E_NOMEM;
   }
   /*
                                                                  
                                                                          
                                                                       
           
    */
   do
   {
      WDI_AddBASessionReqinfoType *wdiBAInfoType = 
                            &wdiAddBASessionReqParam->wdiBASessionInfoType ;
      /*                                       
                                                                         */
      wdiBAInfoType->ucSTAIdx = pAddBAReqParams->staIdx;
      vos_mem_copy(wdiBAInfoType->macPeerAddr,
                       pAddBAReqParams->peerMacAddr, sizeof(tSirMacAddr));
      wdiBAInfoType->ucBaTID = pAddBAReqParams->baTID;
      wdiBAInfoType->ucBaPolicy = pAddBAReqParams->baPolicy;
      wdiBAInfoType->usBaBufferSize = pAddBAReqParams->baBufferSize;
      wdiBAInfoType->usBaTimeout = pAddBAReqParams->baTimeout;
      wdiBAInfoType->usBaSSN = pAddBAReqParams->baSSN;
      wdiBAInfoType->ucBaDirection = pAddBAReqParams->baDirection;
      /*                                                     */
      (eBA_RECIPIENT == wdiBAInfoType->ucBaDirection) 
                                 ? (pWDA->wdaState = WDA_BA_UPDATE_TL_STATE)
                                 : (pWDA->wdaState = WDA_BA_UPDATE_LIM_STATE);
 
   }while(0) ;
   wdiAddBASessionReqParam->wdiReqStatusCB = NULL ;
   pWdaParams->pWdaContext = pWDA;
   /*                                                         */
   pWdaParams->wdaMsgParam = (void *)pAddBAReqParams ;
   /*                             */
   pWdaParams->wdaWdiApiMsgParam = (void *)wdiAddBASessionReqParam ;

   /*                                                                             
                                                                                       
   */
   if((VOS_STATUS_SUCCESS != WDA_TL_GET_STA_STATE(pWDA->pVosContext, pAddBAReqParams->staIdx, &tlSTAState)) ||
    ((WLANTL_STA_CONNECTED != tlSTAState) && (WLANTL_STA_AUTHENTICATED != tlSTAState)))
   {
       VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
        "Peer staIdx %d hasn't established yet(%d). Send ADD BA failure to PE.", pAddBAReqParams->staIdx, tlSTAState );
       status = WDI_STATUS_E_NOT_ALLOWED;
       pAddBAReqParams->status =
             CONVERT_WDI2SIR_STATUS(status) ;
       WDA_SendMsg(pWDA, WDA_ADDBA_RSP, (void *)pAddBAReqParams , 0) ;
       /*                             */
       pWDA->wdaState = WDA_READY_STATE;
       vos_mem_free(pWdaParams->wdaWdiApiMsgParam) ;
       vos_mem_free(pWdaParams);

       return CONVERT_WDI2VOS_STATUS(status) ;
   }

   status = WDI_AddBASessionReq(wdiAddBASessionReqParam, 
              (WDI_AddBASessionRspCb)WDA_AddBASessionReqCallback, pWdaParams);
   if(IS_WDI_STATUS_FAILURE(status))
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
       "Failure in ADD BA Session REQ Params WDI API, free all the memory =%d", status);
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
       "Send ADD BA failure response to PE");
      pAddBAReqParams->status =
            CONVERT_WDI2SIR_STATUS(status) ;
      WDA_SendMsg(pWDA, WDA_ADDBA_RSP, (void *)pAddBAReqParams , 0) ;
      /*                             */
      pWDA->wdaState = WDA_READY_STATE;
      vos_mem_free(pWdaParams->wdaWdiApiMsgParam) ;
      vos_mem_free(pWdaParams);
   }
   return CONVERT_WDI2VOS_STATUS(status) ;
}
/*
                              
                        
 */ 
void WDA_DelBANotifyTL(tWDA_CbContext *pWDA, 
                                           tDelBAParams *pDelBAReqParams)
{
   tpDelBAInd pDelBAInd = (tpDelBAInd)vos_mem_malloc(sizeof( tDelBAInd ));
   //             
   vos_msg_t vosMsg;
   VOS_STATUS vosStatus = VOS_STATUS_SUCCESS;
   if(NULL == pDelBAInd) 
   { 
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                   "%s: VOS MEM Alloc Failure", __func__); 
      VOS_ASSERT(0) ; 
      return; 
   } 
   pDelBAInd->mesgType = WDA_DELETEBA_IND;
   pDelBAInd->staIdx = (tANI_U8) pDelBAReqParams->staIdx;
   pDelBAInd->baTID = (tANI_U8) pDelBAReqParams->baTID;
   pDelBAInd->mesgLen = sizeof( tDelBAInd );
 
 
   vosMsg.type = WDA_DELETEBA_IND;
   vosMsg.bodyptr = pDelBAInd;
   vosStatus = vos_mq_post_message(VOS_MQ_ID_TL, &vosMsg);
   if ( !VOS_IS_STATUS_SUCCESS(vosStatus) )
   {
      vosStatus = VOS_STATUS_E_BADMSG;
   }
}
/*
                                 
                             
 */ 
void WDA_DelBAReqCallback(WDI_Status status, void* pUserData)
{
   tWDA_ReqParams *pWdaParams = (tWDA_ReqParams *)pUserData; 
   tWDA_CbContext *pWDA; 
   tDelBAParams *pDelBAReqParams;
   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                                          "<------ %s " ,__func__);
   if(NULL == pWdaParams)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
              "%s: pWdaParams received NULL", __func__);
      VOS_ASSERT(0) ;
      return ;
   }
   pWDA = (tWDA_CbContext *)pWdaParams->pWdaContext;
   pDelBAReqParams = (tDelBAParams *)pWdaParams->wdaMsgParam ;
   /*                                             */
   if((VOS_STATUS_SUCCESS == CONVERT_WDI2VOS_STATUS(status)) && 
                             (eBA_RECIPIENT == pDelBAReqParams->baDirection))
   {
      WDA_DelBANotifyTL(pWDA, pDelBAReqParams);
   }
   /* 
                                                                     
                
    */
   vos_mem_free(pDelBAReqParams);
   vos_mem_free(pWdaParams->wdaWdiApiMsgParam);
   vos_mem_free(pWdaParams);
   return ;
}

/*
                                
                                                 
 */ 
VOS_STATUS WDA_ProcessDelBAReq(tWDA_CbContext *pWDA, 
                                                tDelBAParams *pDelBAReqParams)
{
   WDI_Status status = WDI_STATUS_SUCCESS ;
   WDI_DelBAReqParamsType *wdiDelBAReqParam = 
                     (WDI_DelBAReqParamsType *)vos_mem_malloc(
                                             sizeof(WDI_DelBAReqParamsType)) ;
   tWDA_ReqParams *pWdaParams ;
   tANI_U16 staIdx = 0;
   tANI_U8 tid = 0;
   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                                          "------> %s " ,__func__);
   if(NULL == wdiDelBAReqParam) 
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                           "%s: VOS MEM Alloc Failure", __func__); 
      VOS_ASSERT(0);
      return VOS_STATUS_E_NOMEM;
   }
   pWdaParams = (tWDA_ReqParams *)vos_mem_malloc(sizeof(tWDA_ReqParams)) ;
   if(NULL == pWdaParams)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                           "%s: VOS MEM Alloc Failure", __func__); 
      VOS_ASSERT(0);
      vos_mem_free(wdiDelBAReqParam);
      return VOS_STATUS_E_NOMEM;
   }
   wdiDelBAReqParam->wdiBAInfo.ucSTAIdx = pDelBAReqParams->staIdx;
   wdiDelBAReqParam->wdiBAInfo.ucBaTID = pDelBAReqParams->baTID;
   wdiDelBAReqParam->wdiBAInfo.ucBaDirection = pDelBAReqParams->baDirection;
   wdiDelBAReqParam->wdiReqStatusCB = NULL ;
   pWdaParams->pWdaContext = pWDA;
   /*                                                         */
   pWdaParams->wdaMsgParam = (void *)pDelBAReqParams ;
   /*                             */
   pWdaParams->wdaWdiApiMsgParam = (void *)wdiDelBAReqParam ;
   /*                                                                  
                                                                     
    */
   staIdx = pDelBAReqParams->staIdx;
   tid = pDelBAReqParams->baTID;
   WDA_CLEAR_BA_TXFLAG(pWDA, staIdx, tid);
   status = WDI_DelBAReq(wdiDelBAReqParam, 
                         (WDI_DelBARspCb)WDA_DelBAReqCallback, pWdaParams);
   if(IS_WDI_STATUS_FAILURE(status))
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
              "Failure in DEL BA REQ Params WDI API, free all the memory " );
      vos_mem_free(pWdaParams->wdaWdiApiMsgParam) ;
      vos_mem_free(pWdaParams->wdaMsgParam);
      vos_mem_free(pWdaParams);
   }
   return CONVERT_WDI2VOS_STATUS(status) ;
}

/*
                                    
  
 */
void WDA_UpdateChReqCallback(WDI_Status status, void* pUserData)
{
   tWDA_ReqParams *pWdaParams;
   WDI_UpdateChReqParamsType *pwdiUpdateChReqParam;
   WDI_UpdateChannelReqType *pwdiUpdateChanReqType;
   WDI_UpdateChannelReqinfoType *pChanInfoType;
   tSirUpdateChanList *pChanList;

   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                                          "<------ %s " ,__func__);
   if(NULL == pUserData)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
              "%s: pUserData received NULL", __func__);
      VOS_ASSERT(0);
      return;
   }

   pWdaParams = (tWDA_ReqParams *)pUserData;
   pwdiUpdateChReqParam =
       (WDI_UpdateChReqParamsType *)pWdaParams->wdaWdiApiMsgParam;
   pwdiUpdateChanReqType = &pwdiUpdateChReqParam->wdiUpdateChanParams;
   pChanInfoType = pwdiUpdateChanReqType->pchanParam;
   pChanList = (tSirUpdateChanList *)pWdaParams->wdaMsgParam;
   /*
                                                                       
                                                        
    */
   vos_mem_free(pChanInfoType);
   vos_mem_free(pChanList);
   vos_mem_free(pWdaParams->wdaWdiApiMsgParam);
   vos_mem_free(pWdaParams);

   return;
}

/*
                                         
                                                   
 */
VOS_STATUS WDA_ProcessUpdateChannelList(tWDA_CbContext *pWDA,
        tSirUpdateChanList *pChanList)
{
   WDI_Status status = WDI_STATUS_SUCCESS;
   WDI_UpdateChReqParamsType *pwdiUpdateChReqParam;
   WDI_UpdateChannelReqType *pwdiUpdateChanReqType;
   WDI_UpdateChannelReqinfoType *pChanInfoType;
   tWDA_ReqParams *pWdaParams;
   wpt_uint8 i;

   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                                          "------> %s " ,__func__);
   if(NULL == pChanList)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
              "%s: NULL pChanList", __func__);
      VOS_ASSERT(0);
      return VOS_STATUS_E_INVAL;
   }

   if(!WDA_getFwWlanFeatCaps(UPDATE_CHANNEL_LIST))
   {
       VOS_TRACE(VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
               "Update channel list capability Not Supported");
       vos_mem_free(pChanList);
       return VOS_STATUS_E_INVAL;
   }

   pwdiUpdateChReqParam = (WDI_UpdateChReqParamsType *)vos_mem_malloc(
           sizeof(WDI_UpdateChReqParamsType));
   if(NULL == pwdiUpdateChReqParam)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
              "%s: VOS MEM Alloc Failed for WDI_UpdateChReqParamsType",
              __func__);
      VOS_ASSERT(0);
      vos_mem_free(pChanList);
      return VOS_STATUS_E_NOMEM;
   }
   pwdiUpdateChanReqType = &pwdiUpdateChReqParam->wdiUpdateChanParams;
   pChanInfoType = (WDI_UpdateChannelReqinfoType *)
       vos_mem_malloc(sizeof(WDI_UpdateChannelReqinfoType) *
               pChanList->numChan);
   if(NULL == pChanInfoType)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                           "%s: VOS MEM Alloc Failure", __func__);
      VOS_ASSERT(0);
      vos_mem_free(pChanList);
      vos_mem_free(pwdiUpdateChReqParam);
      return VOS_STATUS_E_NOMEM;
   }
   vos_mem_zero(pChanInfoType, sizeof(WDI_UpdateChannelReqinfoType)
           * pChanList->numChan);
   pwdiUpdateChanReqType->pchanParam = pChanInfoType;

   pWdaParams = (tWDA_ReqParams *)vos_mem_malloc(sizeof(tWDA_ReqParams));
   if(NULL == pWdaParams)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                           "%s: VOS MEM Alloc Failure", __func__);
      VOS_ASSERT(0);
      vos_mem_free(pChanList);
      vos_mem_free(pChanInfoType);
      vos_mem_free(pwdiUpdateChReqParam);
      return VOS_STATUS_E_NOMEM;
   }
   pwdiUpdateChanReqType->numchan = pChanList->numChan;

   for(i = 0; i < pwdiUpdateChanReqType->numchan; i++)
   {
       pChanInfoType->mhz =
           vos_chan_to_freq(pChanList->chanParam[i].chanId);

       pChanInfoType->band_center_freq1 = pChanInfoType->mhz;
       pChanInfoType->band_center_freq2 = 0;

       VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
               "chan[%d] = %u", i, pChanInfoType->mhz);
       if (pChanList->chanParam[i].dfsSet)
       {
           WDA_SET_CHANNEL_FLAG(pChanInfoType, WLAN_HAL_CHAN_FLAG_PASSIVE);
           VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                   "chan[%d] DFS[%d]", pChanList->chanParam[i].chanId,
                   pChanList->chanParam[i].dfsSet);
       }

       if (pChanInfoType->mhz < WDA_2_4_GHZ_MAX_FREQ)
       {
           WDA_SET_CHANNEL_MODE(pChanInfoType, MODE_11G);
       }
       else
       {
           WDA_SET_CHANNEL_MODE(pChanInfoType, MODE_11A);
           WDA_SET_CHANNEL_FLAG(pChanInfoType, WLAN_HAL_CHAN_FLAG_ALLOW_HT);
           WDA_SET_CHANNEL_FLAG(pChanInfoType, WLAN_HAL_CHAN_FLAG_ALLOW_VHT);
       }

       WDA_SET_CHANNEL_MAX_POWER(pChanInfoType, pChanList->chanParam[i].pwr);
       WDA_SET_CHANNEL_REG_POWER(pChanInfoType, pChanList->chanParam[i].pwr);

       pChanInfoType++;
   }

   pwdiUpdateChReqParam->wdiReqStatusCB = NULL;
   pWdaParams->pWdaContext = pWDA;
   pWdaParams->wdaMsgParam = (void *)pChanList;
   /*                             */
   pWdaParams->wdaWdiApiMsgParam = (void *)pwdiUpdateChReqParam;
   status = WDI_UpdateChannelReq(pwdiUpdateChReqParam,
           (WDI_UpdateChannelRspCb)WDA_UpdateChReqCallback, pWdaParams);
   if(IS_WDI_STATUS_FAILURE(status))
   {
      VOS_TRACE(VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
              "Failure in Update Channel REQ Params WDI API, free all the memory");
      vos_mem_free(pwdiUpdateChanReqType->pchanParam);
      vos_mem_free(pwdiUpdateChReqParam);
      vos_mem_free(pWdaParams->wdaMsgParam);
      vos_mem_free(pWdaParams);
   }
   return CONVERT_WDI2VOS_STATUS(status);
}

/*
                                 
                             
 */ 
void WDA_AddTSReqCallback(WDI_Status status, void* pUserData)
{
   tWDA_ReqParams *pWdaParams = (tWDA_ReqParams *)pUserData; 
   tWDA_CbContext *pWDA; 
   tAddTsParams *pAddTsReqParams;
   
   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                                          "<------ %s " ,__func__);
   if(NULL == pWdaParams)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
              "%s: pWdaParams received NULL", __func__);
      VOS_ASSERT(0) ;
      return ;
   }
   pWDA = (tWDA_CbContext *) pWdaParams->pWdaContext;
   pAddTsReqParams = (tAddTsParams *)pWdaParams->wdaMsgParam ;
   vos_mem_free(pWdaParams->wdaWdiApiMsgParam) ;
   vos_mem_free(pWdaParams);
   
   pAddTsReqParams->status = status ;
   WDA_SendMsg(pWDA, WDA_ADD_TS_RSP, (void *)pAddTsReqParams , 0) ;
   return ;
}

/*
                                
                                                   
 */ 
VOS_STATUS WDA_ProcessAddTSReq(tWDA_CbContext *pWDA, 
                                                tAddTsParams *pAddTsReqParams)
{
   WDI_Status status = WDI_STATUS_SUCCESS ;
   WDI_AddTSReqParamsType *wdiAddTSReqParam = 
                     (WDI_AddTSReqParamsType *)vos_mem_malloc(
                                             sizeof(WDI_AddTSReqParamsType)) ;
   tWDA_ReqParams *pWdaParams ;
   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                                          "------> %s " ,__func__);
   if(NULL == wdiAddTSReqParam) 
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                           "%s: VOS MEM Alloc Failure", __func__); 
      VOS_ASSERT(0);
      return VOS_STATUS_E_NOMEM;
   }
   pWdaParams = (tWDA_ReqParams *)vos_mem_malloc(sizeof(tWDA_ReqParams)) ;
   if(NULL == pWdaParams)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                           "%s: VOS MEM Alloc Failure", __func__); 
      VOS_ASSERT(0);
      vos_mem_free(wdiAddTSReqParam);
      return VOS_STATUS_E_NOMEM;
   }
   wdiAddTSReqParam->wdiTsInfo.ucSTAIdx = pAddTsReqParams->staIdx;
   wdiAddTSReqParam->wdiTsInfo.ucTspecIdx = pAddTsReqParams->tspecIdx;
   //     
   wdiAddTSReqParam->wdiTsInfo.wdiTspecIE.ucType = pAddTsReqParams->tspec.type;
   wdiAddTSReqParam->wdiTsInfo.wdiTspecIE.ucLength = 
                                                pAddTsReqParams->tspec.length;
   
   //                         
   wdiAddTSReqParam->wdiTsInfo.wdiTspecIE.wdiTSinfo.wdiTraffic.ackPolicy =
                           pAddTsReqParams->tspec.tsinfo.traffic.ackPolicy;
   wdiAddTSReqParam->wdiTsInfo.wdiTspecIE.wdiTSinfo.wdiTraffic.userPrio =
                           pAddTsReqParams->tspec.tsinfo.traffic.userPrio;
   wdiAddTSReqParam->wdiTsInfo.wdiTspecIE.wdiTSinfo.wdiTraffic.psb =
                           pAddTsReqParams->tspec.tsinfo.traffic.psb;
   wdiAddTSReqParam->wdiTsInfo.wdiTspecIE.wdiTSinfo.wdiTraffic.aggregation =
                           pAddTsReqParams->tspec.tsinfo.traffic.aggregation;
   wdiAddTSReqParam->wdiTsInfo.wdiTspecIE.wdiTSinfo.wdiTraffic.accessPolicy =
                           pAddTsReqParams->tspec.tsinfo.traffic.accessPolicy;
   wdiAddTSReqParam->wdiTsInfo.wdiTspecIE.wdiTSinfo.wdiTraffic.direction =
                           pAddTsReqParams->tspec.tsinfo.traffic.direction;
   wdiAddTSReqParam->wdiTsInfo.wdiTspecIE.wdiTSinfo.wdiTraffic.tsid =
                           pAddTsReqParams->tspec.tsinfo.traffic.tsid;
   wdiAddTSReqParam->wdiTsInfo.wdiTspecIE.wdiTSinfo.wdiTraffic.trafficType =
                           pAddTsReqParams->tspec.tsinfo.traffic.trafficType;
   
   //                          
   wdiAddTSReqParam->wdiTsInfo.wdiTspecIE.wdiTSinfo.wdiSchedule.schedule = 
                           pAddTsReqParams->tspec.tsinfo.schedule.schedule;
   wdiAddTSReqParam->wdiTsInfo.wdiTspecIE.wdiTSinfo.wdiSchedule.rsvd = 
                           pAddTsReqParams->tspec.tsinfo.schedule.rsvd;
   //     
   wdiAddTSReqParam->wdiTsInfo.wdiTspecIE.usNomMsduSz = 
                           pAddTsReqParams->tspec.nomMsduSz;
   wdiAddTSReqParam->wdiTsInfo.wdiTspecIE.usMaxMsduSz = 
                           pAddTsReqParams->tspec.maxMsduSz;
   wdiAddTSReqParam->wdiTsInfo.wdiTspecIE.uMinSvcInterval = 
                           pAddTsReqParams->tspec.minSvcInterval;
   wdiAddTSReqParam->wdiTsInfo.wdiTspecIE.uMaxSvcInterval = 
                           pAddTsReqParams->tspec.maxSvcInterval;
   wdiAddTSReqParam->wdiTsInfo.wdiTspecIE.uInactInterval = 
                           pAddTsReqParams->tspec.inactInterval;
   wdiAddTSReqParam->wdiTsInfo.wdiTspecIE.uSuspendInterval = 
                           pAddTsReqParams->tspec.suspendInterval;
   wdiAddTSReqParam->wdiTsInfo.wdiTspecIE.uSvcStartTime = 
                           pAddTsReqParams->tspec.svcStartTime;
   wdiAddTSReqParam->wdiTsInfo.wdiTspecIE.uMinDataRate = 
                           pAddTsReqParams->tspec.minDataRate;
   wdiAddTSReqParam->wdiTsInfo.wdiTspecIE.uMeanDataRate = 
                           pAddTsReqParams->tspec.meanDataRate;
   wdiAddTSReqParam->wdiTsInfo.wdiTspecIE.uPeakDataRate = 
                           pAddTsReqParams->tspec.peakDataRate;
   wdiAddTSReqParam->wdiTsInfo.wdiTspecIE.uMaxBurstSz = 
                           pAddTsReqParams->tspec.maxBurstSz;
   wdiAddTSReqParam->wdiTsInfo.wdiTspecIE.uDelayBound = 
                           pAddTsReqParams->tspec.delayBound;
   wdiAddTSReqParam->wdiTsInfo.wdiTspecIE.uMinPhyRate = 
                           pAddTsReqParams->tspec.minPhyRate;
   wdiAddTSReqParam->wdiTsInfo.wdiTspecIE.usSurplusBw = 
                           pAddTsReqParams->tspec.surplusBw;
   wdiAddTSReqParam->wdiTsInfo.wdiTspecIE.usMediumTime = 
                           pAddTsReqParams->tspec.mediumTime;
   /*                                                      */
#if 0 
   wdiAddTSReqParam->wdiTsInfo.ucUapsdFlags = 
   wdiAddTSReqParam->wdiTsInfo.ucServiceInterval = 
   wdiAddTSReqParam->wdiTsInfo.ucSuspendInterval = 
   wdiAddTSReqParam->wdiTsInfo.ucDelayedInterval = 
#endif
   wdiAddTSReqParam->wdiReqStatusCB = NULL ;
   
   pWdaParams->pWdaContext = pWDA;
   /*                                                         */
   pWdaParams->wdaMsgParam = (void *)pAddTsReqParams ;
   /*                             */
   pWdaParams->wdaWdiApiMsgParam = (void *)wdiAddTSReqParam ;
   status = WDI_AddTSReq(wdiAddTSReqParam, 
                   (WDI_AddTsRspCb)WDA_AddTSReqCallback, pWdaParams);
   if(IS_WDI_STATUS_FAILURE(status))
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
              "Failure in ADD TS REQ Params WDI API, free all the memory " );
      vos_mem_free(pWdaParams->wdaWdiApiMsgParam) ;
      vos_mem_free(pWdaParams);
      pAddTsReqParams->status = eSIR_FAILURE ;
      WDA_SendMsg(pWDA, WDA_ADD_TS_RSP, (void *)pAddTsReqParams , 0) ;
   }
   return CONVERT_WDI2VOS_STATUS(status) ;
}

/*
                                 
                             
 */ 
void WDA_DelTSReqCallback(WDI_Status status, void* pUserData)
{
   tWDA_ReqParams *pWdaParams = (tWDA_ReqParams *)pUserData; 
   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                                          "<------ %s " ,__func__);
   vos_mem_free(pWdaParams->wdaWdiApiMsgParam) ;
   vos_mem_free(pWdaParams->wdaMsgParam) ;
   vos_mem_free(pWdaParams);
   /* 
                                                                      
                
    */
   return ;
}

/*
                                
                                                 
 */ 
VOS_STATUS WDA_ProcessDelTSReq(tWDA_CbContext *pWDA, 
                                                 tDelTsParams *pDelTSReqParams)
{
   WDI_Status status = WDI_STATUS_SUCCESS ;
   WDI_DelTSReqParamsType *wdiDelTSReqParam = 
                     (WDI_DelTSReqParamsType *)vos_mem_malloc(
                                             sizeof(WDI_DelTSReqParamsType)) ;
   tWDA_ReqParams *pWdaParams ;
   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                                          "------> %s " ,__func__);
   if(NULL == wdiDelTSReqParam) 
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                           "%s: VOS MEM Alloc Failure", __func__); 
      VOS_ASSERT(0);
      return VOS_STATUS_E_NOMEM;
   }
   pWdaParams = (tWDA_ReqParams *)vos_mem_malloc(sizeof(tWDA_ReqParams)) ;
   if(NULL == pWdaParams)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                           "%s: VOS MEM Alloc Failure", __func__); 
      VOS_ASSERT(0);
      vos_mem_free(wdiDelTSReqParam);
      return VOS_STATUS_E_NOMEM;
   }
   vos_mem_copy(wdiDelTSReqParam->wdiDelTSInfo.macBSSID, 
                                  pDelTSReqParams->bssId, sizeof(tSirMacAddr));
   wdiDelTSReqParam->wdiDelTSInfo.ucSTAIdx = pDelTSReqParams->staIdx;
   wdiDelTSReqParam->wdiDelTSInfo.ucTspecIdx = pDelTSReqParams->tspecIdx;
   wdiDelTSReqParam->wdiReqStatusCB = NULL ;
   pWdaParams->pWdaContext = pWDA;
   /*                                                         */
   pWdaParams->wdaMsgParam = (void *)pDelTSReqParams ;
   /*                             */
   pWdaParams->wdaWdiApiMsgParam = (void *)wdiDelTSReqParam ;
   status = WDI_DelTSReq(wdiDelTSReqParam, 
                       (WDI_DelTsRspCb)WDA_DelTSReqCallback, pWdaParams);
   if(IS_WDI_STATUS_FAILURE(status))
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
              "Failure in DEL TS REQ Params WDI API, free all the memory " );
      vos_mem_free(pWdaParams->wdaWdiApiMsgParam) ;
      vos_mem_free(pWdaParams->wdaMsgParam);
      vos_mem_free(pWdaParams);
   }
   return CONVERT_WDI2VOS_STATUS(status) ;
}
/*
                                           
                                                                    
 */ 
void WDA_UpdateBeaconParamsCallback(WDI_Status status, void* pUserData)
{
   tWDA_ReqParams *pWdaParams = (tWDA_ReqParams *)pUserData; 
   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                                          "<------ %s " ,__func__);
   if(NULL == pWdaParams)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
              "%s: pWdaParams received NULL", __func__);
      VOS_ASSERT(0) ;
      return ;
   }
   vos_mem_free(pWdaParams->wdaWdiApiMsgParam) ;
   vos_mem_free(pWdaParams->wdaMsgParam) ;
   vos_mem_free(pWdaParams);
   /* 
                                                                             
                
    */
   return ;
}
/*
                                          
                                                                              
 */ 
VOS_STATUS WDA_ProcessUpdateBeaconParams(tWDA_CbContext *pWDA, 
                                    tUpdateBeaconParams *pUpdateBeaconParams)
{
   WDI_Status status = WDI_STATUS_SUCCESS ;
   WDI_UpdateBeaconParamsType *wdiUpdateBeaconParams = 
                     (WDI_UpdateBeaconParamsType *)vos_mem_malloc(
                                             sizeof(WDI_UpdateBeaconParamsType)) ;
   tWDA_ReqParams *pWdaParams ;
   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                                          "------> %s " ,__func__);
   if(NULL == wdiUpdateBeaconParams) 
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                           "%s: VOS MEM Alloc Failure", __func__); 
      VOS_ASSERT(0);
      return VOS_STATUS_E_NOMEM;
   }
   pWdaParams = (tWDA_ReqParams *)vos_mem_malloc(sizeof(tWDA_ReqParams)) ;
   if(NULL == pWdaParams)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                           "%s: VOS MEM Alloc Failure", __func__); 
      VOS_ASSERT(0);
      vos_mem_free(wdiUpdateBeaconParams);
      return VOS_STATUS_E_NOMEM;
   }
   wdiUpdateBeaconParams->wdiUpdateBeaconParamsInfo.ucBssIdx = 
                           pUpdateBeaconParams->bssIdx;
   wdiUpdateBeaconParams->wdiUpdateBeaconParamsInfo.ucfShortPreamble = 
                           pUpdateBeaconParams->fShortPreamble;
   wdiUpdateBeaconParams->wdiUpdateBeaconParamsInfo.ucfShortSlotTime = 
                           pUpdateBeaconParams->fShortSlotTime;
   wdiUpdateBeaconParams->wdiUpdateBeaconParamsInfo.usBeaconInterval = 
                           pUpdateBeaconParams->beaconInterval;
   wdiUpdateBeaconParams->wdiUpdateBeaconParamsInfo.ucllaCoexist = 
                           pUpdateBeaconParams->llaCoexist;
   wdiUpdateBeaconParams->wdiUpdateBeaconParamsInfo.ucllbCoexist = 
                           pUpdateBeaconParams->llbCoexist;
   wdiUpdateBeaconParams->wdiUpdateBeaconParamsInfo.ucllgCoexist = 
                           pUpdateBeaconParams->llgCoexist;
   wdiUpdateBeaconParams->wdiUpdateBeaconParamsInfo.ucHt20MhzCoexist= 
                           pUpdateBeaconParams->ht20MhzCoexist;
   wdiUpdateBeaconParams->wdiUpdateBeaconParamsInfo.ucllnNonGFCoexist =
                           pUpdateBeaconParams->llnNonGFCoexist;
   wdiUpdateBeaconParams->wdiUpdateBeaconParamsInfo.ucfLsigTXOPProtectionFullSupport = 
                           pUpdateBeaconParams->fLsigTXOPProtectionFullSupport;
   wdiUpdateBeaconParams->wdiUpdateBeaconParamsInfo.ucfRIFSMode =
                           pUpdateBeaconParams->fRIFSMode;
   wdiUpdateBeaconParams->wdiUpdateBeaconParamsInfo.usChangeBitmap =
                           pUpdateBeaconParams->paramChangeBitmap;
   wdiUpdateBeaconParams->wdiReqStatusCB = NULL ;
   
   pWdaParams->pWdaContext = pWDA;
   /*                                                                   */
   pWdaParams->wdaMsgParam = (void *)pUpdateBeaconParams ;
   /*                             */
   pWdaParams->wdaWdiApiMsgParam = (void *)wdiUpdateBeaconParams ;
   status = WDI_UpdateBeaconParamsReq(wdiUpdateBeaconParams, 
                 (WDI_UpdateBeaconParamsRspCb)WDA_UpdateBeaconParamsCallback,
                 pWdaParams);
   if(IS_WDI_STATUS_FAILURE(status))
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
        "Failure in UPDATE BEACON REQ Params WDI API, free all the memory " );
      vos_mem_free(pWdaParams->wdaWdiApiMsgParam) ;
      vos_mem_free(pWdaParams->wdaMsgParam);
      vos_mem_free(pWdaParams);
   }
   return CONVERT_WDI2VOS_STATUS(status) ;
}
#ifdef FEATURE_WLAN_ESE
/*
                                    
                                
 */ 
void WDA_TSMStatsReqCallback(WDI_TSMStatsRspParamsType *pwdiTSMStatsRspParams, void* pUserData)
{
   tWDA_ReqParams *pWdaParams = (tWDA_ReqParams *)pUserData; 
   tWDA_CbContext *pWDA = NULL; 
   tpAniGetTsmStatsRsp pTsmRspParams = NULL;
   tpAniGetTsmStatsReq  pGetTsmStatsReqParams = NULL;
 
   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                                          "<------ Entering: %s " ,__func__);
   if(NULL == pWdaParams)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
              "%s: pWdaParams received NULL", __func__);
      VOS_ASSERT(0) ;
      return ;
   }
   pWDA = (tWDA_CbContext *) pWdaParams->pWdaContext;
   pGetTsmStatsReqParams = (tAniGetTsmStatsReq *)pWdaParams->wdaMsgParam;

   if(NULL == pGetTsmStatsReqParams)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                  "%s: pGetTsmStatsReqParams received NULL", __func__);
      VOS_ASSERT(0);
      vos_mem_free(pWdaParams->wdaWdiApiMsgParam);
      vos_mem_free(pWdaParams);
      return;
   }

   pTsmRspParams =
     (tAniGetTsmStatsRsp *)vos_mem_malloc(sizeof(tAniGetTsmStatsRsp));
   if( NULL == pTsmRspParams )
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                                          "%s: pTsmRspParams received NULL " ,__func__);
      VOS_ASSERT( 0 );
      return ;
   }
   vos_mem_set(pTsmRspParams, sizeof(tAniGetTsmStatsRsp), 0);
   pTsmRspParams->rc = pwdiTSMStatsRspParams->wdiStatus;
   pTsmRspParams->staId = pGetTsmStatsReqParams->staId;

   pTsmRspParams->tsmMetrics.UplinkPktQueueDly = pwdiTSMStatsRspParams->UplinkPktQueueDly;
   vos_mem_copy(pTsmRspParams->tsmMetrics.UplinkPktQueueDlyHist,
                 pwdiTSMStatsRspParams->UplinkPktQueueDlyHist,
                 sizeof(pwdiTSMStatsRspParams->UplinkPktQueueDlyHist)/
                 sizeof(pwdiTSMStatsRspParams->UplinkPktQueueDlyHist[0]));
   pTsmRspParams->tsmMetrics.UplinkPktTxDly = pwdiTSMStatsRspParams->UplinkPktTxDly;
   pTsmRspParams->tsmMetrics.UplinkPktLoss = pwdiTSMStatsRspParams->UplinkPktLoss;
   pTsmRspParams->tsmMetrics.UplinkPktCount = pwdiTSMStatsRspParams->UplinkPktCount;
   pTsmRspParams->tsmMetrics.RoamingCount = pwdiTSMStatsRspParams->RoamingCount;
   pTsmRspParams->tsmMetrics.RoamingDly = pwdiTSMStatsRspParams->RoamingDly;

   /*                                                          */
   pTsmRspParams->tsmStatsReq = pGetTsmStatsReqParams;

   /*                         */
   vos_mem_free(pWdaParams->wdaWdiApiMsgParam);
   vos_mem_free(pWdaParams);

   WDA_SendMsg(pWDA, WDA_TSM_STATS_RSP, (void *)pTsmRspParams , 0) ;
   return ;
}


/*
                                   
                                              
 */ 
VOS_STATUS WDA_ProcessTsmStatsReq(tWDA_CbContext *pWDA, 
                                  tpAniGetTsmStatsReq pTsmStats)
{
   WDI_Status                 status = WDI_STATUS_SUCCESS ;
   WDI_TSMStatsReqParamsType *wdiTSMReqParam = NULL;
   tWDA_ReqParams            *pWdaParams = NULL;
   tAniGetTsmStatsRsp        *pGetTsmStatsRspParams = NULL;

   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                                          "------> Entering: %s " ,__func__);
   wdiTSMReqParam = (WDI_TSMStatsReqParamsType *)vos_mem_malloc(
                                 sizeof(WDI_TSMStatsReqParamsType));
   if(NULL == wdiTSMReqParam) 
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                           "%s: VOS MEM Alloc Failure", __func__); 
      VOS_ASSERT(0);
      return VOS_STATUS_E_NOMEM;
   }
   pWdaParams = (tWDA_ReqParams *)vos_mem_malloc(sizeof(tWDA_ReqParams)) ;
   if(NULL == pWdaParams)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                           "%s: VOS MEM Alloc Failure", __func__); 
      VOS_ASSERT(0);
      vos_mem_free(wdiTSMReqParam);
      return VOS_STATUS_E_NOMEM;
   }
   wdiTSMReqParam->wdiTsmStatsParamsInfo.ucTid = pTsmStats->tid;
   vos_mem_copy(wdiTSMReqParam->wdiTsmStatsParamsInfo.bssid,
                                           pTsmStats->bssId,
                                         sizeof(wpt_macAddr));
   wdiTSMReqParam->wdiReqStatusCB = NULL ;
   
   pWdaParams->pWdaContext = pWDA;
   /*                                                            */
   pWdaParams->wdaMsgParam = (void *)pTsmStats ;
   pWdaParams->wdaWdiApiMsgParam = NULL ;
   status = WDI_TSMStatsReq(wdiTSMReqParam,
                           (WDI_TsmRspCb)WDA_TSMStatsReqCallback, pWdaParams);
   if(IS_WDI_STATUS_FAILURE(status))
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
             "Failure in TSM STATS REQ Params WDI API, free all the memory " );
      vos_mem_free(pWdaParams);

      pGetTsmStatsRspParams =
         (tAniGetTsmStatsRsp *)vos_mem_malloc(sizeof(tAniGetTsmStatsRsp));
      if(NULL == pGetTsmStatsRspParams)
      {
          VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                           "%s: VOS MEM Alloc Failure", __func__);
          VOS_ASSERT(0);
          vos_mem_free(pTsmStats);
          return VOS_STATUS_E_NOMEM;
      }
      pGetTsmStatsRspParams->staId = pTsmStats->staId;
      pGetTsmStatsRspParams->rc    = eSIR_FAILURE;
      pGetTsmStatsRspParams->tsmStatsReq = pTsmStats;

      WDA_SendMsg(pWDA, WDA_TSM_STATS_RSP, (void *)pGetTsmStatsRspParams , 0) ;
   }
  return CONVERT_WDI2VOS_STATUS(status) ;
} 
#endif
/*
                                         
                                                  
 */ 
void WDA_SendBeaconParamsCallback(WDI_Status status, void* pUserData)
{

   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                                          "<------ %s " ,__func__);
   return ;
}
/*
                                  
                                                                                  
                           
 */ 
VOS_STATUS WDA_ProcessSendBeacon(tWDA_CbContext *pWDA, 
                                       tSendbeaconParams *pSendbeaconParams)
{
   WDI_Status status = WDI_STATUS_SUCCESS ;
   WDI_SendBeaconParamsType wdiSendBeaconReqParam; 
   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                                          "------> %s " ,__func__);
   vos_mem_copy(wdiSendBeaconReqParam.wdiSendBeaconParamsInfo.macBSSID, 
                              pSendbeaconParams->bssId, sizeof(tSirMacAddr));
   wdiSendBeaconReqParam.wdiSendBeaconParamsInfo.beaconLength = 
                              pSendbeaconParams->beaconLength;
   wdiSendBeaconReqParam.wdiSendBeaconParamsInfo.timIeOffset = 
                              pSendbeaconParams->timIeOffset;
   /*                                                        */
   if ((pSendbeaconParams->p2pIeOffset != 0) &&
           (pSendbeaconParams->p2pIeOffset <
            pSendbeaconParams->timIeOffset))
   {
       VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
               "Invalid p2pIeOffset = %hu ", pSendbeaconParams->p2pIeOffset);
       VOS_ASSERT( 0 );
       return WDI_STATUS_E_FAILURE;
   }
   wdiSendBeaconReqParam.wdiSendBeaconParamsInfo.usP2PIeOffset = 
                              pSendbeaconParams->p2pIeOffset;
   /*                                          */
   vos_mem_copy(wdiSendBeaconReqParam.wdiSendBeaconParamsInfo.beacon, 
                 pSendbeaconParams->beacon, pSendbeaconParams->beaconLength);
   wdiSendBeaconReqParam.wdiReqStatusCB = NULL ;

   status = WDI_SendBeaconParamsReq(&wdiSendBeaconReqParam, 
            (WDI_SendBeaconParamsRspCb)WDA_SendBeaconParamsCallback, pWDA);
   if(IS_WDI_STATUS_FAILURE(status))
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
          "Failure in SEND BEACON REQ Params WDI API" );
   }
   vos_mem_free(pSendbeaconParams);
   return CONVERT_WDI2VOS_STATUS(status);
}
/*
                                             
                                                  
 */ 
void WDA_UpdateProbeRspParamsCallback(WDI_Status status, void* pUserData)
{
   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                                          "<------ %s " ,__func__);
   return ;
}

/*
                                              
                                                                                          
                      
 */ 
VOS_STATUS WDA_ProcessUpdateProbeRspTemplate(tWDA_CbContext *pWDA, 
                                 tSendProbeRespParams *pSendProbeRspParams)
{
   WDI_Status status = WDI_STATUS_SUCCESS;
   WDI_UpdateProbeRspTemplateParamsType *wdiSendProbeRspParam =
         vos_mem_malloc(sizeof(WDI_UpdateProbeRspTemplateParamsType));
   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                                          "------> %s " ,__func__);

   if (!wdiSendProbeRspParam)
      return CONVERT_WDI2VOS_STATUS(WDI_STATUS_MEM_FAILURE);

   /*                                     */
   vos_mem_copy(wdiSendProbeRspParam->wdiProbeRspTemplateInfo.macBSSID,
                              pSendProbeRspParams->bssId, sizeof(tSirMacAddr));
   wdiSendProbeRspParam->wdiProbeRspTemplateInfo.uProbeRespTemplateLen =
                              pSendProbeRspParams->probeRespTemplateLen;
   /*                                                  */
   vos_mem_copy(
           wdiSendProbeRspParam->wdiProbeRspTemplateInfo.pProbeRespTemplate,
           pSendProbeRspParams->pProbeRespTemplate, 
           pSendProbeRspParams->probeRespTemplateLen);
   vos_mem_copy(
     wdiSendProbeRspParam->wdiProbeRspTemplateInfo.uaProxyProbeReqValidIEBmap,
     pSendProbeRspParams->ucProxyProbeReqValidIEBmap,
     WDI_PROBE_REQ_BITMAP_IE_LEN);
   
   wdiSendProbeRspParam->wdiReqStatusCB = NULL ;
   
   status = WDI_UpdateProbeRspTemplateReq(wdiSendProbeRspParam,
     (WDI_UpdateProbeRspTemplateRspCb)WDA_UpdateProbeRspParamsCallback, pWDA);
   if(IS_WDI_STATUS_FAILURE(status))
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
          "Failure in SEND Probe RSP Params WDI API" );
   }
   vos_mem_free(pSendProbeRspParams);
   vos_mem_free(wdiSendProbeRspParam);
   return CONVERT_WDI2VOS_STATUS(status);
}
#if defined(WLAN_FEATURE_VOWIFI) || defined(FEATURE_WLAN_ESE)
/*
                                      
                                                             
 */ 
void WDA_SetMaxTxPowerCallBack(WDI_SetMaxTxPowerRspMsg * pwdiSetMaxTxPowerRsp,
                                             void* pUserData)
{
   tWDA_ReqParams *pWdaParams = (tWDA_ReqParams *)pUserData;
   tWDA_CbContext *pWDA = NULL;
   tMaxTxPowerParams *pMaxTxPowerParams = NULL; 
   
   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                                          "<------ %s " ,__func__);
   if(NULL == pWdaParams)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
              "%s: pWdaParams received NULL", __func__);
      VOS_ASSERT(0) ;
      return ;
   }
   pWDA = (tWDA_CbContext *) pWdaParams->pWdaContext;
   pMaxTxPowerParams = (tMaxTxPowerParams *)pWdaParams->wdaMsgParam ;
   if( NULL == pMaxTxPowerParams )
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                                          "%s: pMaxTxPowerParams received NULL " ,__func__);
      VOS_ASSERT(0);
      vos_mem_free(pWdaParams->wdaWdiApiMsgParam);
      vos_mem_free(pWdaParams);
      return ;
   }
  
  
  /*                                                 
                                             */
   vos_mem_free(pWdaParams->wdaWdiApiMsgParam) ;
   vos_mem_free(pWdaParams);
   pMaxTxPowerParams->power = pwdiSetMaxTxPowerRsp->ucPower;
  
  
  /*                      */
   WDA_SendMsg(pWDA, WDA_SET_MAX_TX_POWER_RSP, pMaxTxPowerParams , 0) ;
   
   return;
}
/*
                                        
                                                  
 */ 
 VOS_STATUS WDA_ProcessSetMaxTxPowerReq(tWDA_CbContext *pWDA,
                                          tMaxTxPowerParams *MaxTxPowerParams)
{
   WDI_Status status = WDI_STATUS_SUCCESS;
   WDI_SetMaxTxPowerParamsType *wdiSetMaxTxPowerParams = NULL;
   tWDA_ReqParams *pWdaParams = NULL;

   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                                          "------> %s " ,__func__);

   wdiSetMaxTxPowerParams = (WDI_SetMaxTxPowerParamsType *)vos_mem_malloc(
                                 sizeof(WDI_SetMaxTxPowerParamsType));
   if(NULL == wdiSetMaxTxPowerParams) 
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                           "%s: VOS MEM Alloc Failure", __func__); 
      VOS_ASSERT(0);
      return VOS_STATUS_E_NOMEM;
   }
   pWdaParams = (tWDA_ReqParams *)vos_mem_malloc(sizeof(tWDA_ReqParams)) ;
   if(NULL == pWdaParams)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                           "%s: VOS MEM Alloc Failure", __func__);
      vos_mem_free(wdiSetMaxTxPowerParams);
      VOS_ASSERT(0);
      return VOS_STATUS_E_NOMEM;
   }
   /*                                           */
   vos_mem_copy(wdiSetMaxTxPowerParams->wdiMaxTxPowerInfo.macBSSId,
                 MaxTxPowerParams->bssId, 
                 sizeof(tSirMacAddr));
   vos_mem_copy(wdiSetMaxTxPowerParams->wdiMaxTxPowerInfo.macSelfStaMacAddr,
                 MaxTxPowerParams->selfStaMacAddr, 
                 sizeof(tSirMacAddr));
   wdiSetMaxTxPowerParams->wdiMaxTxPowerInfo.ucPower = 
                                              MaxTxPowerParams->power;
   wdiSetMaxTxPowerParams->wdiReqStatusCB = NULL ;
   pWdaParams->pWdaContext = pWDA;
   pWdaParams->wdaMsgParam = (void *)MaxTxPowerParams ;
   /*                             */
   pWdaParams->wdaWdiApiMsgParam = (void *)wdiSetMaxTxPowerParams ;
   status = WDI_SetMaxTxPowerReq(wdiSetMaxTxPowerParams,
                       (WDA_SetMaxTxPowerRspCb)WDA_SetMaxTxPowerCallBack, pWdaParams);
   if(IS_WDI_STATUS_FAILURE(status))
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
     "Failure in SET MAX TX Power REQ Params WDI API, free all the memory " );
      vos_mem_free(pWdaParams->wdaWdiApiMsgParam) ;
      vos_mem_free(pWdaParams);
      /*                      */
       WDA_SendMsg(pWDA, WDA_SET_MAX_TX_POWER_RSP, MaxTxPowerParams , 0) ;
   }
   return CONVERT_WDI2VOS_STATUS(status);
   
}
#endif

/*
                                             
                                                             
 */
void WDA_SetMaxTxPowerPerBandCallBack(WDI_SetMaxTxPowerPerBandRspMsg
                                      *pwdiSetMaxTxPowerPerBandRsp,
                                      void* pUserData)
{
   tWDA_ReqParams *pWdaParams = (tWDA_ReqParams *)pUserData;
   tWDA_CbContext *pWDA = NULL;
   tMaxTxPowerPerBandParams *pMxTxPwrPerBandParams = NULL;

   VOS_TRACE(VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
             "<------ %s ", __func__);
   if (NULL == pWdaParams)
   {
      VOS_TRACE(VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                "%s: pWdaParams received NULL", __func__);
      VOS_ASSERT(0);
      return ;
   }
   pWDA = (tWDA_CbContext *) pWdaParams->pWdaContext;
   pMxTxPwrPerBandParams = (tMaxTxPowerPerBandParams*)pWdaParams->wdaMsgParam;
   if ( NULL == pMxTxPwrPerBandParams )
   {
      VOS_TRACE(VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                "%s: pMxTxPwrPerBandParams received NULL ", __func__);
      VOS_ASSERT(0);
      vos_mem_free(pWdaParams->wdaWdiApiMsgParam);
      vos_mem_free(pWdaParams);
      return;
   }

  /*                                                
                                             */
   vos_mem_free(pWdaParams->wdaWdiApiMsgParam);
   vos_mem_free(pWdaParams);
   pMxTxPwrPerBandParams->power = pwdiSetMaxTxPowerPerBandRsp->ucPower;

  /*                      */
   WDA_SendMsg(pWDA, WDA_SET_MAX_TX_POWER_PER_BAND_RSP,
               pMxTxPwrPerBandParams, 0);

   return;
}

/*
                                               
                                                           
 */
 VOS_STATUS WDA_ProcessSetMaxTxPowerPerBandReq(tWDA_CbContext *pWDA,
                                               tMaxTxPowerPerBandParams
                                               *MaxTxPowerPerBandParams)
{
   WDI_Status status = WDI_STATUS_SUCCESS;
   WDI_SetMaxTxPowerPerBandParamsType *wdiSetMxTxPwrPerBandParams = NULL;
   tWDA_ReqParams *pWdaParams = NULL;

   VOS_TRACE(VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
             "------> %s ", __func__);

   wdiSetMxTxPwrPerBandParams = vos_mem_malloc(
                                sizeof(WDI_SetMaxTxPowerPerBandParamsType));

   if (NULL == wdiSetMxTxPwrPerBandParams)
   {
      VOS_TRACE(VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                "%s: VOS MEM Alloc Failure", __func__);
      VOS_ASSERT(0);
      return VOS_STATUS_E_NOMEM;
   }
   pWdaParams = (tWDA_ReqParams *)vos_mem_malloc(sizeof(tWDA_ReqParams));
   if (NULL == pWdaParams)
   {
      VOS_TRACE(VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                "%s: VOS MEM Alloc Failure", __func__);
      vos_mem_free(wdiSetMxTxPwrPerBandParams);
      VOS_ASSERT(0);
      return VOS_STATUS_E_NOMEM;
   }
   /*                                                    */
   wdiSetMxTxPwrPerBandParams->wdiMaxTxPowerPerBandInfo.bandInfo = \
                               MaxTxPowerPerBandParams->bandInfo;
   wdiSetMxTxPwrPerBandParams->wdiMaxTxPowerPerBandInfo.ucPower = \
                               MaxTxPowerPerBandParams->power;
   wdiSetMxTxPwrPerBandParams->wdiReqStatusCB = NULL;
   pWdaParams->pWdaContext = pWDA;
   pWdaParams->wdaMsgParam = (void *)MaxTxPowerPerBandParams;
   /*                             */
   pWdaParams->wdaWdiApiMsgParam = (void *)wdiSetMxTxPwrPerBandParams;
   status = WDI_SetMaxTxPowerPerBandReq(wdiSetMxTxPwrPerBandParams,
                                        WDA_SetMaxTxPowerPerBandCallBack,
                                        pWdaParams);
   if (IS_WDI_STATUS_FAILURE(status))
   {
      VOS_TRACE(VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                "Failure in SET MAX TX Power REQ Params WDI API,"
                " free all the memory");
      vos_mem_free(pWdaParams->wdaWdiApiMsgParam);
      vos_mem_free(pWdaParams);
      /*                      */
      WDA_SendMsg(pWDA,
                  WDA_SET_MAX_TX_POWER_PER_BAND_RSP,
                  MaxTxPowerPerBandParams, 0);
   }
   return CONVERT_WDI2VOS_STATUS(status);
}

/*
                                   
                                                             
 */
void WDA_SetTxPowerCallBack(WDI_SetTxPowerRspMsg * pwdiSetMaxTxPowerRsp,
                                             void* pUserData)
{
   tWDA_ReqParams    *pWdaParams = (tWDA_ReqParams *)pUserData;
   tWDA_CbContext    *pWDA = NULL;
   tSirSetTxPowerReq *pTxPowerParams = NULL;

   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                                          "<------ %s ", __func__);
   if(NULL == pWdaParams)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
              "%s: pWdaParams received NULL", __func__);
      VOS_ASSERT(0) ;
      return ;
   }
   pWDA = (tWDA_CbContext *)pWdaParams->pWdaContext;
   pTxPowerParams = (tSirSetTxPowerReq *)pWdaParams->wdaMsgParam;
   if(NULL == pTxPowerParams)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                                          "%s: pTxPowerParams received NULL " ,__func__);
      VOS_ASSERT(0);
      vos_mem_free(pWdaParams->wdaWdiApiMsgParam);
      vos_mem_free(pWdaParams);
      return ;
   }

  /*                                                
                                             */
   vos_mem_free(pWdaParams->wdaWdiApiMsgParam) ;
   vos_mem_free(pWdaParams);

  /*                      */
   WDA_SendMsg(pWDA, WDA_SET_TX_POWER_RSP, pTxPowerParams , 0) ;
   return;
}

/*
                                     
                                              
 */
 VOS_STATUS WDA_ProcessSetTxPowerReq(tWDA_CbContext *pWDA,
                                          tSirSetTxPowerReq  *txPowerParams)
{
   WDI_Status status = WDI_STATUS_SUCCESS;
   WDI_SetTxPowerParamsType *wdiSetTxPowerParams = NULL;
   tWDA_ReqParams *pWdaParams = NULL;

   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                                          "------> %s ", __func__);

   wdiSetTxPowerParams = (WDI_SetTxPowerParamsType *)vos_mem_malloc(
                                 sizeof(WDI_SetTxPowerParamsType));
   if(NULL == wdiSetTxPowerParams)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                           "%s: VOS MEM Alloc Failure", __func__);
      VOS_ASSERT(0);
      return VOS_STATUS_E_NOMEM;
   }
   pWdaParams = (tWDA_ReqParams *)vos_mem_malloc(sizeof(tWDA_ReqParams)) ;
   if(NULL == pWdaParams)
   {
      VOS_TRACE(VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                           "%s: VOS MEM Alloc Failure", __func__);
      vos_mem_free(wdiSetTxPowerParams);
      VOS_ASSERT(0);
      return VOS_STATUS_E_NOMEM;
   }
   wdiSetTxPowerParams->wdiTxPowerInfo.bssIdx =
                                              txPowerParams->bssIdx;
   wdiSetTxPowerParams->wdiTxPowerInfo.ucPower =
                                              txPowerParams->mwPower;
   wdiSetTxPowerParams->wdiReqStatusCB = NULL ;
   pWdaParams->pWdaContext = pWDA;
   pWdaParams->wdaMsgParam = (void *)txPowerParams ;
   /*                             */
   pWdaParams->wdaWdiApiMsgParam = (void *)wdiSetTxPowerParams ;
   status = WDI_SetTxPowerReq(wdiSetTxPowerParams,
                       (WDA_SetTxPowerRspCb)WDA_SetTxPowerCallBack, pWdaParams);
   if(IS_WDI_STATUS_FAILURE(status))
   {
      VOS_TRACE(VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                "Failure in SET TX Power REQ Params WDI API, free all the memory ");
      vos_mem_free(pWdaParams->wdaWdiApiMsgParam) ;
      vos_mem_free(pWdaParams);
      /*                      */
      WDA_SendMsg(pWDA, WDA_SET_TX_POWER_RSP, txPowerParams , 0) ;
   }
   return CONVERT_WDI2VOS_STATUS(status);
}

/*
                                             
                                                                    
 */ 
void WDA_SetP2PGONOAReqParamsCallback(WDI_Status status, void* pUserData)
{
   tWDA_ReqParams *pWdaParams = (tWDA_ReqParams *)pUserData ; 

   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                                          "<------ %s " ,__func__);

   if(NULL == pWdaParams)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
              "%s: pWdaParams received NULL", __func__);
      VOS_ASSERT(0) ;
      return ;
   }

   vos_mem_free(pWdaParams->wdaWdiApiMsgParam) ;
   vos_mem_free(pWdaParams->wdaMsgParam) ;
   vos_mem_free(pWdaParams);

   /* 
                                                         
                                         
    */
   return ;
}

/*
                                      
                                                                   
 */ 
VOS_STATUS WDA_ProcessSetP2PGONOAReq(tWDA_CbContext *pWDA,
                                    tP2pPsParams *pP2pPsConfigParams)
{
   WDI_Status status = WDI_STATUS_SUCCESS ;
   WDI_SetP2PGONOAReqParamsType *wdiSetP2PGONOAReqParam = 
                (WDI_SetP2PGONOAReqParamsType *)vos_mem_malloc(
                                   sizeof(WDI_SetP2PGONOAReqParamsType)) ;
   tWDA_ReqParams *pWdaParams = NULL;

   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                                          "------> %s " ,__func__);
   if(NULL == wdiSetP2PGONOAReqParam) 
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                           "%s: VOS MEM Alloc Failure", __func__); 
      VOS_ASSERT(0);
      return VOS_STATUS_E_NOMEM;
   }

   pWdaParams = (tWDA_ReqParams *)vos_mem_malloc(sizeof(tWDA_ReqParams)) ;
   if(NULL == pWdaParams)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                           "%s: VOS MEM Alloc Failure", __func__);
      vos_mem_free(pP2pPsConfigParams);
      vos_mem_free(wdiSetP2PGONOAReqParam);
      VOS_ASSERT(0);
      return VOS_STATUS_E_NOMEM;
   }

   wdiSetP2PGONOAReqParam->wdiP2PGONOAInfo.ucOpp_ps = 
                                    pP2pPsConfigParams->opp_ps;
   wdiSetP2PGONOAReqParam->wdiP2PGONOAInfo.uCtWindow = 
                                    pP2pPsConfigParams->ctWindow;
   wdiSetP2PGONOAReqParam->wdiP2PGONOAInfo.ucCount = 
                                    pP2pPsConfigParams->count;
   wdiSetP2PGONOAReqParam->wdiP2PGONOAInfo.uDuration = 
                                    pP2pPsConfigParams->duration;
   wdiSetP2PGONOAReqParam->wdiP2PGONOAInfo.uInterval = 
                                    pP2pPsConfigParams->interval;
   wdiSetP2PGONOAReqParam->wdiP2PGONOAInfo.uSingle_noa_duration = 
                                    pP2pPsConfigParams->single_noa_duration;
   wdiSetP2PGONOAReqParam->wdiP2PGONOAInfo.ucPsSelection = 
                                    pP2pPsConfigParams->psSelection;

   wdiSetP2PGONOAReqParam->wdiReqStatusCB = NULL ;
   /*                                                              */
   pWdaParams->wdaMsgParam = (void *)pP2pPsConfigParams ;

   /*                             */
   pWdaParams->wdaWdiApiMsgParam = (void *)wdiSetP2PGONOAReqParam ;
   pWdaParams->pWdaContext = pWDA;

   status = WDI_SetP2PGONOAReq(wdiSetP2PGONOAReqParam, 
       (WDI_SetP2PGONOAReqParamsRspCb)WDA_SetP2PGONOAReqParamsCallback, pWdaParams);

   if(IS_WDI_STATUS_FAILURE(status))
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
          "Failure in Set P2P GO NOA Req WDI API, free all the memory " );
      vos_mem_free(pWdaParams->wdaWdiApiMsgParam) ;
      vos_mem_free(pWdaParams->wdaMsgParam);
      vos_mem_free(pWdaParams);
   }
   return CONVERT_WDI2VOS_STATUS(status);

}

#ifdef FEATURE_WLAN_TDLS
/*
                                             
                                                                    
 */
void WDA_SetTDLSLinkEstablishReqParamsCallback(WDI_SetTdlsLinkEstablishReqResp *wdiSetTdlsLinkEstablishReqRsp,
                                               void* pUserData)
{
   tWDA_ReqParams *pWdaParams = (tWDA_ReqParams *)pUserData;
   tWDA_CbContext *pWDA = NULL;
   tTdlsLinkEstablishParams *pTdlsLinkEstablishParams;


   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                                          "<------ %s " ,__func__);
   if(NULL == pWdaParams)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
              "%s: pWdaParams received NULL", __func__);
      VOS_ASSERT(0) ;
      return ;
   }
   pWDA = (tWDA_CbContext *) pWdaParams->pWdaContext;

   if(NULL == pWdaParams)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
              "%s: pWdaParams received NULL", __func__);
      VOS_ASSERT(0) ;
      return ;
   }
   pTdlsLinkEstablishParams = (tTdlsLinkEstablishParams *)pWdaParams->wdaMsgParam ;
   if( NULL == pTdlsLinkEstablishParams )
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                                          "%s: pTdlsLinkEstablishParams "
                                          "received NULL " ,__func__);
      VOS_ASSERT(0);
      vos_mem_free(pWdaParams->wdaWdiApiMsgParam);
      vos_mem_free(pWdaParams);
      return ;
   }
   pTdlsLinkEstablishParams->status = CONVERT_WDI2SIR_STATUS(
                                               wdiSetTdlsLinkEstablishReqRsp->wdiStatus);
   vos_mem_free(pWdaParams->wdaWdiApiMsgParam) ;
   vos_mem_free(pWdaParams);
   /*                      */
   WDA_SendMsg(pWDA, WDA_SET_TDLS_LINK_ESTABLISH_REQ_RSP, pTdlsLinkEstablishParams, 0) ;

   return ;
}

VOS_STATUS WDA_ProcessSetTdlsLinkEstablishReq(tWDA_CbContext *pWDA,
                                              tTdlsLinkEstablishParams *pTdlsLinkEstablishParams)
{
    WDI_Status status = WDI_STATUS_SUCCESS ;
    WDI_SetTDLSLinkEstablishReqParamsType *wdiSetTDLSLinkEstablishReqParam =
                                      (WDI_SetTDLSLinkEstablishReqParamsType *)vos_mem_malloc(
                                           sizeof(WDI_SetTDLSLinkEstablishReqParamsType)) ;
    tWDA_ReqParams *pWdaParams = NULL;
    VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
               "------> %s " ,__func__);
    if(NULL == wdiSetTDLSLinkEstablishReqParam)
    {
        VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                   "%s: VOS MEM Alloc Failure", __func__);
        VOS_ASSERT(0);
        return VOS_STATUS_E_NOMEM;
    }
    pWdaParams = (tWDA_ReqParams *)vos_mem_malloc(sizeof(tWDA_ReqParams)) ;
    if(NULL == pWdaParams)
    {
        VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                   "%s: VOS MEM Alloc Failure", __func__);
        vos_mem_free(pTdlsLinkEstablishParams);
        vos_mem_free(wdiSetTDLSLinkEstablishReqParam);
        VOS_ASSERT(0);
        return VOS_STATUS_E_NOMEM;
    }
    wdiSetTDLSLinkEstablishReqParam->wdiTDLSLinkEstablishInfo.uStaIdx =
                                                  pTdlsLinkEstablishParams->staIdx;
    wdiSetTDLSLinkEstablishReqParam->wdiTDLSLinkEstablishInfo.uIsResponder =
                                                  pTdlsLinkEstablishParams->isResponder;
    wdiSetTDLSLinkEstablishReqParam->wdiTDLSLinkEstablishInfo.uUapsdQueues =
                                                  pTdlsLinkEstablishParams->uapsdQueues;
    wdiSetTDLSLinkEstablishReqParam->wdiTDLSLinkEstablishInfo.uMaxSp =
                                                  pTdlsLinkEstablishParams->maxSp;
    wdiSetTDLSLinkEstablishReqParam->wdiTDLSLinkEstablishInfo.uIsBufSta =
                                                  pTdlsLinkEstablishParams->isBufsta;
    wdiSetTDLSLinkEstablishReqParam->wdiTDLSLinkEstablishInfo.uIsOffChannelSupported =
                                        pTdlsLinkEstablishParams->isOffChannelSupported;

    vos_mem_copy(wdiSetTDLSLinkEstablishReqParam->wdiTDLSLinkEstablishInfo.validChannels,
                                       pTdlsLinkEstablishParams->validChannels,
                                       pTdlsLinkEstablishParams->validChannelsLen);

    wdiSetTDLSLinkEstablishReqParam->wdiTDLSLinkEstablishInfo.validChannelsLen =
                                  pTdlsLinkEstablishParams->validChannelsLen;

    vos_mem_copy(wdiSetTDLSLinkEstablishReqParam->wdiTDLSLinkEstablishInfo.validOperClasses,
                                       pTdlsLinkEstablishParams->validOperClasses,
                                       pTdlsLinkEstablishParams->validOperClassesLen);
    wdiSetTDLSLinkEstablishReqParam->wdiTDLSLinkEstablishInfo.validOperClassesLen =
                                  pTdlsLinkEstablishParams->validOperClassesLen;

    wdiSetTDLSLinkEstablishReqParam->wdiReqStatusCB = NULL ;
    /*                                                              */
    pWdaParams->wdaMsgParam = (void *)pTdlsLinkEstablishParams ;
    /*                             */
    pWdaParams->wdaWdiApiMsgParam = (void *)wdiSetTDLSLinkEstablishReqParam ;
    pWdaParams->pWdaContext = pWDA;

    status = WDI_SetTDLSLinkEstablishReq(wdiSetTDLSLinkEstablishReqParam,
                                         (WDI_SetTDLSLinkEstablishReqParamsRspCb)
                                         WDA_SetTDLSLinkEstablishReqParamsCallback,
                                         pWdaParams);
    if(IS_WDI_STATUS_FAILURE(status))
    {
        VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                   "Failure in Set P2P GO NOA Req WDI API, free all the memory " );
        vos_mem_free(pWdaParams->wdaWdiApiMsgParam) ;
        vos_mem_free(pWdaParams->wdaMsgParam);
        vos_mem_free(pWdaParams);
    }
    return CONVERT_WDI2VOS_STATUS(status);
}

//            
void WDA_SetTDLSChanSwitchReqParamsCallback(WDI_SetTdlsChanSwitchReqResp *wdiSetTdlsChanSwitchReqRsp,
                                               void* pUserData)
{
   tWDA_ReqParams *pWdaParams = (tWDA_ReqParams *)pUserData;
   tWDA_CbContext *pWDA = NULL;
   tTdlsChanSwitchParams *pTdlsChanSwitchParams;


   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                                          "<------ %s " ,__func__);
   if(NULL == pWdaParams)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
              "%s: pWdaParams received NULL", __func__);
      VOS_ASSERT(0) ;
      return ;
   }
   pWDA = (tWDA_CbContext *) pWdaParams->pWdaContext;

   if(NULL == pWdaParams)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
              "%s: pWdaParams received NULL", __func__);
      VOS_ASSERT(0) ;
      return ;
   }
   pTdlsChanSwitchParams = (tTdlsChanSwitchParams *)pWdaParams->wdaMsgParam ;
   if( NULL == pTdlsChanSwitchParams )
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                                          "%s: pTdlsChanSwitchParams "
                                          "received NULL " ,__func__);
      VOS_ASSERT(0);
      vos_mem_free(pWdaParams->wdaWdiApiMsgParam);
      vos_mem_free(pWdaParams);
      return ;
   }
   pTdlsChanSwitchParams->status = CONVERT_WDI2SIR_STATUS(
                                               wdiSetTdlsChanSwitchReqRsp->wdiStatus);
   vos_mem_free(pWdaParams->wdaWdiApiMsgParam) ;
   vos_mem_free(pWdaParams);
   /*                      */
   WDA_SendMsg(pWDA, WDA_SET_TDLS_CHAN_SWITCH_REQ_RSP, pTdlsChanSwitchParams, 0) ;

   return ;
}
VOS_STATUS WDA_ProcessSetTdlsChanSwitchReq(tWDA_CbContext *pWDA,
                                           tTdlsChanSwitchParams *pTdlsChanSwitchParams)
{
    WDI_Status status = WDI_STATUS_SUCCESS ;
    WDI_SetTDLSChanSwitchReqParamsType *wdiSetTDLSChanSwitchReqParam =
                        (WDI_SetTDLSChanSwitchReqParamsType *)vos_mem_malloc(
                                   sizeof(WDI_SetTDLSChanSwitchReqParamsType));
    tWDA_ReqParams *pWdaParams = NULL;

    VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
               "Enter: %s ",__func__);
    if(NULL == wdiSetTDLSChanSwitchReqParam)
    {
        VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                   "%s: VOS MEM Alloc Failure", __func__);
        VOS_ASSERT(0);
        return VOS_STATUS_E_NOMEM;
    }

    pWdaParams = (tWDA_ReqParams *)vos_mem_malloc(sizeof(tWDA_ReqParams)) ;
    if(NULL == pWdaParams)
    {
        VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                   "%s: VOS MEM Alloc Failure", __func__);
        vos_mem_free(pTdlsChanSwitchParams);
        vos_mem_free(wdiSetTDLSChanSwitchReqParam);
        VOS_ASSERT(0);
        return VOS_STATUS_E_NOMEM;
    }
    wdiSetTDLSChanSwitchReqParam->wdiTDLSChanSwitchReqInfo.staIdx =
                                              pTdlsChanSwitchParams->staIdx;
    wdiSetTDLSChanSwitchReqParam->wdiTDLSChanSwitchReqInfo.isOffchannelInitiator =
                                              pTdlsChanSwitchParams->tdlsSwMode;
    wdiSetTDLSChanSwitchReqParam->wdiTDLSChanSwitchReqInfo.targetOperClass =
                                              pTdlsChanSwitchParams->operClass;
    wdiSetTDLSChanSwitchReqParam->wdiTDLSChanSwitchReqInfo.targetChannel =
                                              pTdlsChanSwitchParams->tdlsOffCh;
    wdiSetTDLSChanSwitchReqParam->wdiTDLSChanSwitchReqInfo.secondaryChannelOffset =
                                          pTdlsChanSwitchParams->tdlsOffChBwOffset;


    /*                                                              */
    pWdaParams->wdaMsgParam = (void *)pTdlsChanSwitchParams ;
    /*                             */
    pWdaParams->wdaWdiApiMsgParam = (void *)wdiSetTDLSChanSwitchReqParam ;
    pWdaParams->pWdaContext = pWDA;
    status = WDI_SetTDLSChanSwitchReq(wdiSetTDLSChanSwitchReqParam,
                                      (WDI_SetTDLSChanSwitchReqParamsRspCb)
                                       WDA_SetTDLSChanSwitchReqParamsCallback,
                                       pWdaParams);
    if(IS_WDI_STATUS_FAILURE(status))
    {
        VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                   "Failure in TDLS Channel Switch Req WDI API, free all the memory" );
        vos_mem_free(pWdaParams->wdaWdiApiMsgParam) ;
        vos_mem_free(pWdaParams->wdaMsgParam);
        vos_mem_free(pWdaParams);
    }
    return CONVERT_WDI2VOS_STATUS(status);
}
#endif /*                 */


#ifdef WLAN_FEATURE_VOWIFI_11R
/*
                                     
                                        
 */ 
void WDA_AggrAddTSReqCallback(WDI_Status status, void* pUserData)
{
   tWDA_ReqParams *pWdaParams = (tWDA_ReqParams *)pUserData ; 
   tWDA_CbContext *pWDA;
   tAggrAddTsParams *pAggrAddTsReqParams;
   int i;
   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                                          "<------ %s " ,__func__);
   if(NULL == pWdaParams)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
              "%s: pWdaParams received NULL", __func__);
      VOS_ASSERT(0) ;
      return ;
   }

   pWDA = pWdaParams->pWdaContext;
   pAggrAddTsReqParams = (tAggrAddTsParams *)pWdaParams->wdaMsgParam ;
   
   for( i = 0; i < HAL_QOS_NUM_AC_MAX; i++ )
   {
      pAggrAddTsReqParams->status[i] = (status) ;
   }
   WDA_SendMsg(pWDA, WDA_AGGR_QOS_RSP, (void *)pAggrAddTsReqParams , 0) ;

   vos_mem_free(pWdaParams->wdaWdiApiMsgParam) ;
   vos_mem_free(pWdaParams);
   return ;
}/*                         */
/*
                                
                                                                      
 */ 
VOS_STATUS WDA_ProcessAggrAddTSReq(tWDA_CbContext *pWDA, 
                                    tAggrAddTsParams *pAggrAddTsReqParams)
{
   WDI_Status status = WDI_STATUS_SUCCESS ;
   int i;
   WDI_AggrAddTSReqParamsType *wdiAggrAddTSReqParam;
   tWDA_ReqParams *pWdaParams = NULL;


   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                                          "------> %s " ,__func__);
   wdiAggrAddTSReqParam = (WDI_AggrAddTSReqParamsType *)vos_mem_malloc(
                           sizeof(WDI_AggrAddTSReqParamsType)) ;
   if(NULL == wdiAggrAddTSReqParam) 
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                           "%s: VOS MEM Alloc Failure", __func__); 
      VOS_ASSERT(0);
      return VOS_STATUS_E_NOMEM;
   }


   pWdaParams = (tWDA_ReqParams *)vos_mem_malloc(sizeof(tWDA_ReqParams)) ;
   if(NULL == pWdaParams)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                           "%s: VOS MEM Alloc Failure", __func__);
      vos_mem_free(pAggrAddTsReqParams);
      vos_mem_free(wdiAggrAddTSReqParam);
      VOS_ASSERT(0);
      return VOS_STATUS_E_NOMEM;
   }
   wdiAggrAddTSReqParam->wdiAggrTsInfo.ucSTAIdx = pAggrAddTsReqParams->staIdx;
   wdiAggrAddTSReqParam->wdiAggrTsInfo.ucTspecIdx = 
      pAggrAddTsReqParams->tspecIdx;
   for( i = 0; i < WDI_MAX_NO_AC; i++ )
   {
      wdiAggrAddTSReqParam->wdiAggrTsInfo.wdiTspecIE[i].ucType = pAggrAddTsReqParams->tspec[i].type;
      wdiAggrAddTSReqParam->wdiAggrTsInfo.wdiTspecIE[i].ucLength = 
                                                   pAggrAddTsReqParams->tspec[i].length;
      wdiAggrAddTSReqParam->wdiAggrTsInfo.wdiTspecIE[i].wdiTSinfo.wdiTraffic.ackPolicy =
                              pAggrAddTsReqParams->tspec[i].tsinfo.traffic.ackPolicy;
      wdiAggrAddTSReqParam->wdiAggrTsInfo.wdiTspecIE[i].wdiTSinfo.wdiTraffic.userPrio =
                              pAggrAddTsReqParams->tspec[i].tsinfo.traffic.userPrio;
      wdiAggrAddTSReqParam->wdiAggrTsInfo.wdiTspecIE[i].wdiTSinfo.wdiTraffic.psb =
                              pAggrAddTsReqParams->tspec[i].tsinfo.traffic.psb;
      wdiAggrAddTSReqParam->wdiAggrTsInfo.wdiTspecIE[i].wdiTSinfo.wdiTraffic.aggregation =
                              pAggrAddTsReqParams->tspec[i].tsinfo.traffic.aggregation;
      wdiAggrAddTSReqParam->wdiAggrTsInfo.wdiTspecIE[i].wdiTSinfo.wdiTraffic.accessPolicy =
                              pAggrAddTsReqParams->tspec[i].tsinfo.traffic.accessPolicy;
      wdiAggrAddTSReqParam->wdiAggrTsInfo.wdiTspecIE[i].wdiTSinfo.wdiTraffic.direction =
                              pAggrAddTsReqParams->tspec[i].tsinfo.traffic.direction;
      wdiAggrAddTSReqParam->wdiAggrTsInfo.wdiTspecIE[i].wdiTSinfo.wdiTraffic.tsid =
                              pAggrAddTsReqParams->tspec[i].tsinfo.traffic.tsid;
      wdiAggrAddTSReqParam->wdiAggrTsInfo.wdiTspecIE[i].wdiTSinfo.wdiTraffic.trafficType =
                              pAggrAddTsReqParams->tspec[i].tsinfo.traffic.trafficType;
      wdiAggrAddTSReqParam->wdiAggrTsInfo.wdiTspecIE[i].wdiTSinfo.wdiSchedule.schedule = 
                              pAggrAddTsReqParams->tspec[i].tsinfo.schedule.schedule;
      wdiAggrAddTSReqParam->wdiAggrTsInfo.wdiTspecIE[i].usNomMsduSz = 
                              pAggrAddTsReqParams->tspec[i].nomMsduSz;
      wdiAggrAddTSReqParam->wdiAggrTsInfo.wdiTspecIE[i].usMaxMsduSz = 
                              pAggrAddTsReqParams->tspec[i].maxMsduSz;
      wdiAggrAddTSReqParam->wdiAggrTsInfo.wdiTspecIE[i].uMinSvcInterval = 
                              pAggrAddTsReqParams->tspec[i].minSvcInterval;
      wdiAggrAddTSReqParam->wdiAggrTsInfo.wdiTspecIE[i].uMaxSvcInterval = 
                              pAggrAddTsReqParams->tspec[i].maxSvcInterval;
      wdiAggrAddTSReqParam->wdiAggrTsInfo.wdiTspecIE[i].uInactInterval = 
                              pAggrAddTsReqParams->tspec[i].inactInterval;
      wdiAggrAddTSReqParam->wdiAggrTsInfo.wdiTspecIE[i].uSuspendInterval = 
                              pAggrAddTsReqParams->tspec[i].suspendInterval;
      wdiAggrAddTSReqParam->wdiAggrTsInfo.wdiTspecIE[i].uSvcStartTime = 
                              pAggrAddTsReqParams->tspec[i].svcStartTime;
      wdiAggrAddTSReqParam->wdiAggrTsInfo.wdiTspecIE[i].uMinDataRate = 
                              pAggrAddTsReqParams->tspec[i].minDataRate;
      wdiAggrAddTSReqParam->wdiAggrTsInfo.wdiTspecIE[i].uMeanDataRate = 
                              pAggrAddTsReqParams->tspec[i].meanDataRate;
      wdiAggrAddTSReqParam->wdiAggrTsInfo.wdiTspecIE[i].uPeakDataRate = 
                              pAggrAddTsReqParams->tspec[i].peakDataRate;
      wdiAggrAddTSReqParam->wdiAggrTsInfo.wdiTspecIE[i].uMaxBurstSz = 
                              pAggrAddTsReqParams->tspec[i].maxBurstSz;
      wdiAggrAddTSReqParam->wdiAggrTsInfo.wdiTspecIE[i].uDelayBound = 
                              pAggrAddTsReqParams->tspec[i].delayBound;
      wdiAggrAddTSReqParam->wdiAggrTsInfo.wdiTspecIE[i].uMinPhyRate = 
                              pAggrAddTsReqParams->tspec[i].minPhyRate;
      wdiAggrAddTSReqParam->wdiAggrTsInfo.wdiTspecIE[i].usSurplusBw = 
                              pAggrAddTsReqParams->tspec[i].surplusBw;
      wdiAggrAddTSReqParam->wdiAggrTsInfo.wdiTspecIE[i].usMediumTime = 
                              pAggrAddTsReqParams->tspec[i].mediumTime;
   }
   
   /*                                                          */
#if 0 
   wdiAggrAddTSReqParam->wdiTsInfo.ucUapsdFlags = 
   wdiAggrAddTSReqParam->wdiTsInfo.ucServiceInterval = 
   wdiAggrAddTSReqParam->wdiTsInfo.ucSuspendInterval = 
   wdiAggrAddTSReqParam->wdiTsInfo.ucDelayedInterval = 
#endif
   wdiAggrAddTSReqParam->wdiReqStatusCB = NULL ;
   
   /*                                                         */
   pWdaParams->wdaMsgParam = (void *)pAggrAddTsReqParams ;
   /*                             */
   pWdaParams->wdaWdiApiMsgParam = (void *)wdiAggrAddTSReqParam ;

   pWdaParams->pWdaContext = pWDA;

   status = WDI_AggrAddTSReq(wdiAggrAddTSReqParam, 
                             (WDI_AggrAddTsRspCb)WDA_AggrAddTSReqCallback, pWdaParams);

   if(IS_WDI_STATUS_FAILURE(status))
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
              "Failure in ADD TS REQ Params WDI API, free all the memory " );
      vos_mem_free(pWdaParams->wdaWdiApiMsgParam) ;
      vos_mem_free(pWdaParams);

      /*                                     */
      for( i = 0; i < HAL_QOS_NUM_AC_MAX; i++ )
      {
         pAggrAddTsReqParams->status[i] = eHAL_STATUS_FAILURE ;
      }

      WDA_SendMsg(pWdaParams->pWdaContext, WDA_AGGR_QOS_RSP, 
                        (void *)pAggrAddTsReqParams , 0) ;
   }
   return CONVERT_WDI2VOS_STATUS(status) ;
}
#endif
/*
                                     
                                 
 */ 
void WDA_EnterImpsRspCallback(WDI_Status status, void* pUserData)
{
   tWDA_ReqParams *pWdaParams = (tWDA_ReqParams *)pUserData;
   tWDA_CbContext *pWDA;

   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                                          "<------ %s status=%d" ,__func__,status);
   if(NULL == pWdaParams)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                 "%s: pWdaParams received NULL", __func__);
      VOS_ASSERT(0);
      return;
   }

   pWDA = (tWDA_CbContext *)pWdaParams->pWdaContext;

   vos_mem_free(pWdaParams->wdaWdiApiMsgParam);
   vos_mem_free(pWdaParams);
   WDA_SendMsg(pWDA, WDA_ENTER_IMPS_RSP, NULL , status) ;
   return ;
}


/*
                                     
                                                  
                                                                              
 */
void WDA_EnterImpsReqCallback(WDI_Status wdiStatus, void* pUserData)
{
   tWDA_ReqParams *pWdaParams = (tWDA_ReqParams *)pUserData;
   tWDA_CbContext *pWDA;

   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
              "<------ %s, wdiStatus: %d", __func__, wdiStatus);

   if(NULL == pWdaParams)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                 "%s: pWdaParams received NULL", __func__);
      VOS_ASSERT(0);
      return;
   }

   pWDA = (tWDA_CbContext *)pWdaParams->pWdaContext;

   if(IS_WDI_STATUS_FAILURE(wdiStatus))
   {
      vos_mem_free(pWdaParams->wdaWdiApiMsgParam);
      vos_mem_free(pWdaParams);
      WDA_SendMsg(pWDA, WDA_ENTER_IMPS_RSP, NULL,
                  CONVERT_WDI2SIR_STATUS(wdiStatus));
   }

   return;
}
/*
                                    
                                            
 */ 
VOS_STATUS WDA_ProcessEnterImpsReq(tWDA_CbContext *pWDA)
{
   WDI_Status status = WDI_STATUS_SUCCESS ;
   WDI_EnterImpsReqParamsType *wdiEnterImpsReqParams;
   tWDA_ReqParams *pWdaParams;
   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                                          "------> %s " ,__func__);


   wdiEnterImpsReqParams = vos_mem_malloc(sizeof(WDI_EnterImpsReqParamsType));
   if (NULL == wdiEnterImpsReqParams)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                           "%s: VOS MEM Alloc Failure", __func__);
      VOS_ASSERT(0);
      WDA_SendMsg(pWDA, WDA_ENTER_IMPS_RSP, NULL,
                  CONVERT_WDI2SIR_STATUS(WDI_STATUS_MEM_FAILURE)) ;
      return VOS_STATUS_E_NOMEM;
   }

   pWdaParams = (tWDA_ReqParams *)vos_mem_malloc(sizeof(tWDA_ReqParams));
   if (NULL == pWdaParams)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                           "%s: VOS MEM Alloc Failure", __func__);
      VOS_ASSERT(0);
      vos_mem_free(wdiEnterImpsReqParams);
      WDA_SendMsg(pWDA, WDA_ENTER_IMPS_RSP, NULL ,
         CONVERT_WDI2SIR_STATUS(WDI_STATUS_MEM_FAILURE)) ;
      return VOS_STATUS_E_NOMEM;
   }

   wdiEnterImpsReqParams->wdiReqStatusCB = WDA_EnterImpsReqCallback;
   wdiEnterImpsReqParams->pUserData = pWdaParams;

   pWdaParams->wdaWdiApiMsgParam = wdiEnterImpsReqParams;
   pWdaParams->wdaMsgParam = NULL;
   pWdaParams->pWdaContext = pWDA;

   status = WDI_EnterImpsReq(wdiEnterImpsReqParams,
                             (WDI_EnterImpsRspCb)WDA_EnterImpsRspCallback,
                             pWdaParams);
   if(IS_WDI_STATUS_FAILURE(status))
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
              "Failure in Enter IMPS REQ WDI API, free all the memory " );
      vos_mem_free(wdiEnterImpsReqParams);
      vos_mem_free(pWdaParams);
      WDA_SendMsg(pWDA, WDA_ENTER_IMPS_RSP, NULL , CONVERT_WDI2SIR_STATUS(status)) ;
   }
   return CONVERT_WDI2VOS_STATUS(status) ;
}
/*
                                    
                                
 */ 
void WDA_ExitImpsReqCallback(WDI_Status status, void* pUserData)
{
   tWDA_CbContext *pWDA = (tWDA_CbContext *)pUserData ;
   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                                          "<------ %s " ,__func__);
   WDA_SendMsg(pWDA, WDA_EXIT_IMPS_RSP, NULL , (status)) ;
   return ;
}
/*
                                   
                                           
 */ 
VOS_STATUS WDA_ProcessExitImpsReq(tWDA_CbContext *pWDA)
{
   WDI_Status status = WDI_STATUS_SUCCESS ;
   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                                          "------> %s " ,__func__);
   status = WDI_ExitImpsReq((WDI_ExitImpsRspCb)WDA_ExitImpsReqCallback, pWDA);
   if(IS_WDI_STATUS_FAILURE(status))
   {
      if (WDI_STATUS_DEV_INTERNAL_FAILURE == status)
      {
          VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                     FL("reload wlan driver"));
          wpalWlanReload();
      }
      else
      {
          VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
              "Failure in Exit IMPS REQ WDI API, free all the memory " );
          WDA_SendMsg(pWDA, WDA_EXIT_IMPS_RSP, NULL , CONVERT_WDI2SIR_STATUS(status)) ;
      }
   }
   return CONVERT_WDI2VOS_STATUS(status) ;
}
/*
                                      
                                 
 */ 
void WDA_EnterBmpsRespCallback(WDI_EnterBmpsRspParamsType *pwdiEnterBmpsRsp, void* pUserData)
{
   tWDA_ReqParams *pWdaParams = (tWDA_ReqParams *)pUserData;
   tWDA_CbContext *pWDA;
   tEnterBmpsParams *pEnterBmpsRspParams;

   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                                          "<------ %s " ,__func__);
   if(NULL == pWdaParams)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
              "%s: pWdaParams received NULL", __func__);
      VOS_ASSERT(0) ;
      return ;
   }

   pWDA = (tWDA_CbContext *)pWdaParams->pWdaContext;
   pEnterBmpsRspParams = (tEnterBmpsParams *)pWdaParams->wdaMsgParam;

   pEnterBmpsRspParams->bssIdx = pwdiEnterBmpsRsp->bssIdx;
   pEnterBmpsRspParams->status = (pwdiEnterBmpsRsp->wdiStatus);

   vos_mem_free(pWdaParams->wdaWdiApiMsgParam) ;
   vos_mem_free(pWdaParams) ;
   WDA_SendMsg(pWDA, WDA_ENTER_BMPS_RSP, (void *)pEnterBmpsRspParams , 0);

   return ;
}
/*
                                     
                                                  
                                                                              
 */
void WDA_EnterBmpsReqCallback(WDI_Status wdiStatus, void* pUserData)
{
   tWDA_ReqParams *pWdaParams = (tWDA_ReqParams *)pUserData;
   tWDA_CbContext *pWDA;
   tEnterBmpsParams *pEnterBmpsRspParams;

   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
              "<------ %s, wdiStatus: %d", __func__, wdiStatus);

   if(NULL == pWdaParams)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                 "%s: pWdaParams received NULL", __func__);
      VOS_ASSERT(0);
      return;
   }

   pWDA = (tWDA_CbContext *)pWdaParams->pWdaContext;
   pEnterBmpsRspParams = (tEnterBmpsParams *)pWdaParams->wdaMsgParam;
   pEnterBmpsRspParams->status = wdiStatus;

   if(IS_WDI_STATUS_FAILURE(wdiStatus))
   {
      vos_mem_free(pWdaParams->wdaWdiApiMsgParam);
      vos_mem_free(pWdaParams);
      WDA_SendMsg(pWDA, WDA_ENTER_BMPS_RSP, (void *)pEnterBmpsRspParams, 0);
   }

   return;
}
/*
                                    
                                            
 */ 
VOS_STATUS WDA_ProcessEnterBmpsReq(tWDA_CbContext *pWDA,
                                   tEnterBmpsParams *pEnterBmpsReqParams)
{
   WDI_Status status = WDI_STATUS_SUCCESS;
   WDI_EnterBmpsReqParamsType *wdiEnterBmpsReqParams;
   tWDA_ReqParams *pWdaParams;
   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                                          "------> %s " ,__func__);
   if ((NULL == pWDA) || (NULL == pEnterBmpsReqParams))
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                           "%s: invalid param", __func__);
      VOS_ASSERT(0);
      return VOS_STATUS_E_FAILURE;
   }
   wdiEnterBmpsReqParams = vos_mem_malloc(sizeof(WDI_EnterBmpsReqParamsType));
   if (NULL == wdiEnterBmpsReqParams)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                           "%s: VOS MEM Alloc Failure", __func__);
      VOS_ASSERT(0);
      WDA_SendMsg(pWDA, WDA_ENTER_BMPS_RSP, NULL ,
         CONVERT_WDI2SIR_STATUS(WDI_STATUS_MEM_FAILURE)) ;
      return VOS_STATUS_E_NOMEM;
   }
   pWdaParams = (tWDA_ReqParams *)vos_mem_malloc(sizeof(tWDA_ReqParams));
   if (NULL == pWdaParams)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                           "%s: VOS MEM Alloc Failure", __func__);
      VOS_ASSERT(0);
      vos_mem_free(wdiEnterBmpsReqParams);
      WDA_SendMsg(pWDA, WDA_ENTER_BMPS_RSP, NULL ,
         CONVERT_WDI2SIR_STATUS(WDI_STATUS_MEM_FAILURE)) ;
      return VOS_STATUS_E_NOMEM;
   }
   wdiEnterBmpsReqParams->wdiEnterBmpsInfo.ucBssIdx = pEnterBmpsReqParams->bssIdx;
   wdiEnterBmpsReqParams->wdiEnterBmpsInfo.ucDtimCount = pEnterBmpsReqParams->dtimCount;
   wdiEnterBmpsReqParams->wdiEnterBmpsInfo.ucDtimPeriod = pEnterBmpsReqParams->dtimPeriod;
   wdiEnterBmpsReqParams->wdiEnterBmpsInfo.uTbtt = pEnterBmpsReqParams->tbtt;
   //                        
   wdiEnterBmpsReqParams->wdiEnterBmpsInfo.rssiFilterPeriod = (wpt_uint32)pEnterBmpsReqParams->rssiFilterPeriod;
   wdiEnterBmpsReqParams->wdiEnterBmpsInfo.numBeaconPerRssiAverage = (wpt_uint32)pEnterBmpsReqParams->numBeaconPerRssiAverage;
   wdiEnterBmpsReqParams->wdiEnterBmpsInfo.bRssiFilterEnable = (wpt_uint8)pEnterBmpsReqParams->bRssiFilterEnable;
   wdiEnterBmpsReqParams->wdiReqStatusCB = WDA_EnterBmpsReqCallback;
   wdiEnterBmpsReqParams->pUserData = pWdaParams;

   /*                                            */
   /*                             */
   pWdaParams->wdaWdiApiMsgParam = wdiEnterBmpsReqParams;
   pWdaParams->wdaMsgParam = pEnterBmpsReqParams;
   pWdaParams->pWdaContext = pWDA;
   status = WDI_EnterBmpsReq(wdiEnterBmpsReqParams,
                    (WDI_EnterBmpsRspCb)WDA_EnterBmpsRespCallback, pWdaParams);
   if (IS_WDI_STATUS_FAILURE(status))
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
              "Failure in Enter BMPS REQ WDI API, free all the memory" );
      vos_mem_free(pWdaParams->wdaWdiApiMsgParam);
      vos_mem_free(pWdaParams->wdaMsgParam);
      vos_mem_free(pWdaParams);
      WDA_SendMsg(pWDA, WDA_ENTER_BMPS_RSP, NULL , CONVERT_WDI2SIR_STATUS(status)) ;
   }
   return CONVERT_WDI2VOS_STATUS(status);
}


static void WDA_SendExitBmpsRsp(tWDA_CbContext *pWDA,
                         WDI_Status wdiStatus,
                         tExitBmpsParams *pExitBmpsReqParams)
{
   pExitBmpsReqParams->status = CONVERT_WDI2SIR_STATUS(wdiStatus) ;

   WDA_SendMsg(pWDA, WDA_EXIT_BMPS_RSP, (void *)pExitBmpsReqParams , 0) ;
}


/*
                                     
                                
 */ 
void WDA_ExitBmpsRespCallback(WDI_ExitBmpsRspParamsType *pwdiExitBmpsRsp, void* pUserData)
{
   tWDA_ReqParams *pWdaParams = (tWDA_ReqParams *)pUserData;
   tWDA_CbContext *pWDA;
   tExitBmpsParams *pExitBmpsRspParams;

   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                                          "<------ %s " ,__func__);
   if(NULL == pWdaParams)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
              "%s: pWdaParams received NULL", __func__);
      VOS_ASSERT(0) ;
      return ;
   }

   pWDA = (tWDA_CbContext *)pWdaParams->pWdaContext;
   pExitBmpsRspParams = (tExitBmpsParams *)pWdaParams->wdaMsgParam;

   pExitBmpsRspParams->bssIdx = pwdiExitBmpsRsp->bssIdx;
   pExitBmpsRspParams->status = (pwdiExitBmpsRsp->wdiStatus);

   vos_mem_free(pWdaParams->wdaWdiApiMsgParam);
   vos_mem_free(pWdaParams) ;

   WDA_SendMsg(pWDA, WDA_EXIT_BMPS_RSP, (void *)pExitBmpsRspParams , 0) ;
   return ;
}
/*
                                    
                                                 
                                                                             
 */
void WDA_ExitBmpsReqCallback(WDI_Status wdiStatus, void* pUserData)
{
   tWDA_ReqParams *pWdaParams = (tWDA_ReqParams *)pUserData;
   tWDA_CbContext *pWDA;
   tExitBmpsParams *pExitBmpsRspParams;

   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
              "<------ %s, wdiStatus: %d", __func__, wdiStatus);

   if(NULL == pWdaParams)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                 "%s: pWdaParams received NULL", __func__);
      VOS_ASSERT(0);
      return;
   }

   pWDA = (tWDA_CbContext *)pWdaParams->pWdaContext;
   pExitBmpsRspParams = (tExitBmpsParams *)pWdaParams->wdaMsgParam;
   pExitBmpsRspParams->status = wdiStatus;

   if(IS_WDI_STATUS_FAILURE(wdiStatus))
   {
      vos_mem_free(pWdaParams->wdaWdiApiMsgParam);
      vos_mem_free(pWdaParams);
      WDA_SendMsg(pWDA, WDA_EXIT_BMPS_RSP, (void *)pExitBmpsRspParams, 0);
   }

   return;
}
/*
                                   
                                           
 */ 
VOS_STATUS WDA_ProcessExitBmpsReq(tWDA_CbContext *pWDA,
                                   tExitBmpsParams *pExitBmpsReqParams)
{
   WDI_Status status = WDI_STATUS_SUCCESS ;
   WDI_ExitBmpsReqParamsType *wdiExitBmpsReqParams = 
      (WDI_ExitBmpsReqParamsType *)vos_mem_malloc(
         sizeof(WDI_ExitBmpsReqParamsType)) ;
   tWDA_ReqParams *pWdaParams ;
   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                                          "------> %s " ,__func__);
   if(NULL == wdiExitBmpsReqParams) 
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                           "%s: VOS MEM Alloc Failure", __func__); 
      VOS_ASSERT(0);
      WDA_SendExitBmpsRsp(pWDA, WDI_STATUS_MEM_FAILURE, pExitBmpsReqParams);
      return VOS_STATUS_E_NOMEM;
   }
   pWdaParams = (tWDA_ReqParams *)vos_mem_malloc(sizeof(tWDA_ReqParams)) ;
   if(NULL == pWdaParams)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                           "%s: VOS MEM Alloc Failure", __func__); 
      VOS_ASSERT(0);
      vos_mem_free(wdiExitBmpsReqParams);
      return VOS_STATUS_E_NOMEM;
   }
   wdiExitBmpsReqParams->wdiExitBmpsInfo.ucSendDataNull = pExitBmpsReqParams->sendDataNull;

   wdiExitBmpsReqParams->wdiExitBmpsInfo.bssIdx = pExitBmpsReqParams->bssIdx;

   wdiExitBmpsReqParams->wdiReqStatusCB = WDA_ExitBmpsReqCallback;
   wdiExitBmpsReqParams->pUserData = pWdaParams;

   /*                                            */
   /*                             */
   pWdaParams->wdaWdiApiMsgParam = wdiExitBmpsReqParams;
   pWdaParams->pWdaContext = pWDA;
   pWdaParams->wdaMsgParam = pExitBmpsReqParams;
   status = WDI_ExitBmpsReq(wdiExitBmpsReqParams,
                             (WDI_ExitBmpsRspCb)WDA_ExitBmpsRespCallback, pWdaParams);
   if(IS_WDI_STATUS_FAILURE(status))
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
              "Failure in Exit BMPS REQ WDI API, free all the memory " );
      vos_mem_free(pWdaParams->wdaWdiApiMsgParam);
      vos_mem_free(pWdaParams) ;
      WDA_SendExitBmpsRsp(pWDA, status, pExitBmpsReqParams);
   }
   return CONVERT_WDI2VOS_STATUS(status) ;
}
/*
                                       
                                  
 */ 
void WDA_EnterUapsdRespCallback(  WDI_EnterUapsdRspParamsType *pwdiEnterUapsdRspParams, void* pUserData)
{
   tWDA_ReqParams *pWdaParams = (tWDA_ReqParams *)pUserData;
   tWDA_CbContext *pWDA;
   tUapsdParams *pEnterUapsdRsqParams;
   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                                          "<------ %s " ,__func__);
   if(NULL == pWdaParams)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
              "%s: pWdaParams received NULL", __func__);
      VOS_ASSERT(0) ;
      return ;
   }

   pWDA = (tWDA_CbContext *)pWdaParams->pWdaContext;
   pEnterUapsdRsqParams = (tUapsdParams *)pWdaParams->wdaMsgParam;

   pEnterUapsdRsqParams->bssIdx = pwdiEnterUapsdRspParams->bssIdx;
   pEnterUapsdRsqParams->status = (pwdiEnterUapsdRspParams->wdiStatus);

   vos_mem_free(pWdaParams->wdaWdiApiMsgParam);
   vos_mem_free(pWdaParams) ;
   WDA_SendMsg(pWDA, WDA_ENTER_UAPSD_RSP, (void *)pEnterUapsdRsqParams , 0) ;
   return ;
}
/*
                                      
                                                   
                                                                               
 */
void WDA_EnterUapsdReqCallback(WDI_Status wdiStatus, void* pUserData)
{
   tWDA_ReqParams *pWdaParams = (tWDA_ReqParams *)pUserData;
   tWDA_CbContext *pWDA;
   tUapsdParams *pEnterUapsdRsqParams;

   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
              "<------ %s, wdiStatus: %d", __func__, wdiStatus);

   if(NULL == pWdaParams)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                 "%s: pWdaParams received NULL", __func__);
      VOS_ASSERT(0);
      return;
   }

   pWDA = (tWDA_CbContext *)pWdaParams->pWdaContext;
   pEnterUapsdRsqParams = (tUapsdParams *)pWdaParams->wdaMsgParam;
   pEnterUapsdRsqParams->status = wdiStatus;

   if(IS_WDI_STATUS_FAILURE(wdiStatus))
   {
      vos_mem_free(pWdaParams->wdaWdiApiMsgParam);
      vos_mem_free(pWdaParams);
      WDA_SendMsg(pWDA, WDA_ENTER_UAPSD_RSP, (void *)pEnterUapsdRsqParams, 0);
   }

   return;
}
/*
                                     
                                             
 */ 
VOS_STATUS WDA_ProcessEnterUapsdReq(tWDA_CbContext *pWDA,
                                    tUapsdParams *pEnterUapsdReqParams)
{
   WDI_Status status = WDI_STATUS_SUCCESS ;
   WDI_EnterUapsdReqParamsType *wdiEnterUapsdReqParams = 
      (WDI_EnterUapsdReqParamsType *)vos_mem_malloc(
         sizeof(WDI_EnterUapsdReqParamsType)) ;
   tWDA_ReqParams *pWdaParams ;
   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                                          "------> %s " ,__func__);
   if(NULL == wdiEnterUapsdReqParams) 
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                           "%s: VOS MEM Alloc Failure", __func__); 
      VOS_ASSERT(0);
      return VOS_STATUS_E_NOMEM;
   }
   pWdaParams = (tWDA_ReqParams *)vos_mem_malloc(sizeof(tWDA_ReqParams)) ;
   if(NULL == pWdaParams)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                           "%s: VOS MEM Alloc Failure", __func__); 
      VOS_ASSERT(0);
      vos_mem_free(wdiEnterUapsdReqParams);
      return VOS_STATUS_E_NOMEM;
   }
   wdiEnterUapsdReqParams->wdiEnterUapsdInfo.ucBeDeliveryEnabled = 
      pEnterUapsdReqParams->beDeliveryEnabled;
   wdiEnterUapsdReqParams->wdiEnterUapsdInfo.ucBeTriggerEnabled = 
      pEnterUapsdReqParams->beTriggerEnabled;
   wdiEnterUapsdReqParams->wdiEnterUapsdInfo.ucBkDeliveryEnabled = 
      pEnterUapsdReqParams->bkDeliveryEnabled;
   wdiEnterUapsdReqParams->wdiEnterUapsdInfo.ucBkTriggerEnabled = 
      pEnterUapsdReqParams->bkTriggerEnabled;
   wdiEnterUapsdReqParams->wdiEnterUapsdInfo.ucViDeliveryEnabled = 
      pEnterUapsdReqParams->viDeliveryEnabled;
   wdiEnterUapsdReqParams->wdiEnterUapsdInfo.ucViTriggerEnabled = 
      pEnterUapsdReqParams->viTriggerEnabled;
   wdiEnterUapsdReqParams->wdiEnterUapsdInfo.ucVoDeliveryEnabled = 
      pEnterUapsdReqParams->voDeliveryEnabled;
   wdiEnterUapsdReqParams->wdiEnterUapsdInfo.ucVoTriggerEnabled = 
      pEnterUapsdReqParams->voTriggerEnabled;
   wdiEnterUapsdReqParams->wdiEnterUapsdInfo.bssIdx = pEnterUapsdReqParams->bssIdx;

   wdiEnterUapsdReqParams->wdiReqStatusCB = WDA_EnterUapsdReqCallback;
   wdiEnterUapsdReqParams->pUserData = pWdaParams;

   /*                                            */
   /*                             */
   pWdaParams->wdaWdiApiMsgParam = wdiEnterUapsdReqParams;
   pWdaParams->pWdaContext = pWDA;
   pWdaParams->wdaMsgParam = pEnterUapsdReqParams;
   status = WDI_EnterUapsdReq(wdiEnterUapsdReqParams,
                              (WDI_EnterUapsdRspCb)WDA_EnterUapsdRespCallback, pWdaParams);
   if(IS_WDI_STATUS_FAILURE(status))
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
              "Failure in Enter UAPSD REQ WDI API, free all the memory " );
      vos_mem_free(pWdaParams->wdaMsgParam) ;
      vos_mem_free(pWdaParams->wdaWdiApiMsgParam);
      vos_mem_free(pWdaParams) ;
   }
   return CONVERT_WDI2VOS_STATUS(status) ;
}
/*
                                      
                                 
 */ 
void WDA_ExitUapsdRespCallback(WDI_ExitUapsdRspParamsType *pwdiExitRspParam, void* pUserData)
{

   tWDA_ReqParams *pWdaParams = (tWDA_ReqParams *)pUserData;
   tWDA_CbContext *pWDA;
   tExitUapsdParams *pExitUapsdRspParams;

   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                                          "<------ %s " ,__func__);
   if(NULL == pWdaParams)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                "%s: pWdaParams received NULL", __func__);
      VOS_ASSERT(0);
      return;
   }

   pWDA = (tWDA_CbContext *)pWdaParams->pWdaContext;
   pExitUapsdRspParams = (tExitUapsdParams *)pWdaParams->wdaMsgParam;

   pExitUapsdRspParams->bssIdx = pwdiExitRspParam->bssIdx;
   pExitUapsdRspParams->status = (pwdiExitRspParam->wdiStatus);

   vos_mem_free(pWdaParams->wdaWdiApiMsgParam);
   vos_mem_free(pWdaParams) ;

   WDA_SendMsg(pWDA, WDA_EXIT_UAPSD_RSP, (void *)pExitUapsdRspParams, 0) ;
   return ;
}
/*
                                     
                                                  
                                                                              
 */
void WDA_ExitUapsdReqCallback(WDI_Status wdiStatus, void* pUserData)
{
   tWDA_ReqParams *pWdaParams = (tWDA_ReqParams *)pUserData;
   tWDA_CbContext *pWDA;
   tExitUapsdParams *pExitUapsdRspParams;

   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
              "<------ %s, wdiStatus: %d", __func__, wdiStatus);

   if(NULL == pWdaParams)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                 "%s: pWdaParams received NULL", __func__);
      VOS_ASSERT(0);
      return;
   }

   pWDA = (tWDA_CbContext *)pWdaParams->pWdaContext;
   pExitUapsdRspParams = (tExitUapsdParams *)pWdaParams->wdaMsgParam;
   pExitUapsdRspParams->status = wdiStatus;

   if(IS_WDI_STATUS_FAILURE(wdiStatus))
   {
      vos_mem_free(pWdaParams->wdaWdiApiMsgParam);
      vos_mem_free(pWdaParams);
      WDA_SendMsg(pWDA, WDA_EXIT_UAPSD_RSP, (void *)pExitUapsdRspParams, 0);
   }

   return;
}
/*
                                    
                                            
 */ 
VOS_STATUS WDA_ProcessExitUapsdReq(tWDA_CbContext *pWDA, 
                                   tExitUapsdParams  *pExitUapsdParams)
{
   WDI_Status status = WDI_STATUS_SUCCESS ;
   tWDA_ReqParams *pWdaParams ;
   WDI_ExitUapsdReqParamsType *wdiExitUapsdReqParams = 
                      (WDI_ExitUapsdReqParamsType *)vos_mem_malloc(
                         sizeof(WDI_ExitUapsdReqParamsType)) ;

   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                                          "------> %s " ,__func__);

   if(NULL == wdiExitUapsdReqParams) 
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                           "%s: VOS MEM Alloc Failure", __func__); 
      VOS_ASSERT(0);
      return VOS_STATUS_E_NOMEM;
   }
   pWdaParams = (tWDA_ReqParams *)vos_mem_malloc(sizeof(tWDA_ReqParams)) ;
   if(NULL == pWdaParams)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                           "%s: VOS MEM Alloc Failure", __func__); 
      VOS_ASSERT(0);
      vos_mem_free(wdiExitUapsdReqParams);
      return VOS_STATUS_E_NOMEM;
   }

   wdiExitUapsdReqParams->wdiExitUapsdInfo.bssIdx = pExitUapsdParams->bssIdx;
   wdiExitUapsdReqParams->wdiReqStatusCB = WDA_ExitUapsdReqCallback;
   wdiExitUapsdReqParams->pUserData = pWdaParams;

     /*                                            */
   /*                             */
   pWdaParams->wdaWdiApiMsgParam = wdiExitUapsdReqParams;
   pWdaParams->pWdaContext = pWDA;
   pWdaParams->wdaMsgParam = pExitUapsdParams;

   status = WDI_ExitUapsdReq(wdiExitUapsdReqParams, (WDI_ExitUapsdRspCb)WDA_ExitUapsdRespCallback, pWdaParams);
   if(IS_WDI_STATUS_FAILURE(status))
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
              "Failure in Exit UAPSD REQ WDI API, free all the memory " );
      vos_mem_free(pWdaParams->wdaMsgParam) ;
      vos_mem_free(pWdaParams->wdaWdiApiMsgParam);
      vos_mem_free(pWdaParams) ;

   }
   return CONVERT_WDI2VOS_STATUS(status) ;
}

/*
                                         
   
 */ 
void WDA_SetPwrSaveCfgReqCallback(WDI_Status status, void* pUserData)
{
   tWDA_ReqParams *pWdaParams = (tWDA_ReqParams *)pUserData;
   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                                          "<------ %s " ,__func__);
   if(NULL == pWdaParams)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
              "%s: pWdaParams received NULL", __func__);
      VOS_ASSERT(0) ;
      return ;
   }
   if( pWdaParams != NULL )
   {
      if( pWdaParams->wdaWdiApiMsgParam != NULL )
      {
         vos_mem_free(pWdaParams->wdaWdiApiMsgParam);
      }
      if( pWdaParams->wdaMsgParam != NULL )
      {
         vos_mem_free(pWdaParams->wdaMsgParam) ;
      }
      vos_mem_free(pWdaParams) ;
   }
   return ;
}
/*
                                        
                                                        
 */ 
VOS_STATUS WDA_ProcessSetPwrSaveCfgReq(tWDA_CbContext *pWDA, 
                                       tSirPowerSaveCfg *pPowerSaveCfg)
{
   WDI_Status status = WDI_STATUS_SUCCESS ;
   tHalCfg        *tlvStruct = NULL ;
   tANI_U8        *tlvStructStart = NULL ;
   v_PVOID_t      *configParam;
   tANI_U32       configParamSize;
   tANI_U32       *configDataValue;
   WDI_UpdateCfgReqParamsType *wdiPowerSaveCfg;
   tWDA_ReqParams *pWdaParams;
   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                                          "------> %s " ,__func__);
   if ((NULL == pWDA) || (NULL == pPowerSaveCfg))
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                           "%s: invalid param", __func__);
      VOS_ASSERT(0);
      vos_mem_free(pPowerSaveCfg);
      return VOS_STATUS_E_FAILURE;
   }
   wdiPowerSaveCfg = vos_mem_malloc(sizeof(WDI_UpdateCfgReqParamsType));
   if (NULL == wdiPowerSaveCfg)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                           "%s: VOS MEM Alloc Failure", __func__);
      VOS_ASSERT(0);
      vos_mem_free(pPowerSaveCfg);
      return VOS_STATUS_E_NOMEM;
   }
   pWdaParams = (tWDA_ReqParams *)vos_mem_malloc(sizeof(tWDA_ReqParams)) ;
   if(NULL == pWdaParams)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                           "%s: VOS MEM Alloc Failure", __func__); 
      VOS_ASSERT(0);
      vos_mem_free(wdiPowerSaveCfg);
      vos_mem_free(pPowerSaveCfg);
      return VOS_STATUS_E_NOMEM;
   }
   configParamSize = (sizeof(tHalCfg) + (sizeof(tANI_U32))) * WDA_NUM_PWR_SAVE_CFG;
   configParam = vos_mem_malloc(configParamSize);
   if(NULL == configParam)
   {
      VOS_TRACE(VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                "%s: VOS MEM Alloc Failure", __func__);
      VOS_ASSERT(0);
      vos_mem_free(pWdaParams);
      vos_mem_free(wdiPowerSaveCfg);
      vos_mem_free(pPowerSaveCfg);
      return VOS_STATUS_E_NOMEM;
   }
   vos_mem_set(configParam, configParamSize, 0);
   wdiPowerSaveCfg->pConfigBuffer = configParam;
   tlvStruct = (tHalCfg *)configParam;
   tlvStructStart = (tANI_U8 *)configParam;
   /*                                                */
   tlvStruct->type = QWLAN_HAL_CFG_PS_BROADCAST_FRAME_FILTER_ENABLE;
   tlvStruct->length = sizeof(tANI_U32);
   configDataValue = (tANI_U32 *)(tlvStruct + 1);
   *configDataValue = (tANI_U32)pPowerSaveCfg->broadcastFrameFilter;
   tlvStruct = (tHalCfg *)(( (tANI_U8 *) tlvStruct 
                            + sizeof(tHalCfg) + tlvStruct->length)) ; 
   /*                                       */
   tlvStruct->type = QWLAN_HAL_CFG_PS_HEART_BEAT_THRESHOLD;
   tlvStruct->length = sizeof(tANI_U32);
   configDataValue = (tANI_U32 *)(tlvStruct + 1);
   *configDataValue = (tANI_U32)pPowerSaveCfg->HeartBeatCount;
   tlvStruct = (tHalCfg *)(( (tANI_U8 *) tlvStruct 
                            + sizeof(tHalCfg) + tlvStruct->length)) ; 
   /*                              */
   tlvStruct->type = QWLAN_HAL_CFG_PS_IGNORE_DTIM;
   tlvStruct->length = sizeof(tANI_U32);
   configDataValue = (tANI_U32 *)(tlvStruct + 1);
   *configDataValue = (tANI_U32)pPowerSaveCfg->ignoreDtim;
   tlvStruct = (tHalCfg *)(( (tANI_U8 *) tlvStruct 
                            + sizeof(tHalCfg) + tlvStruct->length)) ; 
   /*                                  */
   tlvStruct->type = QWLAN_HAL_CFG_PS_LISTEN_INTERVAL;
   tlvStruct->length = sizeof(tANI_U32);
   configDataValue = (tANI_U32 *)(tlvStruct + 1);
   *configDataValue = (tANI_U32)pPowerSaveCfg->listenInterval;
   tlvStruct = (tHalCfg *)(( (tANI_U8 *) tlvStruct 
                            + sizeof(tHalCfg) + tlvStruct->length)) ; 
   /*                              */
   tlvStruct->type = QWLAN_HAL_CFG_PS_MAX_PS_POLL;
   tlvStruct->length = sizeof(tANI_U32);
   configDataValue = (tANI_U32 *)(tlvStruct + 1);
   *configDataValue = (tANI_U32)pPowerSaveCfg->maxPsPoll;
   tlvStruct = (tHalCfg *)(( (tANI_U8 *) tlvStruct 
                            + sizeof(tHalCfg) + tlvStruct->length)) ; 
   /*                                     */
   tlvStruct->type = QWLAN_HAL_CFG_PS_MIN_RSSI_THRESHOLD;
   tlvStruct->length = sizeof(tANI_U32);
   configDataValue = (tANI_U32 *)(tlvStruct + 1);
   *configDataValue = (tANI_U32)pPowerSaveCfg->minRssiThreshold;
   tlvStruct = (tHalCfg *)(( (tANI_U8 *) tlvStruct 
                            + sizeof(tHalCfg) + tlvStruct->length)) ; 
   /*                                    */
   tlvStruct->type = QWLAN_HAL_CFG_PS_NTH_BEACON_FILTER;
   tlvStruct->length = sizeof(tANI_U32);
   configDataValue = (tANI_U32 *)(tlvStruct + 1);
   *configDataValue = (tANI_U32)pPowerSaveCfg->nthBeaconFilter;
   tlvStruct = (tHalCfg *)(( (tANI_U8 *) tlvStruct 
                            + sizeof(tHalCfg) + tlvStruct->length)) ; 
   /*                                        */
   tlvStruct->type = QWLAN_HAL_CFG_PS_ENABLE_BCN_EARLY_TERM;
   tlvStruct->length = sizeof(tANI_U32);
   configDataValue = (tANI_U32 *)(tlvStruct + 1);
   *configDataValue = (tANI_U32)pPowerSaveCfg->fEnableBeaconEarlyTermination;
   tlvStruct = (tHalCfg *)(( (tANI_U8 *) tlvStruct 
                            + sizeof(tHalCfg) + tlvStruct->length)) ; 
   /*                                              */
   tlvStruct->type = QWLAN_HAL_CFG_BCN_EARLY_TERM_WAKEUP_INTERVAL;
   tlvStruct->length = sizeof(tANI_U32);
   configDataValue = (tANI_U32 *)(tlvStruct + 1);
   *configDataValue = (tANI_U32)pPowerSaveCfg->bcnEarlyTermWakeInterval;
   tlvStruct = (tHalCfg *)(( (tANI_U8 *) tlvStruct 
                            + sizeof(tHalCfg) + tlvStruct->length)) ; 
   /*                                           */
   tlvStruct->type = QWLAN_HAL_CFG_NUM_BEACON_PER_RSSI_AVERAGE;
   tlvStruct->length = sizeof(tANI_U32);
   configDataValue = (tANI_U32 *)(tlvStruct + 1);
   *configDataValue = (tANI_U32)pPowerSaveCfg->numBeaconPerRssiAverage;
   tlvStruct = (tHalCfg *)(( (tANI_U8 *) tlvStruct 
                            + sizeof(tHalCfg) + tlvStruct->length)) ; 
   /*                                     */
   tlvStruct->type = QWLAN_HAL_CFG_PS_RSSI_FILTER_PERIOD;
   tlvStruct->length = sizeof(tANI_U32);
   configDataValue = (tANI_U32 *)(tlvStruct + 1);
   *configDataValue = (tANI_U32)pPowerSaveCfg->rssiFilterPeriod;
   tlvStruct = (tHalCfg *)(( (tANI_U8 *) tlvStruct 
                            + sizeof(tHalCfg) + tlvStruct->length)) ; 
   wdiPowerSaveCfg->uConfigBufferLen = (tANI_U8 *)tlvStruct - tlvStructStart ;
   wdiPowerSaveCfg->wdiReqStatusCB = NULL;
   /*                             */
   pWdaParams->wdaMsgParam = configParam;
   pWdaParams->wdaWdiApiMsgParam = wdiPowerSaveCfg;
   pWdaParams->pWdaContext = pWDA;
   status = WDI_SetPwrSaveCfgReq(wdiPowerSaveCfg, 
                                 (WDI_SetPwrSaveCfgCb)WDA_SetPwrSaveCfgReqCallback, pWdaParams);
   if(IS_WDI_STATUS_FAILURE(status))
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
              "Failure in Set Pwr Save CFG REQ WDI API, free all the memory " );
      vos_mem_free(pWdaParams->wdaMsgParam);
      vos_mem_free(pWdaParams->wdaWdiApiMsgParam);
      vos_mem_free(pWdaParams);
   }
   vos_mem_free(pPowerSaveCfg);
   return CONVERT_WDI2VOS_STATUS(status);
}
/*
                                             
   
 */ 
void WDA_SetUapsdAcParamsRespCallback(WDI_Status status, void* pUserData)
{
   tWDA_ReqParams *pWdaParams = (tWDA_ReqParams *)pUserData ;

   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                                          "<------ %s " ,__func__);

   if(NULL == pWdaParams)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                 "%s: pWdaParams received NULL", __func__);
      VOS_ASSERT(0);
      return ;
   }

   vos_mem_free(pWdaParams->wdaWdiApiMsgParam);
   vos_mem_free(pWdaParams);

   return ;
}
/*
                                            
               
                                                                                    
 */
void WDA_SetUapsdAcParamsReqCallback(WDI_Status wdiStatus, void* pUserData)
{
   tWDA_ReqParams *pWdaParams = (tWDA_ReqParams *)pUserData;

   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
              "<------ %s, wdiStatus: %d", __func__, wdiStatus);

   if(NULL == pWdaParams)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                 "%s: pWdaParams received NULL", __func__);
      VOS_ASSERT(0);
      return;
   }

   if(IS_WDI_STATUS_FAILURE(wdiStatus))
   {
      vos_mem_free(pWdaParams->wdaWdiApiMsgParam);
      vos_mem_free(pWdaParams);
   }

   return;
}
/*
                                    
                                                               
 */ 
VOS_STATUS WDA_SetUapsdAcParamsReq(v_PVOID_t pVosContext, v_U8_t staIdx,
                                                 tUapsdInfo *pUapsdInfo)
{
   WDI_Status status = WDI_STATUS_SUCCESS;
   tWDA_CbContext *pWDA = NULL ; 
   WDI_SetUapsdAcParamsReqParamsType *wdiUapsdParams = 
      (WDI_SetUapsdAcParamsReqParamsType *)vos_mem_malloc(
         sizeof(WDI_SetUapsdAcParamsReqParamsType)) ;
   tWDA_ReqParams *pWdaParams ;
   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                                          "------> %s " ,__func__);
   if(NULL == wdiUapsdParams) 
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                           "%s: VOS MEM Alloc Failure", __func__); 
      VOS_ASSERT(0);
      return VOS_STATUS_E_NOMEM;
   }
   pWdaParams = (tWDA_ReqParams *)vos_mem_malloc(sizeof(tWDA_ReqParams)) ;
   if(NULL == pWdaParams)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                           "%s: VOS MEM Alloc Failure", __func__); 
      VOS_ASSERT(0);
      vos_mem_free(wdiUapsdParams);
      return VOS_STATUS_E_NOMEM;
   }
   wdiUapsdParams->wdiUapsdInfo.ucAc = pUapsdInfo->ac;
   wdiUapsdParams->wdiUapsdInfo.uDelayInterval = pUapsdInfo->delayInterval;
   wdiUapsdParams->wdiUapsdInfo.uSrvInterval = pUapsdInfo->srvInterval;
   wdiUapsdParams->wdiUapsdInfo.ucSTAIdx = pUapsdInfo->staidx;
   wdiUapsdParams->wdiUapsdInfo.uSusInterval = pUapsdInfo->susInterval;
   wdiUapsdParams->wdiUapsdInfo.ucUp = pUapsdInfo->up;
   wdiUapsdParams->wdiReqStatusCB = WDA_SetUapsdAcParamsReqCallback;
   wdiUapsdParams->pUserData = pWdaParams;

   pWDA = vos_get_context( VOS_MODULE_ID_WDA, pVosContext );
   pWdaParams->pWdaContext = pWDA;
   /*                                            */
   pWdaParams->wdaMsgParam = pUapsdInfo;
   /*                             */
   pWdaParams->wdaWdiApiMsgParam = (void *)wdiUapsdParams;
   status = WDI_SetUapsdAcParamsReq(wdiUapsdParams, 
              (WDI_SetUapsdAcParamsCb)WDA_SetUapsdAcParamsRespCallback,
              pWdaParams);
   if(IS_WDI_STATUS_FAILURE(status))
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
              "Failure in Set UAPSD params REQ WDI API, free all the memory " );
      vos_mem_free(pWdaParams->wdaWdiApiMsgParam) ;
      vos_mem_free(pWdaParams);
   }
   if((WDI_STATUS_SUCCESS == status) || (WDI_STATUS_PENDING == status))
     return VOS_STATUS_SUCCESS;
   else
     return VOS_STATUS_E_FAILURE;

}
/* 
                                       
                                                                               
                                                                             
                                                                        
                           
   
 */
VOS_STATUS WDA_ClearUapsdAcParamsReq(v_PVOID_t pVosContext, v_U8_t staIdx, wpt_uint8 ac)
{
   /*            */
   return VOS_STATUS_SUCCESS;
}
/*
                                              
   
 */ 
void WDA_UpdateUapsdParamsRespCallback(WDI_Status status, void* pUserData)
{
   tWDA_ReqParams *pWdaParams = (tWDA_ReqParams *)pUserData ; 

   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                                          "<------ %s " ,__func__);

   if(NULL == pWdaParams)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
              "%s: pWdaParams received NULL", __func__);
      VOS_ASSERT(0) ;
      return ;
   }

   vos_mem_free(pWdaParams->wdaWdiApiMsgParam) ;
   vos_mem_free(pWdaParams->wdaMsgParam);
   vos_mem_free(pWdaParams);

   //                               
   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
           "WDA_UpdateUapsdParamsRespCallback invoked " );
   return ;
}
/*
                                             
               
                                                                                     
 */
void WDA_UpdateUapsdParamsReqCallback(WDI_Status wdiStatus, void* pUserData)
{
   tWDA_ReqParams *pWdaParams = (tWDA_ReqParams *)pUserData;

   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
              "<------ %s, wdiStatus: %d", __func__, wdiStatus);

   if(NULL == pWdaParams)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                 "%s: pWdaParams received NULL", __func__);
      VOS_ASSERT(0);
      return;
   }

   if(IS_WDI_STATUS_FAILURE(wdiStatus))
   {
      vos_mem_free(pWdaParams->wdaWdiApiMsgParam);
      vos_mem_free(pWdaParams->wdaMsgParam);
      vos_mem_free(pWdaParams);
   }

   return;
}
/*
                                     
                                                                        
 */ 
VOS_STATUS WDA_UpdateUapsdParamsReq(tWDA_CbContext *pWDA, 
                                    tUpdateUapsdParams* pUpdateUapsdInfo)
{
   VOS_STATUS status = VOS_STATUS_SUCCESS;
   WDI_Status wstatus;
   WDI_UpdateUapsdReqParamsType *wdiUpdateUapsdParams = 
      (WDI_UpdateUapsdReqParamsType *)vos_mem_malloc(
         sizeof(WDI_UpdateUapsdReqParamsType)) ;
   tWDA_ReqParams *pWdaParams = NULL;

   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                                          "------> %s " ,__func__);
   if(NULL == wdiUpdateUapsdParams) 
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                           "%s: VOS MEM Alloc Failure", __func__); 
      VOS_ASSERT(0);
      return VOS_STATUS_E_NOMEM;
   }
   wdiUpdateUapsdParams->wdiUpdateUapsdInfo.uMaxSpLen = pUpdateUapsdInfo->maxSpLen;
   wdiUpdateUapsdParams->wdiUpdateUapsdInfo.ucSTAIdx = pUpdateUapsdInfo->staIdx;
   wdiUpdateUapsdParams->wdiUpdateUapsdInfo.ucUapsdACMask = pUpdateUapsdInfo->uapsdACMask;
   wdiUpdateUapsdParams->wdiReqStatusCB = WDA_UpdateUapsdParamsReqCallback;
   wdiUpdateUapsdParams->pUserData = pWdaParams;

   pWdaParams = (tWDA_ReqParams *)vos_mem_malloc(sizeof(tWDA_ReqParams)) ;
   if(NULL == pWdaParams)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                           "%s: VOS MEM Alloc Failure", __func__); 
      VOS_ASSERT(0);
      vos_mem_free(pUpdateUapsdInfo);
      vos_mem_free(wdiUpdateUapsdParams);
      return VOS_STATUS_E_NOMEM;
   }
   /*                                            */
   pWdaParams->wdaMsgParam = pUpdateUapsdInfo;
   /*                             */
   pWdaParams->wdaWdiApiMsgParam = (void *)wdiUpdateUapsdParams;
   pWdaParams->pWdaContext = pWDA;

   wstatus = WDI_UpdateUapsdParamsReq(wdiUpdateUapsdParams, 
                    (WDI_UpdateUapsdParamsCb)WDA_UpdateUapsdParamsRespCallback,
                    pWdaParams);

   if(IS_WDI_STATUS_FAILURE(wstatus))
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
              "Failure in Set UAPSD params REQ WDI API, free all the memory " );
      status = CONVERT_WDI2VOS_STATUS(wstatus) ;
      vos_mem_free(pWdaParams->wdaWdiApiMsgParam) ;
      vos_mem_free(pWdaParams->wdaMsgParam);
      vos_mem_free(pWdaParams);
   }
   return status;
}
/*
                                               
   
 */ 
void WDA_ConfigureRxpFilterRespCallback(WDI_Status   wdiStatus, void* pUserData)
{
   tWDA_ReqParams *pWdaParams = (tWDA_ReqParams *)pUserData ; 
   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                                          "<------ %s " ,__func__);
   if(WDI_STATUS_SUCCESS != wdiStatus)
   {
      VOS_TRACE(VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                  "%s: RXP config filter failure", __func__ );
   }
   if(NULL == pWdaParams)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
              "%s: pWdaParams received NULL", __func__);
      VOS_ASSERT(0) ;
      return ;
   }
   vos_mem_free(pWdaParams->wdaWdiApiMsgParam) ;
   vos_mem_free(pWdaParams->wdaMsgParam);
   vos_mem_free(pWdaParams);
   return ;
}
/*
                                              
               
                                                                                      
 */
void WDA_ConfigureRxpFilterReqCallback(WDI_Status wdiStatus, void* pUserData)
{
   tWDA_ReqParams *pWdaParams = (tWDA_ReqParams *)pUserData;

   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
              "<------ %s, wdiStatus: %d", __func__, wdiStatus);

   if(NULL == pWdaParams)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                 "%s: pWdaParams received NULL", __func__);
      VOS_ASSERT(0);
      return;
   }

   if(IS_WDI_STATUS_FAILURE(wdiStatus))
   {
      vos_mem_free(pWdaParams->wdaWdiApiMsgParam);
      vos_mem_free(pWdaParams->wdaMsgParam);
      vos_mem_free(pWdaParams);
   }

   return;
}
/*
                                             
   
 */ 
VOS_STATUS WDA_ProcessConfigureRxpFilterReq(tWDA_CbContext *pWDA, 
                          tSirWlanSetRxpFilters *pWlanSuspendParam)
{
   VOS_STATUS status = VOS_STATUS_SUCCESS;
   WDI_Status wstatus;
   WDI_ConfigureRxpFilterReqParamsType *wdiRxpFilterParams;
   tWDA_ReqParams *pWdaParams ;
   /*             
                                                                          */
   if(NULL == pWlanSuspendParam)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                           "%s: pWlanSuspendParam received NULL", __func__);
      VOS_ASSERT(0) ;
      return VOS_STATUS_E_FAULT;
   }
   wdiRxpFilterParams = (WDI_ConfigureRxpFilterReqParamsType *)vos_mem_malloc(
         sizeof(WDI_ConfigureRxpFilterReqParamsType)) ;
   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                                          "------> %s " ,__func__);
   if(NULL == wdiRxpFilterParams) 
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                           "%s: VOS MEM Alloc Failure", __func__); 
      VOS_ASSERT(0);
      vos_mem_free(pWlanSuspendParam);
      return VOS_STATUS_E_NOMEM;
   }
   pWdaParams = (tWDA_ReqParams *)vos_mem_malloc(sizeof(tWDA_ReqParams)) ;
   if(NULL == pWdaParams)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                           "%s: VOS MEM Alloc Failure", __func__); 
      VOS_ASSERT(0);
      vos_mem_free(wdiRxpFilterParams);
      vos_mem_free(pWlanSuspendParam);
      return VOS_STATUS_E_NOMEM;
   }
   wdiRxpFilterParams->wdiRxpFilterParam.ucSetMcstBcstFilter = 
             pWlanSuspendParam->setMcstBcstFilter;
   wdiRxpFilterParams->wdiRxpFilterParam.ucSetMcstBcstFilterSetting = 
             pWlanSuspendParam->configuredMcstBcstFilterSetting;
   
   wdiRxpFilterParams->wdiReqStatusCB = WDA_ConfigureRxpFilterReqCallback;
   wdiRxpFilterParams->pUserData = pWdaParams;

   pWdaParams->pWdaContext = pWDA;
   pWdaParams->wdaMsgParam = pWlanSuspendParam;
   pWdaParams->wdaWdiApiMsgParam = (void *)wdiRxpFilterParams;
   wstatus = WDI_ConfigureRxpFilterReq(wdiRxpFilterParams, 
                      (WDI_ConfigureRxpFilterCb)WDA_ConfigureRxpFilterRespCallback,
                      pWdaParams);
   if(IS_WDI_STATUS_FAILURE(wstatus))
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
              "Failure in configure RXP filter REQ WDI API, free all the memory " );
      status = CONVERT_WDI2VOS_STATUS(wstatus);
      vos_mem_free(pWdaParams->wdaWdiApiMsgParam) ;
      vos_mem_free(pWdaParams->wdaMsgParam);
      vos_mem_free(pWdaParams);
   }
   return status;
}
/*
                                      
   
 */ 
void WDA_WdiIndicationCallback( WDI_Status   wdiStatus,
                                void*        pUserData)
{
   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                                          "<------ %s " ,__func__);
}
/*
                                      
   
 */ 
VOS_STATUS WDA_ProcessWlanSuspendInd(tWDA_CbContext *pWDA,
                              tSirWlanSuspendParam *pWlanSuspendParam)
{
   WDI_Status wdiStatus;
   WDI_SuspendParamsType wdiSuspendParams;
   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                                          "------> %s " ,__func__);
   wdiSuspendParams.wdiSuspendParams.ucConfiguredMcstBcstFilterSetting =
                          pWlanSuspendParam->configuredMcstBcstFilterSetting;
   wdiSuspendParams.wdiReqStatusCB = WDA_WdiIndicationCallback;
   wdiSuspendParams.pUserData = pWDA;
   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO, "%s: %d" ,__func__, pWlanSuspendParam->configuredMcstBcstFilterSetting);
   wdiStatus = WDI_HostSuspendInd(&wdiSuspendParams);
   if(WDI_STATUS_PENDING == wdiStatus)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
              "Pending received for %s:%d ",__func__,__LINE__ );
   }
   else if( WDI_STATUS_SUCCESS_SYNC != wdiStatus )
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
              "Failure in %s:%d ",__func__,__LINE__ );
   }
   vos_mem_free(pWlanSuspendParam);
   return CONVERT_WDI2VOS_STATUS(wdiStatus) ;
}

#ifdef WLAN_FEATURE_11W
/*
                                          
  
 */
VOS_STATUS WDA_ProcessExcludeUnecryptInd(tWDA_CbContext *pWDA,
                              tSirWlanExcludeUnencryptParam *pExclUnencryptParam)
{
   WDI_Status wdiStatus;
   WDI_ExcludeUnencryptIndType wdiExclUnencryptParams;
   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                                          "------> %s ", __func__);

   wdiExclUnencryptParams.bExcludeUnencrypt = pExclUnencryptParam->excludeUnencrypt;
   vos_mem_copy(wdiExclUnencryptParams.bssid, pExclUnencryptParam->bssId,
                sizeof(tSirMacAddr));

   wdiExclUnencryptParams.wdiReqStatusCB = NULL;
   wdiExclUnencryptParams.pUserData = pWDA;

   wdiStatus = WDI_ExcludeUnencryptedInd(&wdiExclUnencryptParams);
   if(WDI_STATUS_PENDING == wdiStatus)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
              "Pending received for %s:%d ", __func__, __LINE__ );
   }
   else if( WDI_STATUS_SUCCESS_SYNC != wdiStatus )
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
              "Failure in %s:%d ", __func__, __LINE__ );
   }
   vos_mem_free(pExclUnencryptParam);
   return CONVERT_WDI2VOS_STATUS(wdiStatus) ;
}
#endif

/*
                                          
   
 */ 
void WDA_ProcessWlanResumeCallback(
                        WDI_SuspendResumeRspParamsType   *resumeRspParams,
                        void*        pUserData)
{
   tWDA_ReqParams *pWdaParams = (tWDA_ReqParams *)pUserData; 
   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                                          "<------ %s " ,__func__);
   if(NULL == pWdaParams)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
              "%s: pWdaParams received NULL", __func__);
      VOS_ASSERT(0) ;
      return ;
   }
   if(WDI_STATUS_SUCCESS != resumeRspParams->wdiStatus)
   {
      VOS_TRACE(VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                  "%s: Process Wlan Resume failure", __func__ );
   }
   vos_mem_free(pWdaParams->wdaWdiApiMsgParam) ;
   vos_mem_free(pWdaParams->wdaMsgParam);
   vos_mem_free(pWdaParams);
   return ;
}
/*
                                     
   
 */ 
VOS_STATUS WDA_ProcessWlanResumeReq(tWDA_CbContext *pWDA,
                              tSirWlanResumeParam *pWlanResumeParam)
{
   WDI_Status wdiStatus;
   WDI_ResumeParamsType *wdiResumeParams = 
            (WDI_ResumeParamsType *)vos_mem_malloc(
                                 sizeof(WDI_ResumeParamsType) ) ;
   tWDA_ReqParams *pWdaParams ;
   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                                          "------> %s " ,__func__);
   if(NULL == wdiResumeParams) 
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                           "%s: VOS MEM Alloc Failure", __func__); 
      VOS_ASSERT(0);
      return VOS_STATUS_E_NOMEM;
   }
   pWdaParams = (tWDA_ReqParams *)vos_mem_malloc(sizeof(tWDA_ReqParams)) ;
   if(NULL == pWdaParams)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                           "%s: VOS MEM Alloc Failure", __func__); 
      VOS_ASSERT(0);
      vos_mem_free(wdiResumeParams);
      return VOS_STATUS_E_NOMEM;
   }
   wdiResumeParams->wdiResumeParams.ucConfiguredMcstBcstFilterSetting =
                          pWlanResumeParam->configuredMcstBcstFilterSetting;
   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO, "%s: %d" ,__func__, pWlanResumeParam->configuredMcstBcstFilterSetting);
   wdiResumeParams->wdiReqStatusCB = NULL;
   pWdaParams->wdaMsgParam = pWlanResumeParam;
   pWdaParams->wdaWdiApiMsgParam = wdiResumeParams;
   pWdaParams->pWdaContext = pWDA;
   wdiStatus = WDI_HostResumeReq(wdiResumeParams, 
                      (WDI_HostResumeEventRspCb)WDA_ProcessWlanResumeCallback,
                      pWdaParams);
   if(IS_WDI_STATUS_FAILURE(wdiStatus))
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
              "Failure in Host Resume REQ WDI API, free all the memory " );
      VOS_ASSERT(0);
      vos_mem_free(pWdaParams->wdaWdiApiMsgParam) ;
      vos_mem_free(pWdaParams->wdaMsgParam);
      vos_mem_free(pWdaParams);
   }
   return CONVERT_WDI2VOS_STATUS(wdiStatus) ;
}

/*
                                            
   
 */ 
void WDA_SetBeaconFilterRespCallback(WDI_Status status, void* pUserData)
{
   tWDA_ReqParams *pWdaParams = (tWDA_ReqParams *)pUserData;
   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                                          "<------ %s " ,__func__);
   if(NULL == pWdaParams)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
              "%s: pWdaParams received NULL", __func__);
      VOS_ASSERT(0) ;
      return ;
   }
   
   vos_mem_free(pWdaParams->wdaMsgParam) ;
   vos_mem_free(pWdaParams->wdaWdiApiMsgParam);
   vos_mem_free(pWdaParams) ;
   /* 
                                                                           
                
    */

   return ;
}
/*
                                           
               
                                                                                   
 */
void WDA_SetBeaconFilterReqCallback(WDI_Status wdiStatus, void* pUserData)
{
   tWDA_ReqParams *pWdaParams = (tWDA_ReqParams *)pUserData;

   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
              "<------ %s, wdiStatus: %d", __func__, wdiStatus);

   if(NULL == pWdaParams)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                 "%s: pWdaParams received NULL", __func__);
      VOS_ASSERT(0);
      return;
   }

   if(IS_WDI_STATUS_FAILURE(wdiStatus))
   {
      vos_mem_free(pWdaParams->wdaWdiApiMsgParam);
      vos_mem_free(pWdaParams->wdaMsgParam);
      vos_mem_free(pWdaParams);
   }

   return;
}
/*
                                   
                                                                   
 */ 
VOS_STATUS WDA_SetBeaconFilterReq(tWDA_CbContext *pWDA, 
                                  tBeaconFilterMsg* pBeaconFilterInfo)
{
   WDI_Status status = WDI_STATUS_SUCCESS;
   tANI_U8            *dstPtr, *srcPtr;
   tANI_U8             filterLength;
   WDI_BeaconFilterReqParamsType *wdiBeaconFilterInfo = 
            (WDI_BeaconFilterReqParamsType *)vos_mem_malloc(
                                 sizeof(WDI_BeaconFilterReqParamsType) ) ;
   tWDA_ReqParams *pWdaParams ;
   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                                          "------> %s " ,__func__);
   if(NULL == wdiBeaconFilterInfo) 
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                           "%s: VOS MEM Alloc Failure", __func__); 
      VOS_ASSERT(0);
      return VOS_STATUS_E_NOMEM;
   }
   pWdaParams = (tWDA_ReqParams *)vos_mem_malloc(sizeof(tWDA_ReqParams)) ;
   if(NULL == pWdaParams)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                           "%s: VOS MEM Alloc Failure", __func__); 
      VOS_ASSERT(0);
      vos_mem_free(wdiBeaconFilterInfo);
      return VOS_STATUS_E_NOMEM;
   }
   wdiBeaconFilterInfo->wdiBeaconFilterInfo.usBeaconInterval = 
      pBeaconFilterInfo->beaconInterval;
   wdiBeaconFilterInfo->wdiBeaconFilterInfo.usCapabilityInfo = 
      pBeaconFilterInfo->capabilityInfo;
   wdiBeaconFilterInfo->wdiBeaconFilterInfo.usCapabilityMask = 
      pBeaconFilterInfo->capabilityMask;
   wdiBeaconFilterInfo->wdiBeaconFilterInfo.usIeNum = pBeaconFilterInfo->ieNum;

   //               
   wdiBeaconFilterInfo->wdiBeaconFilterInfo.bssIdx = pBeaconFilterInfo->bssIdx;

   //                                                           
   dstPtr = (tANI_U8 *)wdiBeaconFilterInfo + sizeof(WDI_BeaconFilterInfoType);
   srcPtr = (tANI_U8 *)pBeaconFilterInfo + sizeof(tBeaconFilterMsg);
   filterLength = wdiBeaconFilterInfo->wdiBeaconFilterInfo.usIeNum * sizeof(tBeaconFilterIe);
   if(WDI_BEACON_FILTER_LEN < filterLength)
   {
      filterLength = WDI_BEACON_FILTER_LEN;
   }
   vos_mem_copy(dstPtr, srcPtr, filterLength);
   wdiBeaconFilterInfo->wdiReqStatusCB = WDA_SetBeaconFilterReqCallback;
   wdiBeaconFilterInfo->pUserData = pWdaParams;

   /*                                            */
   /*                             */
   pWdaParams->wdaWdiApiMsgParam = wdiBeaconFilterInfo;
   pWdaParams->pWdaContext = pWDA;
   pWdaParams->wdaMsgParam = pBeaconFilterInfo;

   status = WDI_SetBeaconFilterReq(wdiBeaconFilterInfo, 
                                   (WDI_SetBeaconFilterCb)WDA_SetBeaconFilterRespCallback, pWdaParams);
   if(IS_WDI_STATUS_FAILURE(status))
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
              "Failure in Set Beacon Filter REQ WDI API, free all the memory " );
      vos_mem_free(pWdaParams->wdaMsgParam) ;
      vos_mem_free(pWdaParams->wdaWdiApiMsgParam);
      vos_mem_free(pWdaParams) ;
   }
   return CONVERT_WDI2VOS_STATUS(status) ;
}
/*
                                            
   
 */ 
void WDA_RemBeaconFilterRespCallback(WDI_Status status, void* pUserData)
{
   tWDA_ReqParams *pWdaParams = (tWDA_ReqParams *)pUserData ; 

   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                                          "<------ %s " ,__func__);

   if(NULL == pWdaParams)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
              "%s: pWdaParams received NULL", __func__);
      VOS_ASSERT(0) ;
      return ;
   }

   vos_mem_free(pWdaParams->wdaWdiApiMsgParam) ;
   vos_mem_free(pWdaParams->wdaMsgParam);
   vos_mem_free(pWdaParams);

   //                               
   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
              "WDA_RemBeaconFilterRespCallback invoked " );
   return ;
}
/*
                                           
               
                                                                                   
 */
void WDA_RemBeaconFilterReqCallback(WDI_Status wdiStatus, void* pUserData)
{
   tWDA_ReqParams *pWdaParams = (tWDA_ReqParams *)pUserData;

   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
              "<------ %s, wdiStatus: %d", __func__, wdiStatus);

   if(NULL == pWdaParams)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                 "%s: pWdaParams received NULL", __func__);
      VOS_ASSERT(0);
      return;
   }

   if(IS_WDI_STATUS_FAILURE(wdiStatus))
   {
      vos_mem_free(pWdaParams->wdaWdiApiMsgParam);
      vos_mem_free(pWdaParams->wdaMsgParam);
      vos_mem_free(pWdaParams);
   }

   return;
}
    //                                                         
    //                                                          
    //                                                            
    //                                        
/*
                                   
                                                                              
 */ 
VOS_STATUS WDA_RemBeaconFilterReq(tWDA_CbContext *pWDA, 
                                  tRemBeaconFilterMsg* pBeaconFilterInfo)
{
   VOS_STATUS status = VOS_STATUS_SUCCESS;
   WDI_Status wstatus;
   WDI_RemBeaconFilterReqParamsType *wdiBeaconFilterInfo = 
      (WDI_RemBeaconFilterReqParamsType *)vos_mem_malloc(
         sizeof(WDI_RemBeaconFilterReqParamsType) + pBeaconFilterInfo->ucIeCount) ;
   tWDA_ReqParams *pWdaParams ;

   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                                          "------> %s " ,__func__);
   if(NULL == wdiBeaconFilterInfo) 
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                           "%s: VOS MEM Alloc Failure", __func__); 
      VOS_ASSERT(0);
      return VOS_STATUS_E_NOMEM;
   }
   pWdaParams = (tWDA_ReqParams *)vos_mem_malloc(sizeof(tWDA_ReqParams)) ;
   if(NULL == pWdaParams)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                           "%s: VOS MEM Alloc Failure", __func__); 
      VOS_ASSERT(0);
      vos_mem_free(wdiBeaconFilterInfo);
      vos_mem_free(pBeaconFilterInfo);
      return VOS_STATUS_E_NOMEM;
   }

   wdiBeaconFilterInfo->wdiBeaconFilterInfo.ucIeCount =
      pBeaconFilterInfo->ucIeCount;
   //                                                   
   vos_mem_copy(wdiBeaconFilterInfo->wdiBeaconFilterInfo.ucRemIeId,
                pBeaconFilterInfo->ucRemIeId,
                wdiBeaconFilterInfo->wdiBeaconFilterInfo.ucIeCount);
   wdiBeaconFilterInfo->wdiReqStatusCB = WDA_RemBeaconFilterReqCallback;
   wdiBeaconFilterInfo->pUserData = pWdaParams;
   
   /*                                            */
   pWdaParams->wdaMsgParam = pBeaconFilterInfo;
   /*                             */
   pWdaParams->wdaWdiApiMsgParam = (void *)wdiBeaconFilterInfo;

   pWdaParams->pWdaContext = pWDA;

   wstatus = WDI_RemBeaconFilterReq(wdiBeaconFilterInfo, 
                                   (WDI_RemBeaconFilterCb)WDA_RemBeaconFilterRespCallback, pWdaParams);
   if(IS_WDI_STATUS_FAILURE(wstatus))
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
              "Failure in Remove Beacon Filter REQ WDI API, free all the memory " );
      status = CONVERT_WDI2VOS_STATUS(wstatus);
      vos_mem_free(pWdaParams->wdaWdiApiMsgParam) ;
      vos_mem_free(pWdaParams->wdaMsgParam);
      vos_mem_free(pWdaParams);
   }
   return status;
}
/*
                                              
   
 */ 
void WDA_SetRSSIThresholdsRespCallback(WDI_Status status, void* pUserData)
{
   tWDA_ReqParams *pWdaParams = (tWDA_ReqParams *)pUserData;
   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                                          "<------ %s " ,__func__);
   if(NULL == pWdaParams)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
              "%s: pWdaParams received NULL", __func__);
      VOS_ASSERT(0) ;
      return ;
   }
   
   vos_mem_free(pWdaParams->wdaWdiApiMsgParam);
   vos_mem_free(pWdaParams) ;

   return ;
}
/*
                                             
               
                                                                                     
 */
void WDA_SetRSSIThresholdsReqCallback(WDI_Status wdiStatus, void* pUserData)
{
   tWDA_ReqParams *pWdaParams = (tWDA_ReqParams *)pUserData;

   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
              "<------ %s, wdiStatus: %d", __func__, wdiStatus);

   if(NULL == pWdaParams)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                 "%s: pWdaParams received NULL", __func__);
      VOS_ASSERT(0);
      return;
   }

   if(IS_WDI_STATUS_FAILURE(wdiStatus))
   {
      vos_mem_free(pWdaParams->wdaWdiApiMsgParam);
      vos_mem_free(pWdaParams);
   }

   return;
}
/*
                                     
                                                        
 */ 
VOS_STATUS WDA_SetRSSIThresholdsReq(tpAniSirGlobal pMac, tSirRSSIThresholds *pBmpsThresholds)
{
   VOS_STATUS status = VOS_STATUS_SUCCESS;
   WDI_Status wstatus;
   tWDA_CbContext *pWDA = NULL ;
   v_PVOID_t pVosContext = NULL; 
   WDI_SetRSSIThresholdsReqParamsType *wdiRSSIThresholdsInfo = 
      (WDI_SetRSSIThresholdsReqParamsType *)vos_mem_malloc(
         sizeof(WDI_SetRSSIThresholdsReqParamsType)) ;
   tWDA_ReqParams *pWdaParams ;
   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                                          "------> %s " ,__func__);
   if(NULL == wdiRSSIThresholdsInfo) 
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                           "%s: VOS MEM Alloc Failure", __func__); 
      VOS_ASSERT(0);
      return VOS_STATUS_E_NOMEM;
   }
   pWdaParams = (tWDA_ReqParams *)vos_mem_malloc(sizeof(tWDA_ReqParams)) ;
   if(NULL == pWdaParams)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                           "%s: VOS MEM Alloc Failure", __func__); 
      VOS_ASSERT(0);
      vos_mem_free(wdiRSSIThresholdsInfo);
      return VOS_STATUS_E_NOMEM;
   }
   wdiRSSIThresholdsInfo->wdiRSSIThresholdsInfo.bReserved10 = pBmpsThresholds->bReserved10;
   wdiRSSIThresholdsInfo->wdiRSSIThresholdsInfo.ucRssiThreshold1 = pBmpsThresholds->ucRssiThreshold1;
   wdiRSSIThresholdsInfo->wdiRSSIThresholdsInfo.ucRssiThreshold2 = pBmpsThresholds->ucRssiThreshold2;
   wdiRSSIThresholdsInfo->wdiRSSIThresholdsInfo.ucRssiThreshold3 = pBmpsThresholds->ucRssiThreshold3;
   wdiRSSIThresholdsInfo->wdiRSSIThresholdsInfo.bRssiThres1NegNotify = pBmpsThresholds->bRssiThres1NegNotify;
   wdiRSSIThresholdsInfo->wdiRSSIThresholdsInfo.bRssiThres2NegNotify = pBmpsThresholds->bRssiThres2NegNotify;
   wdiRSSIThresholdsInfo->wdiRSSIThresholdsInfo.bRssiThres3NegNotify = pBmpsThresholds->bRssiThres3NegNotify;
   wdiRSSIThresholdsInfo->wdiRSSIThresholdsInfo.bRssiThres1PosNotify = pBmpsThresholds->bRssiThres1PosNotify;
   wdiRSSIThresholdsInfo->wdiRSSIThresholdsInfo.bRssiThres2PosNotify = pBmpsThresholds->bRssiThres2PosNotify;
   wdiRSSIThresholdsInfo->wdiRSSIThresholdsInfo.bRssiThres3PosNotify = pBmpsThresholds->bRssiThres3PosNotify;
   wdiRSSIThresholdsInfo->wdiReqStatusCB = WDA_SetRSSIThresholdsReqCallback;
   wdiRSSIThresholdsInfo->pUserData = pWdaParams;
   pVosContext = vos_get_global_context(VOS_MODULE_ID_PE, (void *)pMac);
   pWDA = vos_get_context( VOS_MODULE_ID_WDA, pVosContext );

   /*                                            */
   /*                             */
   pWdaParams->wdaWdiApiMsgParam = wdiRSSIThresholdsInfo;
   pWdaParams->pWdaContext = pWDA;
   pWdaParams->wdaMsgParam = pBmpsThresholds;
   wstatus = WDI_SetRSSIThresholdsReq(wdiRSSIThresholdsInfo, 
                                     (WDI_SetRSSIThresholdsCb)WDA_SetRSSIThresholdsRespCallback, pWdaParams);
   if(IS_WDI_STATUS_FAILURE(wstatus))
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
              "Failure in Set RSSI thresholds REQ WDI API, free all the memory " );
      status = CONVERT_WDI2VOS_STATUS(wstatus);
      vos_mem_free(pWdaParams->wdaWdiApiMsgParam);
      vos_mem_free(pWdaParams) ;
   }
   return status;

}/*                        */
/*
                                        
   
 */ 
void WDA_HostOffloadRespCallback(WDI_Status status, void* pUserData)
{
   tWDA_ReqParams *pWdaParams = (tWDA_ReqParams *)pUserData ; 

   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                                          "<------ %s " ,__func__);
   if(NULL == pWdaParams)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
              "%s: pWdaParams received NULL", __func__);
      VOS_ASSERT(0) ;
      return ;
   }
   
   vos_mem_free(pWdaParams->wdaWdiApiMsgParam);
   vos_mem_free(pWdaParams->wdaMsgParam);
   vos_mem_free(pWdaParams) ;

   //                               
   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
              "WDA_HostOffloadRespCallback invoked " );
   return ;
}
/*
                                       
               
                                                                               
 */
void WDA_HostOffloadReqCallback(WDI_Status wdiStatus, void* pUserData)
{
   tWDA_ReqParams *pWdaParams = (tWDA_ReqParams *)pUserData;

   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
              "<------ %s, wdiStatus: %d", __func__, wdiStatus);

   if(NULL == pWdaParams)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                 "%s: Invalid pWdaParams pointer", __func__);
      VOS_ASSERT(0);
      return;
   }

   if(IS_WDI_STATUS_FAILURE(wdiStatus))
   {
      vos_mem_free(pWdaParams->wdaWdiApiMsgParam);
      vos_mem_free(pWdaParams->wdaMsgParam);
      vos_mem_free(pWdaParams);
   }

   return;
}
/*
                                      
                                                                            
                                     
 */ 
VOS_STATUS WDA_ProcessHostOffloadReq(tWDA_CbContext *pWDA, 
                                     tSirHostOffloadReq *pHostOffloadParams)
{
   VOS_STATUS status = VOS_STATUS_SUCCESS;
   WDI_Status wstatus;
   WDI_HostOffloadReqParamsType *wdiHostOffloadInfo = 
      (WDI_HostOffloadReqParamsType *)vos_mem_malloc(
         sizeof(WDI_HostOffloadReqParamsType)) ;
   tWDA_ReqParams *pWdaParams ;

   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                                          "------> %s: offloadType=%x" ,__func__, pHostOffloadParams->offloadType);

   if(NULL == wdiHostOffloadInfo) 
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                           "%s: VOS MEM Alloc Failure", __func__); 
      VOS_ASSERT(0);
      return VOS_STATUS_E_NOMEM;
   }
   pWdaParams = (tWDA_ReqParams *)vos_mem_malloc(sizeof(tWDA_ReqParams)) ;
   if(NULL == pWdaParams)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                           "%s: VOS MEM Alloc Failure", __func__); 
      VOS_ASSERT(0);
      vos_mem_free(wdiHostOffloadInfo);
      vos_mem_free(pHostOffloadParams);
      return VOS_STATUS_E_NOMEM;
   }

   wdiHostOffloadInfo->wdiHostOffloadInfo.ucOffloadType = 
      pHostOffloadParams->offloadType;
   wdiHostOffloadInfo->wdiHostOffloadInfo.ucEnableOrDisable = 
      pHostOffloadParams->enableOrDisable;

   vos_mem_copy(wdiHostOffloadInfo->wdiHostOffloadInfo.bssId,
             pHostOffloadParams->bssId, sizeof(wpt_macAddr));

   switch (wdiHostOffloadInfo->wdiHostOffloadInfo.ucOffloadType)
   {
      case SIR_IPV4_ARP_REPLY_OFFLOAD:
         vos_mem_copy(wdiHostOffloadInfo->wdiHostOffloadInfo.params.aHostIpv4Addr,
                   pHostOffloadParams->params.hostIpv4Addr,
                   4);
         break;
      case SIR_IPV6_NEIGHBOR_DISCOVERY_OFFLOAD:
         vos_mem_copy(wdiHostOffloadInfo->wdiHostOffloadInfo.params.aHostIpv6Addr,
                   pHostOffloadParams->params.hostIpv6Addr,
                   16);
         break;
      case SIR_IPV6_NS_OFFLOAD:
         vos_mem_copy(wdiHostOffloadInfo->wdiHostOffloadInfo.params.aHostIpv6Addr,
                   pHostOffloadParams->params.hostIpv6Addr,
                   16);

#ifdef WLAN_NS_OFFLOAD
         if(pHostOffloadParams->nsOffloadInfo.srcIPv6AddrValid)
         {
            vos_mem_copy(wdiHostOffloadInfo->wdiNsOffloadParams.srcIPv6Addr,
                   pHostOffloadParams->nsOffloadInfo.srcIPv6Addr,
                   16);
            wdiHostOffloadInfo->wdiNsOffloadParams.srcIPv6AddrValid = 1;
         }
         else
         {
            wdiHostOffloadInfo->wdiNsOffloadParams.srcIPv6AddrValid = 0;
         }

         vos_mem_copy(wdiHostOffloadInfo->wdiNsOffloadParams.selfIPv6Addr,
                   pHostOffloadParams->nsOffloadInfo.selfIPv6Addr,
                   16);
         vos_mem_copy(wdiHostOffloadInfo->wdiNsOffloadParams.selfMacAddr,
                   pHostOffloadParams->nsOffloadInfo.selfMacAddr,
                   6);

         //                                                              
         if(pHostOffloadParams->nsOffloadInfo.targetIPv6AddrValid[0])
         {
            vos_mem_copy(wdiHostOffloadInfo->wdiNsOffloadParams.targetIPv6Addr1,
                      pHostOffloadParams->nsOffloadInfo.targetIPv6Addr[0],
                      16);
            wdiHostOffloadInfo->wdiNsOffloadParams.targetIPv6Addr1Valid = 1;
         }
         else
         {
            wdiHostOffloadInfo->wdiNsOffloadParams.targetIPv6Addr1Valid = 0;
         }

         if(pHostOffloadParams->nsOffloadInfo.targetIPv6AddrValid[1])
         {
            vos_mem_copy(wdiHostOffloadInfo->wdiNsOffloadParams.targetIPv6Addr2,
                      pHostOffloadParams->nsOffloadInfo.targetIPv6Addr[1],
                      16);
            wdiHostOffloadInfo->wdiNsOffloadParams.targetIPv6Addr2Valid = 1;
         }
         else
         {
            wdiHostOffloadInfo->wdiNsOffloadParams.targetIPv6Addr2Valid = 0;
         }
         wdiHostOffloadInfo->wdiNsOffloadParams.slotIdx =
                        pHostOffloadParams->nsOffloadInfo.slotIdx;
         break;
#endif //               
      default:
      {
         VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                                  "No Handling for Offload Type %x in WDA " 
                                  , wdiHostOffloadInfo->wdiHostOffloadInfo.ucOffloadType);
         //                   
      }
   }
   wdiHostOffloadInfo->wdiReqStatusCB = WDA_HostOffloadReqCallback;
   wdiHostOffloadInfo->pUserData = pWdaParams;

   /*                                            */
   pWdaParams->wdaMsgParam = pHostOffloadParams;
   /*                             */
   pWdaParams->wdaWdiApiMsgParam = wdiHostOffloadInfo;
   pWdaParams->pWdaContext = pWDA;


   wstatus = WDI_HostOffloadReq(wdiHostOffloadInfo, 
                               (WDI_HostOffloadCb)WDA_HostOffloadRespCallback, pWdaParams);

   if(IS_WDI_STATUS_FAILURE(wstatus))
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
              "Failure in host offload REQ WDI API, free all the memory %d",
               wstatus);
      status = CONVERT_WDI2VOS_STATUS(wstatus);
      vos_mem_free(pWdaParams->wdaWdiApiMsgParam);
      vos_mem_free(pWdaParams->wdaMsgParam);
      vos_mem_free(pWdaParams) ;
   }
   return status;

}/*                  */
/*
                                      
   
 */ 
void WDA_KeepAliveRespCallback(WDI_Status status, void* pUserData)
{
   tWDA_ReqParams *pWdaParams = (tWDA_ReqParams *)pUserData ; 

   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                                          "<------ %s " ,__func__);

   if(NULL == pWdaParams)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
              "%s: pWdaParams received NULL", __func__);
      VOS_ASSERT(0) ;
      return ;
   }

   vos_mem_free(pWdaParams->wdaWdiApiMsgParam);
   vos_mem_free(pWdaParams->wdaMsgParam);
   vos_mem_free(pWdaParams);

   //                               
   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
              "WDA_KeepAliveRespCallback invoked " );
   return ;
}
/*
                                     
               
                                                                             
 */
void WDA_KeepAliveReqCallback(WDI_Status wdiStatus, void* pUserData)
{
   tWDA_ReqParams *pWdaParams = (tWDA_ReqParams *)pUserData;

   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
              "<------ %s, wdiStatus: %d", __func__, wdiStatus);

   if(NULL == pWdaParams)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                 "%s: pWdaParams received NULL", __func__);
      VOS_ASSERT(0);
      return;
   }

   if(IS_WDI_STATUS_FAILURE(wdiStatus))
   {
      vos_mem_free(pWdaParams->wdaWdiApiMsgParam);
      vos_mem_free(pWdaParams->wdaMsgParam);
      vos_mem_free(pWdaParams);
   }

   return;
}
/*
                                    
                                                                          
                                                
 */ 
VOS_STATUS WDA_ProcessKeepAliveReq(tWDA_CbContext *pWDA, 
                                   tSirKeepAliveReq *pKeepAliveParams)
{
    VOS_STATUS status = VOS_STATUS_SUCCESS;
    WDI_Status wstatus;
    WDI_KeepAliveReqParamsType *wdiKeepAliveInfo = 
      (WDI_KeepAliveReqParamsType *)vos_mem_malloc(
         sizeof(WDI_KeepAliveReqParamsType)) ;
   tWDA_ReqParams *pWdaParams;

    VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                                          "------> %s " ,__func__);
    if(NULL == wdiKeepAliveInfo) 
    {
        VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                   "%s: VOS MEM Alloc Failure", __func__); 
        VOS_ASSERT(0);
        vos_mem_free(pKeepAliveParams);
        return VOS_STATUS_E_NOMEM;
    }

   pWdaParams = (tWDA_ReqParams *)vos_mem_malloc(sizeof(tWDA_ReqParams)) ;
   if(NULL == pWdaParams)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                           "%s: VOS MEM Alloc Failure", __func__); 
      VOS_ASSERT(0);
      vos_mem_free(wdiKeepAliveInfo);
      vos_mem_free(pKeepAliveParams);
      return VOS_STATUS_E_NOMEM;
   }

    wdiKeepAliveInfo->wdiKeepAliveInfo.ucPacketType = 
      pKeepAliveParams->packetType;
    wdiKeepAliveInfo->wdiKeepAliveInfo.ucTimePeriod = 
      pKeepAliveParams->timePeriod;

    vos_mem_copy(&wdiKeepAliveInfo->wdiKeepAliveInfo.bssId,
                 pKeepAliveParams->bssId,
                 sizeof(wpt_macAddr));

    if(pKeepAliveParams->packetType == SIR_KEEP_ALIVE_UNSOLICIT_ARP_RSP)
    {
       vos_mem_copy(&wdiKeepAliveInfo->wdiKeepAliveInfo.aHostIpv4Addr,
                    pKeepAliveParams->hostIpv4Addr,
                    SIR_IPV4_ADDR_LEN);
       vos_mem_copy(&wdiKeepAliveInfo->wdiKeepAliveInfo.aDestIpv4Addr,
                    pKeepAliveParams->destIpv4Addr,
                    SIR_IPV4_ADDR_LEN);   
       vos_mem_copy(&wdiKeepAliveInfo->wdiKeepAliveInfo.aDestMacAddr,
                    pKeepAliveParams->destMacAddr,
                    SIR_MAC_ADDR_LEN);
    }
    else if(pKeepAliveParams->packetType == SIR_KEEP_ALIVE_NULL_PKT)
    {
        vos_mem_set(&wdiKeepAliveInfo->wdiKeepAliveInfo.aHostIpv4Addr,
                    SIR_IPV4_ADDR_LEN,
                    0);
        vos_mem_set(&wdiKeepAliveInfo->wdiKeepAliveInfo.aDestIpv4Addr,
                    SIR_IPV4_ADDR_LEN,
                    0);   
        vos_mem_set(&wdiKeepAliveInfo->wdiKeepAliveInfo.aDestMacAddr,
                    SIR_MAC_ADDR_LEN,
                    0);
    }
    wdiKeepAliveInfo->wdiReqStatusCB = WDA_KeepAliveReqCallback;
    wdiKeepAliveInfo->pUserData = pWdaParams;

    /*                                            */
    pWdaParams->wdaMsgParam = pKeepAliveParams;
    /*                             */
    pWdaParams->wdaWdiApiMsgParam = (void *)wdiKeepAliveInfo;
    pWdaParams->pWdaContext = pWDA;

    VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,"WDA HIP : %d.%d.%d.%d",
              wdiKeepAliveInfo->wdiKeepAliveInfo.aHostIpv4Addr[0],
              wdiKeepAliveInfo->wdiKeepAliveInfo.aHostIpv4Addr[1],
              wdiKeepAliveInfo->wdiKeepAliveInfo.aHostIpv4Addr[2],
              wdiKeepAliveInfo->wdiKeepAliveInfo.aHostIpv4Addr[3]); 
    VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,"WDA DIP : %d.%d.%d.%d",
              wdiKeepAliveInfo->wdiKeepAliveInfo.aDestIpv4Addr[0],
              wdiKeepAliveInfo->wdiKeepAliveInfo.aDestIpv4Addr[1],
              wdiKeepAliveInfo->wdiKeepAliveInfo.aDestIpv4Addr[2],
              wdiKeepAliveInfo->wdiKeepAliveInfo.aDestIpv4Addr[3]); 
    VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
              "WDA DMAC : %d:%d:%d:%d:%d:%d",
              wdiKeepAliveInfo->wdiKeepAliveInfo.aDestMacAddr[0],
              wdiKeepAliveInfo->wdiKeepAliveInfo.aDestMacAddr[1],
              wdiKeepAliveInfo->wdiKeepAliveInfo.aDestMacAddr[2],
              wdiKeepAliveInfo->wdiKeepAliveInfo.aDestMacAddr[3],
              wdiKeepAliveInfo->wdiKeepAliveInfo.aDestMacAddr[4],
              wdiKeepAliveInfo->wdiKeepAliveInfo.aDestMacAddr[5]); 
    VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO, 
              "TimePeriod %d PacketType %d", 
              wdiKeepAliveInfo->wdiKeepAliveInfo.ucTimePeriod,
              wdiKeepAliveInfo->wdiKeepAliveInfo.ucPacketType); 
    wstatus = WDI_KeepAliveReq(wdiKeepAliveInfo, 
                             (WDI_KeepAliveCb)WDA_KeepAliveRespCallback, pWdaParams);

    if(IS_WDI_STATUS_FAILURE(wstatus))
    {
        VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                   "Failure in Keep Alive REQ WDI API, free all the memory " );
        status = CONVERT_WDI2VOS_STATUS(wstatus);
        vos_mem_free(pWdaParams->wdaWdiApiMsgParam) ;
        vos_mem_free(pWdaParams->wdaMsgParam);
        vos_mem_free(pWdaParams);
    }
    return status;

}/*                */
/*
                                          
   
 */ 
void WDA_WowlAddBcPtrnRespCallback(
                                 WDI_WowlAddBcPtrnRspParamsType *pWdiWowlAddBcstPtrRsp, 
                                 void* pUserData)
{
   tWDA_ReqParams *pWdaParams = (tWDA_ReqParams *)pUserData;
   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                                          "<------ %s " ,__func__);
   if(NULL == pWdaParams)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
              "%s: pWdaParams received NULL", __func__);
      VOS_ASSERT(0) ;
      return ;
   }
   vos_mem_free(pWdaParams->wdaWdiApiMsgParam);
   vos_mem_free(pWdaParams->wdaMsgParam);
   vos_mem_free(pWdaParams) ;
   return ;
}
/*
                                         
               
                                                                                 
 */
void WDA_WowlAddBcPtrnReqCallback(WDI_Status wdiStatus, void* pUserData)
{
   tWDA_ReqParams *pWdaParams = (tWDA_ReqParams *)pUserData;

   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
              "<------ %s, wdiStatus: %d", __func__, wdiStatus);

   if(NULL == pWdaParams)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                 "%s: pWdaParams received NULL", __func__);
      VOS_ASSERT(0);
      return;
   }

   if(IS_WDI_STATUS_FAILURE(wdiStatus))
   {
      vos_mem_free(pWdaParams->wdaWdiApiMsgParam);
      vos_mem_free(pWdaParams->wdaMsgParam);
      vos_mem_free(pWdaParams);
   }

   return;
}

/*
                                        
                                           
 */ 
VOS_STATUS WDA_ProcessWowlAddBcPtrnReq(tWDA_CbContext *pWDA, 
                                       tSirWowlAddBcastPtrn *pWowlAddBcPtrnParams)
{
   VOS_STATUS status = VOS_STATUS_SUCCESS;
   WDI_Status wstatus;
   WDI_WowlAddBcPtrnReqParamsType *wdiWowlAddBcPtrnInfo = 
      (WDI_WowlAddBcPtrnReqParamsType *)vos_mem_malloc(
         sizeof(WDI_WowlAddBcPtrnReqParamsType)) ;
   tWDA_ReqParams *pWdaParams ;
   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                                          "------> %s " ,__func__);
   if(NULL == wdiWowlAddBcPtrnInfo) 
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                           "%s: VOS MEM Alloc Failure", __func__); 
      VOS_ASSERT(0);
      return VOS_STATUS_E_NOMEM;
   }
   pWdaParams = (tWDA_ReqParams *)vos_mem_malloc(sizeof(tWDA_ReqParams)) ;
   if(NULL == pWdaParams)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                           "%s: VOS MEM Alloc Failure", __func__); 
      VOS_ASSERT(0);
      vos_mem_free(wdiWowlAddBcPtrnInfo);
      return VOS_STATUS_E_NOMEM;
   }
   wdiWowlAddBcPtrnInfo->wdiWowlAddBcPtrnInfo.ucPatternId = 
      pWowlAddBcPtrnParams->ucPatternId;
   wdiWowlAddBcPtrnInfo->wdiWowlAddBcPtrnInfo.ucPatternByteOffset = 
      pWowlAddBcPtrnParams->ucPatternByteOffset;
   wdiWowlAddBcPtrnInfo->wdiWowlAddBcPtrnInfo.ucPatternMaskSize = 
      pWowlAddBcPtrnParams->ucPatternMaskSize;
   wdiWowlAddBcPtrnInfo->wdiWowlAddBcPtrnInfo.ucPatternSize = 
      pWowlAddBcPtrnParams->ucPatternSize;
   if (wdiWowlAddBcPtrnInfo->wdiWowlAddBcPtrnInfo.ucPatternSize <= WDI_WOWL_BCAST_PATTERN_MAX_SIZE)
   {
       vos_mem_copy(wdiWowlAddBcPtrnInfo->wdiWowlAddBcPtrnInfo.ucPattern,
                    pWowlAddBcPtrnParams->ucPattern,
                    wdiWowlAddBcPtrnInfo->wdiWowlAddBcPtrnInfo.ucPatternSize);
       vos_mem_copy(wdiWowlAddBcPtrnInfo->wdiWowlAddBcPtrnInfo.ucPatternMask,
                    pWowlAddBcPtrnParams->ucPatternMask,
                    wdiWowlAddBcPtrnInfo->wdiWowlAddBcPtrnInfo.ucPatternMaskSize);
   }
   else
   {
       vos_mem_copy(wdiWowlAddBcPtrnInfo->wdiWowlAddBcPtrnInfo.ucPattern,
                    pWowlAddBcPtrnParams->ucPattern,
                    WDI_WOWL_BCAST_PATTERN_MAX_SIZE);
       vos_mem_copy(wdiWowlAddBcPtrnInfo->wdiWowlAddBcPtrnInfo.ucPatternMask,
                    pWowlAddBcPtrnParams->ucPatternMask,
                    WDI_WOWL_BCAST_PATTERN_MAX_SIZE);

       vos_mem_copy(wdiWowlAddBcPtrnInfo->wdiWowlAddBcPtrnInfo.ucPatternExt,
                    pWowlAddBcPtrnParams->ucPatternExt,
                    wdiWowlAddBcPtrnInfo->wdiWowlAddBcPtrnInfo.ucPatternSize - WDI_WOWL_BCAST_PATTERN_MAX_SIZE);
       vos_mem_copy(wdiWowlAddBcPtrnInfo->wdiWowlAddBcPtrnInfo.ucPatternMaskExt,
                    pWowlAddBcPtrnParams->ucPatternMaskExt,
                    wdiWowlAddBcPtrnInfo->wdiWowlAddBcPtrnInfo.ucPatternMaskSize - WDI_WOWL_BCAST_PATTERN_MAX_SIZE);
   }

   vos_mem_copy(wdiWowlAddBcPtrnInfo->wdiWowlAddBcPtrnInfo.bssId,
                         pWowlAddBcPtrnParams->bssId, sizeof(wpt_macAddr));

   wdiWowlAddBcPtrnInfo->wdiReqStatusCB = WDA_WowlAddBcPtrnReqCallback;
   wdiWowlAddBcPtrnInfo->pUserData = pWdaParams;
   /*                                            */
   /*                             */
   pWdaParams->wdaWdiApiMsgParam = wdiWowlAddBcPtrnInfo;
   pWdaParams->pWdaContext = pWDA;
   pWdaParams->wdaMsgParam = pWowlAddBcPtrnParams;
   wstatus = WDI_WowlAddBcPtrnReq(wdiWowlAddBcPtrnInfo, 
                                 (WDI_WowlAddBcPtrnCb)WDA_WowlAddBcPtrnRespCallback, pWdaParams);
   if(IS_WDI_STATUS_FAILURE(wstatus))
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
              "Failure in Wowl add Bcast ptrn REQ WDI API, free all the memory " );
      status = CONVERT_WDI2VOS_STATUS(wstatus);
      vos_mem_free(pWdaParams->wdaMsgParam) ;
      vos_mem_free(pWdaParams->wdaWdiApiMsgParam);
      vos_mem_free(pWdaParams) ;
   }
   return status;

}/*                           */
/*
                                          
   
 */ 
void WDA_WowlDelBcPtrnRespCallback(
                        WDI_WowlDelBcPtrnRspParamsType *pWdiWowlDelBcstPtrRsp, 
                        void* pUserData)
{
   tWDA_ReqParams *pWdaParams = (tWDA_ReqParams *)pUserData;
   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                                          "<------ %s " ,__func__);
   if(NULL == pWdaParams)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
              "%s: pWdaParams received NULL", __func__);
      VOS_ASSERT(0) ;
      return ;
   }
   vos_mem_free(pWdaParams->wdaWdiApiMsgParam);
   vos_mem_free(pWdaParams->wdaMsgParam);
   vos_mem_free(pWdaParams) ;
   return ;
}
/*
                                         
               
                                                                                 
 */
void WDA_WowlDelBcPtrnReqCallback(WDI_Status wdiStatus, void* pUserData)
{
   tWDA_ReqParams *pWdaParams = (tWDA_ReqParams *)pUserData;

   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
              "<------ %s, wdiStatus: %d", __func__, wdiStatus);

   if(NULL == pWdaParams)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                 "%s: pWdaParams received NULL", __func__);
      VOS_ASSERT(0);
      return;
   }

   if(IS_WDI_STATUS_FAILURE(wdiStatus))
   {
      vos_mem_free(pWdaParams->wdaWdiApiMsgParam);
      vos_mem_free(pWdaParams->wdaMsgParam);
      vos_mem_free(pWdaParams);
   }

   return;
}
/*
                                        
                                              
 */ 
VOS_STATUS WDA_ProcessWowlDelBcPtrnReq(tWDA_CbContext *pWDA, 
                                       tSirWowlDelBcastPtrn *pWowlDelBcPtrnParams)
{
   VOS_STATUS status = VOS_STATUS_SUCCESS;
   WDI_Status wstatus;
   WDI_WowlDelBcPtrnReqParamsType *wdiWowlDelBcPtrnInfo = 
      (WDI_WowlDelBcPtrnReqParamsType *)vos_mem_malloc(
         sizeof(WDI_WowlDelBcPtrnReqParamsType)) ;
   tWDA_ReqParams *pWdaParams ;
   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                                          "------> %s " ,__func__);
   if(NULL == wdiWowlDelBcPtrnInfo) 
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                           "%s: VOS MEM Alloc Failure", __func__); 
      VOS_ASSERT(0);
      return VOS_STATUS_E_NOMEM;
   }
   pWdaParams = (tWDA_ReqParams *)vos_mem_malloc(sizeof(tWDA_ReqParams)) ;
   if(NULL == pWdaParams)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                           "%s: VOS MEM Alloc Failure", __func__); 
      VOS_ASSERT(0);
      vos_mem_free(wdiWowlDelBcPtrnInfo);
      return VOS_STATUS_E_NOMEM;
   }
   wdiWowlDelBcPtrnInfo->wdiWowlDelBcPtrnInfo.ucPatternId = 
      pWowlDelBcPtrnParams->ucPatternId;
   
   vos_mem_copy(wdiWowlDelBcPtrnInfo->wdiWowlDelBcPtrnInfo.bssId,
                         pWowlDelBcPtrnParams->bssId, sizeof(wpt_macAddr));

   wdiWowlDelBcPtrnInfo->wdiReqStatusCB = WDA_WowlDelBcPtrnReqCallback;
   wdiWowlDelBcPtrnInfo->pUserData = pWdaParams;
   /*                                            */
   /*                             */
   pWdaParams->wdaWdiApiMsgParam = wdiWowlDelBcPtrnInfo;
   pWdaParams->pWdaContext = pWDA;
   pWdaParams->wdaMsgParam = pWowlDelBcPtrnParams;
   wstatus = WDI_WowlDelBcPtrnReq(wdiWowlDelBcPtrnInfo, 
                                 (WDI_WowlDelBcPtrnCb)WDA_WowlDelBcPtrnRespCallback, pWdaParams);
   if(IS_WDI_STATUS_FAILURE(wstatus))
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
              "Failure in Wowl delete Bcast ptrn REQ WDI API, free all the memory " );
      status = CONVERT_WDI2VOS_STATUS(wstatus);
      vos_mem_free(pWdaParams->wdaMsgParam) ;
      vos_mem_free(pWdaParams->wdaWdiApiMsgParam);
      vos_mem_free(pWdaParams) ;
   }
   return status;

}/*                           */
/*
                                      
   
 */ 
void WDA_WowlEnterRespCallback(WDI_WowlEnterRspParamsType *pwdiWowlEnterRspParam, void* pUserData)
{
   tWDA_ReqParams *pWdaParams = (tWDA_ReqParams *)pUserData;
   tWDA_CbContext *pWDA;
   tSirHalWowlEnterParams *pWowlEnterParams;
   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                                          "<------ %s " ,__func__);
   if(NULL == pWdaParams)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
              "%s: pWdaParams received NULL", __func__);
      VOS_ASSERT(0) ;
      return ;
   }
   pWDA = (tWDA_CbContext *)pWdaParams->pWdaContext ;
   pWowlEnterParams =  (tSirHalWowlEnterParams *)pWdaParams->wdaMsgParam ;

   pWowlEnterParams->bssIdx = pwdiWowlEnterRspParam->bssIdx;

   vos_mem_free(pWdaParams->wdaWdiApiMsgParam);
   vos_mem_free(pWdaParams) ;

   pWowlEnterParams->status = 
               (pwdiWowlEnterRspParam->status);
   WDA_SendMsg(pWDA, WDA_WOWL_ENTER_RSP, (void *)pWowlEnterParams , 0) ;
   return ;
}
/*
                                     
                                                  
                                                                              
 */
void WDA_WowlEnterReqCallback(WDI_Status wdiStatus, void* pUserData)
{
   tWDA_ReqParams *pWdaParams = (tWDA_ReqParams *)pUserData;
   tWDA_CbContext *pWDA;
   tSirHalWowlEnterParams *pWowlEnterParams;

   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
              "<------ %s, wdiStatus: %d", __func__, wdiStatus);

   if(NULL == pWdaParams)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                 "%s: pWdaParams received NULL", __func__);
      VOS_ASSERT(0);
      return;
   }

   pWDA = (tWDA_CbContext *)pWdaParams->pWdaContext;
   pWowlEnterParams =  (tSirHalWowlEnterParams *)pWdaParams->wdaMsgParam;
   pWowlEnterParams->status = wdiStatus;

   if(IS_WDI_STATUS_FAILURE(wdiStatus))
   {
      vos_mem_free(pWdaParams->wdaWdiApiMsgParam);
      vos_mem_free(pWdaParams);
      WDA_SendMsg(pWDA, WDA_WOWL_ENTER_RSP, (void *)pWowlEnterParams , 0);
   }

   return;
}
/*
                                    
                                
 */ 
VOS_STATUS WDA_ProcessWowlEnterReq(tWDA_CbContext *pWDA, 
                                   tSirHalWowlEnterParams *pWowlEnterParams)
{
   VOS_STATUS status = VOS_STATUS_SUCCESS;
   WDI_Status wstatus;
   WDI_WowlEnterReqParamsType *wdiWowlEnterInfo = 
      (WDI_WowlEnterReqParamsType *)vos_mem_malloc(
         sizeof(WDI_WowlEnterReqParamsType)) ;
   tWDA_ReqParams *pWdaParams ;
   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                                          "------> %s " ,__func__);
   if(NULL == wdiWowlEnterInfo) 
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                           "%s: VOS MEM Alloc Failure", __func__); 
      VOS_ASSERT(0);
      return VOS_STATUS_E_NOMEM;
   }
   pWdaParams = (tWDA_ReqParams *)vos_mem_malloc(sizeof(tWDA_ReqParams)) ;
   if(NULL == pWdaParams)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                           "%s: VOS MEM Alloc Failure", __func__); 
      VOS_ASSERT(0);
      vos_mem_free(wdiWowlEnterInfo);
      return VOS_STATUS_E_NOMEM;
   }

   vos_mem_zero(pWdaParams, sizeof(tWDA_ReqParams));

   vos_mem_copy(wdiWowlEnterInfo->wdiWowlEnterInfo.magicPtrn,
                pWowlEnterParams->magicPtrn,
                sizeof(tSirMacAddr));
   wdiWowlEnterInfo->wdiWowlEnterInfo.ucMagicPktEnable = 
      pWowlEnterParams->ucMagicPktEnable;
   wdiWowlEnterInfo->wdiWowlEnterInfo.ucPatternFilteringEnable = 
      pWowlEnterParams->ucPatternFilteringEnable;
   wdiWowlEnterInfo->wdiWowlEnterInfo.ucUcastPatternFilteringEnable = 
      pWowlEnterParams->ucUcastPatternFilteringEnable;
   wdiWowlEnterInfo->wdiWowlEnterInfo.ucWowChnlSwitchRcv = 
      pWowlEnterParams->ucWowChnlSwitchRcv;
   wdiWowlEnterInfo->wdiWowlEnterInfo.ucWowDeauthRcv = 
      pWowlEnterParams->ucWowDeauthRcv;
   wdiWowlEnterInfo->wdiWowlEnterInfo.ucWowDisassocRcv = 
      pWowlEnterParams->ucWowDisassocRcv;
   wdiWowlEnterInfo->wdiWowlEnterInfo.ucWowMaxMissedBeacons = 
      pWowlEnterParams->ucWowMaxMissedBeacons;
   wdiWowlEnterInfo->wdiWowlEnterInfo.ucWowMaxSleepUsec = 
      pWowlEnterParams->ucWowMaxSleepUsec;
#ifdef WLAN_WAKEUP_EVENTS
   wdiWowlEnterInfo->wdiWowlEnterInfo.ucWoWEAPIDRequestEnable = 
      pWowlEnterParams->ucWoWEAPIDRequestEnable;

   wdiWowlEnterInfo->wdiWowlEnterInfo.ucWoWEAPOL4WayEnable =
      pWowlEnterParams->ucWoWEAPOL4WayEnable;

   wdiWowlEnterInfo->wdiWowlEnterInfo.ucWowNetScanOffloadMatch = 
      pWowlEnterParams->ucWowNetScanOffloadMatch;

   wdiWowlEnterInfo->wdiWowlEnterInfo.ucWowGTKRekeyError = 
      pWowlEnterParams->ucWowGTKRekeyError;

   wdiWowlEnterInfo->wdiWowlEnterInfo.ucWoWBSSConnLoss = 
      pWowlEnterParams->ucWoWBSSConnLoss;
#endif //                   

   wdiWowlEnterInfo->wdiWowlEnterInfo.bssIdx = 
      pWowlEnterParams->bssIdx;

   wdiWowlEnterInfo->wdiReqStatusCB = WDA_WowlEnterReqCallback;
   wdiWowlEnterInfo->pUserData = pWdaParams;
   /*                                            */
   /*                             */
   pWdaParams->wdaWdiApiMsgParam = wdiWowlEnterInfo;
   pWdaParams->pWdaContext = pWDA;
   pWdaParams->wdaMsgParam = pWowlEnterParams;
   wstatus = WDI_WowlEnterReq(wdiWowlEnterInfo, 
                             (WDI_WowlEnterReqCb)WDA_WowlEnterRespCallback, pWdaParams);
   if(IS_WDI_STATUS_FAILURE(wstatus))
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
              "Failure in Wowl enter REQ WDI API, free all the memory " );
      status = CONVERT_WDI2VOS_STATUS(wstatus);
      vos_mem_free(pWdaParams->wdaMsgParam) ;
      vos_mem_free(pWdaParams->wdaWdiApiMsgParam);
      vos_mem_free(pWdaParams) ;
   }
   return status;

}/*                       */
/*
                                     
   
 */ 
void WDA_WowlExitRespCallback( WDI_WowlExitRspParamsType *pwdiWowlExitRsp, void* pUserData)
{
   tWDA_ReqParams *pWdaParams = (tWDA_ReqParams *)pUserData;
   tWDA_CbContext *pWDA;
   tSirHalWowlExitParams *pWowlExitParams;
   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                                          "<------ %s " ,__func__);
   if(NULL == pWdaParams)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
              "%s: pWdaParams received NULL", __func__);
      VOS_ASSERT(0) ;
      return ;
   }
   pWDA = (tWDA_CbContext *)pWdaParams->pWdaContext ;
   pWowlExitParams =  (tSirHalWowlExitParams *)pWdaParams->wdaMsgParam ;

   pWowlExitParams->bssIdx = pwdiWowlExitRsp->bssIdx;
   pWowlExitParams->status = (pwdiWowlExitRsp->status);

   vos_mem_free(pWdaParams->wdaWdiApiMsgParam);
   vos_mem_free(pWdaParams) ;

   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                                          "<------ %s " ,__func__);
   WDA_SendMsg(pWDA, WDA_WOWL_EXIT_RSP, (void *)pWowlExitParams, 0) ;
   return ;
}
/*
                                    
                                                 
                                                                             
 */
void WDA_WowlExitReqCallback(WDI_Status wdiStatus, void* pUserData)
{
   tWDA_ReqParams *pWdaParams = (tWDA_ReqParams *)pUserData;
   tWDA_CbContext *pWDA;
   tSirHalWowlExitParams *pWowlExitParams;

   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
              "<------ %s, wdiStatus: %d", __func__, wdiStatus);

   if(NULL == pWdaParams)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                 "%s: pWdaParams received NULL", __func__);
      VOS_ASSERT(0);
      return;
   }

   pWDA = (tWDA_CbContext *)pWdaParams->pWdaContext;
   pWowlExitParams =  (tSirHalWowlExitParams *)pWdaParams->wdaMsgParam;
   pWowlExitParams->status = wdiStatus;

   if(IS_WDI_STATUS_FAILURE(wdiStatus))
   {
      vos_mem_free(pWdaParams->wdaWdiApiMsgParam);
      vos_mem_free(pWdaParams);
      WDA_SendMsg(pWDA, WDA_WOWL_EXIT_RSP, (void *)pWowlExitParams, 0);
   }

   return;
}
/*
                                   
                                           
 */ 
VOS_STATUS WDA_ProcessWowlExitReq(tWDA_CbContext *pWDA, 
                                            tSirHalWowlExitParams  *pWowlExitParams)
{
   VOS_STATUS status = VOS_STATUS_SUCCESS;
   WDI_Status wstatus;
   WDI_WowlExitReqParamsType *wdiWowlExitInfo = 
      (WDI_WowlExitReqParamsType *)vos_mem_malloc(
         sizeof(WDI_WowlExitReqParamsType)) ;
   tWDA_ReqParams *pWdaParams ;
   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                                          "------> %s " ,__func__);
   if(NULL == wdiWowlExitInfo) 
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                           "%s: VOS MEM Alloc Failure", __func__); 
      VOS_ASSERT(0);
      return VOS_STATUS_E_NOMEM;
   }
   pWdaParams = (tWDA_ReqParams *)vos_mem_malloc(sizeof(tWDA_ReqParams)) ;
   if(NULL == pWdaParams)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                           "%s: VOS MEM Alloc Failure", __func__); 
      VOS_ASSERT(0);
      vos_mem_free(wdiWowlExitInfo);
      return VOS_STATUS_E_NOMEM;
   }

   wdiWowlExitInfo->wdiWowlExitInfo.bssIdx = 
      pWowlExitParams->bssIdx;

   wdiWowlExitInfo->wdiReqStatusCB = WDA_WowlExitReqCallback;
   wdiWowlExitInfo->pUserData = pWdaParams;

   /*                                            */
   /*                             */
   pWdaParams->wdaWdiApiMsgParam = wdiWowlExitInfo;
   pWdaParams->pWdaContext = pWDA;
   pWdaParams->wdaMsgParam = pWowlExitParams;

   wstatus = WDI_WowlExitReq(wdiWowlExitInfo,
                            (WDI_WowlExitReqCb)WDA_WowlExitRespCallback, pWdaParams);

   if(IS_WDI_STATUS_FAILURE(wstatus))
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
              "Failure in Wowl exit REQ WDI API, free all the memory " );
      status = CONVERT_WDI2VOS_STATUS(wstatus);
      vos_mem_free(pWdaParams->wdaWdiApiMsgParam) ;
      vos_mem_free(pWdaParams->wdaMsgParam);
      vos_mem_free(pWdaParams) ;
   }
   return status;
}/*                      */
/*
                                              
                                                                     
                                   
 */ 
v_BOOL_t WDA_IsHwFrameTxTranslationCapable(v_PVOID_t pVosGCtx, 
                                                      tANI_U8 staIdx)
{
   return WDI_IsHwFrameTxTranslationCapable(staIdx);
}

/*
                          
                                                                    
         
 */
v_BOOL_t WDA_IsSelfSTA(v_PVOID_t pVosContext, tANI_U8 ucSTAIdx)
{

  tWDA_CbContext *pWDA = (tWDA_CbContext *)VOS_GET_WDA_CTXT(pVosContext);

  if (NULL != pWDA)
     return WDI_IsSelfSTA(pWDA->pWdiContext,ucSTAIdx);
  else
     return VOS_TRUE;
}
/*
                                      
                                  
 */ 
void WDA_NvDownloadReqCallback(WDI_NvDownloadRspInfoType *pNvDownloadRspParams, 
                                                            void* pUserData)
{

   tWDA_ReqParams *pWdaParams= ( tWDA_ReqParams *)pUserData;
   tWDA_CbContext *pWDA;

   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                                          "<------ %s " ,__func__);

   if(NULL == pWdaParams)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
              "%s: pWdaParams received NULL", __func__);
      VOS_ASSERT(0) ;
      return ;
   }

   pWDA = pWdaParams->pWdaContext;

   /*         */
   vos_mem_free(pWdaParams->wdaWdiApiMsgParam) ;
   vos_mem_free(pWdaParams);

   vos_WDAComplete_cback(pWDA->pVosContext);
   return ;
}
/*
                                     
                                                                                            
 */ 
VOS_STATUS WDA_NVDownload_Start(v_PVOID_t pVosContext)
{
   /*                               */
   tWDA_CbContext *pWDA = (tWDA_CbContext *)VOS_GET_WDA_CTXT(pVosContext);
   v_VOID_t *pNvBuffer=NULL;
   v_SIZE_t bufferSize = 0;
   WDI_Status status = WDI_STATUS_E_FAILURE;
   WDI_NvDownloadReqParamsType * wdiNvDownloadReqParam =NULL;
   tWDA_ReqParams *pWdaParams ;

   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                                          "------> %s " ,__func__);
   if(NULL == pWDA) 
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                           "%s:pWDA is NULL", __func__); 
      VOS_ASSERT(0);
      return VOS_STATUS_E_FAILURE;
   }
   
   /*                                                     */
   vos_nv_getNVEncodedBuffer(&pNvBuffer,&bufferSize);

   wdiNvDownloadReqParam = (WDI_NvDownloadReqParamsType *)vos_mem_malloc(
                                          sizeof(WDI_NvDownloadReqParamsType)) ;
   if(NULL == wdiNvDownloadReqParam) 
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                           "%s: VOS MEM Alloc Failure", __func__); 
      VOS_ASSERT(0);
      return VOS_STATUS_E_NOMEM;
   }
   /*                                     */
   wdiNvDownloadReqParam->wdiBlobInfo.pBlobAddress = pNvBuffer;
   wdiNvDownloadReqParam->wdiBlobInfo.uBlobSize = bufferSize;

   pWdaParams = (tWDA_ReqParams *)vos_mem_malloc(sizeof(tWDA_ReqParams)) ;
   if(NULL == pWdaParams)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                           "%s: VOS MEM Alloc Failure", __func__); 
      VOS_ASSERT(0);
      vos_mem_free(wdiNvDownloadReqParam);
      return VOS_STATUS_E_NOMEM;
   }

   /*                             */
   pWdaParams->wdaWdiApiMsgParam = (void *)wdiNvDownloadReqParam ;
   pWdaParams->wdaMsgParam = NULL;
   pWdaParams->pWdaContext = pWDA;
   

   wdiNvDownloadReqParam->wdiReqStatusCB = NULL ;

   status = WDI_NvDownloadReq(wdiNvDownloadReqParam, 
                    (WDI_NvDownloadRspCb)WDA_NvDownloadReqCallback,(void *)pWdaParams);

   if(IS_WDI_STATUS_FAILURE(status))
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
            "Failure in NV Download REQ Params WDI API, free all the memory " );
      vos_mem_free(pWdaParams->wdaWdiApiMsgParam) ;
      vos_mem_free(pWdaParams);
   }
   return CONVERT_WDI2VOS_STATUS(status) ;
}
/*
                                   
                               
 */ 
void WDA_FlushAcReqCallback(WDI_Status status, void* pUserData)
{
   vos_msg_t wdaMsg = {0} ;
   tWDA_ReqParams *pWdaParams = (tWDA_ReqParams *)pUserData; 
   tFlushACReq *pFlushACReqParams;
   tFlushACRsp *pFlushACRspParams;
   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                                          "<------ %s " ,__func__);
   if(NULL == pWdaParams)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
              "%s: pWdaParams received NULL", __func__);
      VOS_ASSERT(0) ;
      return ;
   }
   
   pFlushACReqParams = (tFlushACReq *)pWdaParams->wdaMsgParam;
   pFlushACRspParams = vos_mem_malloc(sizeof(tFlushACRsp));
   if(NULL == pFlushACRspParams) 
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                           "%s: VOS MEM Alloc Failure", __func__); 
      VOS_ASSERT(0);
      vos_mem_free(pWdaParams);
      return ;
   }
   vos_mem_zero(pFlushACRspParams,sizeof(tFlushACRsp));   
   pFlushACRspParams->mesgLen = sizeof(tFlushACRsp);
   pFlushACRspParams->mesgType = WDA_TL_FLUSH_AC_RSP;
   pFlushACRspParams->ucSTAId = pFlushACReqParams->ucSTAId;
   pFlushACRspParams->ucTid = pFlushACReqParams->ucTid;
   pFlushACRspParams->status = (status) ;
   vos_mem_free(pWdaParams->wdaMsgParam) ;
   vos_mem_free(pWdaParams->wdaWdiApiMsgParam);
   vos_mem_free(pWdaParams);
   wdaMsg.type = WDA_TL_FLUSH_AC_RSP ; 
   wdaMsg.bodyptr = (void *)pFlushACRspParams;
   //                   
   vos_mq_post_message(VOS_MQ_ID_TL, (vos_msg_t *) &wdaMsg);

   return ;
}
/*
                                  
                                                 
 */ 
VOS_STATUS WDA_ProcessFlushAcReq(tWDA_CbContext *pWDA, 
                                 tFlushACReq *pFlushAcReqParams)
{
   WDI_Status status = WDI_STATUS_SUCCESS ;
   WDI_FlushAcReqParamsType *wdiFlushAcReqParam = 
               (WDI_FlushAcReqParamsType *)vos_mem_malloc(
                                       sizeof(WDI_FlushAcReqParamsType)) ;
   tWDA_ReqParams *pWdaParams ;
   if(NULL == wdiFlushAcReqParam) 
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                           "%s: VOS MEM Alloc Failure", __func__); 
      VOS_ASSERT(0);
      return VOS_STATUS_E_NOMEM;
   }
   pWdaParams = (tWDA_ReqParams *)vos_mem_malloc(sizeof(tWDA_ReqParams)) ;
   if(NULL == pWdaParams)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                           "%s: VOS MEM Alloc Failure", __func__); 
      VOS_ASSERT(0);
      vos_mem_free(wdiFlushAcReqParam);
      return VOS_STATUS_E_NOMEM;
   }
   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                                          "------> %s " ,__func__);
   wdiFlushAcReqParam->wdiFlushAcInfo.ucSTAId = pFlushAcReqParams->ucSTAId;
   wdiFlushAcReqParam->wdiFlushAcInfo.ucTid = pFlushAcReqParams->ucTid;
   wdiFlushAcReqParam->wdiFlushAcInfo.usMesgLen = pFlushAcReqParams->mesgLen;
   wdiFlushAcReqParam->wdiFlushAcInfo.usMesgType = pFlushAcReqParams->mesgType;
   /*                                                           */
   /*                             */
   pWdaParams->pWdaContext = pWDA;
   pWdaParams->wdaMsgParam = pFlushAcReqParams;
   pWdaParams->wdaWdiApiMsgParam = wdiFlushAcReqParam;
   status = WDI_FlushAcReq(wdiFlushAcReqParam, 
                           (WDI_FlushAcRspCb)WDA_FlushAcReqCallback, pWdaParams);
   if(IS_WDI_STATUS_FAILURE(status))
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
              "Failure in Flush AC REQ Params WDI API, free all the memory " );
      vos_mem_free(pWdaParams->wdaMsgParam) ;
      vos_mem_free(pWdaParams->wdaWdiApiMsgParam);
      vos_mem_free(pWdaParams) ;
      //                                
   }
   return CONVERT_WDI2VOS_STATUS(status) ;
}
/*
                                       
   
 */ 
void WDA_BtAmpEventRespCallback(WDI_Status status, void* pUserData)
{
   tWDA_ReqParams *pWdaParams = (tWDA_ReqParams *)pUserData; 
   tWDA_CbContext *pWDA; 
   WDI_BtAmpEventParamsType *wdiBtAmpEventParam;

   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                                          "<------ %s " ,__func__);
   if(NULL == pWdaParams)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
              "%s: pWdaParams received NULL", __func__);
      VOS_ASSERT(0) ;
      return ;
   }
   pWDA = (tWDA_CbContext *)pWdaParams->pWdaContext;
   wdiBtAmpEventParam = (WDI_BtAmpEventParamsType *)pWdaParams->wdaWdiApiMsgParam;
   if(BTAMP_EVENT_CONNECTION_TERMINATED == 
      wdiBtAmpEventParam->wdiBtAmpEventInfo.ucBtAmpEventType)
   {
      pWDA->wdaAmpSessionOn = VOS_FALSE;
   }
   vos_mem_free(pWdaParams->wdaMsgParam) ;
   vos_mem_free(pWdaParams->wdaWdiApiMsgParam);
   vos_mem_free(pWdaParams) ;
   /* 
                                                                              
                
    */
   return ;
}
/*
                                      
               
                                                                              
 */
void WDA_BtAmpEventReqCallback(WDI_Status wdiStatus, void* pUserData)
{
   tWDA_ReqParams *pWdaParams = (tWDA_ReqParams *)pUserData;
   tWDA_CbContext *pWDA;
   WDI_BtAmpEventParamsType *wdiBtAmpEventParam;

   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
              "<------ %s, wdiStatus: %d", __func__, wdiStatus);

   if(NULL == pWdaParams)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                 "%s: pWdaParams received NULL", __func__);
      VOS_ASSERT(0);
      return;
   }

   pWDA = (tWDA_CbContext *)pWdaParams->pWdaContext;
   wdiBtAmpEventParam = (WDI_BtAmpEventParamsType *)pWdaParams->wdaWdiApiMsgParam;

   if(BTAMP_EVENT_CONNECTION_TERMINATED ==
      wdiBtAmpEventParam->wdiBtAmpEventInfo.ucBtAmpEventType)
   {
      pWDA->wdaAmpSessionOn = VOS_FALSE;
   }

   if(IS_WDI_STATUS_FAILURE(wdiStatus))
   {
      vos_mem_free(pWdaParams->wdaWdiApiMsgParam);
      vos_mem_free(pWdaParams->wdaMsgParam);
      vos_mem_free(pWdaParams);
   }

   return;
}
/*
                                     
                                               
 */ 
VOS_STATUS WDA_ProcessBtAmpEventReq(tWDA_CbContext *pWDA, 
                                    tSmeBtAmpEvent *pBtAmpEventParams)
{
   WDI_Status status = WDI_STATUS_SUCCESS ;
   WDI_BtAmpEventParamsType *wdiBtAmpEventParam = 
            (WDI_BtAmpEventParamsType *)vos_mem_malloc(
                                 sizeof(WDI_BtAmpEventParamsType)) ;
   tWDA_ReqParams *pWdaParams ;
   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                                          "------> %s " ,__func__);
   if(NULL == wdiBtAmpEventParam) 
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                           "%s: VOS MEM Alloc Failure", __func__); 
      VOS_ASSERT(0);
      return VOS_STATUS_E_NOMEM;
   }
   pWdaParams = (tWDA_ReqParams *)vos_mem_malloc(sizeof(tWDA_ReqParams)) ;
   if(NULL == pWdaParams)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                           "%s: VOS MEM Alloc Failure", __func__); 
      VOS_ASSERT(0);
      vos_mem_free(wdiBtAmpEventParam);
      return VOS_STATUS_E_NOMEM;
   }
   wdiBtAmpEventParam->wdiBtAmpEventInfo.ucBtAmpEventType = 
      pBtAmpEventParams->btAmpEventType;
   wdiBtAmpEventParam->wdiReqStatusCB = WDA_BtAmpEventReqCallback;
   wdiBtAmpEventParam->pUserData = pWdaParams;
   /*                                                               */
   /*                             */
   pWdaParams->pWdaContext = pWDA;
   pWdaParams->wdaMsgParam = pBtAmpEventParams;
   pWdaParams->wdaWdiApiMsgParam = wdiBtAmpEventParam;
   status = WDI_BtAmpEventReq(wdiBtAmpEventParam, 
                              (WDI_BtAmpEventRspCb)WDA_BtAmpEventRespCallback, pWdaParams);
   if(IS_WDI_STATUS_FAILURE(status))
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
              "Failure in BT AMP event REQ Params WDI API, free all the memory " );
      vos_mem_free(pWdaParams->wdaMsgParam) ;
      vos_mem_free(pWdaParams->wdaWdiApiMsgParam);
      vos_mem_free(pWdaParams) ;
   }
   if(BTAMP_EVENT_CONNECTION_START == wdiBtAmpEventParam->wdiBtAmpEventInfo.ucBtAmpEventType)
   {
      pWDA->wdaAmpSessionOn = VOS_TRUE;
   }
   return CONVERT_WDI2VOS_STATUS(status) ;
}

/*
                                      
                                        
                            
 */ 
void WDA_FTMCommandReqCallback(void *ftmCmdRspData,
                               void *usrData)
{
   tWDA_CbContext *pWDA = (tWDA_CbContext *)usrData ;
   if((NULL == pWDA) || (NULL == ftmCmdRspData))
   {
      VOS_TRACE(VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                "%s, invalid input %p, %p",__func__,  pWDA, ftmCmdRspData);
      return;
   }
   /*                                     */
   vos_mem_free(pWDA->wdaFTMCmdReq);
   pWDA->wdaFTMCmdReq = NULL;
   /*                              */
   wlan_sys_ftm(ftmCmdRspData);
   return;
}
/*
                                  
                          
 */ 
VOS_STATUS WDA_ProcessFTMCommand(tWDA_CbContext *pWDA, 
                                 tPttMsgbuffer  *pPTTFtmCmd)
{
   WDI_Status             status = WDI_STATUS_SUCCESS;
   WDI_FTMCommandReqType *ftmCMDReq = NULL;
   ftmCMDReq = (WDI_FTMCommandReqType *)
                vos_mem_malloc(sizeof(WDI_FTMCommandReqType));
   if(NULL == ftmCMDReq)
   {
      VOS_TRACE(VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                "WDA FTM Command buffer alloc fail");
      return VOS_STATUS_E_NOMEM;
   }
   ftmCMDReq->bodyLength     = pPTTFtmCmd->msgBodyLength;
   ftmCMDReq->FTMCommandBody = (void *)pPTTFtmCmd;
   pWDA->wdaFTMCmdReq        = (void *)ftmCMDReq;
   /*                     */
   status = WDI_FTMCommandReq(ftmCMDReq, WDA_FTMCommandReqCallback, pWDA);
   return status;
}
#ifdef FEATURE_OEM_DATA_SUPPORT
/*
                                        
   
 */
void WDA_StartOemDataReqCallback(
                   WDI_oemDataRspParamsType *wdiOemDataRspParams, 
                                                        void* pUserData)
{
   VOS_STATUS status = VOS_STATUS_E_FAILURE;
   tWDA_ReqParams *pWdaParams = (tWDA_ReqParams *)pUserData;
   tWDA_CbContext *pWDA; 
   tStartOemDataRsp *pOemDataRspParams = NULL ;

   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                                          "<------ %s " ,__func__);

   if(NULL == pWdaParams)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
              "%s: pWdaParams received NULL", __func__);
      VOS_ASSERT(0) ;
      return ;
   }
   pWDA = (tWDA_CbContext *)pWdaParams->pWdaContext;

   if(NULL == pWDA) 
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                           "%s:pWDA is NULL", __func__); 
      VOS_ASSERT(0);
      return ;
   }
   
   /* 
                                                    
    */
   pOemDataRspParams = vos_mem_malloc(sizeof(tStartOemDataRsp));

   //                                                        
   if(NULL == pOemDataRspParams)
   {
      VOS_TRACE(VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                "OEM DATA WDA callback alloc fail");
      VOS_ASSERT(0) ;
      return;
   }

   //                                          
   vos_mem_free(pWdaParams->wdaWdiApiMsgParam) ;
   vos_mem_free(pWdaParams->wdaMsgParam);
   vos_mem_free(pWdaParams) ;

   /* 
                                                                         
                                                                     
    */
   vos_mem_copy(pOemDataRspParams->oemDataRsp, wdiOemDataRspParams->oemDataRsp, OEM_DATA_RSP_SIZE);
 
   //         
   status = WDA_ResumeDataTx(pWDA);
   if(status != VOS_STATUS_SUCCESS)
   {
      VOS_TRACE(VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_FATAL, "WDA Resume Data Tx fail");
   }
   WDA_SendMsg(pWDA, WDA_START_OEM_DATA_RSP,  (void *)pOemDataRspParams, 0) ;
   return ;
}
/*
                                       
                                 
 */
VOS_STATUS WDA_ProcessStartOemDataReq(tWDA_CbContext *pWDA, 
                                 tStartOemDataReq  *pOemDataReqParams)
{
   WDI_Status             status = WDI_STATUS_SUCCESS;
   WDI_oemDataReqParamsType     *wdiOemDataReqParams = NULL;
   tWDA_ReqParams *pWdaParams ;

   wdiOemDataReqParams = (WDI_oemDataReqParamsType*)vos_mem_malloc(sizeof(WDI_oemDataReqParamsType)) ;
   
   if(NULL == wdiOemDataReqParams)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                           "%s: VOS MEM Alloc Failure", __func__); 
      VOS_ASSERT(0);
      return VOS_STATUS_E_NOMEM;
   }
   
   vos_mem_copy(wdiOemDataReqParams->wdiOemDataReqInfo.selfMacAddr, 
      pOemDataReqParams->selfMacAddr, sizeof(tSirMacAddr));
   vos_mem_copy(wdiOemDataReqParams->wdiOemDataReqInfo.oemDataReq, 
      pOemDataReqParams->oemDataReq, OEM_DATA_REQ_SIZE);

   wdiOemDataReqParams->wdiReqStatusCB = NULL;

   pWdaParams = (tWDA_ReqParams *)vos_mem_malloc(sizeof(tWDA_ReqParams)) ;
   if(NULL == pWdaParams)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                           "%s: VOS MEM Alloc Failure", __func__); 
      vos_mem_free(wdiOemDataReqParams);
      vos_mem_free(pOemDataReqParams);
      VOS_ASSERT(0);
      return VOS_STATUS_E_NOMEM;
   }

   pWdaParams->pWdaContext       = (void*)pWDA;
   pWdaParams->wdaMsgParam       = (void*)pOemDataReqParams;
   pWdaParams->wdaWdiApiMsgParam = (void*)wdiOemDataReqParams;

   status = WDI_StartOemDataReq(wdiOemDataReqParams, 
      (WDI_oemDataRspCb)WDA_StartOemDataReqCallback, pWdaParams);

   if(IS_WDI_STATUS_FAILURE(status))
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
         "Failure in Start OEM DATA REQ Params WDI API, free all the memory " );
      vos_mem_free(pWdaParams->wdaWdiApiMsgParam) ;
      vos_mem_free(pWdaParams->wdaMsgParam);
      vos_mem_free(pWdaParams);
   }
   return CONVERT_WDI2VOS_STATUS(status) ;
}
#endif /*                          */
/*
                                             
   
 */ 
void WDA_SetTxPerTrackingRespCallback(WDI_Status status, void* pUserData)
{
   tWDA_ReqParams *pWdaParams = (tWDA_ReqParams *)pUserData;
   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                                          "<------ %s " ,__func__);
   if(NULL == pWdaParams)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                 "%s: pWdaParams received NULL", __func__);
      VOS_ASSERT(0) ;
      return ;
   }

   vos_mem_free(pWdaParams->wdaWdiApiMsgParam);
   vos_mem_free(pWdaParams->wdaMsgParam);
   vos_mem_free(pWdaParams);

   return ;
}
/*
                                            
               
                                                                                    
 */
void WDA_SetTxPerTrackingReqCallback(WDI_Status wdiStatus, void* pUserData)
{
   tWDA_ReqParams *pWdaParams = (tWDA_ReqParams *)pUserData;

   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
              "<------ %s, wdiStatus: %d", __func__, wdiStatus);

   if(NULL == pWdaParams)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                 "%s: pWdaParams received NULL", __func__);
      VOS_ASSERT(0);
      return;
   }

   if(IS_WDI_STATUS_FAILURE(wdiStatus))
   {
      vos_mem_free(pWdaParams->wdaWdiApiMsgParam);
      vos_mem_free(pWdaParams->wdaMsgParam);
      vos_mem_free(pWdaParams);
   }

   return;
}
#ifdef WLAN_FEATURE_GTK_OFFLOAD
/*
                                       
   
 */ 
void WDA_GTKOffloadRespCallback( WDI_GtkOffloadRspParams  *pwdiGtkOffloadRsparams,
                                        void* pUserData)
{
   tWDA_ReqParams *pWdaParams = (tWDA_ReqParams *)pUserData;

   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                                          "<------ %s " ,__func__);
   if(NULL == pWdaParams)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                 "%s: pWdaParams received NULL", __func__);
      VOS_ASSERT(0);
      return;
   }

   vos_mem_free(pWdaParams->wdaMsgParam) ;
   vos_mem_free(pWdaParams->wdaWdiApiMsgParam);
   vos_mem_free(pWdaParams) ;

   //                               
   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
              "WDA_GTKOffloadRespCallback invoked " );

   return ;
}
/*
                                      
               
                                                                              
 */
void WDA_GTKOffloadReqCallback(WDI_Status wdiStatus, void* pUserData)
{
   tWDA_ReqParams *pWdaParams = (tWDA_ReqParams *)pUserData;

   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
              "<------ %s, wdiStatus: %d", __func__, wdiStatus);

   if(NULL == pWdaParams)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                 "%s: pWdaParams received NULL", __func__);
      VOS_ASSERT(0);
      return;
   }

   if(IS_WDI_STATUS_FAILURE(wdiStatus))
   {
      vos_mem_zero(pWdaParams->wdaWdiApiMsgParam,
                    sizeof(WDI_GtkOffloadReqMsg));
      vos_mem_zero(pWdaParams->wdaMsgParam,
                    sizeof(tSirGtkOffloadParams));
      vos_mem_free(pWdaParams->wdaWdiApiMsgParam);
      vos_mem_free(pWdaParams->wdaMsgParam);
      vos_mem_free(pWdaParams);
   }

   return;
}
/*
                                     
                                                                            
                                     
 */ 
VOS_STATUS WDA_ProcessGTKOffloadReq(tWDA_CbContext *pWDA, 
                                    tpSirGtkOffloadParams pGtkOffloadParams)
{
   WDI_Status status = WDI_STATUS_E_FAILURE;
   WDI_GtkOffloadReqMsg *wdiGtkOffloadReqMsg = 
      (WDI_GtkOffloadReqMsg *)vos_mem_malloc(
         sizeof(WDI_GtkOffloadReqMsg)) ;
   tWDA_ReqParams *pWdaParams ;

   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                                          "------> %s " ,__func__);

   if(NULL == wdiGtkOffloadReqMsg) 
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                           "%s: VOS MEM Alloc Failure", __func__); 
      VOS_ASSERT(0);
      return VOS_STATUS_E_NOMEM;
   }

   pWdaParams = (tWDA_ReqParams *)vos_mem_malloc(sizeof(tWDA_ReqParams)) ;
   if(NULL == pWdaParams)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                           "%s: VOS MEM Alloc Failure", __func__); 
      VOS_ASSERT(0);
      vos_mem_free(wdiGtkOffloadReqMsg);
      return VOS_STATUS_E_NOMEM;
   }

   //
   //                                              
   //

   vos_mem_copy(wdiGtkOffloadReqMsg->gtkOffloadReqParams.bssId,
             pGtkOffloadParams->bssId, sizeof (wpt_macAddr));

   wdiGtkOffloadReqMsg->gtkOffloadReqParams.ulFlags = pGtkOffloadParams->ulFlags;
   //         
   vos_mem_copy(&(wdiGtkOffloadReqMsg->gtkOffloadReqParams.aKCK[0]), &(pGtkOffloadParams->aKCK[0]), 16);
   //         
   vos_mem_copy(&(wdiGtkOffloadReqMsg->gtkOffloadReqParams.aKEK[0]), &(pGtkOffloadParams->aKEK[0]), 16);
   //                      
   vos_mem_copy(&(wdiGtkOffloadReqMsg->gtkOffloadReqParams.ullKeyReplayCounter), 
                &(pGtkOffloadParams->ullKeyReplayCounter), sizeof(v_U64_t));

   wdiGtkOffloadReqMsg->wdiReqStatusCB = WDA_GTKOffloadReqCallback;
   wdiGtkOffloadReqMsg->pUserData = pWdaParams;


   /*                             */
   pWdaParams->wdaWdiApiMsgParam = (void *)wdiGtkOffloadReqMsg;
   pWdaParams->pWdaContext = pWDA;
   /*                                            */
   pWdaParams->wdaMsgParam = pGtkOffloadParams;

   status = WDI_GTKOffloadReq(wdiGtkOffloadReqMsg, (WDI_GtkOffloadCb)WDA_GTKOffloadRespCallback, pWdaParams);

   if(IS_WDI_STATUS_FAILURE(status))
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
              "Failure in WDA_ProcessGTKOffloadReq(), free all the memory " );
      vos_mem_zero(wdiGtkOffloadReqMsg, sizeof(WDI_GtkOffloadReqMsg));
      vos_mem_zero(pGtkOffloadParams, sizeof(tSirGtkOffloadParams));
      vos_mem_free(pWdaParams->wdaWdiApiMsgParam) ;
      vos_mem_free(pWdaParams->wdaMsgParam);
      vos_mem_free(pWdaParams);
   }

   return CONVERT_WDI2VOS_STATUS(status) ;
}

/*
                                              
   
 */ 
void WDA_GtkOffloadGetInfoRespCallback( WDI_GtkOffloadGetInfoRspParams *pwdiGtkOffloadGetInfoRsparams,
                                    void * pUserData)
{
   tWDA_ReqParams *pWdaParams = (tWDA_ReqParams *)pUserData;
   tWDA_CbContext *pWDA;
   tpSirGtkOffloadGetInfoRspParams pGtkOffloadGetInfoReq;
   tpSirGtkOffloadGetInfoRspParams pGtkOffloadGetInfoRsp;
   vos_msg_t vosMsg;

   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                                          "<------ %s " ,__func__);

   if(NULL == pWdaParams)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                 "%s: pWdaParams received NULL", __func__);
      VOS_ASSERT(0);
      return;
   }

   pGtkOffloadGetInfoRsp = vos_mem_malloc(sizeof(tSirGtkOffloadGetInfoRspParams));
   if(NULL == pGtkOffloadGetInfoRsp)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                 "%s: vos_mem_malloc failed ", __func__);
      VOS_ASSERT(0);
      return;
   }

   pWDA = (tWDA_CbContext *)pWdaParams->pWdaContext ;
   pGtkOffloadGetInfoReq = (tpSirGtkOffloadGetInfoRspParams)pWdaParams->wdaMsgParam;

   //                                                               
   vos_mem_zero(pGtkOffloadGetInfoRsp, sizeof(tSirGtkOffloadGetInfoRspParams));

   /*                */
   pGtkOffloadGetInfoRsp->mesgType = eWNI_PMC_GTK_OFFLOAD_GETINFO_RSP;
   pGtkOffloadGetInfoRsp->mesgLen = sizeof(tSirGtkOffloadGetInfoRspParams);

   pGtkOffloadGetInfoRsp->ulStatus            = pwdiGtkOffloadGetInfoRsparams->ulStatus;
   pGtkOffloadGetInfoRsp->ullKeyReplayCounter = pwdiGtkOffloadGetInfoRsparams->ullKeyReplayCounter;
   pGtkOffloadGetInfoRsp->ulTotalRekeyCount   = pwdiGtkOffloadGetInfoRsparams->ulTotalRekeyCount;
   pGtkOffloadGetInfoRsp->ulGTKRekeyCount     = pwdiGtkOffloadGetInfoRsparams->ulGTKRekeyCount;
   pGtkOffloadGetInfoRsp->ulIGTKRekeyCount    = pwdiGtkOffloadGetInfoRsparams->ulIGTKRekeyCount;

   vos_mem_copy( pGtkOffloadGetInfoRsp->bssId,
                       pwdiGtkOffloadGetInfoRsparams->bssId,
                       sizeof (wpt_macAddr));
   /*                     */
   vosMsg.type = eWNI_PMC_GTK_OFFLOAD_GETINFO_RSP;
   vosMsg.bodyptr = (void *)pGtkOffloadGetInfoRsp;
   vosMsg.bodyval = 0;

   if (VOS_STATUS_SUCCESS != vos_mq_post_message(VOS_MQ_ID_SME, (vos_msg_t*)&vosMsg))
   {
      /*                         */
      vos_mem_zero(pGtkOffloadGetInfoRsp,
                   sizeof(tSirGtkOffloadGetInfoRspParams));
      vos_mem_free((v_VOID_t *) pGtkOffloadGetInfoRsp);
   }

   vos_mem_free(pWdaParams->wdaMsgParam) ;
   vos_mem_free(pWdaParams->wdaWdiApiMsgParam);
   vos_mem_free(pWdaParams) ;

   return;
}
/*
                                             
                                        
                                                                                     
 */
void WDA_GtkOffloadGetInfoReqCallback(WDI_Status wdiStatus, void * pUserData)
{
   tWDA_ReqParams *pWdaParams = (tWDA_ReqParams *)pUserData;
   vos_msg_t vosMsg;

   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
              "<------ %s, wdiStatus: %d", __func__, wdiStatus);

   if(NULL == pWdaParams)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                 "%s: pWdaParams received NULL", __func__);
      VOS_ASSERT(0);
      return;
   }

   /*                     */
   vosMsg.type = eWNI_PMC_GTK_OFFLOAD_GETINFO_RSP;
   vosMsg.bodyptr = NULL;
   vosMsg.bodyval = 0;

   if(IS_WDI_STATUS_FAILURE(wdiStatus))
   {
      vos_mem_free(pWdaParams->wdaWdiApiMsgParam);
      vos_mem_free(pWdaParams->wdaMsgParam);
      vos_mem_free(pWdaParams);
      vos_mq_post_message(VOS_MQ_ID_SME, (vos_msg_t*)&vosMsg);
   }

   return;
}
#endif

/*
                                           
                                                       
 */ 
VOS_STATUS WDA_ProcessSetTxPerTrackingReq(tWDA_CbContext *pWDA, tSirTxPerTrackingParam *pTxPerTrackingParams)
{
   VOS_STATUS status = VOS_STATUS_SUCCESS;
   WDI_Status wstatus;
   WDI_SetTxPerTrackingReqParamsType *pwdiSetTxPerTrackingReqParams = 
      (WDI_SetTxPerTrackingReqParamsType *)vos_mem_malloc(
         sizeof(WDI_SetTxPerTrackingReqParamsType)) ;
   tWDA_ReqParams *pWdaParams ;
   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                                          "------> %s " ,__func__);
   if(NULL == pwdiSetTxPerTrackingReqParams) 
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                           "%s: VOS MEM Alloc Failure", __func__); 
      vos_mem_free(pTxPerTrackingParams);
      VOS_ASSERT(0);
      return VOS_STATUS_E_NOMEM;
   }
   pWdaParams = (tWDA_ReqParams *)vos_mem_malloc(sizeof(tWDA_ReqParams)) ;
   if(NULL == pWdaParams)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                           "%s: VOS MEM Alloc Failure", __func__); 
      vos_mem_free(pwdiSetTxPerTrackingReqParams);
      vos_mem_free(pTxPerTrackingParams);
      VOS_ASSERT(0);
      return VOS_STATUS_E_NOMEM;
   }
   pwdiSetTxPerTrackingReqParams->wdiTxPerTrackingParam.ucTxPerTrackingEnable = 
      pTxPerTrackingParams->ucTxPerTrackingEnable;
   pwdiSetTxPerTrackingReqParams->wdiTxPerTrackingParam.ucTxPerTrackingPeriod = 
      pTxPerTrackingParams->ucTxPerTrackingPeriod;
   pwdiSetTxPerTrackingReqParams->wdiTxPerTrackingParam.ucTxPerTrackingRatio = 
      pTxPerTrackingParams->ucTxPerTrackingRatio;
   pwdiSetTxPerTrackingReqParams->wdiTxPerTrackingParam.uTxPerTrackingWatermark = 
      pTxPerTrackingParams->uTxPerTrackingWatermark;
   pwdiSetTxPerTrackingReqParams->wdiReqStatusCB = WDA_SetTxPerTrackingReqCallback;
   pwdiSetTxPerTrackingReqParams->pUserData = pWdaParams;
   /*                                            */
   /*                             
                                                                                         */
   pWdaParams->wdaWdiApiMsgParam = pwdiSetTxPerTrackingReqParams;
   pWdaParams->pWdaContext = pWDA;
   pWdaParams->wdaMsgParam = pTxPerTrackingParams;
   wstatus = WDI_SetTxPerTrackingReq(pwdiSetTxPerTrackingReqParams, 
                                    (WDI_SetTxPerTrackingRspCb)WDA_SetTxPerTrackingRespCallback, pWdaParams);
   if(IS_WDI_STATUS_FAILURE(wstatus))
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
              "Failure in Set Tx PER REQ WDI API, free all the memory " );
      status = CONVERT_WDI2VOS_STATUS(wstatus);
      vos_mem_free(pWdaParams->wdaMsgParam) ;
      vos_mem_free(pWdaParams->wdaWdiApiMsgParam);
      vos_mem_free(pWdaParams) ;
   }
   return status;

}/*                              */
/*
                                   
                           
 */
void WDA_HALDumpCmdCallback(WDI_HALDumpCmdRspParamsType *wdiRspParams, 
                                                            void* pUserData)
{
   tANI_U8 *buffer = NULL;
   tWDA_CbContext *pWDA = NULL;
   tWDA_ReqParams *pWdaParams = (tWDA_ReqParams *)pUserData;
   if(NULL == pWdaParams)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                 "%s: pWdaParams received NULL", __func__);
      VOS_ASSERT(0) ;
      return ;
   }
   
   pWDA = pWdaParams->pWdaContext;
   buffer = (tANI_U8 *)pWdaParams->wdaMsgParam;
   if(wdiRspParams->usBufferLen > 0)
   {
      /*                                          */
      vos_mem_copy(buffer, wdiRspParams->pBuffer, wdiRspParams->usBufferLen);
   }
   vos_mem_free(pWdaParams->wdaWdiApiMsgParam);
   vos_mem_free(pWdaParams);
   
   /*                                        */
   vos_WDAComplete_cback(pWDA->pVosContext);
   return ;
}

/*
                                     
                           
 */ 
VOS_STATUS WDA_HALDumpCmdReq(tpAniSirGlobal   pMac, tANI_U32  cmd, 
                 tANI_U32   arg1, tANI_U32   arg2, tANI_U32   arg3,
                 tANI_U32   arg4, tANI_U8   *pBuffer)
{
   WDI_Status             status = WDI_STATUS_SUCCESS;
   WDI_HALDumpCmdReqParamsType *wdiHALDumpCmdReqParam = NULL;
   WDI_HALDumpCmdReqInfoType *wdiHalDumpCmdInfo = NULL ;
   tWDA_ReqParams *pWdaParams ;
   pVosContextType pVosContext = NULL; 
   VOS_STATUS vStatus;
   pVosContext = (pVosContextType)vos_get_global_context(VOS_MODULE_ID_PE,
                                                           (void *)pMac);
   if(pVosContext)
   {
      if (pVosContext->isLogpInProgress)
      {
         VOS_TRACE(VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_FATAL,
                                "%s:LOGP in Progress. Ignore!!!", __func__);
         return VOS_STATUS_E_BUSY;
      }
   }
   else
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                          "%s: VOS Context Null", __func__);
      return VOS_STATUS_E_RESOURCES;
   }

   pWdaParams = (tWDA_ReqParams *)vos_mem_malloc(sizeof(tWDA_ReqParams)) ;
   if(NULL == pWdaParams)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                           "%s: VOS MEM Alloc Failure", __func__); 
      return VOS_STATUS_E_NOMEM;
   }
   /*                                      */
   wdiHALDumpCmdReqParam = (WDI_HALDumpCmdReqParamsType *)
                vos_mem_malloc(sizeof(WDI_HALDumpCmdReqParamsType));
   if(NULL == wdiHALDumpCmdReqParam)
   {
      VOS_TRACE(VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                "WDA HAL DUMP Command buffer alloc fail");
      vos_mem_free(pWdaParams);
      return WDI_STATUS_E_FAILURE;
   }
   wdiHalDumpCmdInfo = &wdiHALDumpCmdReqParam->wdiHALDumpCmdInfoType;
   /*                       */
   wdiHalDumpCmdInfo->command     = cmd;
   wdiHalDumpCmdInfo->argument1   = arg1;
   wdiHalDumpCmdInfo->argument2   = arg2;
   wdiHalDumpCmdInfo->argument3   = arg3;
   wdiHalDumpCmdInfo->argument4   = arg4;
   wdiHALDumpCmdReqParam->wdiReqStatusCB = NULL ;
   pWdaParams->pWdaContext = pVosContext->pWDAContext;
   
   /*                                                     */
   pWdaParams->wdaMsgParam = (void *)pBuffer;
   
   /*                             */
   pWdaParams->wdaWdiApiMsgParam = (void *)wdiHALDumpCmdReqParam ;
   /*                     */
   status = WDI_HALDumpCmdReq(wdiHALDumpCmdReqParam, WDA_HALDumpCmdCallback, pWdaParams);
   vStatus = vos_wait_single_event( &(pVosContext->wdaCompleteEvent), WDA_DUMPCMD_WAIT_TIMEOUT );
   if ( vStatus != VOS_STATUS_SUCCESS )
   {
      if ( vStatus == VOS_STATUS_E_TIMEOUT )
      {
         VOS_TRACE( VOS_MODULE_ID_VOSS, VOS_TRACE_LEVEL_ERROR,
         "%s: Timeout occurred before WDA_HALDUMP complete",__func__);
      }
      else
      {
         VOS_TRACE( VOS_MODULE_ID_VOSS, VOS_TRACE_LEVEL_ERROR,
         "%s: WDA_HALDUMP reporting  other error",__func__);
      }
      VOS_ASSERT(0);
   }
   return status;
}
#ifdef WLAN_FEATURE_GTK_OFFLOAD
/*
                                            
                                                
 */ 
VOS_STATUS WDA_ProcessGTKOffloadGetInfoReq(tWDA_CbContext *pWDA, 
                                           tpSirGtkOffloadGetInfoRspParams pGtkOffloadGetInfoRsp)
{
   WDI_Status status = WDI_STATUS_E_FAILURE;
   WDI_GtkOffloadGetInfoReqMsg *pwdiGtkOffloadGetInfoReqMsg = 
      (WDI_GtkOffloadGetInfoReqMsg *)vos_mem_malloc(sizeof(WDI_GtkOffloadGetInfoReqMsg));
   tWDA_ReqParams *pWdaParams ;

   if(NULL == pwdiGtkOffloadGetInfoReqMsg) 
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                           "%s: VOS MEM Alloc Failure", __func__); 
      VOS_ASSERT(0);
      return VOS_STATUS_E_NOMEM;
   }

   pWdaParams = (tWDA_ReqParams *)vos_mem_malloc(sizeof(tWDA_ReqParams)) ;
   if(NULL == pWdaParams)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                           "%s: VOS MEM Alloc Failure", __func__); 
      VOS_ASSERT(0);
      vos_mem_free(pwdiGtkOffloadGetInfoReqMsg);
      return VOS_STATUS_E_NOMEM;
   }

   pwdiGtkOffloadGetInfoReqMsg->wdiReqStatusCB = WDA_GtkOffloadGetInfoReqCallback;
   pwdiGtkOffloadGetInfoReqMsg->pUserData = pWdaParams;

   /*                             */
   pWdaParams->wdaWdiApiMsgParam = (void *)pwdiGtkOffloadGetInfoReqMsg;
   pWdaParams->pWdaContext = pWDA;
   /*                                            */
   pWdaParams->wdaMsgParam = pGtkOffloadGetInfoRsp;

   vos_mem_copy(pwdiGtkOffloadGetInfoReqMsg->WDI_GtkOffloadGetInfoReqParams.bssId,
             pGtkOffloadGetInfoRsp->bssId, sizeof (wpt_macAddr));

   status = WDI_GTKOffloadGetInfoReq(pwdiGtkOffloadGetInfoReqMsg, (WDI_GtkOffloadGetInfoCb)WDA_GtkOffloadGetInfoRespCallback, pWdaParams);

   if(IS_WDI_STATUS_FAILURE(status))
   {
      /*                             */
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
              "Failure in WDA_ProcessGTKOffloadGetInfoReq(), free all the memory " );
      vos_mem_free(pWdaParams->wdaWdiApiMsgParam);
      vos_mem_free(pWdaParams) ;
      pGtkOffloadGetInfoRsp->ulStatus = eSIR_FAILURE ;
      WDA_SendMsg(pWDA, WDA_GTK_OFFLOAD_GETINFO_RSP, (void *)pGtkOffloadGetInfoRsp, 0) ;
   }

   return CONVERT_WDI2VOS_STATUS(status) ;
}
#endif //                         

/*
                                            
  
 */
VOS_STATUS WDA_ProcessAddPeriodicTxPtrnInd(tWDA_CbContext *pWDA,
                               tSirAddPeriodicTxPtrn *pAddPeriodicTxPtrnParams)
{
   WDI_Status wdiStatus;
   WDI_AddPeriodicTxPtrnParamsType *addPeriodicTxPtrnParams;

   addPeriodicTxPtrnParams =
      vos_mem_malloc(sizeof(WDI_AddPeriodicTxPtrnParamsType));

   if (NULL == addPeriodicTxPtrnParams)
   {
      VOS_TRACE(VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                "%s: Not able to allocate memory for addPeriodicTxPtrnParams!",
                __func__);

      return VOS_STATUS_E_NOMEM;
   }

   vos_mem_copy(&(addPeriodicTxPtrnParams->wdiAddPeriodicTxPtrnParams),
                pAddPeriodicTxPtrnParams, sizeof(tSirAddPeriodicTxPtrn));

   addPeriodicTxPtrnParams->wdiReqStatusCB = WDA_WdiIndicationCallback;
   addPeriodicTxPtrnParams->pUserData = pWDA;

   wdiStatus = WDI_AddPeriodicTxPtrnInd(addPeriodicTxPtrnParams);

   if (WDI_STATUS_PENDING == wdiStatus)
   {
      VOS_TRACE(VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                "Pending received for %s:%d", __func__, __LINE__ );
   }
   else if (WDI_STATUS_SUCCESS_SYNC != wdiStatus)
   {
      VOS_TRACE(VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                "Failure in %s:%d", __func__, __LINE__ );
   }

   vos_mem_free(addPeriodicTxPtrnParams);

   return CONVERT_WDI2VOS_STATUS(wdiStatus);
}

/*
                                            
  
 */
VOS_STATUS WDA_ProcessDelPeriodicTxPtrnInd(tWDA_CbContext *pWDA,
                               tSirDelPeriodicTxPtrn *pDelPeriodicTxPtrnParams)
{
   WDI_Status wdiStatus;
   WDI_DelPeriodicTxPtrnParamsType *delPeriodicTxPtrnParams;

   delPeriodicTxPtrnParams =
      vos_mem_malloc(sizeof(WDI_DelPeriodicTxPtrnParamsType));

   if (NULL == delPeriodicTxPtrnParams)
   {
      VOS_TRACE(VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                "%s: Not able to allocate memory for delPeriodicTxPtrnParams!",
                __func__);

      return VOS_STATUS_E_NOMEM;
   }

   vos_mem_copy(&(delPeriodicTxPtrnParams->wdiDelPeriodicTxPtrnParams),
                pDelPeriodicTxPtrnParams, sizeof(tSirDelPeriodicTxPtrn));

   delPeriodicTxPtrnParams->wdiReqStatusCB = WDA_WdiIndicationCallback;
   delPeriodicTxPtrnParams->pUserData = pWDA;

   wdiStatus = WDI_DelPeriodicTxPtrnInd(delPeriodicTxPtrnParams);

   if (WDI_STATUS_PENDING == wdiStatus)
   {
      VOS_TRACE(VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                "Pending received for %s:%d", __func__, __LINE__ );
   }
   else if (WDI_STATUS_SUCCESS_SYNC != wdiStatus)
   {
      VOS_TRACE(VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                "Failure in %s:%d", __func__, __LINE__ );
   }

   vos_mem_free(delPeriodicTxPtrnParams);

   return CONVERT_WDI2VOS_STATUS(wdiStatus);
}

#ifdef FEATURE_WLAN_BATCH_SCAN
/*
                                        
  
                                                                             
  
         
                               
                                           
 */
VOS_STATUS WDA_ProcessStopBatchScanInd(tWDA_CbContext *pWDA,
                               tSirStopBatchScanInd *pReq)
{
   WDI_Status wdiStatus;
   WDI_StopBatchScanIndType wdiReq;

   wdiReq.param = pReq->param;

   wdiStatus = WDI_StopBatchScanInd(&wdiReq);

   if (WDI_STATUS_SUCCESS_SYNC != wdiStatus)
   {
      VOS_TRACE(VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                "Stop batch scan ind failed %s:%d", __func__, wdiStatus);
   }

   vos_mem_free(pReq);

   return CONVERT_WDI2VOS_STATUS(wdiStatus);
}
/*                                                                          
                                               

             
                                         

            
                                
                                                                 

              
        

                                                                           */
VOS_STATUS WDA_ProcessTriggerBatchScanResultInd(tWDA_CbContext *pWDA,
       tSirTriggerBatchScanResultInd *pReq)
{
   WDI_Status wdiStatus;
   WDI_TriggerBatchScanResultIndType wdiReq;

   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO_HIGH,
                                          "------> %s " ,__func__);

   wdiReq.param = pReq->param;

   wdiStatus = WDI_TriggerBatchScanResultInd(&wdiReq);

   if (WDI_STATUS_SUCCESS_SYNC != wdiStatus)
   {
      VOS_TRACE(VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
          "Trigger batch scan result ind failed %s:%d",
          __func__, wdiStatus);
   }

   vos_mem_free(pReq);

   return CONVERT_WDI2VOS_STATUS(wdiStatus);
}

/*                                                                          
                                       

             
                                                  

            
                                            
                                   

              
        

                                                                           */
void WDA_SetBatchScanRespCallback
(
    WDI_SetBatchScanRspType *pRsp,
    void* pUserData
)
{
   tSirSetBatchScanRsp *pHddSetBatchScanRsp;
   tpAniSirGlobal pMac;
   void *pCallbackContext;
   tWDA_CbContext *pWDA = NULL ;
   tWDA_ReqParams *pWdaParams = (tWDA_ReqParams *)pUserData;


   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO_HIGH,
                                          "<------ %s " ,__func__);
   if (NULL == pWdaParams)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
              "%s: pWdaParams received NULL", __func__);
      VOS_ASSERT(0) ;
      return ;
   }

   /*                   */
   pWDA = pWdaParams->pWdaContext;
   if (NULL == pWDA)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
          "%s:pWDA is NULL can't invole HDD callback",
           __func__);
      vos_mem_free(pWdaParams->wdaWdiApiMsgParam);
      vos_mem_free(pWdaParams->wdaMsgParam);
      vos_mem_free(pWdaParams);
      VOS_ASSERT(0);
      return;
   }

   vos_mem_free(pWdaParams->wdaWdiApiMsgParam);
   vos_mem_free(pWdaParams->wdaMsgParam);
   vos_mem_free(pWdaParams);

   pMac = (tpAniSirGlobal )VOS_GET_MAC_CTXT(pWDA->pVosContext);
   if (NULL == pMac)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
          "%s:pMac is NULL", __func__);
      VOS_ASSERT(0);
      return;
   }

   pHddSetBatchScanRsp =
     (tSirSetBatchScanRsp *)vos_mem_malloc(sizeof(tSirSetBatchScanRsp));
   if (NULL == pHddSetBatchScanRsp)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
          "%s: VOS MEM Alloc Failure can't invoke HDD callback", __func__);
      VOS_ASSERT(0);
      return;
   }

   pHddSetBatchScanRsp->nScansToBatch = pRsp->nScansToBatch;

   pCallbackContext = pMac->pmc.setBatchScanReqCallbackContext;
   /*                                                   */
   if(pMac->pmc.setBatchScanReqCallback)
   {
       pMac->pmc.setBatchScanReqCallback(pCallbackContext, pHddSetBatchScanRsp);
   }
   else
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
          "%s:HDD callback is null", __func__);
      VOS_ASSERT(0);
   }

   vos_mem_free(pHddSetBatchScanRsp);
   return ;
}

/*                                                                          
                                     

             
                                             

            
                                
                                                   

              
        

                                                                           */
VOS_STATUS WDA_ProcessSetBatchScanReq(tWDA_CbContext *pWDA,
       tSirSetBatchScanReq *pSetBatchScanReq)
{
   WDI_Status status;
   tWDA_ReqParams *pWdaParams ;
   WDI_SetBatchScanReqType *pWdiSetBatchScanReq;

   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO_HIGH,
                                          "------> %s " ,__func__);

   pWdiSetBatchScanReq =
     (WDI_SetBatchScanReqType *)vos_mem_malloc(sizeof(WDI_SetBatchScanReqType));
   if (NULL == pWdiSetBatchScanReq)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                           "%s: VOS MEM Alloc Failure", __func__);
      vos_mem_free(pSetBatchScanReq);
      VOS_ASSERT(0);
      return VOS_STATUS_E_NOMEM;
   }

   pWdaParams = (tWDA_ReqParams *)vos_mem_malloc(sizeof(tWDA_ReqParams));
   if (NULL == pWdaParams)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                           "%s: VOS MEM Alloc Failure", __func__);
      VOS_ASSERT(0);
      vos_mem_free(pSetBatchScanReq);
      vos_mem_free(pWdiSetBatchScanReq);
      return VOS_STATUS_E_NOMEM;
   }

   pWdiSetBatchScanReq->scanFrequency = pSetBatchScanReq->scanFrequency;
   pWdiSetBatchScanReq->numberOfScansToBatch =
               pSetBatchScanReq->numberOfScansToBatch;
   pWdiSetBatchScanReq->bestNetwork = pSetBatchScanReq->bestNetwork;
   pWdiSetBatchScanReq->rfBand = pSetBatchScanReq->rfBand;
   pWdiSetBatchScanReq->rtt = pSetBatchScanReq->rtt;

   pWdaParams->wdaWdiApiMsgParam = pWdiSetBatchScanReq;
   pWdaParams->pWdaContext = pWDA;
   pWdaParams->wdaMsgParam = pSetBatchScanReq;

   status = WDI_SetBatchScanReq(pWdiSetBatchScanReq, pWdaParams,
                (WDI_SetBatchScanCb)WDA_SetBatchScanRespCallback);
   if (IS_WDI_STATUS_FAILURE(status))
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
              "Failure in Set Batch Scan REQ WDI API, free all the memory " );
      vos_mem_free(pWdaParams->wdaWdiApiMsgParam);
      vos_mem_free(pWdaParams->wdaMsgParam);
      vos_mem_free(pWdaParams);
   }
   return CONVERT_WDI2VOS_STATUS(status);
}

#endif
/*
                                       
  
                                                          
                                                  
  
         
                               
                                            
 */
VOS_STATUS WDA_ProcessHT40OBSSScanInd(tWDA_CbContext *pWDA,
                               tSirHT40OBSSScanInd *pReq)
{
   WDI_Status status;
   WDI_HT40ObssScanParamsType wdiOBSSScanParams;
   WDI_HT40ObssScanIndType *pWdiOBSSScanInd;

   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                             "------> %s " ,__func__);
   wdiOBSSScanParams.wdiReqStatusCB = WDA_WdiIndicationCallback;
   wdiOBSSScanParams.pUserData = pWDA;

   pWdiOBSSScanInd = &(wdiOBSSScanParams.wdiHT40ObssScanParam);
   pWdiOBSSScanInd->cmdType  = pReq->cmdType;
   pWdiOBSSScanInd->scanType = pReq->scanType;
   pWdiOBSSScanInd->OBSSScanActiveDwellTime =
      pReq->OBSSScanActiveDwellTime;
   pWdiOBSSScanInd->OBSSScanPassiveDwellTime =
      pReq->OBSSScanPassiveDwellTime;
   pWdiOBSSScanInd->BSSChannelWidthTriggerScanInterval =
      pReq->BSSChannelWidthTriggerScanInterval;
   pWdiOBSSScanInd->BSSWidthChannelTransitionDelayFactor =
      pReq->BSSWidthChannelTransitionDelayFactor;
   pWdiOBSSScanInd->OBSSScanActiveTotalPerChannel =
      pReq->OBSSScanActiveTotalPerChannel;
   pWdiOBSSScanInd->OBSSScanPassiveTotalPerChannel =
      pReq->OBSSScanPassiveTotalPerChannel;
   pWdiOBSSScanInd->OBSSScanActivityThreshold =
      pReq->OBSSScanActivityThreshold;
   pWdiOBSSScanInd->channelCount = pReq->channelCount;
   vos_mem_copy(pWdiOBSSScanInd->channels,
                pReq->channels,
                pReq->channelCount);
   pWdiOBSSScanInd->selfStaIdx = pReq->selfStaIdx;
   pWdiOBSSScanInd->fortyMHZIntolerent =  pReq->fortyMHZIntolerent;
   pWdiOBSSScanInd->bssIdx = pReq->bssIdx;
   pWdiOBSSScanInd->currentOperatingClass = pReq->currentOperatingClass;
   pWdiOBSSScanInd->ieFieldLen = pReq->ieFieldLen;

   vos_mem_copy(pWdiOBSSScanInd->ieField,
                pReq->ieField,
                pReq->ieFieldLen);

   status = WDI_HT40OBSSScanInd(&wdiOBSSScanParams);
   if (WDI_STATUS_PENDING == status)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
              "Pending received for %s:%d ",__func__,__LINE__ );
   }
   else if (WDI_STATUS_SUCCESS_SYNC != status)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
              "Failure in %s:%d ",__func__,__LINE__ );
   }
   return CONVERT_WDI2VOS_STATUS(status) ;
}
/*
                                           
  
                                                                            
  
         
                               
                                           
 */
VOS_STATUS WDA_ProcessHT40OBSSStopScanInd(tWDA_CbContext *pWDA,
                             tANI_U8   *bssIdx)
{
   WDI_Status status;

   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                       "------> %s " ,__func__);

   status = WDI_HT40OBSSStopScanInd(*bssIdx);
   if (WDI_STATUS_PENDING == status)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
              "Pending received for %s:%d ",__func__,__LINE__ );
   }
   else if (WDI_STATUS_SUCCESS_SYNC != status)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
              "Failure in %s:%d ",__func__,__LINE__ );
   }
   return CONVERT_WDI2VOS_STATUS(status) ;
}
/*
                                     
  
 */
VOS_STATUS WDA_ProcessRateUpdateInd(tWDA_CbContext *pWDA,
                               tSirRateUpdateInd *pRateUpdateParams)
{
   WDI_Status wdiStatus;
   WDI_RateUpdateIndParams rateUpdateParams;

   vos_mem_copy(rateUpdateParams.bssid,
            pRateUpdateParams->bssid, sizeof(tSirMacAddr));

   rateUpdateParams.ucastDataRateTxFlag =
                     pRateUpdateParams->ucastDataRateTxFlag;
   rateUpdateParams.reliableMcastDataRateTxFlag =
                     pRateUpdateParams->reliableMcastDataRateTxFlag;
   rateUpdateParams.mcastDataRate24GHzTxFlag =
                     pRateUpdateParams->mcastDataRate24GHzTxFlag;
   rateUpdateParams.mcastDataRate5GHzTxFlag =
                     pRateUpdateParams->mcastDataRate5GHzTxFlag;

   rateUpdateParams.ucastDataRate = pRateUpdateParams->ucastDataRate;
   rateUpdateParams.reliableMcastDataRate =
                                 pRateUpdateParams->reliableMcastDataRate;
   rateUpdateParams.mcastDataRate24GHz = pRateUpdateParams->mcastDataRate24GHz;
   rateUpdateParams.mcastDataRate5GHz = pRateUpdateParams->mcastDataRate5GHz;

   rateUpdateParams.wdiReqStatusCB = WDA_WdiIndicationCallback;
   rateUpdateParams.pUserData = pWDA;

   wdiStatus = WDI_RateUpdateInd(&rateUpdateParams);

   if (WDI_STATUS_PENDING == wdiStatus)
   {
      VOS_TRACE(VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                "Pending received for %s:%d", __func__, __LINE__ );
   }
   else if (WDI_STATUS_SUCCESS_SYNC != wdiStatus)
   {
      VOS_TRACE(VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                "Failure in %s:%d", __func__, __LINE__ );
   }

   vos_mem_free(pRateUpdateParams);

   return CONVERT_WDI2VOS_STATUS(wdiStatus);
}

/*
                                                                            
                                          
                                                                             
 */
/*
                           
                                         
 */ 
VOS_STATUS WDA_TxComplete( v_PVOID_t pVosContext, vos_pkt_t *pData, 
                                                VOS_STATUS status )
{
   
   tWDA_CbContext *wdaContext= (tWDA_CbContext *)VOS_GET_WDA_CTXT(pVosContext);
   tpAniSirGlobal pMac = (tpAniSirGlobal)VOS_GET_MAC_CTXT((void *)pVosContext) ;
   tANI_U32 uUserData; 

   if(NULL == wdaContext)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                           "%s:pWDA is NULL", 
                           __func__); 
      VOS_ASSERT(0);
      return VOS_STATUS_E_FAILURE;
   }

    /*                                   */
    vos_pkt_get_user_data_ptr(  pData, VOS_PKT_USER_DATA_ID_WDA,
                               (v_PVOID_t)&uUserData);

    if ( WDA_TL_TX_MGMT_TIMED_OUT == uUserData )
    {
       /*                                               */
       vos_pkt_return_packet(pData); 
       return VOS_STATUS_SUCCESS; 
    }

   /*                                                                                          */
   if( NULL!=wdaContext->pTxCbFunc) 
   {
      /*                                */
      if(vos_atomic_set((uintptr_t*)&wdaContext->VosPacketToFree, (uintptr_t)WDA_TX_PACKET_FREED) == (uintptr_t)pData)
      {
         wdaContext->pTxCbFunc(pMac, pData); 
      }
      else
      {
         VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_WARN,
                           "%s:packet (%p) is already freed",
                           __func__, pData);
         //                                                                          
         return status;
      }
   }

   /* 
                                                                         
                  
                                                                             
                                                                                           
                             
    */
   status  = vos_event_set(&wdaContext->txFrameEvent);
   if(!VOS_IS_STATUS_SUCCESS(status))
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR, 
                      "NEW VOS Event Set failed - status = %d", status);
   }
   return status;
}
/*
                         
                                     
 */ 
VOS_STATUS WDA_TxPacket(tWDA_CbContext *pWDA, 
                           void *pFrmBuf,
                           tANI_U16 frmLen,
                           eFrameType frmType,
                           eFrameTxDir txDir,
                           tANI_U8 tid,
                           pWDATxRxCompFunc pCompFunc,
                           void *pData,
                           pWDAAckFnTxComp pAckTxComp,
                           tANI_U32 txFlag)
{
   VOS_STATUS status = VOS_STATUS_SUCCESS ;
   tpSirMacFrameCtl pFc = (tpSirMacFrameCtl ) pData;
   tANI_U8 ucTypeSubType = pFc->type <<4 | pFc->subType;
   tANI_U8 eventIdx = 0;
   tBssSystemRole systemRole = eSYSTEM_UNKNOWN_ROLE;
   tpAniSirGlobal pMac;
   if((NULL == pWDA)||(NULL == pFrmBuf)) 
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                           "%s:pWDA %p or pFrmBuf %p is NULL",
                           __func__,pWDA,pFrmBuf); 
      VOS_ASSERT(0);
      return VOS_STATUS_E_FAILURE;
   }
   
   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO_HIGH, 
               "Tx Mgmt Frame Subtype: %d alloc(%p)", pFc->subType, pFrmBuf);
   pMac = (tpAniSirGlobal )VOS_GET_MAC_CTXT(pWDA->pVosContext);
   if(NULL == pMac)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                           "%s:pMac is NULL", __func__); 
      VOS_ASSERT(0);
      return VOS_STATUS_E_FAILURE;
   }
   
   
   
   /*                                             */
   pWDA->pTxCbFunc = pCompFunc;
   /*                                                       */
   if( pAckTxComp )
   {
       if( NULL != pWDA->pAckTxCbFunc )
       {
           /*                                                  */
           VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR, 
                   "There is already one request pending for tx complete");
           pWDA->pAckTxCbFunc( pMac, 0);
           pWDA->pAckTxCbFunc = NULL;

           if( VOS_STATUS_SUCCESS !=
                   WDA_STOP_TIMER(&pWDA->wdaTimers.TxCompleteTimer))
           {
               VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                       "Tx Complete timeout Timer Stop Failed ");
           }
           else
           {
               VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                       "Tx Complete timeout Timer Stop Success ");
           }
       }

       txFlag |= HAL_TXCOMP_REQUESTED_MASK;
       pWDA->pAckTxCbFunc = pAckTxComp;
       if( VOS_STATUS_SUCCESS !=
               WDA_START_TIMER(&pWDA->wdaTimers.TxCompleteTimer) ) 
       {
           VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                   "Tx Complete Timer Start Failed ");
           pWDA->pAckTxCbFunc = NULL;
           return eHAL_STATUS_FAILURE;
       }
   } 
   /*                                     */
   status = vos_event_reset(&pWDA->txFrameEvent);
   if(!VOS_IS_STATUS_SUCCESS(status))
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR, 
                            "VOS Event reset failed - status = %d",status);
      pCompFunc(pWDA->pVosContext, (vos_pkt_t *)pFrmBuf);
      if( pAckTxComp )
      {
         pWDA->pAckTxCbFunc = NULL;
         if( VOS_STATUS_SUCCESS !=
                           WDA_STOP_TIMER(&pWDA->wdaTimers.TxCompleteTimer))
         {
            VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                                "Tx Complete timeout Timer Stop Failed ");
         }
      }
      return VOS_STATUS_E_FAILURE;
   }

   /*                                                     */
   if(txFlag & HAL_USE_PEER_STA_REQUESTED_MASK)
   {
      txFlag &= ~HAL_USE_PEER_STA_REQUESTED_MASK;
   }
   else
   {
      /*                                                                      */
      systemRole = wdaGetGlobalSystemRole(pMac);
      if (( eSYSTEM_UNKNOWN_ROLE == systemRole ) || 
          (( eSYSTEM_STA_ROLE == systemRole )
#if defined FEATURE_WLAN_ESE || defined FEATURE_WLAN_TDLS
         && frmType == HAL_TXRX_FRM_802_11_MGMT
#endif
         ))
      {
         txFlag |= HAL_USE_SELF_STA_REQUESTED_MASK;
      }
   }

   /*                                                                        
                                                                              */
   if ((pFc->type == SIR_MAC_MGMT_FRAME))
   {
       if ((pFc->subType == SIR_MAC_MGMT_REASSOC_RSP) ||
               (pFc->subType == SIR_MAC_MGMT_PROBE_REQ))
       {
             /*                                         */
             txFlag |= HAL_USE_SELF_STA_REQUESTED_MASK;
       }
       /*                                                                        
                                    */
       if (pFc->subType == SIR_MAC_MGMT_PROBE_RSP) 
       {
           //                                                                     
           txFlag |= (HAL_USE_NO_ACK_REQUESTED_MASK | HAL_USE_SELF_STA_REQUESTED_MASK);
       }
       if(VOS_TRUE == pWDA->wdaAmpSessionOn)
       {
          txFlag |= HAL_USE_BD_RATE2_FOR_MANAGEMENT_FRAME;
       }
   }
   vos_atomic_set((uintptr_t*)&pWDA->VosPacketToFree, (uintptr_t)pFrmBuf);/*                              */

   /*                   
                                                                     */
   vos_pkt_set_user_data_ptr( (vos_pkt_t *)pFrmBuf, VOS_PKT_USER_DATA_ID_WDA, 
                       (v_PVOID_t)0);
   

   if((status = WLANTL_TxMgmtFrm(pWDA->pVosContext, (vos_pkt_t *)pFrmBuf, 
                     frmLen, ucTypeSubType, tid, 
                     WDA_TxComplete, NULL, txFlag)) != VOS_STATUS_SUCCESS) 
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR, 
                       "Sending Mgmt Frame failed - status = %d", status);
      pCompFunc(VOS_GET_MAC_CTXT(pWDA->pVosContext), (vos_pkt_t *)pFrmBuf);
      vos_atomic_set((uintptr_t*)&pWDA->VosPacketToFree, (v_U32_t)WDA_TX_PACKET_FREED);/*                         */
      if( pAckTxComp )
      {
         pWDA->pAckTxCbFunc = NULL;
         if( VOS_STATUS_SUCCESS !=
                           WDA_STOP_TIMER(&pWDA->wdaTimers.TxCompleteTimer))
         {
            VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                                "Tx Complete timeout Timer Stop Failed ");
         }
      } 
      return VOS_STATUS_E_FAILURE;
   }
   /* 
                                                                        
                                                                               
    */
   status = vos_wait_events(&pWDA->txFrameEvent, 1, WDA_TL_TX_FRAME_TIMEOUT,
                            &eventIdx);
   if(!VOS_IS_STATUS_SUCCESS(status))
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR, 
                 "%s: Status %d when waiting for TX Frame Event",
                 __func__, status);
      pWDA->pTxCbFunc = NULL;   /*                                                
                                                                                  */

      /*                                                         */
      WDA_TransportChannelDebug(pMac, 1, 0);

      /*                                         */
      vos_pkt_set_user_data_ptr( (vos_pkt_t *)pFrmBuf, VOS_PKT_USER_DATA_ID_WDA, 
                       (v_PVOID_t)WDA_TL_TX_MGMT_TIMED_OUT);

      /*                                                                        
                                           
      */
      vos_atomic_set((uintptr_t*)&pWDA->VosPacketToFree, (uintptr_t)WDA_TX_PACKET_FREED);
      /*                                                                                                          
       
                                                                              
        */

      if( pAckTxComp )
      {
         pWDA->pAckTxCbFunc = NULL;
         if( VOS_STATUS_SUCCESS !=
                           WDA_STOP_TIMER(&pWDA->wdaTimers.TxCompleteTimer))
         {
            VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                                "Tx Complete timeout Timer Stop Failed ");
         }
      }
      status = VOS_STATUS_E_FAILURE;
   }
#ifdef WLAN_DUMP_MGMTFRAMES
   if (VOS_IS_STATUS_SUCCESS(status))
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                      "%s() TX packet : SubType %d", __func__,pFc->subType);
      VOS_TRACE_HEX_DUMP( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                          pData, frmLen);
   }
#endif

   if (VOS_IS_STATUS_SUCCESS(status))
   {
      if (pMac->fEnableDebugLog & 0x1)
      {
         if ((pFc->type == SIR_MAC_MGMT_FRAME) &&
             (pFc->subType != SIR_MAC_MGMT_PROBE_REQ) &&
             (pFc->subType != SIR_MAC_MGMT_PROBE_RSP))
         {
            VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR, "TX MGMT - Type %hu, SubType %hu",
                       pFc->type, pFc->subType);
         }
      }
   }


   return status;
}
/*
                                    
                            
 */
static VOS_STATUS WDA_ProcessDHCPStartInd (tWDA_CbContext *pWDA,
                                           tAniDHCPInd *dhcpStartInd)
{
   WDI_Status status;
   WDI_DHCPInd wdiDHCPInd;

   wdiDHCPInd.device_mode = dhcpStartInd->device_mode;
   vos_mem_copy(wdiDHCPInd.macAddr, dhcpStartInd->macAddr,
                                               sizeof(tSirMacAddr));

   status = WDI_dhcpStartInd(&wdiDHCPInd);

   if (WDI_STATUS_PENDING == status)
   {
       VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                "Pending received for %s:%d ",__func__,__LINE__ );
   }
   else if (WDI_STATUS_SUCCESS_SYNC != status)
   {
       VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
              "Failure status: %d in %s:%d ", status, __func__, __LINE__);
   }

   vos_mem_free(dhcpStartInd);
   return CONVERT_WDI2VOS_STATUS(status) ;
}

 /*
                                    
                            
  */
 static VOS_STATUS WDA_ProcessDHCPStopInd (tWDA_CbContext *pWDA,
                                           tAniDHCPInd *dhcpStopInd)
 {
   WDI_Status status;
   WDI_DHCPInd wdiDHCPInd;

   wdiDHCPInd.device_mode = dhcpStopInd->device_mode;
   vos_mem_copy(wdiDHCPInd.macAddr, dhcpStopInd->macAddr, sizeof(tSirMacAddr));

   status = WDI_dhcpStopInd(&wdiDHCPInd);

   if (WDI_STATUS_PENDING == status)
   {
       VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                "Pending received for %s:%d ",__func__,__LINE__ );
   }
   else if (WDI_STATUS_SUCCESS_SYNC != status)
   {
       VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
              "Failure status: %d in %s:%d ", status, __func__, __LINE__);
   }

   vos_mem_free(dhcpStopInd);

   return CONVERT_WDI2VOS_STATUS(status) ;
 }

/*
                                          
  
                                                             
  
         
                               
                                           
 */
VOS_STATUS WDA_ProcessSetSpoofMacAddrReq(tWDA_CbContext *pWDA,
                               tpSpoofMacAddrReqParams pReq)
{
    WDI_Status wdiStatus;
    WDI_SpoofMacAddrInfoType *WDI_SpoofMacAddrInfoParams;
    tWDA_ReqParams *pWdaParams;

    WDI_SpoofMacAddrInfoParams = (WDI_SpoofMacAddrInfoType *)vos_mem_malloc(
        sizeof(WDI_SpoofMacAddrInfoType));
    if(NULL == WDI_SpoofMacAddrInfoParams) {
        VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                           "%s: VOS MEM Alloc Failure in Spoof Req", __func__);
        VOS_ASSERT(0);
        return VOS_STATUS_E_NOMEM;
    }
    pWdaParams = (tWDA_ReqParams *)vos_mem_malloc(sizeof(tWDA_ReqParams)) ;
    if(NULL == pWdaParams) {
        VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                           "%s: VOS MEM Alloc Failure", __func__);
        VOS_ASSERT(0);
        return VOS_STATUS_E_NOMEM;
    }

    vos_mem_copy(WDI_SpoofMacAddrInfoParams->macAddr,
        pReq->macAddr, VOS_MAC_ADDRESS_LEN);

    pWdaParams->pWdaContext = pWDA;
    /*                                                                  */
    pWdaParams->wdaMsgParam = (void *)pReq ;
    /*                             */
    pWdaParams->wdaWdiApiMsgParam = (void *)WDI_SpoofMacAddrInfoParams ;

    wdiStatus = WDI_SetSpoofMacAddrReq(WDI_SpoofMacAddrInfoParams,
        (WDI_SetSpoofMacAddrRspCb) WDA_SpoofMacAddrRspCallback,
        pWdaParams );

    if(IS_WDI_STATUS_FAILURE(wdiStatus))
    {
        VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
         "Failure in SetSpoofMacAddrReq Params WDI API, free all the memory " );
        vos_mem_free(pWdaParams->wdaWdiApiMsgParam) ;
        vos_mem_free(pWdaParams->wdaMsgParam);
        vos_mem_free(pWdaParams);
    }

    return CONVERT_WDI2VOS_STATUS(wdiStatus) ;
}

/*
                             
                                        
 */ 
VOS_STATUS WDA_McProcessMsg( v_CONTEXT_t pVosContext, vos_msg_t *pMsg )
{
   VOS_STATUS status = VOS_STATUS_SUCCESS;
   tWDA_CbContext *pWDA = NULL ; 
   if(NULL == pMsg) 
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                           "%s:pMsg is NULL", __func__); 
      VOS_ASSERT(0);
      return VOS_STATUS_E_FAILURE;
   }
   
   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO_LOW,
                    "=========> %s msgType: %x " ,__func__, pMsg->type);
   
   pWDA = (tWDA_CbContext *)vos_get_context( VOS_MODULE_ID_WDA, pVosContext );
   if(NULL == pWDA )
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                           "%s:pWDA is NULL", __func__); 
      VOS_ASSERT(0);
      vos_mem_free(pMsg->bodyptr);
      return VOS_STATUS_E_FAILURE;
   }
   /*                                */
   switch( pMsg->type )
   {
      case WNI_CFG_DNLD_REQ:
      {
         status = WDA_WniCfgDnld(pWDA);
         /*                                                    */
         if( VOS_IS_STATUS_SUCCESS(status) )
         {
            vos_WDAComplete_cback(pVosContext);
         }
         else
         {
            VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                                     "WDA Config Download failure" );
         }
         break ;
      }
      /* 
                                                               
                            
       */ 
      case WDA_INIT_SCAN_REQ:
      {
         WDA_ProcessInitScanReq(pWDA, (tInitScanParams *)pMsg->bodyptr) ;
         break ;    
      }
      /*                            */
      case WDA_START_SCAN_REQ:
      {
         WDA_ProcessStartScanReq(pWDA, (tStartScanParams *)pMsg->bodyptr) ;
         break ;    
      }
      /*                          */
      case WDA_END_SCAN_REQ:
      {
         WDA_ProcessEndScanReq(pWDA, (tEndScanParams *)pMsg->bodyptr) ;
         break ;
      }
      /*                          */
      case WDA_FINISH_SCAN_REQ:
      {
         WDA_ProcessFinishScanReq(pWDA, (tFinishScanParams *)pMsg->bodyptr) ;
         break ;    
      }
      /*                      */
      case WDA_CHNL_SWITCH_REQ:
      {
         if(WDA_PRE_ASSOC_STATE == pWDA->wdaState)
         {
            WDA_ProcessJoinReq(pWDA, (tSwitchChannelParams *)pMsg->bodyptr) ;
         }
         else
         {
            if (IS_FEATURE_SUPPORTED_BY_FW(CH_SWITCH_V1) &&
                 eHAL_CHANNEL_SWITCH_SOURCE_CSA ==
                ((tSwitchChannelParams*)pMsg->bodyptr)->channelSwitchSrc )
            {
                VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                             "call ProcessChannelSwitchReq_V1" );
                WDA_ProcessChannelSwitchReq_V1(pWDA,
                             (tSwitchChannelParams*)pMsg->bodyptr) ;
            }
            else
            {
                VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                                 "call ProcessChannelSwitchReq" );
               WDA_ProcessChannelSwitchReq(pWDA,
                             (tSwitchChannelParams*)pMsg->bodyptr) ;
            }
         }
         break ;
      }
      /*                         */
      case WDA_ADD_BSS_REQ:
      {
         WDA_ProcessConfigBssReq(pWDA, (tAddBssParams*)pMsg->bodyptr) ;
         break ;
      }
      case WDA_ADD_STA_REQ:
      {
         WDA_ProcessAddStaReq(pWDA, (tAddStaParams *)pMsg->bodyptr) ;
         break ;
      }
      case WDA_DELETE_BSS_REQ:
      {
         WDA_ProcessDelBssReq(pWDA, (tDeleteBssParams *)pMsg->bodyptr) ;
         break ;
      }
      case WDA_DELETE_STA_REQ:
      {
         WDA_ProcessDelStaReq(pWDA, (tDeleteStaParams *)pMsg->bodyptr) ;
         break ;
      }
      case WDA_CONFIG_PARAM_UPDATE_REQ:
      {
         WDA_UpdateCfg(pWDA, (tSirMsgQ *)pMsg) ;
         break ;
      }
      case WDA_SET_BSSKEY_REQ:
      {
         WDA_ProcessSetBssKeyReq(pWDA, (tSetBssKeyParams *)pMsg->bodyptr);
         break ;
      }
      case WDA_SET_STAKEY_REQ:
      {
         WDA_ProcessSetStaKeyReq(pWDA, (tSetStaKeyParams *)pMsg->bodyptr);
         break ;
      }
      case WDA_SET_STA_BCASTKEY_REQ:
      {
         WDA_ProcessSetBcastStaKeyReq(pWDA, (tSetStaKeyParams *)pMsg->bodyptr);
         break ;
      }
      case WDA_REMOVE_BSSKEY_REQ:
      {
         WDA_ProcessRemoveBssKeyReq(pWDA, 
                                    (tRemoveBssKeyParams *)pMsg->bodyptr);
         break ;
      }
      case WDA_REMOVE_STAKEY_REQ:
      {
         WDA_ProcessRemoveStaKeyReq(pWDA, 
                                    (tRemoveStaKeyParams *)pMsg->bodyptr);
         break ;
      }
      case WDA_REMOVE_STA_BCASTKEY_REQ:
      {
         /*                                                                    
                                                  */
         break;
      }
#ifdef FEATURE_WLAN_ESE
      case WDA_TSM_STATS_REQ:
      {
         WDA_ProcessTsmStatsReq(pWDA, (tpAniGetTsmStatsReq)pMsg->bodyptr);
         break;
      }
#endif
      case WDA_UPDATE_EDCA_PROFILE_IND:
      {
         WDA_ProcessUpdateEDCAParamReq(pWDA, (tEdcaParams *)pMsg->bodyptr);
         break;
      }
      case WDA_ADD_TS_REQ:
      {
         WDA_ProcessAddTSReq(pWDA, (tAddTsParams *)pMsg->bodyptr);
         break;
      }
      case WDA_DEL_TS_REQ:
      {
         WDA_ProcessDelTSReq(pWDA, (tDelTsParams *)pMsg->bodyptr);
         break;
      }
      case WDA_ADDBA_REQ:
      {
         WDA_ProcessAddBASessionReq(pWDA, (tAddBAParams *)pMsg->bodyptr);
         break;
      }
      case WDA_DELBA_IND:
      {
         WDA_ProcessDelBAReq(pWDA, (tDelBAParams *)pMsg->bodyptr);
         break;
      }
      case WDA_UPDATE_CHAN_LIST_REQ:
      {
         WDA_ProcessUpdateChannelList(pWDA,
                 (tSirUpdateChanList *)pMsg->bodyptr);
         break;
      }
      case WDA_SET_LINK_STATE:
      {
         WDA_ProcessSetLinkState(pWDA, (tLinkStateParams *)pMsg->bodyptr);
         break;
      }
      case WDA_GET_STATISTICS_REQ:
      {
         WDA_ProcessGetStatsReq(pWDA, (tAniGetPEStatsReq *)pMsg->bodyptr);
         break;
      }
#if defined WLAN_FEATURE_VOWIFI_11R || defined FEATURE_WLAN_ESE || defined(FEATURE_WLAN_LFR)
      case WDA_GET_ROAM_RSSI_REQ:
      {
         WDA_ProcessGetRoamRssiReq(pWDA, (tAniGetRssiReq *)pMsg->bodyptr);
         break;
      }
#endif
      case WDA_PWR_SAVE_CFG:
      {
         if(pWDA->wdaState == WDA_READY_STATE)
         {
            WDA_ProcessSetPwrSaveCfgReq(pWDA, (tSirPowerSaveCfg *)pMsg->bodyptr);
         }
         else
         {
            if(NULL != pMsg->bodyptr)
            {
               vos_mem_free(pMsg->bodyptr);
            }
            VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                       "WDA_PWR_SAVE_CFG req in wrong state %d", pWDA->wdaState );
         }
         break;
      }
      case WDA_ENTER_IMPS_REQ:
      {
         if(pWDA->wdaState == WDA_READY_STATE)
         {
            WDA_ProcessEnterImpsReq(pWDA);
         }
         else
         {
            VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                       "WDA_ENTER_IMPS_REQ req in wrong state %d", pWDA->wdaState );
         }
         break;
      }
      case WDA_EXIT_IMPS_REQ:
      {
         if(pWDA->wdaState == WDA_READY_STATE)
         {
            WDA_ProcessExitImpsReq(pWDA);
         }
         else
         {
            VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                       "WDA_EXIT_IMPS_REQ req in wrong state %d", pWDA->wdaState );
         }
         break;
      }
      case WDA_ENTER_BMPS_REQ:
      {
         if(pWDA->wdaState == WDA_READY_STATE)
         {
            WDA_ProcessEnterBmpsReq(pWDA, (tEnterBmpsParams *)pMsg->bodyptr);
         }
         else
         {
            VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                       "WDA_ENTER_BMPS_REQ req in wrong state %d", pWDA->wdaState );
         }
         break;
      }
      case WDA_EXIT_BMPS_REQ:
      {
         if(pWDA->wdaState == WDA_READY_STATE)
         {
            WDA_ProcessExitBmpsReq(pWDA, (tExitBmpsParams *)pMsg->bodyptr);
         }
         else
         {
         VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                       "WDA_EXIT_BMPS_REQ req in wrong state %d", pWDA->wdaState );
         }
         break;
      }
      case WDA_ENTER_UAPSD_REQ:
      {
         if(pWDA->wdaState == WDA_READY_STATE)
         {
            WDA_ProcessEnterUapsdReq(pWDA, (tUapsdParams *)pMsg->bodyptr);
         }
         else
         {
            VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                       "WDA_ENTER_UAPSD_REQ req in wrong state %d", pWDA->wdaState );
         }
         break;
      }
      case WDA_EXIT_UAPSD_REQ:
      {
         if(pWDA->wdaState == WDA_READY_STATE)
         {
            WDA_ProcessExitUapsdReq(pWDA, (tExitUapsdParams *)pMsg->bodyptr);
         }
         else
         {
            VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                       "WDA_EXIT_UAPSD_REQ req in wrong state %d", pWDA->wdaState );
         }
         break;
      }
      case WDA_UPDATE_UAPSD_IND:
      {
         if(pWDA->wdaState == WDA_READY_STATE)
         {
            WDA_UpdateUapsdParamsReq(pWDA, (tUpdateUapsdParams *)pMsg->bodyptr);
         }
         else
         {
            VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                       "WDA_UPDATE_UAPSD_IND req in wrong state %d", pWDA->wdaState );
         }
         break;
      }
      case WDA_REGISTER_PE_CALLBACK :
      {
         VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO_HIGH,
                           "Handling msg type WDA_REGISTER_PE_CALLBACK " );
         /*                            */
         /*                                              */
         if(NULL != pMsg->bodyptr)
         {
            vos_mem_free(pMsg->bodyptr);
         }
         break;
      }
      case WDA_SYS_READY_IND :
      {
         VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO_HIGH,
                                  "Handling msg type WDA_SYS_READY_IND " );
         pWDA->wdaState = WDA_READY_STATE;
         if(NULL != pMsg->bodyptr)
         {
            vos_mem_free(pMsg->bodyptr);
         }
         break;
      }
      case WDA_BEACON_FILTER_IND  :
      {
         WDA_SetBeaconFilterReq(pWDA, (tBeaconFilterMsg *)pMsg->bodyptr);
         break;
      }
      case WDA_BTC_SET_CFG:
      {
         /*                                         */
         VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO_HIGH,
                                  "Handling msg type WDA_BTC_SET_CFG  " );
         /*                                              */
         if(NULL != pMsg->bodyptr)
         {
            vos_mem_free(pMsg->bodyptr);
         }
         break;
      }
      case WDA_SIGNAL_BT_EVENT:
      {
         /*                                         */
         VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO_HIGH,
                                  "Handling msg type WDA_SIGNAL_BT_EVENT  " );
         /*                                              */
         if(NULL != pMsg->bodyptr)
         {
            vos_mem_free(pMsg->bodyptr);
         }
         break;
      }
      case WDA_CFG_RXP_FILTER_REQ:
      {
         WDA_ProcessConfigureRxpFilterReq(pWDA, 
                             (tSirWlanSetRxpFilters *)pMsg->bodyptr);
         break;
      }
      case WDA_SET_HOST_OFFLOAD:
      {
         WDA_ProcessHostOffloadReq(pWDA, (tSirHostOffloadReq *)pMsg->bodyptr);
         break;
      }
      case WDA_SET_KEEP_ALIVE:
      {
         WDA_ProcessKeepAliveReq(pWDA, (tSirKeepAliveReq *)pMsg->bodyptr);
         break;
      }
#ifdef WLAN_NS_OFFLOAD
      case WDA_SET_NS_OFFLOAD:
      {
         WDA_ProcessHostOffloadReq(pWDA, (tSirHostOffloadReq *)pMsg->bodyptr);
         break;
      }
#endif //               
      case WDA_ADD_STA_SELF_REQ:
      {
         WDA_ProcessAddStaSelfReq(pWDA, (tAddStaSelfParams *)pMsg->bodyptr);
         break;
      }
      case WDA_DEL_STA_SELF_REQ:
      {
         WDA_ProcessDelSTASelfReq(pWDA, (tDelStaSelfParams *)pMsg->bodyptr);
         break;
      }
      case WDA_WOWL_ADD_BCAST_PTRN:
      {
         WDA_ProcessWowlAddBcPtrnReq(pWDA, (tSirWowlAddBcastPtrn *)pMsg->bodyptr);
         break;
      }
      case WDA_WOWL_DEL_BCAST_PTRN:
      {
         WDA_ProcessWowlDelBcPtrnReq(pWDA, (tSirWowlDelBcastPtrn *)pMsg->bodyptr);
         break;
      }
      case WDA_WOWL_ENTER_REQ:
      {
         WDA_ProcessWowlEnterReq(pWDA, (tSirHalWowlEnterParams *)pMsg->bodyptr);
         break;
      }
      case WDA_WOWL_EXIT_REQ:
      {
         WDA_ProcessWowlExitReq(pWDA, (tSirHalWowlExitParams *)pMsg->bodyptr);
         break;
      }
      case WDA_TL_FLUSH_AC_REQ:
      {
         WDA_ProcessFlushAcReq(pWDA, (tFlushACReq *)pMsg->bodyptr);
         break;
      }
      case WDA_SIGNAL_BTAMP_EVENT:
      {
         WDA_ProcessBtAmpEventReq(pWDA, (tSmeBtAmpEvent *)pMsg->bodyptr);
         break;
      }
#ifdef WLAN_FEATURE_LINK_LAYER_STATS
      case WDA_LINK_LAYER_STATS_SET_REQ:
      {
         WDA_ProcessLLStatsSetReq(pWDA, (tSirLLStatsSetReq *)pMsg->bodyptr);
         break;
      }
      case WDA_LINK_LAYER_STATS_GET_REQ:
      {
         WDA_ProcessLLStatsGetReq(pWDA, (tSirLLStatsGetReq *)pMsg->bodyptr);
         break;
      }
      case WDA_LINK_LAYER_STATS_CLEAR_REQ:
      {
         WDA_ProcessLLStatsClearReq(pWDA, (tSirLLStatsClearReq *)pMsg->bodyptr);
         break;
      }
#endif /*                               */
#ifdef WLAN_FEATURE_EXTSCAN
      case WDA_EXTSCAN_GET_CAPABILITIES_REQ:
      {
         WDA_ProcessEXTScanGetCapabilitiesReq(pWDA,
                 (tSirGetEXTScanCapabilitiesReqParams *)pMsg->bodyptr);
         break;
      }
      case WDA_EXTSCAN_START_REQ:
      {
         WDA_ProcessEXTScanStartReq(pWDA,
                 (tSirEXTScanStartReqParams *)pMsg->bodyptr);
         break;
      }
      case WDA_EXTSCAN_STOP_REQ:
      {
         WDA_ProcessEXTScanStopReq(pWDA,
                 (tSirEXTScanStopReqParams *)pMsg->bodyptr);
         break;
      }
      case WDA_EXTSCAN_GET_CACHED_RESULTS_REQ:
      {
         WDA_ProcessEXTScanGetCachedResultsReq(pWDA,
                         (tSirEXTScanGetCachedResultsReqParams *)pMsg->bodyptr);
         break;
      }
      case WDA_EXTSCAN_SET_BSSID_HOTLIST_REQ:
      {
         WDA_ProcessEXTScanSetBSSIDHotlistReq(pWDA,
                         (tSirEXTScanSetBssidHotListReqParams *)pMsg->bodyptr);
         break;
      }
      case WDA_EXTSCAN_RESET_BSSID_HOTLIST_REQ:
      {
         WDA_ProcessEXTScanResetBSSIDHotlistReq(pWDA,
                        (tSirEXTScanResetBssidHotlistReqParams *)pMsg->bodyptr);
         break;
      }
      case WDA_EXTSCAN_SET_SIGNF_RSSI_CHANGE_REQ:
      {
         WDA_ProcessEXTScanSetSignfRSSIChangeReq(pWDA,
                     (tSirEXTScanSetSignificantChangeReqParams *)pMsg->bodyptr);
         break;
      }
      case WDA_EXTSCAN_RESET_SIGNF_RSSI_CHANGE_REQ:
      {
         WDA_ProcessEXTScanResetSignfRSSIChangeReq(pWDA,
                   (tSirEXTScanResetSignificantChangeReqParams *)pMsg->bodyptr);
         break;
      }
#endif /*                      */
#ifdef WDA_UT
      case WDA_WDI_EVENT_MSG:
      {
         WDI_processEvent(pMsg->bodyptr,(void *)pMsg->bodyval);
         break ;
      }
#endif
      case WDA_UPDATE_BEACON_IND:
      {
          WDA_ProcessUpdateBeaconParams(pWDA, 
                                    (tUpdateBeaconParams *)pMsg->bodyptr);
          break;
      }
      case WDA_SEND_BEACON_REQ:
      {
          WDA_ProcessSendBeacon(pWDA, (tSendbeaconParams *)pMsg->bodyptr);
          break;
      }
      case WDA_UPDATE_PROBE_RSP_TEMPLATE_IND:
      {
          WDA_ProcessUpdateProbeRspTemplate(pWDA, 
                                      (tSendProbeRespParams *)pMsg->bodyptr);
          break;
      }
#if defined(WLAN_FEATURE_VOWIFI) || defined(FEATURE_WLAN_ESE)
      case WDA_SET_MAX_TX_POWER_REQ:
      {
         WDA_ProcessSetMaxTxPowerReq(pWDA,
                                       (tMaxTxPowerParams *)pMsg->bodyptr);
         break;
      }
#endif
      case WDA_SET_MAX_TX_POWER_PER_BAND_REQ:
      {
         WDA_ProcessSetMaxTxPowerPerBandReq(pWDA, (tMaxTxPowerPerBandParams *)
                                            pMsg->bodyptr);
         break;
      }
      case WDA_SET_TX_POWER_REQ:
      {
         WDA_ProcessSetTxPowerReq(pWDA,
                                       (tSirSetTxPowerReq *)pMsg->bodyptr);
         break;
      }
      case WDA_SET_P2P_GO_NOA_REQ:
      {
         WDA_ProcessSetP2PGONOAReq(pWDA,
                                    (tP2pPsParams *)pMsg->bodyptr);
         break;
      }
      /*                        */
      case WDA_TIMER_BA_ACTIVITY_REQ:
      {
         WDA_BaCheckActivity(pWDA) ;
         break ;
      }

      /*                        */
      case WDA_TIMER_TRAFFIC_STATS_IND:
      {
         WDA_TimerTrafficStatsInd(pWDA);
         break;
      }
#ifdef WLAN_FEATURE_VOWIFI_11R
      case WDA_AGGR_QOS_REQ:
      {
         WDA_ProcessAggrAddTSReq(pWDA, (tAggrAddTsParams *)pMsg->bodyptr);
         break;
      }
#endif /*                         */
      case WDA_FTM_CMD_REQ:
      {
         WDA_ProcessFTMCommand(pWDA, (tPttMsgbuffer *)pMsg->bodyptr) ;
         break ;
      }
#ifdef FEATURE_OEM_DATA_SUPPORT
      case WDA_START_OEM_DATA_REQ:
      {
         WDA_ProcessStartOemDataReq(pWDA, (tStartOemDataReq *)pMsg->bodyptr) ;
         break;
      }
#endif /*                          */
      /*                                 */
      case WDA_TX_COMPLETE_TIMEOUT_IND:
      {
         WDA_ProcessTxCompleteTimeOutInd(pWDA); 
         break;
      }         
      case WDA_WLAN_SUSPEND_IND:
      {
         WDA_ProcessWlanSuspendInd(pWDA, 
                        (tSirWlanSuspendParam *)pMsg->bodyptr) ;
         break;
      }
      case WDA_WLAN_RESUME_REQ:
      {
         WDA_ProcessWlanResumeReq(pWDA, 
                        (tSirWlanResumeParam *)pMsg->bodyptr) ;
         break;
      }
      
      case WDA_UPDATE_CF_IND:
      {
         vos_mem_free((v_VOID_t*)pMsg->bodyptr);
         pMsg->bodyptr = NULL;
         break;
      }
#ifdef FEATURE_WLAN_SCAN_PNO
      case WDA_SET_PNO_REQ:
      {
         WDA_ProcessSetPrefNetworkReq(pWDA, (tSirPNOScanReq *)pMsg->bodyptr);
         break;
      }
      case WDA_UPDATE_SCAN_PARAMS_REQ:
      {
         WDA_ProcessUpdateScanParams(pWDA, (tSirUpdateScanParams *)pMsg->bodyptr);
         break;
      }
      case WDA_SET_RSSI_FILTER_REQ:
      {
         WDA_ProcessSetRssiFilterReq(pWDA, (tSirSetRSSIFilterReq *)pMsg->bodyptr);
         break;
      }
#endif //                      
#ifdef WLAN_FEATURE_ROAM_SCAN_OFFLOAD
      case WDA_ROAM_SCAN_OFFLOAD_REQ:
      {
         WDA_ProcessRoamScanOffloadReq(pWDA, (tSirRoamOffloadScanReq *)pMsg->bodyptr);
         break;
      }
#endif
      case WDA_SET_TX_PER_TRACKING_REQ:
      {
         WDA_ProcessSetTxPerTrackingReq(pWDA, (tSirTxPerTrackingParam *)pMsg->bodyptr);
         break;
      }
      
#ifdef WLAN_FEATURE_PACKET_FILTERING
      case WDA_8023_MULTICAST_LIST_REQ:
      {
         WDA_Process8023MulticastListReq(pWDA, (tSirRcvFltMcAddrList *)pMsg->bodyptr);
         break;
      }
      case WDA_RECEIVE_FILTER_SET_FILTER_REQ:
      {
         WDA_ProcessReceiveFilterSetFilterReq(pWDA, (tSirRcvPktFilterCfgType *)pMsg->bodyptr);
         break;
      }
      case WDA_PACKET_COALESCING_FILTER_MATCH_COUNT_REQ:
      {
         WDA_ProcessPacketFilterMatchCountReq(pWDA, (tpSirRcvFltPktMatchRsp)pMsg->bodyptr);
         break;
      }
      case WDA_RECEIVE_FILTER_CLEAR_FILTER_REQ:
      {
         WDA_ProcessReceiveFilterClearFilterReq(pWDA, (tSirRcvFltPktClearParam *)pMsg->bodyptr);
         break;
      }
#endif //                              
  
  
      case WDA_TRANSMISSION_CONTROL_IND:
      {
         WDA_ProcessTxControlInd(pWDA, (tpTxControlParams)pMsg->bodyptr);
         break;
      }
      case WDA_SET_POWER_PARAMS_REQ:
      {
         WDA_ProcessSetPowerParamsReq(pWDA, (tSirSetPowerParamsReq *)pMsg->bodyptr);
         break;
      }
#ifdef WLAN_FEATURE_GTK_OFFLOAD
      case WDA_GTK_OFFLOAD_REQ:
      {
         WDA_ProcessGTKOffloadReq(pWDA, (tpSirGtkOffloadParams)pMsg->bodyptr);
         break;
      }

      case WDA_GTK_OFFLOAD_GETINFO_REQ:
      {
         WDA_ProcessGTKOffloadGetInfoReq(pWDA, (tpSirGtkOffloadGetInfoRspParams)pMsg->bodyptr);
         break;
      }
#endif //                        

      case WDA_SET_TM_LEVEL_REQ:
      {
         WDA_ProcessSetTmLevelReq(pWDA, (tAniSetTmLevelReq *)pMsg->bodyptr);
         break;
      }

      case WDA_UPDATE_OP_MODE:
      {
           if(WDA_getHostWlanFeatCaps(HT40_OBSS_SCAN) &&
              WDA_getFwWlanFeatCaps(HT40_OBSS_SCAN))
          {
              WDA_ProcessUpdateOpMode(pWDA, (tUpdateVHTOpMode *)pMsg->bodyptr);
          }
          else if(WDA_getHostWlanFeatCaps(DOT11AC) && WDA_getFwWlanFeatCaps(DOT11AC))
          {
              if(WDA_getHostWlanFeatCaps(DOT11AC_OPMODE) && WDA_getFwWlanFeatCaps(DOT11AC_OPMODE))
                   WDA_ProcessUpdateOpMode(pWDA, (tUpdateVHTOpMode *)pMsg->bodyptr);
              else
                   VOS_TRACE(VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                                            " VHT OpMode Feature is Not Supported");
          } 
          else 
                   VOS_TRACE(VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                                            " 11AC Feature is Not Supported");
          break;
      }
#ifdef WLAN_FEATURE_11W
      case WDA_EXCLUDE_UNENCRYPTED_IND:
      {
         WDA_ProcessExcludeUnecryptInd(pWDA, (tSirWlanExcludeUnencryptParam *)pMsg->bodyptr);
         break;
      }
#endif
#ifdef FEATURE_WLAN_TDLS
      case WDA_SET_TDLS_LINK_ESTABLISH_REQ:
      {
          WDA_ProcessSetTdlsLinkEstablishReq(pWDA, (tTdlsLinkEstablishParams *)pMsg->bodyptr);
          break;
      }
#endif
      case WDA_DHCP_START_IND:
      {
          WDA_ProcessDHCPStartInd(pWDA, (tAniDHCPInd *)pMsg->bodyptr);
          break;
      }
      case WDA_DHCP_STOP_IND:
      {
          WDA_ProcessDHCPStopInd(pWDA, (tAniDHCPInd *)pMsg->bodyptr);
          break;
      }
#ifdef FEATURE_WLAN_LPHB
      case WDA_LPHB_CONF_REQ:
      {
         WDA_ProcessLPHBConfReq(pWDA, (tSirLPHBReq *)pMsg->bodyptr);
         break;
      }
#endif
      case WDA_ADD_PERIODIC_TX_PTRN_IND:
      {
         WDA_ProcessAddPeriodicTxPtrnInd(pWDA,
            (tSirAddPeriodicTxPtrn *)pMsg->bodyptr);
         break;
      }
      case WDA_DEL_PERIODIC_TX_PTRN_IND:
      {
         WDA_ProcessDelPeriodicTxPtrnInd(pWDA,
            (tSirDelPeriodicTxPtrn *)pMsg->bodyptr);
         break;
      }

#ifdef FEATURE_WLAN_BATCH_SCAN
      case WDA_SET_BATCH_SCAN_REQ:
      {
          WDA_ProcessSetBatchScanReq(pWDA,
            (tSirSetBatchScanReq *)pMsg->bodyptr);
          break;
      }
      case WDA_RATE_UPDATE_IND:
      {
          WDA_ProcessRateUpdateInd(pWDA, (tSirRateUpdateInd *)pMsg->bodyptr);
          break;
      }
      case WDA_TRIGGER_BATCH_SCAN_RESULT_IND:
      {
          WDA_ProcessTriggerBatchScanResultInd(pWDA,
            (tSirTriggerBatchScanResultInd *)pMsg->bodyptr);
          break;
      }
      case WDA_STOP_BATCH_SCAN_IND:
      {
          WDA_ProcessStopBatchScanInd(pWDA,
            (tSirStopBatchScanInd *)pMsg->bodyptr);
          break;
      }
      case WDA_GET_BCN_MISS_RATE_REQ:
          WDA_ProcessGetBcnMissRateReq(pWDA,
                                      (tSirBcnMissRateReq *)pMsg->bodyptr);
          break;
#endif

      case WDA_HT40_OBSS_SCAN_IND:
      {
          WDA_ProcessHT40OBSSScanInd(pWDA,
            (tSirHT40OBSSScanInd *)pMsg->bodyptr);
          break;
      }
      case WDA_HT40_OBSS_STOP_SCAN_IND:
      {
          WDA_ProcessHT40OBSSStopScanInd(pWDA,
            (tANI_U8*)pMsg->bodyptr);
          break;
      }
//            
#ifdef FEATURE_WLAN_TDLS
      case WDA_SET_TDLS_CHAN_SWITCH_REQ:
      {
          WDA_ProcessSetTdlsChanSwitchReq(pWDA, (tTdlsChanSwitchParams *)pMsg->bodyptr);
          break;
      }
#endif
      case WDA_SPOOF_MAC_ADDR_REQ:
      {
          WDA_ProcessSetSpoofMacAddrReq(pWDA, (tpSpoofMacAddrReqParams)pMsg->bodyptr);
          break;
      }
      default:
      {
         VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                                  "No Handling for msg type %x in WDA " 
                                  ,pMsg->type);
         /*                                              */
         if(NULL != pMsg->bodyptr)
         {
            vos_mem_free(pMsg->bodyptr);
         }
         //                   
      }
   }
   return status ;
}

/*
                                    
                                            
 */ 
void WDA_lowLevelIndCallback(WDI_LowLevelIndType *wdiLowLevelInd, 
                                                         void* pUserData )
{
   tWDA_CbContext *pWDA = (tWDA_CbContext *)pUserData;
#if defined WLAN_FEATURE_NEIGHBOR_ROAMING
   tSirRSSINotification rssiNotification;
#endif
   if(NULL == pWDA)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                           "%s:pWDA is NULL", __func__); 
      VOS_ASSERT(0);
      return ;
   }
   
   switch(wdiLowLevelInd->wdiIndicationType)
   {
      case WDI_RSSI_NOTIFICATION_IND:
      {
         VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                     "Received WDI_HAL_RSSI_NOTIFICATION_IND from WDI ");
#if defined WLAN_FEATURE_NEIGHBOR_ROAMING
         rssiNotification.bReserved = 
            wdiLowLevelInd->wdiIndicationData.wdiLowRSSIInfo.bReserved;
         rssiNotification.bRssiThres1NegCross = 
            wdiLowLevelInd->wdiIndicationData.wdiLowRSSIInfo.bRssiThres1NegCross;
         rssiNotification.bRssiThres1PosCross = 
            wdiLowLevelInd->wdiIndicationData.wdiLowRSSIInfo.bRssiThres1PosCross;
         rssiNotification.bRssiThres2NegCross = 
            wdiLowLevelInd->wdiIndicationData.wdiLowRSSIInfo.bRssiThres2NegCross;
         rssiNotification.bRssiThres2PosCross = 
            wdiLowLevelInd->wdiIndicationData.wdiLowRSSIInfo.bRssiThres2PosCross;
         rssiNotification.bRssiThres3NegCross = 
            wdiLowLevelInd->wdiIndicationData.wdiLowRSSIInfo.bRssiThres3NegCross;
         rssiNotification.bRssiThres3PosCross = 
            wdiLowLevelInd->wdiIndicationData.wdiLowRSSIInfo.bRssiThres3PosCross;
         rssiNotification.avgRssi = (v_S7_t) 
            ((-1)*wdiLowLevelInd->wdiIndicationData.wdiLowRSSIInfo.avgRssi);
         WLANTL_BMPSRSSIRegionChangedNotification(
            pWDA->pVosContext,
            &rssiNotification);
#endif
         break ;
      }
      case WDI_MISSED_BEACON_IND:
      {
         tpSirSmeMissedBeaconInd pMissBeacInd =
            (tpSirSmeMissedBeaconInd)vos_mem_malloc(sizeof(tSirSmeMissedBeaconInd));
         VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                     "Received WDI_MISSED_BEACON_IND from WDI ");
         /*                */
         if(NULL == pMissBeacInd)
         {
             VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                                 "%s: VOS MEM Alloc Failure", __func__);
             break;
         }
         pMissBeacInd->messageType = WDA_MISSED_BEACON_IND;
         pMissBeacInd->length = sizeof(tSirSmeMissedBeaconInd);
         pMissBeacInd->bssIdx =
             wdiLowLevelInd->wdiIndicationData.wdiMissedBeaconInd.bssIdx;
         WDA_SendMsg(pWDA, WDA_MISSED_BEACON_IND, (void *)pMissBeacInd , 0) ;
         break ;
      }
      case WDI_UNKNOWN_ADDR2_FRAME_RX_IND:
      {
         /*                                     */
         VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                     "Received WDI_UNKNOWN_ADDR2_FRAME_RX_IND from WDI ");
         break ;
      }
       
      case WDI_MIC_FAILURE_IND:
      {
         tpSirSmeMicFailureInd pMicInd =
           (tpSirSmeMicFailureInd)vos_mem_malloc(sizeof(tSirSmeMicFailureInd));

         if(NULL == pMicInd)
         {
            VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                                  "%s: VOS MEM Alloc Failure", __func__);
            break;
         }
         VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                                  "Received WDI_MIC_FAILURE_IND from WDI ");
         pMicInd->messageType = eWNI_SME_MIC_FAILURE_IND;
         pMicInd->length = sizeof(tSirSmeMicFailureInd);
         vos_mem_copy(pMicInd->bssId,
             wdiLowLevelInd->wdiIndicationData.wdiMICFailureInfo.bssId,
             sizeof(tSirMacAddr));
         vos_mem_copy(pMicInd->info.srcMacAddr,
             wdiLowLevelInd->wdiIndicationData.wdiMICFailureInfo.macSrcAddr,
             sizeof(tSirMacAddr));
         vos_mem_copy(pMicInd->info.taMacAddr,
             wdiLowLevelInd->wdiIndicationData.wdiMICFailureInfo.macTaAddr,
             sizeof(tSirMacAddr));
         vos_mem_copy(pMicInd->info.dstMacAddr,
             wdiLowLevelInd->wdiIndicationData.wdiMICFailureInfo.macDstAddr,
             sizeof(tSirMacAddr));
         vos_mem_copy(pMicInd->info.rxMacAddr,
             wdiLowLevelInd->wdiIndicationData.wdiMICFailureInfo.macRxAddr,
             sizeof(tSirMacAddr));
         pMicInd->info.multicast = 
             wdiLowLevelInd->wdiIndicationData.wdiMICFailureInfo.ucMulticast;
         pMicInd->info.keyId= 
             wdiLowLevelInd->wdiIndicationData.wdiMICFailureInfo.keyId;
         pMicInd->info.IV1= 
             wdiLowLevelInd->wdiIndicationData.wdiMICFailureInfo.ucIV1;
         vos_mem_copy(pMicInd->info.TSC,
             wdiLowLevelInd->wdiIndicationData.wdiMICFailureInfo.TSC,SIR_CIPHER_SEQ_CTR_SIZE);
         WDA_SendMsg(pWDA, SIR_HAL_MIC_FAILURE_IND, 
                                       (void *)pMicInd , 0) ;
         break ;
      }
      case WDI_FATAL_ERROR_IND:
      {
         pWDA->wdiFailed = true;
         /*                                     */
         VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                                  "Received WDI_FATAL_ERROR_IND from WDI ");
         break ;
      }
      case WDI_DEL_STA_IND:
      {
         tpDeleteStaContext  pDelSTACtx = 
            (tpDeleteStaContext)vos_mem_malloc(sizeof(tDeleteStaContext));
         
         VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                                  "Received WDI_DEL_STA_IND from WDI ");
         if(NULL == pDelSTACtx)
         {
            VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                                  "%s: VOS MEM Alloc Failure", __func__);
            break;
         }
         vos_mem_copy(pDelSTACtx->addr2,
             wdiLowLevelInd->wdiIndicationData.wdiDeleteSTAIndType.macADDR2,
             sizeof(tSirMacAddr));
         vos_mem_copy(pDelSTACtx->bssId,
             wdiLowLevelInd->wdiIndicationData.wdiDeleteSTAIndType.macBSSID,
             sizeof(tSirMacAddr));
         pDelSTACtx->assocId    = 
          wdiLowLevelInd->wdiIndicationData.wdiDeleteSTAIndType.usAssocId;
         pDelSTACtx->reasonCode = 
          wdiLowLevelInd->wdiIndicationData.wdiDeleteSTAIndType.wptReasonCode;
         pDelSTACtx->staId      = 
          wdiLowLevelInd->wdiIndicationData.wdiDeleteSTAIndType.ucSTAIdx;
         WDA_SendMsg(pWDA, SIR_LIM_DELETE_STA_CONTEXT_IND, 
                                       (void *)pDelSTACtx , 0) ;
         break ;
      }
      case WDI_COEX_IND:
      {
         tANI_U32 index;
         vos_msg_t vosMsg;
         tSirSmeCoexInd *pSmeCoexInd;

         if (SIR_COEX_IND_TYPE_CXM_FEATURES_NOTIFICATION ==
                wdiLowLevelInd->wdiIndicationData.wdiCoexInfo.coexIndType)
         {
            if(wdiLowLevelInd->wdiIndicationData.wdiCoexInfo.coexIndData)
            {
                VOS_TRACE(VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                  FL("Coex state: 0x%x coex feature: 0x%x"),
                  wdiLowLevelInd->wdiIndicationData.wdiCoexInfo.coexIndData[0],
                  wdiLowLevelInd->wdiIndicationData.wdiCoexInfo.coexIndData[1]);

                 if (wdiLowLevelInd->wdiIndicationData.wdiCoexInfo.coexIndData[2] << 16)
                 {
                     VOS_TRACE(VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR, FL("power limit: 0x%x"),
                     (tANI_U16)(wdiLowLevelInd->wdiIndicationData.wdiCoexInfo.coexIndData[2]));
                 }
            }
            break;
         }

         pSmeCoexInd = (tSirSmeCoexInd *)vos_mem_malloc(sizeof(tSirSmeCoexInd));
         if(NULL == pSmeCoexInd)
         {
            VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                             "%s: VOS MEM Alloc Failure-pSmeCoexInd", __func__);
            break;
         }
         VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                                  "Received WDI_COEX_IND from WDI ");
         /*                */
         pSmeCoexInd->mesgType = eWNI_SME_COEX_IND;
         pSmeCoexInd->mesgLen = sizeof(tSirSmeCoexInd);
         /*                          */
         pSmeCoexInd->coexIndType = wdiLowLevelInd->wdiIndicationData.wdiCoexInfo.coexIndType; 
         for (index = 0; index < SIR_COEX_IND_DATA_SIZE; index++)
         {
            pSmeCoexInd->coexIndData[index] = wdiLowLevelInd->wdiIndicationData.wdiCoexInfo.coexIndData[index]; 
         }
         /*                     */
         vosMsg.type = eWNI_SME_COEX_IND;
         vosMsg.bodyptr = (void *)pSmeCoexInd;
         vosMsg.bodyval = 0;
         /*                     */
         if (VOS_STATUS_SUCCESS != vos_mq_post_message(VOS_MQ_ID_SME, (vos_msg_t*)&vosMsg))
         {
            /*                         */
            vos_mem_free((v_VOID_t *)pSmeCoexInd);
         }
         else
         {
            /*       */
            VOS_TRACE(VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                      "[COEX WDA] Coex Ind Type (%x) data (%x %x %x %x)",
                      pSmeCoexInd->coexIndType, 
                      pSmeCoexInd->coexIndData[0], 
                      pSmeCoexInd->coexIndData[1], 
                      pSmeCoexInd->coexIndData[2], 
                      pSmeCoexInd->coexIndData[3]); 
         }
         break;
      }
      case WDI_TX_COMPLETE_IND:
      {
         tpAniSirGlobal pMac = (tpAniSirGlobal )VOS_GET_MAC_CTXT(pWDA->pVosContext) ;
         /*                                                  */
         VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                        "Complete Indication received from HAL");
         if( pWDA->pAckTxCbFunc )
         {
            if( VOS_STATUS_SUCCESS !=
                              WDA_STOP_TIMER(&pWDA->wdaTimers.TxCompleteTimer))
            {  
               VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                                  "Tx Complete timeout Timer Stop Failed ");
            }  
            pWDA->pAckTxCbFunc( pMac, wdiLowLevelInd->wdiIndicationData.tx_complete_status);
            pWDA->pAckTxCbFunc = NULL;
         }
         else
         {
             VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                          "Tx Complete Indication is received after timeout ");
         }
         break;
      }
      case WDI_P2P_NOA_START_IND :
      {
          tSirP2PNoaStart   *pP2pNoaStart = 
             (tSirP2PNoaStart *)vos_mem_malloc(sizeof(tSirP2PNoaStart));

          if (NULL == pP2pNoaStart)
          {
             VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                        "Memory allocation failure, "
                        "WDI_P2P_NOA_START_IND not forwarded");
             break;
          }
          pP2pNoaStart->status            = 
                     wdiLowLevelInd->wdiIndicationData.wdiP2pNoaStartInfo.status;
          pP2pNoaStart->bssIdx        = 
                     wdiLowLevelInd->wdiIndicationData.wdiP2pNoaStartInfo.bssIdx;
          WDA_SendMsg(pWDA, SIR_HAL_P2P_NOA_START_IND, 
                                        (void *)pP2pNoaStart , 0) ;
          break;
      }

#ifdef FEATURE_WLAN_TDLS
      case WDI_TDLS_IND :
      {
          tSirTdlsInd  *pTdlsInd =
             (tSirTdlsInd *)vos_mem_malloc(sizeof(tSirTdlsInd));

          if (NULL == pTdlsInd)
          {
             VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                        "Memory allocation failure, "
                        "WDI_TDLS_IND not forwarded");
             break;
          }
          pTdlsInd->status            =
                     wdiLowLevelInd->wdiIndicationData.wdiTdlsIndInfo.status;
          pTdlsInd->assocId        =
                     wdiLowLevelInd->wdiIndicationData.wdiTdlsIndInfo.assocId;
          pTdlsInd->staIdx =
                     wdiLowLevelInd->wdiIndicationData.wdiTdlsIndInfo.staIdx;
          pTdlsInd->reasonCode    =
                     wdiLowLevelInd->wdiIndicationData.wdiTdlsIndInfo.reasonCode;
          WDA_SendMsg(pWDA, SIR_HAL_TDLS_IND,
                                        (void *)pTdlsInd , 0) ;
          break;
      }
#endif
      case WDI_P2P_NOA_ATTR_IND :
      {
         tSirP2PNoaAttr   *pP2pNoaAttr = 
            (tSirP2PNoaAttr *)vos_mem_malloc(sizeof(tSirP2PNoaAttr));
         VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                              "Received WDI_P2P_NOA_ATTR_IND from WDI");
         if (NULL == pP2pNoaAttr)
         {
            VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                       "Memory allocation failure, "
                       "WDI_P2P_NOA_ATTR_IND not forwarded");
            break;
         }
         pP2pNoaAttr->index            = 
                    wdiLowLevelInd->wdiIndicationData.wdiP2pNoaAttrInfo.ucIndex;
         pP2pNoaAttr->oppPsFlag        = 
                    wdiLowLevelInd->wdiIndicationData.wdiP2pNoaAttrInfo.ucOppPsFlag;
         pP2pNoaAttr->ctWin            = 
                    wdiLowLevelInd->wdiIndicationData.wdiP2pNoaAttrInfo.usCtWin;
         
         pP2pNoaAttr->uNoa1IntervalCnt = 
                    wdiLowLevelInd->wdiIndicationData.wdiP2pNoaAttrInfo.usNoa1IntervalCnt;
         pP2pNoaAttr->uNoa1Duration    = 
                    wdiLowLevelInd->wdiIndicationData.wdiP2pNoaAttrInfo.uslNoa1Duration;
         pP2pNoaAttr->uNoa1Interval    = 
                    wdiLowLevelInd->wdiIndicationData.wdiP2pNoaAttrInfo.uslNoa1Interval;
         pP2pNoaAttr->uNoa1StartTime   = 
                    wdiLowLevelInd->wdiIndicationData.wdiP2pNoaAttrInfo.uslNoa1StartTime;
         pP2pNoaAttr->uNoa2IntervalCnt = 
                    wdiLowLevelInd->wdiIndicationData.wdiP2pNoaAttrInfo.usNoa2IntervalCnt;
         pP2pNoaAttr->uNoa2Duration    = 
                    wdiLowLevelInd->wdiIndicationData.wdiP2pNoaAttrInfo.uslNoa2Duration;
         pP2pNoaAttr->uNoa2Interval    = 
                    wdiLowLevelInd->wdiIndicationData.wdiP2pNoaAttrInfo.uslNoa2Interval;
         pP2pNoaAttr->uNoa2StartTime   = 
                    wdiLowLevelInd->wdiIndicationData.wdiP2pNoaAttrInfo.uslNoa2StartTime;
         WDA_SendMsg(pWDA, SIR_HAL_P2P_NOA_ATTR_IND, 
                                       (void *)pP2pNoaAttr , 0) ;
         break;
      }
#ifdef FEATURE_WLAN_SCAN_PNO
      case WDI_PREF_NETWORK_FOUND_IND:
      {
         vos_msg_t vosMsg;
         v_U32_t size = sizeof(tSirPrefNetworkFoundInd) +
             wdiLowLevelInd->wdiIndicationData.wdiPrefNetworkFoundInd.frameLength;
         tSirPrefNetworkFoundInd *pPrefNetworkFoundInd =
             (tSirPrefNetworkFoundInd *)vos_mem_malloc(size);

         VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                              "Received WDI_PREF_NETWORK_FOUND_IND from WDI");
         if (NULL == pPrefNetworkFoundInd)
         {
            VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                       "Memory allocation failure, "
                       "WDI_PREF_NETWORK_FOUND_IND not forwarded");
            if (NULL !=
                wdiLowLevelInd->wdiIndicationData.wdiPrefNetworkFoundInd.pData)
            {
                wpalMemoryFree(
                 wdiLowLevelInd->wdiIndicationData.wdiPrefNetworkFoundInd.pData
                );
                wdiLowLevelInd->wdiIndicationData.wdiPrefNetworkFoundInd.pData = NULL;
            }
            break;
         }
         /*                */
         pPrefNetworkFoundInd->mesgType = eWNI_SME_PREF_NETWORK_FOUND_IND;
         pPrefNetworkFoundInd->mesgLen = size;

         /*                          */ 
         pPrefNetworkFoundInd->ssId.length = 
            wdiLowLevelInd->wdiIndicationData.wdiPrefNetworkFoundInd.ssId.ucLength;
         vos_mem_set( pPrefNetworkFoundInd->ssId.ssId, 32, 0);
         vos_mem_copy( pPrefNetworkFoundInd->ssId.ssId, 
                  wdiLowLevelInd->wdiIndicationData.wdiPrefNetworkFoundInd.ssId.sSSID, 
                  pPrefNetworkFoundInd->ssId.length);
         if (NULL !=
             wdiLowLevelInd->wdiIndicationData.wdiPrefNetworkFoundInd.pData)
         {
            pPrefNetworkFoundInd->frameLength =
                wdiLowLevelInd->wdiIndicationData.wdiPrefNetworkFoundInd.frameLength;
            vos_mem_copy( pPrefNetworkFoundInd->data,
                wdiLowLevelInd->wdiIndicationData.wdiPrefNetworkFoundInd.pData,
                pPrefNetworkFoundInd->frameLength);
            wpalMemoryFree(wdiLowLevelInd->wdiIndicationData.wdiPrefNetworkFoundInd.pData);
            wdiLowLevelInd->wdiIndicationData.wdiPrefNetworkFoundInd.pData = NULL;
         }
         else
         {
            pPrefNetworkFoundInd->frameLength = 0;
         }
         pPrefNetworkFoundInd ->rssi = wdiLowLevelInd->wdiIndicationData.wdiPrefNetworkFoundInd.rssi; 
         /*                     */
         vosMsg.type = eWNI_SME_PREF_NETWORK_FOUND_IND;
         vosMsg.bodyptr = (void *) pPrefNetworkFoundInd;
         vosMsg.bodyval = 0;
         /*                     */
         if (VOS_STATUS_SUCCESS != vos_mq_post_message(VOS_MQ_ID_SME, (vos_msg_t*)&vosMsg))
         {
            /*                         */
            vos_mem_free((v_VOID_t *) pPrefNetworkFoundInd);
         }
         break;
      }
#endif //                      
      
#ifdef WLAN_WAKEUP_EVENTS
      case WDI_WAKE_REASON_IND:
      {
         vos_msg_t vosMsg;
         tANI_U32 allocSize = sizeof(tSirWakeReasonInd) 
                                  + (wdiLowLevelInd->wdiIndicationData.wdiWakeReasonInd.ulStoredDataLen - 1);
         tSirWakeReasonInd *pWakeReasonInd = (tSirWakeReasonInd *)vos_mem_malloc(allocSize);

         VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                    "[WAKE_REASON WDI] WAKE_REASON_IND Type (%d) data (ulReason=0x%x, ulReasonArg=0x%x, ulStoredDataLen=0x%x)",
                    wdiLowLevelInd->wdiIndicationType,
                    wdiLowLevelInd->wdiIndicationData.wdiWakeReasonInd.ulReason,
                    wdiLowLevelInd->wdiIndicationData.wdiWakeReasonInd.ulReasonArg,
                    wdiLowLevelInd->wdiIndicationData.wdiWakeReasonInd.ulStoredDataLen);

         if (NULL == pWakeReasonInd)
         {
            VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                       "Memory allocation failure, "
                       "WDI_WAKE_REASON_IND not forwarded");
            break;
         }

         vos_mem_zero(pWakeReasonInd, allocSize);

         /*                */
         pWakeReasonInd->mesgType = eWNI_SME_WAKE_REASON_IND;
         pWakeReasonInd->mesgLen = allocSize;

         /*                          */
         //                                                                                      
         pWakeReasonInd->ulReason = wdiLowLevelInd->wdiIndicationData.wdiWakeReasonInd.ulReason;
         pWakeReasonInd->ulReasonArg = wdiLowLevelInd->wdiIndicationData.wdiWakeReasonInd.ulReasonArg;
         pWakeReasonInd->ulStoredDataLen = wdiLowLevelInd->wdiIndicationData.wdiWakeReasonInd.ulStoredDataLen;
         pWakeReasonInd->ulActualDataLen = wdiLowLevelInd->wdiIndicationData.wdiWakeReasonInd.ulActualDataLen;         
         vos_mem_copy( (void *)&(pWakeReasonInd->aDataStart[0]), 
                        &(wdiLowLevelInd->wdiIndicationData.wdiWakeReasonInd.aDataStart[0]), 
                        wdiLowLevelInd->wdiIndicationData.wdiWakeReasonInd.ulStoredDataLen);

         /*                     */
         vosMsg.type = eWNI_SME_WAKE_REASON_IND;
         vosMsg.bodyptr = (void *) pWakeReasonInd;
         vosMsg.bodyval = 0;

         /*                     */
         if (VOS_STATUS_SUCCESS != vos_mq_post_message(VOS_MQ_ID_SME, (vos_msg_t*)&vosMsg))
         {
            /*                         */
            vos_mem_free((v_VOID_t *) pWakeReasonInd);
         }

         break;
      }
#endif //                   
      
      case WDI_TX_PER_HIT_IND:
      {
         vos_msg_t vosMsg;
         VOS_TRACE(VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO, "Get WDI_TX_PER_HIT_IND");
         /*                                       */
         /*                     */
         vosMsg.type = eWNI_SME_TX_PER_HIT_IND;
         vosMsg.bodyptr = NULL;
         vosMsg.bodyval = 0;
         /*                     */
         if (VOS_STATUS_SUCCESS != vos_mq_post_message(VOS_MQ_ID_SME, (vos_msg_t*)&vosMsg))
         {
            VOS_TRACE(VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_WARN, "post eWNI_SME_TX_PER_HIT_IND to SME Failed");
         }
         break;
      }
  
#ifdef FEATURE_WLAN_LPHB
      case WDI_LPHB_IND:
      {
         vos_msg_t     vosMsg;
         tSirLPHBInd  *lphbInd;

         lphbInd =
           (tSirLPHBInd *)vos_mem_malloc(sizeof(tSirLPHBInd));
         if (NULL == lphbInd)
         {
            VOS_TRACE(VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                           "%s: LPHB IND buffer alloc Fail", __func__);
            return ;
         }

         lphbInd->sessionIdx =
              wdiLowLevelInd->wdiIndicationData.wdiLPHBTimeoutInd.sessionIdx;
         lphbInd->protocolType =
              wdiLowLevelInd->wdiIndicationData.wdiLPHBTimeoutInd.protocolType;
         lphbInd->eventReason =
              wdiLowLevelInd->wdiIndicationData.wdiLPHBTimeoutInd.eventReason;

         VOS_TRACE(VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                   "Get WDI_LPHB_IND bssIdx %d",
                   wdiLowLevelInd->wdiIndicationData.wdiLPHBTimeoutInd.bssIdx);

         vosMsg.type    = eWNI_SME_LPHB_IND;
         vosMsg.bodyptr = lphbInd;
         vosMsg.bodyval = 0;
         /*                     */
         if (VOS_STATUS_SUCCESS !=
             vos_mq_post_message(VOS_MQ_ID_SME, (vos_msg_t*)&vosMsg))
         {
            VOS_TRACE(VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_WARN,
                      "post WDI_LPHB_WAIT_TIMEOUT_IND to SME Failed");
            vos_mem_free(lphbInd);
         }
         break;
      }
#endif /*                   */
      case WDI_PERIODIC_TX_PTRN_FW_IND:
      {
         VOS_TRACE(VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
            "%s: WDI_PERIODIC_TX_PTRN_FW_IND received, bssIdx: %d, "
            "selfStaIdx: %d, status: %d, patternIdBitmap: %d", __func__,
            (int)wdiLowLevelInd->wdiIndicationData.wdiPeriodicTxPtrnFwInd.bssIdx,
            (int)wdiLowLevelInd->wdiIndicationData.wdiPeriodicTxPtrnFwInd.selfStaIdx,
            (int)wdiLowLevelInd->wdiIndicationData.wdiPeriodicTxPtrnFwInd.status,
            (int)wdiLowLevelInd->wdiIndicationData.wdiPeriodicTxPtrnFwInd.patternIdBitmap);

         break;
      }

      case WDI_IBSS_PEER_INACTIVITY_IND:
      {
         tSirIbssPeerInactivityInd  *pIbssInd =
            (tSirIbssPeerInactivityInd *)
            vos_mem_malloc(sizeof(tSirIbssPeerInactivityInd));

         if (NULL == pIbssInd)
         {
            VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                  "Memory allocation failure, "
                  "WDI_IBSS_PEER_INACTIVITY_IND not forwarded");
            break;
         }

         pIbssInd->bssIdx =
            wdiLowLevelInd->wdiIndicationData.wdiIbssPeerInactivityInd.bssIdx;
         pIbssInd->staIdx =
            wdiLowLevelInd->wdiIndicationData.wdiIbssPeerInactivityInd.staIdx;
         vos_mem_copy(pIbssInd->peerAddr,
               wdiLowLevelInd->wdiIndicationData.wdiIbssPeerInactivityInd.staMacAddr,
               sizeof(tSirMacAddr));
         WDA_SendMsg(pWDA, WDA_IBSS_PEER_INACTIVITY_IND, (void *)pIbssInd, 0) ;
         break;
      }

#ifdef FEATURE_WLAN_BATCH_SCAN
     case  WDI_BATCH_SCAN_RESULT_IND:
     {
         void *pBatchScanResult;
         void *pCallbackContext;
         tpAniSirGlobal pMac;

         VOS_TRACE(VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO_HIGH,
                   "Received WDI_BATCHSCAN_RESULT_IND from FW");

         /*            */
         if (NULL == pWDA)
         {
            VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
              "%s:pWDA is NULL", __func__);
            VOS_ASSERT(0);
            return;
         }

         pBatchScanResult =
            (void *)wdiLowLevelInd->wdiIndicationData.pBatchScanResult;
         if (NULL == pBatchScanResult)
         {
            VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
              "%s:Batch scan result from FW is null can't invoke HDD callback",
              __func__);
            VOS_ASSERT(0);
            return;
         }

         pMac = (tpAniSirGlobal )VOS_GET_MAC_CTXT(pWDA->pVosContext);
         if (NULL == pMac)
         {
            VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
              "%s:pMac is NULL", __func__);
            VOS_ASSERT(0);
            return;
         }

         pCallbackContext = pMac->pmc.batchScanResultCallbackContext;
         /*                                                   */
         if(pMac->pmc.batchScanResultCallback)
         {
            pMac->pmc.batchScanResultCallback(pCallbackContext,
               pBatchScanResult);
         }
         else
         {
            VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
               "%s:HDD callback is null", __func__);
            VOS_ASSERT(0);
         }
         break;
     }
#endif

#ifdef FEATURE_WLAN_CH_AVOID
      case WDI_CH_AVOID_IND:
      {
         vos_msg_t            vosMsg;
         tSirChAvoidIndType  *chAvoidInd;

         chAvoidInd =
           (tSirChAvoidIndType *)vos_mem_malloc(sizeof(tSirChAvoidIndType));
         if (NULL == chAvoidInd)
         {
            VOS_TRACE(VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                           "%s: CH_AVOID IND buffer alloc Fail", __func__);
            return ;
         }

         chAvoidInd->avoidRangeCount =
              wdiLowLevelInd->wdiIndicationData.wdiChAvoidInd.avoidRangeCount;
         wpalMemoryCopy((void *)chAvoidInd->avoidFreqRange,
             (void *)wdiLowLevelInd->wdiIndicationData.wdiChAvoidInd.avoidFreqRange,
             chAvoidInd->avoidRangeCount * sizeof(tSirChAvoidFreqType));

         VOS_TRACE(VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                   "%s : WDA CH avoid notification", __func__);

         vosMsg.type    = eWNI_SME_CH_AVOID_IND;
         vosMsg.bodyptr = chAvoidInd;
         vosMsg.bodyval = 0;
         /*                     */
         if (VOS_STATUS_SUCCESS !=
             vos_mq_post_message(VOS_MQ_ID_SME, (vos_msg_t*)&vosMsg))
         {
            VOS_TRACE(VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                      "post eWNI_SME_CH_AVOID_IND to SME Failed");
            vos_mem_free(chAvoidInd);
         }
         break;
      }
#endif /*                       */

#ifdef WLAN_FEATURE_LINK_LAYER_STATS
     case  WDI_LL_STATS_RESULTS_IND:
     {
         void *pLinkLayerStatsInd;
         tpAniSirGlobal pMac;

         VOS_TRACE(VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                   "Received WDI_LL_STATS_RESULTS_IND from FW");

         /*            */
         if (NULL == pWDA)
         {
            VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
              "%s:pWDA is NULL", __func__);
            VOS_ASSERT(0);
            return;
         }

         pLinkLayerStatsInd =
            (void *)wdiLowLevelInd->
            wdiIndicationData.wdiLinkLayerStatsResults.pLinkLayerStatsResults;
         if (NULL == pLinkLayerStatsInd)
         {
            VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
              "%s:Link Layer Statistics from FW is null can't invoke HDD callback",
              __func__);
            VOS_ASSERT(0);
            return;
         }

         pMac = (tpAniSirGlobal )VOS_GET_MAC_CTXT(pWDA->pVosContext);
         if (NULL == pMac)
         {
            VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
              "%s:pMac is NULL", __func__);
            VOS_ASSERT(0);
            return;
         }

         /*                                              
                                                        
                                                    
          */
         if (pMac->sme.pLinkLayerStatsIndCallback)
         {
            pMac->sme.pLinkLayerStatsIndCallback(pMac->pAdapter,
                WDA_LINK_LAYER_STATS_RESULTS_RSP,
               pLinkLayerStatsInd,
               wdiLowLevelInd->
               wdiIndicationData.wdiLinkLayerStatsResults.macAddr);
         }
         else
         {
            VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
               "%s:HDD callback is null", __func__);
         }
         break;
     }
#endif /*                               */

#ifdef WLAN_FEATURE_EXTSCAN
     case  WDI_EXTSCAN_PROGRESS_IND:
     case  WDI_EXTSCAN_SCAN_AVAILABLE_IND:
     case  WDI_EXTSCAN_SCAN_RESULT_IND:
     case  WDI_EXTSCAN_BSSID_HOTLIST_RESULT_IND:
     case  WDI_EXTSCAN_SIGN_RSSI_RESULT_IND:
     {
         void *pEXTScanData;
         void *pCallbackContext;
         tpAniSirGlobal pMac;
         tANI_U16 indType;

         VOS_TRACE(VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                   "Received WDI_EXTSCAN Indications from FW");
         /*            */
         if (NULL == pWDA)
         {
            VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
              "%s:pWDA is NULL", __func__);
            VOS_ASSERT(0);
            return;
         }
         if (wdiLowLevelInd->wdiIndicationType == WDI_EXTSCAN_PROGRESS_IND)
         {
             indType = WDA_EXTSCAN_PROGRESS_IND;

             VOS_TRACE(VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                       "WDI_EXTSCAN Indication is WDI_EXTSCAN_PROGRESS_IND");
         }
         if (wdiLowLevelInd->wdiIndicationType ==
                                            WDI_EXTSCAN_SCAN_AVAILABLE_IND)
         {
             indType = WDA_EXTSCAN_SCAN_AVAILABLE_IND;

             VOS_TRACE(VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                   "WDI_EXTSCAN Indication is WDI_EXTSCAN_SCAN_AVAILABLE_IND");
         }
         if (wdiLowLevelInd->wdiIndicationType == WDI_EXTSCAN_SCAN_RESULT_IND)
         {
             indType = WDA_EXTSCAN_SCAN_RESULT_IND;

             VOS_TRACE(VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                    "WDI_EXTSCAN Indication is WDI_EXTSCAN_SCAN_RESULT_IND");
         }
         if (wdiLowLevelInd->wdiIndicationType ==
                 WDI_EXTSCAN_BSSID_HOTLIST_RESULT_IND)
         {
             indType = WDA_EXTSCAN_BSSID_HOTLIST_RESULT_IND;

             VOS_TRACE(VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
              "WDI_EXTSCAN Indication is WDI_EXTSCAN_BSSID_HOTLIST_RESULT_IND");
         }
         if (wdiLowLevelInd->wdiIndicationType ==
                 WDI_EXTSCAN_SIGN_RSSI_RESULT_IND)
         {
             indType = WDA_EXTSCAN_SIGNF_RSSI_RESULT_IND;

             VOS_TRACE(VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                 "WDI_EXTSCAN Indication is WDA_EXTSCAN_SIGNF_RSSI_RESULT_IND");
         }

         pEXTScanData =
            (void *)wdiLowLevelInd->wdiIndicationData.pEXTScanIndData;
         if (NULL == pEXTScanData)
         {
             VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
              "%s: EXTSCAN Indication Data is null, can't invoke HDD callback",
                     __func__);
             VOS_ASSERT(0);
             return;
         }

         pMac = (tpAniSirGlobal )VOS_GET_MAC_CTXT(pWDA->pVosContext);
         if (NULL == pMac)
         {
             VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                     "%s:pMac is NULL", __func__);
             VOS_ASSERT(0);
             return;
         }

         pCallbackContext = pMac->sme.pEXTScanCallbackContext;

         if(pMac->sme.pEXTScanIndCb)
         {
             pMac->sme.pEXTScanIndCb(pCallbackContext,
                     indType,
                     pEXTScanData);
         }
         else
         {
             VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                     "%s:HDD callback is null", __func__);
         }
         break;
     }
#endif /*                      */
      case WDI_DEL_BA_IND:
      {
         tpBADeleteParams  pDelBAInd =
           (tpBADeleteParams)vos_mem_malloc(sizeof(tpBADeleteParams));

         VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                                  "Received WDI_DEL_BA_IND from WDI ");
         if(NULL == pDelBAInd)
         {
            VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                                  "%s: VOS MEM Alloc Failure", __func__);
            break;
         }
         vos_mem_copy(pDelBAInd->peerMacAddr,
             wdiLowLevelInd->wdiIndicationData.wdiDeleteBAInd.peerMacAddr,
             sizeof(tSirMacAddr));
         vos_mem_copy(pDelBAInd->bssId,
             wdiLowLevelInd->wdiIndicationData.wdiDeleteBAInd.bssId,
             sizeof(tSirMacAddr));
         pDelBAInd->staIdx  =
          wdiLowLevelInd->wdiIndicationData.wdiDeleteBAInd.staIdx;
         pDelBAInd->baTID  =
          wdiLowLevelInd->wdiIndicationData.wdiDeleteBAInd.baTID;
         pDelBAInd->baDirection  =
          wdiLowLevelInd->wdiIndicationData.wdiDeleteBAInd.baDirection;
         pDelBAInd->reasonCode   =
          wdiLowLevelInd->wdiIndicationData.wdiDeleteBAInd.reasonCode;

         WDA_SendMsg(pWDA, SIR_LIM_DEL_BA_IND,
                               (void *)pDelBAInd , 0) ;
         break;
      }

      default:
      {
         /*            */
         VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                                  "Received UNKNOWN Indication from WDI ");
      } 
   }
   return ;
}

/*
                                
 */
void WDA_TriggerBaReqCallback(WDI_TriggerBARspParamsType *wdiTriggerBaRsp, 
                                                             void* pUserData)
{
   tWDA_ReqParams *pWdaParams = (tWDA_ReqParams *)pUserData; 
   tWDA_CbContext *pWDA;
   if(NULL == pWdaParams)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
              "%s: pWdaParams received NULL", __func__);
      VOS_ASSERT(0) ;
      return ;
   }
   pWDA = (tWDA_CbContext *)pWdaParams->pWdaContext;
   vos_mem_free(pWdaParams->wdaMsgParam) ;
   vos_mem_free(pWdaParams->wdaWdiApiMsgParam);
   vos_mem_free(pWdaParams) ;
   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                                          "<------ %s " ,__func__);
   if(WDI_STATUS_SUCCESS == wdiTriggerBaRsp->wdiStatus)
   {
      tANI_U8 i = 0 ;
      tBaActivityInd *baActivityInd = NULL ;
      tANI_U8 baCandidateCount = wdiTriggerBaRsp->usBaCandidateCnt ;
      tANI_U8 allocSize = sizeof(tBaActivityInd) 
                           + sizeof(tAddBaCandidate) * (baCandidateCount) ;
      WDI_TriggerBARspCandidateType *wdiBaCandidate = NULL ; 
      tAddBaCandidate *baCandidate = NULL ;
      baActivityInd =  (tBaActivityInd *)vos_mem_malloc(allocSize) ;
      if(NULL == baActivityInd) 
      { 
         VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
           "%s: VOS MEM Alloc Failure", __func__);
         VOS_ASSERT(0) ;
         return; 
      }
      vos_mem_copy(baActivityInd->bssId, wdiTriggerBaRsp->macBSSID, 
                                                    sizeof(tSirMacAddr)) ;
      baActivityInd->baCandidateCnt = baCandidateCount ;
       
      wdiBaCandidate = (WDI_TriggerBARspCandidateType*)(wdiTriggerBaRsp + 1) ;
      baCandidate = (tAddBaCandidate*)(baActivityInd + 1) ;
 
      for(i = 0 ; i < baCandidateCount ; i++)
      {
         tANI_U8 tid = 0 ;
         vos_mem_copy(baCandidate->staAddr, wdiBaCandidate->macSTA, 
                                                   sizeof(tSirMacAddr)) ;
         for(tid = 0 ; tid < STACFG_MAX_TC; tid++)
         {
             baCandidate->baInfo[tid].fBaEnable = 
                              wdiBaCandidate->wdiBAInfo[tid].fBaEnable ;
             baCandidate->baInfo[tid].startingSeqNum = 
                              wdiBaCandidate->wdiBAInfo[tid].startingSeqNum ;
         }
         wdiBaCandidate++ ;
         baCandidate++ ;
      }
      WDA_SendMsg(pWDA, SIR_LIM_ADD_BA_IND, (void *)baActivityInd , 0) ;
   }
   else
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                            "BA Trigger RSP with Failure received ");
   }
   return ;
}


/*
                                                                                     
             
 */
void WDA_TrafficStatsTimerActivate(wpt_boolean activate)
{
   wpt_uint32 enabled;
   v_VOID_t * pVosContext = vos_get_global_context(VOS_MODULE_ID_WDA, NULL);
   tWDA_CbContext *pWDA =  vos_get_context(VOS_MODULE_ID_WDA, pVosContext);
   tpAniSirGlobal pMac = (tpAniSirGlobal )VOS_GET_MAC_CTXT(pVosContext);
   
   if (NULL == pMac )
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                 "%s: Invoked with invalid MAC context ", __func__ );
      VOS_ASSERT(0);
      return;
   }

   if(wlan_cfgGetInt(pMac, WNI_CFG_ENABLE_MCC_ADAPTIVE_SCHED, &enabled) 
                                                      != eSIR_SUCCESS)
   {
      VOS_TRACE(VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                 "Failed to get WNI_CFG_ENABLE_MCC_ADAPTIVE_SCHED");
      return;
   }

   if(!enabled)
   {
      return;
   }

   if(NULL == pWDA)
   {
      VOS_TRACE(VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                 "%s:WDA context is NULL", __func__);
      VOS_ASSERT(0);
      return;
   }

   if(activate)
   {
      if( VOS_STATUS_SUCCESS != 
         WDA_START_TIMER(&pWDA->wdaTimers.trafficStatsTimer))
      {
         VOS_TRACE(VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                    "Traffic Stats Timer Start Failed ");
         return;
      }
      WDI_DS_ActivateTrafficStats();
   }
   else
   {
      WDI_DS_DeactivateTrafficStats();
      WDI_DS_ClearTrafficStats();

      if( VOS_STATUS_SUCCESS !=
         WDA_STOP_TIMER(&pWDA->wdaTimers.trafficStatsTimer))
      {
         VOS_TRACE(VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                    "Traffic Stats Timer Stop Failed ");
         return;
      }
   }
}

/*
                              
 */
void WDA_TimerTrafficStatsInd(tWDA_CbContext *pWDA)
{
   WDI_Status wdiStatus;
   WDI_TrafficStatsType *pWdiTrafficStats = NULL;
   WDI_TrafficStatsIndType trafficStatsIndParams;
   wpt_uint32 length, enabled;
   tpAniSirGlobal pMac = (tpAniSirGlobal )VOS_GET_MAC_CTXT(pWDA->pVosContext);

   if (NULL == pMac )
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                 "%s: Invoked with invalid MAC context ", __func__ );
      VOS_ASSERT(0);
      return;
   }

   if(wlan_cfgGetInt(pMac, WNI_CFG_ENABLE_MCC_ADAPTIVE_SCHED, &enabled) 
                                                      != eSIR_SUCCESS)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                    "Failed to get WNI_CFG_ENABLE_MCC_ADAPTIVE_SCHED");
      return;
   }

   if(!enabled)
   {
      WDI_DS_DeactivateTrafficStats();
      return;
   }

   WDI_DS_GetTrafficStats(&pWdiTrafficStats, &length);

   if(pWdiTrafficStats != NULL)
   {
      trafficStatsIndParams.pTrafficStats = pWdiTrafficStats;
      trafficStatsIndParams.length = length;
      trafficStatsIndParams.duration =
         pWDA->wdaTimers.trafficStatsTimer.initScheduleTimeInMsecs;
      trafficStatsIndParams.wdiReqStatusCB = WDA_WdiIndicationCallback;
      trafficStatsIndParams.pUserData = pWDA;

      wdiStatus = WDI_TrafficStatsInd(&trafficStatsIndParams);

      if(WDI_STATUS_PENDING == wdiStatus)
      {
         VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                 "Pending received for %s:%d ",__func__,__LINE__ );
      }
      else if( WDI_STATUS_SUCCESS_SYNC != wdiStatus )
      {
         VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                 "Failure in %s:%d ",__func__,__LINE__ );
      }
      
      WDI_DS_ClearTrafficStats();
   }
   else
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_WARN,
         "pWdiTrafficStats is Null");
   }

   if( VOS_STATUS_SUCCESS != 
      WDA_START_TIMER(&pWDA->wdaTimers.trafficStatsTimer))
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_WARN,
                              "Traffic Stats Timer Start Failed ");
      return;
   }
}

/*
                                  
 */
void WDA_BaCheckActivity(tWDA_CbContext *pWDA)
{
   tANI_U8 curSta = 0 ;
   tANI_U8 tid = 0 ;
   tANI_U8 size = 0 ;
   tANI_U8 baCandidateCount = 0 ;
   tANI_U8 newBaCandidate = 0 ;
   tANI_U32 val;
   WDI_TriggerBAReqCandidateType baCandidate[WDA_MAX_STA] = {{0}} ;
   tpAniSirGlobal pMac;

   if(NULL == pWDA)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                           "%s:pWDA is NULL", __func__); 
      VOS_ASSERT(0);
      return ;
   }
   if(WDA_MAX_STA < pWDA->wdaMaxSta)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                              "Inconsistent STA entries in WDA");
      VOS_ASSERT(0) ;
   }
   if(NULL == pWDA->pVosContext)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                          "%s: pVosContext is NULL",__func__);
      VOS_ASSERT(0);
      return ;
   }
   pMac = (tpAniSirGlobal )VOS_GET_MAC_CTXT(pWDA->pVosContext);
   if(NULL == pMac)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                          "%s: pMac is NULL",__func__);
      VOS_ASSERT(0);
      return ;
   }

   if (wlan_cfgGetInt(pMac,
           WNI_CFG_DEL_ALL_RX_TX_BA_SESSIONS_2_4_G_BTC, &val) !=
                                                      eSIR_SUCCESS)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
            "Unable to get WNI_CFG_DEL_ALL_RX_TX_BA_SESSIONS_2_4_G_BTC");
      val = 0;
   }

   /*                                                           */ 
   for(curSta = 0 ; curSta < pWDA->wdaMaxSta ; curSta++)
   {
      tANI_U32 currentOperChan = pWDA->wdaStaInfo[curSta].currentOperChan;
#ifdef WLAN_SOFTAP_VSTA_FEATURE
      //                                  
      if (!(IS_HWSTA_IDX(curSta)))
      {
          continue;
      }
#endif //                        
      for(tid = 0 ; tid < STACFG_MAX_TC ; tid++)
      {
         WLANTL_STAStateType tlSTAState ;
         tANI_U32 txPktCount = 0 ;
         tANI_U8 validStaIndex = pWDA->wdaStaInfo[curSta].ucValidStaIndex ;
         if((WDA_VALID_STA_INDEX == validStaIndex) &&
            (VOS_STATUS_SUCCESS == WDA_TL_GET_STA_STATE( pWDA->pVosContext,
                                                    curSta, &tlSTAState)) &&
            (VOS_STATUS_SUCCESS == WDA_TL_GET_TX_PKTCOUNT( pWDA->pVosContext,
                                                    curSta, tid, &txPktCount)))
         {
#if 0
            VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO_LOW,
             "************* %d:%d, %d ",curSta, txPktCount,
                                    pWDA->wdaStaInfo[curSta].framesTxed[tid]);
#endif
            if(val && ( (currentOperChan >= SIR_11B_CHANNEL_BEGIN) &&
                                (currentOperChan <= SIR_11B_CHANNEL_END)))
            {
                 VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                        "%s: BTC disabled aggregation - dont start "
                         "TX ADDBA req",__func__);
            }
            else if(!WDA_GET_BA_TXFLAG(pWDA, curSta, tid)
                   && (WLANTL_STA_AUTHENTICATED == tlSTAState)
                   && (txPktCount >= WDA_LAST_POLLED_THRESHOLD(pWDA, 
                                                               curSta, tid)))
            {
               /*                                        */
               //                                               
               baCandidate[baCandidateCount].ucTidBitmap |= 1 << tid ;
               newBaCandidate = WDA_ENABLE_BA ;
            }
            pWDA->wdaStaInfo[curSta].framesTxed[tid] = txPktCount ;
         }
      }
      /*                                                 */
      if(WDA_ENABLE_BA == newBaCandidate)
      { 
         /*                           */
         baCandidate[baCandidateCount].ucSTAIdx = curSta ;
         size += sizeof(WDI_TriggerBAReqCandidateType) ; 
         baCandidateCount++ ;
         newBaCandidate = WDA_DISABLE_BA ;
      } 
   }
   /*                                 */
   if( 0 < baCandidateCount)
   {
      WDI_Status status = WDI_STATUS_SUCCESS ;
      WDI_TriggerBAReqParamsType *wdiTriggerBaReq;
      tWDA_ReqParams *pWdaParams = 
               (tWDA_ReqParams *)vos_mem_malloc(sizeof(tWDA_ReqParams)) ;
      if(NULL == pWdaParams) 
      {
         VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
            "%s: VOS MEM Alloc Failure", __func__); 
         VOS_ASSERT(0) ; 
         return; 
      }
      wdiTriggerBaReq = (WDI_TriggerBAReqParamsType *)
                    vos_mem_malloc(sizeof(WDI_TriggerBAReqParamsType) + size) ;
      if(NULL == wdiTriggerBaReq) 
      {
         VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
            "%s: VOS MEM Alloc Failure", __func__); 
         VOS_ASSERT(0) ; 
         vos_mem_free(pWdaParams);
         return; 
      }
      do
      {
         WDI_TriggerBAReqinfoType *triggerBaInfo = 
                                   &wdiTriggerBaReq->wdiTriggerBAInfoType ;
         triggerBaInfo->usBACandidateCnt = baCandidateCount ;
         /*                                                               
                             */
         triggerBaInfo->ucSTAIdx = baCandidate[0].ucSTAIdx ;
         triggerBaInfo->ucBASessionID = 0;
         vos_mem_copy((wdiTriggerBaReq + 1), baCandidate, size) ;
      } while(0) ;
      wdiTriggerBaReq->wdiReqStatusCB = NULL ;
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                                          "------> %s " ,__func__);
      pWdaParams->pWdaContext = pWDA;
      pWdaParams->wdaWdiApiMsgParam = wdiTriggerBaReq ;
      pWdaParams->wdaMsgParam = NULL; 
      status = WDI_TriggerBAReq(wdiTriggerBaReq, 
                                   WDA_TriggerBaReqCallback, pWdaParams) ;
      if(IS_WDI_STATUS_FAILURE(status))
      {
         VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                 "Failure in Trigger BA REQ Params WDI API, free all the memory " );
         vos_mem_free(pWdaParams->wdaMsgParam) ;
         vos_mem_free(pWdaParams->wdaWdiApiMsgParam);
         vos_mem_free(pWdaParams) ;
      }
   }
   else
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO_LOW,
                              "There is no TID for initiating BA");
   }
   if( VOS_STATUS_SUCCESS != 
         WDA_STOP_TIMER(&pWDA->wdaTimers.baActivityChkTmr))
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                              "BA Activity Timer Stop Failed ");
      return ;
   }
   if( VOS_STATUS_SUCCESS != 
      WDA_START_TIMER(&pWDA->wdaTimers.baActivityChkTmr))
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                              "BA Activity Timer Start Failed ");
      return;
   }
   return ;
}
/*
                                                  
 */
static VOS_STATUS wdaCreateTimers(tWDA_CbContext *pWDA)
{
   VOS_STATUS status = VOS_STATUS_SUCCESS ;
   tANI_U32 val = 0 ;
   tpAniSirGlobal pMac = (tpAniSirGlobal )VOS_GET_MAC_CTXT(pWDA->pVosContext);
   
   if(NULL == pMac)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                           "%s:MAC context is NULL", __func__); 
      VOS_ASSERT(0);
      return VOS_STATUS_E_FAILURE;
   }
   if(wlan_cfgGetInt(pMac, WNI_CFG_BA_ACTIVITY_CHECK_TIMEOUT, &val ) 
                                                    != eSIR_SUCCESS)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                        "Failed to get value for WNI_CFG_CURRENT_TX_ANTENNA");
      return VOS_STATUS_E_FAILURE;
   }
   val = SYS_MS_TO_TICKS(val) ;
 
   /*                         */
   status = WDA_CREATE_TIMER(&pWDA->wdaTimers.baActivityChkTmr, 
                         "BA Activity Check timer", WDA_TimerHandler, 
                         WDA_TIMER_BA_ACTIVITY_REQ, val, val, TX_NO_ACTIVATE) ;
   if(status != TX_SUCCESS)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                               "Unable to create BA activity timer");
      return VOS_STATUS_E_FAILURE ;
   }
   val = SYS_MS_TO_TICKS( WDA_TX_COMPLETE_TIME_OUT_VALUE ) ; 
   /*                           */
   status = WDA_CREATE_TIMER(&pWDA->wdaTimers.TxCompleteTimer,
                         "Tx Complete Check timer", WDA_TimerHandler,
                         WDA_TX_COMPLETE_TIMEOUT_IND, val, val, TX_NO_ACTIVATE) ;
   if(status != TX_SUCCESS)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                               "Unable to create Tx Complete Timeout timer");
      /*                                          */
      status = WDA_DESTROY_TIMER(&pWDA->wdaTimers.baActivityChkTmr); 
      if(status != TX_SUCCESS)
      {
         VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                                  "Unable to Destroy BA activity timer");
         return VOS_STATUS_E_FAILURE ;
      } 
      return VOS_STATUS_E_FAILURE ;
   }

   val = SYS_MS_TO_TICKS( WDA_TRAFFIC_STATS_TIME_OUT_VALUE );

   /*                     */
   status = WDA_CREATE_TIMER(&pWDA->wdaTimers.trafficStatsTimer,
                         "Traffic Stats timer", WDA_TimerHandler,
                         WDA_TIMER_TRAFFIC_STATS_IND, val, val, TX_NO_ACTIVATE) ;
   if(status != TX_SUCCESS)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                               "Unable to create traffic stats timer");
      /*                                          */
      status = WDA_DESTROY_TIMER(&pWDA->wdaTimers.baActivityChkTmr);
      if(status != TX_SUCCESS)
      {
         VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                                  "Unable to Destroy BA activity timer");
      }
      /*                                    */
      status = WDA_DESTROY_TIMER(&pWDA->wdaTimers.TxCompleteTimer);
      if(status != TX_SUCCESS)
      {
         VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                                  "Unable to Tx complete timer");
      }
      return VOS_STATUS_E_FAILURE ;
   }
   return VOS_STATUS_SUCCESS ;
}
/*
                                                   
 */
static VOS_STATUS wdaDestroyTimers(tWDA_CbContext *pWDA)
{
   VOS_STATUS status = VOS_STATUS_SUCCESS ;
   status = WDA_DESTROY_TIMER(&pWDA->wdaTimers.TxCompleteTimer);
   if(status != TX_SUCCESS)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                               "Unable to Destroy Tx Complete Timeout timer");
      return eSIR_FAILURE ;
   }
   status = WDA_DESTROY_TIMER(&pWDA->wdaTimers.baActivityChkTmr);
   if(status != TX_SUCCESS)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                               "Unable to Destroy BA activity timer");
      return eSIR_FAILURE ;
   }
   status = WDA_DESTROY_TIMER(&pWDA->wdaTimers.trafficStatsTimer);
   if(status != TX_SUCCESS)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                               "Unable to Destroy traffic stats timer");
      return eSIR_FAILURE ;
   }
   return eSIR_SUCCESS ;
}
/*
                     
 */
void WDA_TimerHandler(v_VOID_t* pContext, tANI_U32 timerInfo)
{
   VOS_STATUS vosStatus = VOS_STATUS_SUCCESS;
   vos_msg_t wdaMsg = {0} ;
   /*
                                                                 
    */ 
   wdaMsg.type = timerInfo ; 
   wdaMsg.bodyptr = NULL;
   wdaMsg.bodyval = 0;
   /*                    */
   vosStatus = vos_mq_post_message( VOS_MQ_ID_WDA, &wdaMsg );
   if ( !VOS_IS_STATUS_SUCCESS(vosStatus) )
   {
      vosStatus = VOS_STATUS_E_BADMSG;
   }
}
/*
                                      
 */
void WDA_ProcessTxCompleteTimeOutInd(tWDA_CbContext* pWDA)
{
   tpAniSirGlobal pMac = (tpAniSirGlobal )VOS_GET_MAC_CTXT(pWDA->pVosContext) ;
   if( pWDA->pAckTxCbFunc )
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                                      "TxComplete timer expired");
      pWDA->pAckTxCbFunc( pMac, 0);
      pWDA->pAckTxCbFunc = NULL;
   }
   else
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
            "There is no request pending for TxComplete and wait timer expired");
   }
}
/*
                               
 */
eHalStatus WDA_SetRegDomain(void * clientCtxt, v_REGDOMAIN_t regId,
                                                tAniBool sendRegHint)
{
   if(VOS_STATUS_SUCCESS != vos_nv_setRegDomain(clientCtxt, regId, sendRegHint))
   {
      return eHAL_STATUS_INVALID_PARAMETER;
   }
   return eHAL_STATUS_SUCCESS;
}

#ifdef FEATURE_WLAN_SCAN_PNO
/*
                                    
   
 */ 
void WDA_PNOScanRespCallback(WDI_Status status, void* pUserData)
{
   tWDA_ReqParams *pWdaParams = (tWDA_ReqParams *)pUserData; 
   tSirPNOScanReq *pPNOScanReqParams;

   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
              "<------ %s, wdiStatus: %d",__func__, status);
   if(NULL == pWdaParams)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
              "%s: pWdaParams received NULL", __func__);
      VOS_ASSERT(0) ;
      return ;
   }

   pPNOScanReqParams = (tSirPNOScanReq *)pWdaParams->wdaMsgParam;
   if(pPNOScanReqParams->statusCallback)
   {
      pPNOScanReqParams->statusCallback(pPNOScanReqParams->callbackContext,
                          (status == WDI_STATUS_SUCCESS) ?
                           VOS_STATUS_SUCCESS : VOS_STATUS_E_FAILURE);
   }

   vos_mem_free(pWdaParams->wdaWdiApiMsgParam);
   vos_mem_free(pWdaParams->wdaMsgParam);
   vos_mem_free(pWdaParams);

   return ;
}
/*
                                   
               
                                                                           
 */
void WDA_PNOScanReqCallback(WDI_Status wdiStatus, void* pUserData)
{
   tWDA_ReqParams *pWdaParams = (tWDA_ReqParams *)pUserData;
   tSirPNOScanReq *pPNOScanReqParams;

   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
              "<------ %s, wdiStatus: %d", __func__, wdiStatus);

   if(NULL == pWdaParams)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                 "%s: pWdaParams received NULL", __func__);
      VOS_ASSERT(0);
      return;
   }

   if(IS_WDI_STATUS_FAILURE(wdiStatus))
   {
      pPNOScanReqParams = (tSirPNOScanReq *)pWdaParams->wdaMsgParam;
      if(pPNOScanReqParams->statusCallback)
      {
         pPNOScanReqParams->statusCallback(pPNOScanReqParams->callbackContext,
                                           VOS_STATUS_E_FAILURE);
      }

      vos_mem_free(pWdaParams->wdaWdiApiMsgParam);
      vos_mem_free(pWdaParams->wdaMsgParam);
      vos_mem_free(pWdaParams);
   }

   return;
}
/*
                                             
  
 */
void WDA_UpdateScanParamsRespCallback(WDI_Status status, void* pUserData)
{
   tWDA_ReqParams *pWdaParams = (tWDA_ReqParams *)pUserData;
   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                                          "<------ %s " ,__func__);
   if(NULL == pWdaParams)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
              "%s: pWdaParams received NULL", __func__);
      VOS_ASSERT(0) ;
      return ;
   }

   vos_mem_free(pWdaParams->wdaWdiApiMsgParam);
   vos_mem_free(pWdaParams->wdaMsgParam);
   vos_mem_free(pWdaParams);

   return ;
}
/*
                                            
               
                                                                                    
 */
void WDA_UpdateScanParamsReqCallback(WDI_Status wdiStatus, void* pUserData)
{
   tWDA_ReqParams *pWdaParams = (tWDA_ReqParams *)pUserData;

   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
              "<------ %s, wdiStatus: %d", __func__, wdiStatus);

   if(NULL == pWdaParams)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                 "%s: pWdaParams received NULL", __func__);
      VOS_ASSERT(0);
      return;
   }

   if(IS_WDI_STATUS_FAILURE(wdiStatus))
   {
      vos_mem_free(pWdaParams->wdaWdiApiMsgParam);
      vos_mem_free(pWdaParams->wdaMsgParam);
      vos_mem_free(pWdaParams);
   }

   return;
}
/*
                                               
                                                       
 */ 
VOS_STATUS WDA_ProcessSetPrefNetworkReq(tWDA_CbContext *pWDA, 
                                       tSirPNOScanReq *pPNOScanReqParams)
{
   WDI_Status status;
   WDI_PNOScanReqParamsType *pwdiPNOScanReqInfo = 
      (WDI_PNOScanReqParamsType *)vos_mem_malloc(sizeof(WDI_PNOScanReqParamsType)) ;
   tWDA_ReqParams *pWdaParams ;
   v_U8_t   i; 
   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                                          "------> %s " ,__func__);
   if(NULL == pwdiPNOScanReqInfo) 
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                           "%s: VOS MEM Alloc Failure", __func__); 
      VOS_ASSERT(0);
      return VOS_STATUS_E_NOMEM;
   }
   pWdaParams = (tWDA_ReqParams *)vos_mem_malloc(sizeof(tWDA_ReqParams)) ;
   if(NULL == pWdaParams)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                           "%s: VOS MEM Alloc Failure", __func__); 
      VOS_ASSERT(0);
      vos_mem_free(pwdiPNOScanReqInfo);
      return VOS_STATUS_E_NOMEM;
   }
   //
   //                                                              
   //
   pwdiPNOScanReqInfo->wdiPNOScanInfo.bEnable = pPNOScanReqParams->enable;
   pwdiPNOScanReqInfo->wdiPNOScanInfo.wdiModePNO = pPNOScanReqParams->modePNO;
   pwdiPNOScanReqInfo->wdiPNOScanInfo.ucNetworksCount = 
      ( pPNOScanReqParams->ucNetworksCount < WDI_PNO_MAX_SUPP_NETWORKS )? 
        pPNOScanReqParams->ucNetworksCount : WDI_PNO_MAX_SUPP_NETWORKS ;
   for ( i = 0; i < pwdiPNOScanReqInfo->wdiPNOScanInfo.ucNetworksCount ; i++)
   {
      vos_mem_copy(&pwdiPNOScanReqInfo->wdiPNOScanInfo.aNetworks[i],
                   &pPNOScanReqParams->aNetworks[i],
                   sizeof(pwdiPNOScanReqInfo->wdiPNOScanInfo.aNetworks[i]));
   }
   /*                    */
   vos_mem_copy(&pwdiPNOScanReqInfo->wdiPNOScanInfo.scanTimers,
                &pPNOScanReqParams->scanTimers,
                sizeof(pwdiPNOScanReqInfo->wdiPNOScanInfo.scanTimers));
   /*                              */
   pwdiPNOScanReqInfo->wdiPNOScanInfo.us24GProbeSize = 
      (pPNOScanReqParams->us24GProbeTemplateLen<WDI_PNO_MAX_PROBE_SIZE)?
      pPNOScanReqParams->us24GProbeTemplateLen:WDI_PNO_MAX_PROBE_SIZE; 
   vos_mem_copy( &pwdiPNOScanReqInfo->wdiPNOScanInfo.a24GProbeTemplate,
                pPNOScanReqParams->p24GProbeTemplate,
                pwdiPNOScanReqInfo->wdiPNOScanInfo.us24GProbeSize);
   /*                            */
   pwdiPNOScanReqInfo->wdiPNOScanInfo.us5GProbeSize = 
      (pPNOScanReqParams->us5GProbeTemplateLen<WDI_PNO_MAX_PROBE_SIZE)?
      pPNOScanReqParams->us5GProbeTemplateLen:WDI_PNO_MAX_PROBE_SIZE; 
   vos_mem_copy( &pwdiPNOScanReqInfo->wdiPNOScanInfo.a5GProbeTemplate,
                pPNOScanReqParams->p5GProbeTemplate,
                pwdiPNOScanReqInfo->wdiPNOScanInfo.us5GProbeSize);
   pwdiPNOScanReqInfo->wdiReqStatusCB = WDA_PNOScanReqCallback;
   pwdiPNOScanReqInfo->pUserData = pWdaParams;

   /*                             */
   pWdaParams->wdaWdiApiMsgParam = (void *)pwdiPNOScanReqInfo;
   pWdaParams->pWdaContext = pWDA;
   /*                                            */
   pWdaParams->wdaMsgParam = pPNOScanReqParams;
   status = WDI_SetPreferredNetworkReq(pwdiPNOScanReqInfo, 
                           (WDI_PNOScanCb)WDA_PNOScanRespCallback, pWdaParams);
   if(IS_WDI_STATUS_FAILURE(status))
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
              "Failure in Set PNO REQ WDI API, free all the memory " );
      if(pPNOScanReqParams->statusCallback)
      {
         pPNOScanReqParams->statusCallback(pPNOScanReqParams->callbackContext,
                                           VOS_STATUS_E_FAILURE);
      }

      vos_mem_free(pWdaParams->wdaWdiApiMsgParam) ;
      vos_mem_free(pWdaParams->wdaMsgParam);
      pWdaParams->wdaWdiApiMsgParam = NULL;
      pWdaParams->wdaMsgParam = NULL;
   }
   return CONVERT_WDI2VOS_STATUS(status) ;
}

#ifdef WLAN_FEATURE_ROAM_SCAN_OFFLOAD

void WDA_ConvertSirAuthToWDIAuth(WDI_AuthType *AuthType, v_U8_t csrAuthType)
{
   /*                                             */
   switch (csrAuthType)
   {
      case eCSR_AUTH_TYPE_OPEN_SYSTEM:
           *AuthType = eWDA_AUTH_TYPE_OPEN_SYSTEM;
           break;
#ifdef FEATURE_WLAN_ESE
      case eCSR_AUTH_TYPE_CCKM_WPA:
           *AuthType = eWDA_AUTH_TYPE_CCKM_WPA;
           break;
#endif
      case eCSR_AUTH_TYPE_WPA:
           *AuthType = eWDA_AUTH_TYPE_WPA;
           break;
      case eCSR_AUTH_TYPE_WPA_PSK:
           *AuthType = eWDA_AUTH_TYPE_WPA_PSK;
           break;
#ifdef FEATURE_WLAN_ESE
      case eCSR_AUTH_TYPE_CCKM_RSN:
           *AuthType = eWDA_AUTH_TYPE_CCKM_RSN;
           break;
#endif
      case eCSR_AUTH_TYPE_RSN:
           *AuthType = eWDA_AUTH_TYPE_RSN;
           break;
      case eCSR_AUTH_TYPE_RSN_PSK:
           *AuthType = eWDA_AUTH_TYPE_RSN_PSK;
           break;
#if defined WLAN_FEATURE_VOWIFI_11R
      case eCSR_AUTH_TYPE_FT_RSN:
           *AuthType = eWDA_AUTH_TYPE_FT_RSN;
           break;
      case eCSR_AUTH_TYPE_FT_RSN_PSK:
           *AuthType = eWDA_AUTH_TYPE_FT_RSN_PSK;
           break;
#endif
#ifdef FEATURE_WLAN_WAPI
      case eCSR_AUTH_TYPE_WAPI_WAI_CERTIFICATE:
           *AuthType = eWDA_AUTH_TYPE_WAPI_WAI_CERTIFICATE;
           break;
      case eCSR_AUTH_TYPE_WAPI_WAI_PSK:
           *AuthType = eWDA_AUTH_TYPE_WAPI_WAI_PSK;
           break;
#endif /*                   */
      case eCSR_AUTH_TYPE_SHARED_KEY:
      case eCSR_AUTH_TYPE_AUTOSWITCH:
           *AuthType = eWDA_AUTH_TYPE_OPEN_SYSTEM;
           break;
#if 0
      case eCSR_AUTH_TYPE_SHARED_KEY:
           *AuthType = eWDA_AUTH_TYPE_SHARED_KEY;
           break;
      case eCSR_AUTH_TYPE_AUTOSWITCH:
           *AuthType = eWDA_AUTH_TYPE_AUTOSWITCH;
#endif
      default:
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                 "%s: Unknown Auth Type", __func__);
           break;
   }
}
void WDA_ConvertSirEncToWDIEnc(WDI_EdType *EncrType, v_U8_t csrEncrType)
{
   switch (csrEncrType)
   {
      case eCSR_ENCRYPT_TYPE_NONE:
           *EncrType = WDI_ED_NONE;
           break;
      case eCSR_ENCRYPT_TYPE_WEP40_STATICKEY:
      case eCSR_ENCRYPT_TYPE_WEP40:
           *EncrType = WDI_ED_WEP40;
           break;
      case eCSR_ENCRYPT_TYPE_WEP104:
      case eCSR_ENCRYPT_TYPE_WEP104_STATICKEY:
           *EncrType = WDI_ED_WEP104;
           break;
      case eCSR_ENCRYPT_TYPE_TKIP:
           *EncrType = WDI_ED_TKIP;
           break;
      case eCSR_ENCRYPT_TYPE_AES:
           *EncrType = WDI_ED_CCMP;
           break;
#ifdef WLAN_FEATURE_11W
      case eCSR_ENCRYPT_TYPE_AES_CMAC:
           *EncrType = WDI_ED_AES_128_CMAC;
           break;
#endif
#ifdef FEATURE_WLAN_WAPI
      case eCSR_ENCRYPT_TYPE_WPI:
           *EncrType = WDI_ED_WPI;
           break;
#endif
      case eCSR_ENCRYPT_TYPE_ANY:
           *EncrType = WDI_ED_ANY;
           break;

      default:
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                 "%s: Unknown Encryption Type", __func__);
           break;
   }
}

/*
                                          
                                          
 */
VOS_STATUS WDA_ProcessRoamScanOffloadReq(tWDA_CbContext *pWDA,
                                                  tSirRoamOffloadScanReq *pRoamOffloadScanReqParams)
{
   WDI_Status status;
   WDI_RoamScanOffloadReqParamsType *pwdiRoamScanOffloadReqParams =
   (WDI_RoamScanOffloadReqParamsType *)vos_mem_malloc(sizeof(WDI_RoamScanOffloadReqParamsType));
   tWDA_ReqParams *pWdaParams ;
   v_U8_t csrAuthType;
   WDI_RoamNetworkType *pwdiRoamNetworkType;
   WDI_RoamOffloadScanInfo *pwdiRoamOffloadScanInfo;
   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
              "------> %s " ,__func__);
   if (NULL == pwdiRoamScanOffloadReqParams)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                           "%s: VOS MEM Alloc Failure", __func__);
      VOS_ASSERT(0);
      return VOS_STATUS_E_NOMEM;
   }
   pWdaParams = (tWDA_ReqParams *)vos_mem_malloc(sizeof(tWDA_ReqParams)) ;
   if (NULL == pWdaParams)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                           "%s: VOS MEM Alloc Failure", __func__);
      VOS_ASSERT(0);
      vos_mem_free(pwdiRoamScanOffloadReqParams);
      return VOS_STATUS_E_NOMEM;
   }

   pwdiRoamNetworkType =
   &pwdiRoamScanOffloadReqParams->wdiRoamOffloadScanInfo.ConnectedNetwork;
   pwdiRoamOffloadScanInfo =
   &pwdiRoamScanOffloadReqParams->wdiRoamOffloadScanInfo;
   vos_mem_zero (pwdiRoamScanOffloadReqParams,sizeof(WDI_RoamScanOffloadReqParamsType));
   csrAuthType = pRoamOffloadScanReqParams->ConnectedNetwork.authentication;
   pwdiRoamOffloadScanInfo->RoamScanOffloadEnabled =
          pRoamOffloadScanReqParams->RoamScanOffloadEnabled;
   vos_mem_copy(pwdiRoamNetworkType->currAPbssid,
                pRoamOffloadScanReqParams->ConnectedNetwork.currAPbssid,
                sizeof(pwdiRoamNetworkType->currAPbssid));
   WDA_ConvertSirAuthToWDIAuth(&pwdiRoamNetworkType->authentication,
                                csrAuthType);
   WDA_ConvertSirEncToWDIEnc(&pwdiRoamNetworkType->encryption,
       pRoamOffloadScanReqParams->ConnectedNetwork.encryption);
   WDA_ConvertSirEncToWDIEnc(&pwdiRoamNetworkType->mcencryption,
       pRoamOffloadScanReqParams->ConnectedNetwork.mcencryption);
   pwdiRoamOffloadScanInfo->LookupThreshold =
           pRoamOffloadScanReqParams->LookupThreshold ;
   pwdiRoamOffloadScanInfo->RxSensitivityThreshold =
           pRoamOffloadScanReqParams->RxSensitivityThreshold;
   pwdiRoamOffloadScanInfo->RoamRssiDiff =
           pRoamOffloadScanReqParams->RoamRssiDiff ;
   pwdiRoamOffloadScanInfo->MAWCEnabled =
           pRoamOffloadScanReqParams->MAWCEnabled ;
   pwdiRoamOffloadScanInfo->Command =
           pRoamOffloadScanReqParams->Command ;
   pwdiRoamOffloadScanInfo->StartScanReason =
           pRoamOffloadScanReqParams->StartScanReason ;
   pwdiRoamOffloadScanInfo->NeighborScanTimerPeriod =
           pRoamOffloadScanReqParams->NeighborScanTimerPeriod ;
   pwdiRoamOffloadScanInfo->NeighborRoamScanRefreshPeriod =
           pRoamOffloadScanReqParams->NeighborRoamScanRefreshPeriod ;
   pwdiRoamOffloadScanInfo->NeighborScanChannelMinTime =
           pRoamOffloadScanReqParams->NeighborScanChannelMinTime ;
   pwdiRoamOffloadScanInfo->NeighborScanChannelMaxTime =
           pRoamOffloadScanReqParams->NeighborScanChannelMaxTime ;
   pwdiRoamOffloadScanInfo->EmptyRefreshScanPeriod =
           pRoamOffloadScanReqParams->EmptyRefreshScanPeriod ;
   pwdiRoamOffloadScanInfo->IsESEEnabled =
           pRoamOffloadScanReqParams->IsESEEnabled ;
   vos_mem_copy(&pwdiRoamNetworkType->ssId.sSSID,
                &pRoamOffloadScanReqParams->ConnectedNetwork.ssId.ssId,
                pRoamOffloadScanReqParams->ConnectedNetwork.ssId.length);
   pwdiRoamNetworkType->ssId.ucLength =
           pRoamOffloadScanReqParams->ConnectedNetwork.ssId.length;
   vos_mem_copy(pwdiRoamNetworkType->ChannelCache,
                pRoamOffloadScanReqParams->ConnectedNetwork.ChannelCache,
                pRoamOffloadScanReqParams->ConnectedNetwork.ChannelCount);
   pwdiRoamNetworkType->ChannelCount =
           pRoamOffloadScanReqParams->ConnectedNetwork.ChannelCount;
   pwdiRoamOffloadScanInfo->ChannelCacheType =
           pRoamOffloadScanReqParams->ChannelCacheType;
   vos_mem_copy(pwdiRoamOffloadScanInfo->ValidChannelList,
                pRoamOffloadScanReqParams->ValidChannelList,
                pRoamOffloadScanReqParams->ValidChannelCount);
   pwdiRoamOffloadScanInfo->ValidChannelCount =
           pRoamOffloadScanReqParams->ValidChannelCount;
   pwdiRoamOffloadScanInfo->us24GProbeSize =
    (pRoamOffloadScanReqParams->us24GProbeTemplateLen<WDI_PNO_MAX_PROBE_SIZE)?
    pRoamOffloadScanReqParams->us24GProbeTemplateLen:WDI_PNO_MAX_PROBE_SIZE;
   vos_mem_copy(&pwdiRoamOffloadScanInfo->a24GProbeTemplate,
                pRoamOffloadScanReqParams->p24GProbeTemplate,
                pwdiRoamOffloadScanInfo->us24GProbeSize);
   pwdiRoamOffloadScanInfo->us5GProbeSize =
    (pRoamOffloadScanReqParams->us5GProbeTemplateLen<WDI_PNO_MAX_PROBE_SIZE)?
    pRoamOffloadScanReqParams->us5GProbeTemplateLen:WDI_PNO_MAX_PROBE_SIZE;
   vos_mem_copy(&pwdiRoamOffloadScanInfo->a5GProbeTemplate,
                pRoamOffloadScanReqParams->p5GProbeTemplate,
                pwdiRoamOffloadScanInfo->us5GProbeSize);
   pwdiRoamOffloadScanInfo->MDID.mdiePresent =
           pRoamOffloadScanReqParams->MDID.mdiePresent;
   pwdiRoamOffloadScanInfo->MDID.mobilityDomain =
           pRoamOffloadScanReqParams->MDID.mobilityDomain;
   pwdiRoamOffloadScanInfo->nProbes =
           pRoamOffloadScanReqParams->nProbes;
   pwdiRoamOffloadScanInfo->HomeAwayTime =
           pRoamOffloadScanReqParams->HomeAwayTime;
   pwdiRoamScanOffloadReqParams->wdiReqStatusCB = NULL;
   /*                             */
   pWdaParams->wdaWdiApiMsgParam = (void *)pwdiRoamScanOffloadReqParams;
   pWdaParams->pWdaContext = pWDA;
   /*                                            */
   pWdaParams->wdaMsgParam = pRoamOffloadScanReqParams;
   status = WDI_RoamScanOffloadReq(pwdiRoamScanOffloadReqParams,
                           (WDI_RoamOffloadScanCb)WDA_RoamOffloadScanReqCallback, pWdaParams);
   if(IS_WDI_STATUS_FAILURE(status))
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
              "Failure in Start Roam Candidate Lookup Req WDI API, free all the memory " );
      vos_mem_free(pWdaParams->wdaWdiApiMsgParam) ;
      vos_mem_free(pWdaParams->wdaMsgParam);
      pWdaParams->wdaWdiApiMsgParam = NULL;
      pWdaParams->wdaMsgParam = NULL;
   }
   return CONVERT_WDI2VOS_STATUS(status) ;
}
#endif

/*
                                       
   
 */ 
void WDA_RssiFilterRespCallback(WDI_Status status, void* pUserData)
{
   tWDA_ReqParams *pWdaParams = (tWDA_ReqParams *)pUserData;

   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                                          "<------ %s " ,__func__);

   if(NULL == pWdaParams)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                 "%s: pWdaParams received NULL", __func__);
      VOS_ASSERT(0);
      return;
   }

   vos_mem_free(pWdaParams->wdaMsgParam) ;
   vos_mem_free(pWdaParams->wdaWdiApiMsgParam);
   vos_mem_free(pWdaParams) ;

   return ;
}
/*
                                      
               
                                                                              
 */
void WDA_RssiFilterReqCallback(WDI_Status wdiStatus, void* pUserData)
{
   tWDA_ReqParams *pWdaParams = (tWDA_ReqParams *)pUserData;

   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
              "<------ %s, wdiStatus: %d", __func__, wdiStatus);

   if(NULL == pWdaParams)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                 "%s: pWdaParams received NULL", __func__);
      VOS_ASSERT(0);
      return;
   }

   if(IS_WDI_STATUS_FAILURE(wdiStatus))
   {
      vos_mem_free(pWdaParams->wdaWdiApiMsgParam);
      vos_mem_free(pWdaParams->wdaMsgParam);
      vos_mem_free(pWdaParams);
   }

   return;
}
/*
                                               
                                                       
 */ 
VOS_STATUS WDA_ProcessSetRssiFilterReq(tWDA_CbContext *pWDA, 
                                        tSirSetRSSIFilterReq* pRssiFilterParams)
{
   WDI_Status status;
   WDI_SetRssiFilterReqParamsType *pwdiSetRssiFilterReqInfo = 
      (WDI_SetRssiFilterReqParamsType *)vos_mem_malloc(sizeof(WDI_SetRssiFilterReqParamsType)) ;
   tWDA_ReqParams *pWdaParams ;
   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                                          "------> %s " ,__func__);
   if(NULL == pwdiSetRssiFilterReqInfo) 
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                           "%s: VOS MEM Alloc Failure", __func__); 
      VOS_ASSERT(0);
      return VOS_STATUS_E_NOMEM;
   }
   pWdaParams = (tWDA_ReqParams *)vos_mem_malloc(sizeof(tWDA_ReqParams)) ;
   if(NULL == pWdaParams)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                           "%s: VOS MEM Alloc Failure", __func__); 
      VOS_ASSERT(0);
      vos_mem_free(pwdiSetRssiFilterReqInfo);
      return VOS_STATUS_E_NOMEM;
   }
   pwdiSetRssiFilterReqInfo->rssiThreshold = pRssiFilterParams->rssiThreshold;
   pwdiSetRssiFilterReqInfo->wdiReqStatusCB = WDA_RssiFilterReqCallback;
   pwdiSetRssiFilterReqInfo->pUserData = pWdaParams;

   /*                             */
   pWdaParams->wdaWdiApiMsgParam = (void *)pwdiSetRssiFilterReqInfo;
   pWdaParams->pWdaContext = pWDA;
   /*                                            */
   pWdaParams->wdaMsgParam = pRssiFilterParams;
   status = WDI_SetRssiFilterReq( pwdiSetRssiFilterReqInfo, 
                                 (WDI_PNOScanCb)WDA_RssiFilterRespCallback,
                                 pWdaParams);
   if(IS_WDI_STATUS_FAILURE(status))
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
              "Failure in Set RSSI Filter REQ WDI API, free all the memory " );
      vos_mem_free(pWdaParams->wdaWdiApiMsgParam) ;
      vos_mem_free(pWdaParams->wdaMsgParam);
      pWdaParams->wdaWdiApiMsgParam = NULL;
      pWdaParams->wdaMsgParam = NULL;
   }
   return CONVERT_WDI2VOS_STATUS(status) ;
}

/*
                                        
                                           
 */ 
VOS_STATUS WDA_ProcessUpdateScanParams(tWDA_CbContext *pWDA, 
                                       tSirUpdateScanParams *pUpdateScanParams)
{
   WDI_Status status;
   WDI_UpdateScanParamsInfoType *wdiUpdateScanParamsInfoType = 
      (WDI_UpdateScanParamsInfoType *)vos_mem_malloc(
         sizeof(WDI_UpdateScanParamsInfoType)) ;
   tWDA_ReqParams *pWdaParams ;
   v_U8_t i; 
   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                                          "------> %s " ,__func__);
   if(NULL == wdiUpdateScanParamsInfoType) 
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                           "%s: VOS MEM Alloc Failure", __func__); 
      VOS_ASSERT(0);
      return VOS_STATUS_E_NOMEM;
   }
   pWdaParams = (tWDA_ReqParams *)vos_mem_malloc(sizeof(tWDA_ReqParams)) ;
   if ( NULL == pWdaParams )
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                           "%s: VOS MEM Alloc Failure", __func__); 
      VOS_ASSERT(0);
      vos_mem_free(wdiUpdateScanParamsInfoType);
      return VOS_STATUS_E_NOMEM;
   }
   //
   //                                                                                 
   //
   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
          "Update Scan Parameters b11dEnabled %d b11dResolved %d "
          "ucChannelCount %d usPassiveMinChTime %d usPassiveMaxChTime"
          " %d usActiveMinChTime %d usActiveMaxChTime %d sizeof "
          "sir struct %zu wdi struct %zu",
              pUpdateScanParams->b11dEnabled,
              pUpdateScanParams->b11dResolved,
              pUpdateScanParams->ucChannelCount, 
              pUpdateScanParams->usPassiveMinChTime, 
              pUpdateScanParams->usPassiveMaxChTime, 
              pUpdateScanParams->usActiveMinChTime,
              pUpdateScanParams->usActiveMaxChTime,
              sizeof(tSirUpdateScanParams),
              sizeof(wdiUpdateScanParamsInfoType->wdiUpdateScanParamsInfo) ); 

   wdiUpdateScanParamsInfoType->wdiUpdateScanParamsInfo.b11dEnabled  =
      pUpdateScanParams->b11dEnabled;
   wdiUpdateScanParamsInfoType->wdiUpdateScanParamsInfo.b11dResolved =
      pUpdateScanParams->b11dResolved;
   wdiUpdateScanParamsInfoType->wdiUpdateScanParamsInfo.cbState = 
      pUpdateScanParams->ucCBState;
   wdiUpdateScanParamsInfoType->wdiUpdateScanParamsInfo.usActiveMaxChTime  =
      pUpdateScanParams->usActiveMaxChTime; 
   wdiUpdateScanParamsInfoType->wdiUpdateScanParamsInfo.usActiveMinChTime  = 
      pUpdateScanParams->usActiveMinChTime;
   wdiUpdateScanParamsInfoType->wdiUpdateScanParamsInfo.usPassiveMaxChTime = 
      pUpdateScanParams->usPassiveMaxChTime;
   wdiUpdateScanParamsInfoType->wdiUpdateScanParamsInfo.usPassiveMinChTime = 
     pUpdateScanParams->usPassiveMinChTime;

   wdiUpdateScanParamsInfoType->wdiUpdateScanParamsInfo.ucChannelCount = 
      (pUpdateScanParams->ucChannelCount < WDI_PNO_MAX_NETW_CHANNELS_EX)?
      pUpdateScanParams->ucChannelCount:WDI_PNO_MAX_NETW_CHANNELS_EX;

   for ( i = 0; i < 
         wdiUpdateScanParamsInfoType->wdiUpdateScanParamsInfo.ucChannelCount ; 
         i++)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
             "Update Scan Parameters channel: %d",
                 pUpdateScanParams->aChannels[i]);
   
      wdiUpdateScanParamsInfoType->wdiUpdateScanParamsInfo.aChannels[i] = 
         pUpdateScanParams->aChannels[i];
   }

   wdiUpdateScanParamsInfoType->wdiReqStatusCB = WDA_UpdateScanParamsReqCallback;
   wdiUpdateScanParamsInfoType->pUserData = pWdaParams;

     /*                             */
   pWdaParams->wdaWdiApiMsgParam = wdiUpdateScanParamsInfoType;
   pWdaParams->pWdaContext = pWDA;
   /*                                            */
   pWdaParams->wdaMsgParam = pUpdateScanParams;
 
 

   status = WDI_UpdateScanParamsReq(wdiUpdateScanParamsInfoType, 
                    (WDI_UpdateScanParamsCb)WDA_UpdateScanParamsRespCallback,
                    pWdaParams);
   if(IS_WDI_STATUS_FAILURE(status))
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
              "Failure in Update Scan Params EQ WDI API, free all the memory " );
      vos_mem_free(pWdaParams->wdaWdiApiMsgParam) ;
      vos_mem_free(pWdaParams->wdaMsgParam);
      vos_mem_free(pWdaParams);
   }
   return CONVERT_WDI2VOS_STATUS(status) ;
}
#endif //                      

#ifdef WLAN_FEATURE_ROAM_SCAN_OFFLOAD
/*
                                           
  
 */
void WDA_RoamOffloadScanReqCallback(WDI_Status status, void* pUserData)
{
   tWDA_ReqParams *pWdaParams = (tWDA_ReqParams *)pUserData;
   vos_msg_t vosMsg;
   wpt_uint8 reason = 0;

   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                                          "<------ %s " ,__func__);
    if (NULL == pWdaParams)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
              "%s: pWdaParams received NULL", __func__);
      VOS_ASSERT(0) ;
      return ;
   }
   if ( pWdaParams != NULL )
   {
      if ( pWdaParams->wdaWdiApiMsgParam != NULL )
      {
         reason = ((WDI_RoamScanOffloadReqParamsType *)pWdaParams->wdaWdiApiMsgParam)->wdiRoamOffloadScanInfo.StartScanReason;
         vos_mem_free(pWdaParams->wdaWdiApiMsgParam);
      }
      if ( pWdaParams->wdaMsgParam != NULL)
      {
         vos_mem_free(pWdaParams->wdaMsgParam);
      }

      vos_mem_free(pWdaParams) ;
   }
   vosMsg.type = eWNI_SME_ROAM_SCAN_OFFLOAD_RSP;
   vosMsg.bodyptr = NULL;
   if (WDI_STATUS_SUCCESS != status)
   {
      reason = 0;
   }
   vosMsg.bodyval = reason;
   if (VOS_STATUS_SUCCESS != vos_mq_post_message(VOS_MQ_ID_SME, (vos_msg_t*)&vosMsg))
   {
      /*                         */
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                 "%s: Failed to post the rsp to UMAC", __func__);
   }

   return ;
}
#endif

/*
                                           
  
 */
void WDA_SetPowerParamsRespCallback(WDI_Status status, void* pUserData)
{
   tWDA_ReqParams *pWdaParams = (tWDA_ReqParams *)pUserData;

   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
         "<------ %s " ,__func__);

   if(NULL == pWdaParams)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
            "%s: pWdaParams received NULL", __func__);
      VOS_ASSERT(0);
      return;
   }

   vos_mem_free(pWdaParams->wdaWdiApiMsgParam);
   vos_mem_free(pWdaParams->wdaMsgParam);
   vos_mem_free(pWdaParams);

   return;
}
/*
                                          
               
                                                                                  
 */
void WDA_SetPowerParamsReqCallback(WDI_Status wdiStatus, void* pUserData)
{
   tWDA_ReqParams *pWdaParams = (tWDA_ReqParams *)pUserData;

   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
              "<------ %s, wdiStatus: %d", __func__, wdiStatus);

   if(NULL == pWdaParams)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                 "%s: pWdaParams received NULL", __func__);
      VOS_ASSERT(0);
      return;
   }

   if(IS_WDI_STATUS_FAILURE(wdiStatus))
   {
      vos_mem_free(pWdaParams->wdaWdiApiMsgParam);
      vos_mem_free(pWdaParams->wdaMsgParam);
      vos_mem_free(pWdaParams);
   }

   return;
}

#ifdef WLAN_FEATURE_PACKET_FILTERING
/*
                                              
   
 */ 
void WDA_8023MulticastListRespCallback(
                        WDI_RcvFltPktSetMcListRspParamsType  *pwdiRcvFltPktSetMcListRspInfo,
                        void * pUserData)
{
   tWDA_ReqParams *pWdaParams = (tWDA_ReqParams *)pUserData;
   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                                          "<------ %s " ,__func__);
   if(NULL == pWdaParams)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                 "%s: pWdaParams received NULL", __func__);
      VOS_ASSERT(0) ;
      return ;
   }
   
   vos_mem_free(pWdaParams->wdaMsgParam) ;
   vos_mem_free(pWdaParams->wdaWdiApiMsgParam);
   vos_mem_free(pWdaParams) ;
   //                               
   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
              "WDA_8023MulticastListRespCallback invoked " );
   return ;
}
/*
                                             
               
                                                                                     
 */
void WDA_8023MulticastListReqCallback(WDI_Status wdiStatus, void* pUserData)
{
   tWDA_ReqParams *pWdaParams = (tWDA_ReqParams *)pUserData;

   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
              "<------ %s, wdiStatus: %d", __func__, wdiStatus);

   if(NULL == pWdaParams)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                 "%s: pWdaParams received NULL", __func__);
      VOS_ASSERT(0);
      return;
   }

   if(IS_WDI_STATUS_FAILURE(wdiStatus))
   {
      vos_mem_free(pWdaParams->wdaWdiApiMsgParam);
      vos_mem_free(pWdaParams->wdaMsgParam);
      vos_mem_free(pWdaParams);
   }

   return;
}
/*
                                            
                                            
 */ 
VOS_STATUS WDA_Process8023MulticastListReq (tWDA_CbContext *pWDA, 
                                       tSirRcvFltMcAddrList *pRcvFltMcAddrList)
{
   WDI_Status status;
   WDI_RcvFltPktSetMcListReqParamsType *pwdiFltPktSetMcListReqParamsType = NULL;
   tWDA_ReqParams *pWdaParams ;
   tANI_U8         i;
   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                                          "------> %s " ,__func__);
   pwdiFltPktSetMcListReqParamsType = 
      (WDI_RcvFltPktSetMcListReqParamsType *)vos_mem_malloc(
                             sizeof(WDI_RcvFltPktSetMcListReqParamsType)
                                                           ) ;
   if(NULL == pwdiFltPktSetMcListReqParamsType) 
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                           "%s: VOS MEM Alloc Failure", __func__); 
      return VOS_STATUS_E_NOMEM;
   }
   pWdaParams = (tWDA_ReqParams *)vos_mem_malloc(sizeof(tWDA_ReqParams)) ;
   if(NULL == pWdaParams)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                           "%s: VOS MEM Alloc Failure", __func__); 
      vos_mem_free(pwdiFltPktSetMcListReqParamsType);
      return VOS_STATUS_E_NOMEM;
   }

   //
   //                                                             
   //
   pwdiFltPktSetMcListReqParamsType->mcAddrList.ulMulticastAddrCnt = 
                                   pRcvFltMcAddrList->ulMulticastAddrCnt;

    vos_mem_copy(pwdiFltPktSetMcListReqParamsType->mcAddrList.selfMacAddr,
                 pRcvFltMcAddrList->selfMacAddr, sizeof(tSirMacAddr));
    vos_mem_copy(pwdiFltPktSetMcListReqParamsType->mcAddrList.bssId,
                 pRcvFltMcAddrList->bssId, sizeof(tSirMacAddr));

   for( i = 0; i < pRcvFltMcAddrList->ulMulticastAddrCnt; i++ )
   {
      vos_mem_copy(&(pwdiFltPktSetMcListReqParamsType->mcAddrList.multicastAddr[i]),
                   &(pRcvFltMcAddrList->multicastAddr[i]),
                   sizeof(tSirMacAddr));
   }
   pwdiFltPktSetMcListReqParamsType->wdiReqStatusCB = WDA_8023MulticastListReqCallback;
   pwdiFltPktSetMcListReqParamsType->pUserData = pWdaParams;

   /*                             */
   pWdaParams->wdaWdiApiMsgParam = (void *)pwdiFltPktSetMcListReqParamsType;
   pWdaParams->pWdaContext = pWDA;
   /*                                            */
   pWdaParams->wdaMsgParam = pRcvFltMcAddrList;
   status = WDI_8023MulticastListReq(
                        pwdiFltPktSetMcListReqParamsType, 
                        (WDI_8023MulticastListCb)WDA_8023MulticastListRespCallback,
                        pWdaParams);
   if(IS_WDI_STATUS_FAILURE(status))
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
        "Failure in WDA_Process8023MulticastListReq(), free all the memory " );
      vos_mem_free(pWdaParams->wdaWdiApiMsgParam) ;
      vos_mem_free(pWdaParams->wdaMsgParam);
      vos_mem_free(pWdaParams);
   }
   return CONVERT_WDI2VOS_STATUS(status) ;
}
/*
                                                   
   
 */ 
void WDA_ReceiveFilterSetFilterRespCallback(
                        WDI_SetRcvPktFilterRspParamsType *pwdiSetRcvPktFilterRspInfo, 
                        void * pUserData)
{
   tWDA_ReqParams *pWdaParams = (tWDA_ReqParams *)pUserData;
   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                                          "<------ %s " ,__func__);
   /*                                   */
   if(NULL == pWdaParams)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                 "%s: pWdaParams received NULL", __func__);
      VOS_ASSERT(0) ;
      return ;
   }
   
   vos_mem_free(pWdaParams->wdaMsgParam) ;
   vos_mem_free(pWdaParams->wdaWdiApiMsgParam);
   vos_mem_free(pWdaParams) ;
   //                               
   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
              "WDA_ReceiveFilterSetFilterRespCallback invoked " );
   return ;
}

/*
                                                  
               
                                                                                          
 */
void WDA_ReceiveFilterSetFilterReqCallback(WDI_Status   wdiStatus,
                     void*        pUserData)
{
   tWDA_ReqParams        *pWdaParams = (tWDA_ReqParams *)pUserData;

   VOS_TRACE(VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
             "<------ %s, wdiStatus: %d",
              __func__, wdiStatus);

   if (NULL == pWdaParams)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
              "%s: Invalid pWdaParams pointer", __func__);
      VOS_ASSERT(0);
      return;
   }

   if (IS_WDI_STATUS_FAILURE(wdiStatus))
   {
         vos_mem_free(pWdaParams->wdaWdiApiMsgParam);
         vos_mem_free(pWdaParams->wdaMsgParam);
         vos_mem_free(pWdaParams);
   }

   return;
}

/*
                                                 
                                        
 */ 
VOS_STATUS WDA_ProcessReceiveFilterSetFilterReq (tWDA_CbContext *pWDA, 
                                       tSirRcvPktFilterCfgType *pRcvPktFilterCfg)
{
   WDI_Status status;
   v_SIZE_t   allocSize = sizeof(WDI_SetRcvPktFilterReqParamsType) + 
      ((pRcvPktFilterCfg->numFieldParams - 1) * sizeof(tSirRcvPktFilterFieldParams));
   WDI_SetRcvPktFilterReqParamsType *pwdiSetRcvPktFilterReqParamsType = 
      (WDI_SetRcvPktFilterReqParamsType *)vos_mem_malloc(allocSize) ;
   tWDA_ReqParams *pWdaParams ;
   tANI_U8         i;
   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                                          "------> %s " ,__func__);
   if(NULL == pwdiSetRcvPktFilterReqParamsType) 
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                           "%s: VOS MEM Alloc Failure", __func__); 
      VOS_ASSERT(0);
      return VOS_STATUS_E_NOMEM;
   }
   pWdaParams = (tWDA_ReqParams *)vos_mem_malloc(sizeof(tWDA_ReqParams)) ;
   if(NULL == pWdaParams)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                           "%s: VOS MEM Alloc Failure", __func__); 
      VOS_ASSERT(0);
      vos_mem_free(pwdiSetRcvPktFilterReqParamsType);
      return VOS_STATUS_E_NOMEM;
   }
   pwdiSetRcvPktFilterReqParamsType->wdiPktFilterCfg.filterId = pRcvPktFilterCfg->filterId;
   pwdiSetRcvPktFilterReqParamsType->wdiPktFilterCfg.filterType = pRcvPktFilterCfg->filterType;   
   pwdiSetRcvPktFilterReqParamsType->wdiPktFilterCfg.numFieldParams = pRcvPktFilterCfg->numFieldParams;
   pwdiSetRcvPktFilterReqParamsType->wdiPktFilterCfg.coalesceTime = pRcvPktFilterCfg->coalesceTime;
   vos_mem_copy(pwdiSetRcvPktFilterReqParamsType->wdiPktFilterCfg.selfMacAddr,
                pRcvPktFilterCfg->selfMacAddr, sizeof(wpt_macAddr));

   vos_mem_copy(pwdiSetRcvPktFilterReqParamsType->wdiPktFilterCfg.bssId,
                      pRcvPktFilterCfg->bssId, sizeof(wpt_macAddr));

   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
              "FID %d FT %d NParams %d CT %d",
              pwdiSetRcvPktFilterReqParamsType->wdiPktFilterCfg.filterId, 
              pwdiSetRcvPktFilterReqParamsType->wdiPktFilterCfg.filterType,
              pwdiSetRcvPktFilterReqParamsType->wdiPktFilterCfg.numFieldParams, 
              pwdiSetRcvPktFilterReqParamsType->wdiPktFilterCfg.coalesceTime);
   for ( i = 0; i < pwdiSetRcvPktFilterReqParamsType->wdiPktFilterCfg.numFieldParams; i++ )
   {
     wpalMemoryCopy(&pwdiSetRcvPktFilterReqParamsType->wdiPktFilterCfg.paramsData[i],
                    &pRcvPktFilterCfg->paramsData[i],
                    sizeof(pwdiSetRcvPktFilterReqParamsType->wdiPktFilterCfg.paramsData[i]));
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO, 
                 "Proto %d Comp Flag %d",
                 pwdiSetRcvPktFilterReqParamsType->
                         wdiPktFilterCfg.paramsData[i].protocolLayer, 
                 pwdiSetRcvPktFilterReqParamsType->
                         wdiPktFilterCfg.paramsData[i].cmpFlag);
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO, 
                 "Data Offset %d Data Len %d",
                 pwdiSetRcvPktFilterReqParamsType->
                         wdiPktFilterCfg.paramsData[i].dataOffset, 
                 pwdiSetRcvPktFilterReqParamsType->
                         wdiPktFilterCfg.paramsData[i].dataLength);
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO, 
                 "CData: %d:%d:%d:%d:%d:%d",
                 pwdiSetRcvPktFilterReqParamsType->
                         wdiPktFilterCfg.paramsData[i].compareData[0], 
                 pwdiSetRcvPktFilterReqParamsType->
                         wdiPktFilterCfg.paramsData[i].compareData[1], 
                 pwdiSetRcvPktFilterReqParamsType->
                         wdiPktFilterCfg.paramsData[i].compareData[2], 
                 pwdiSetRcvPktFilterReqParamsType->
                         wdiPktFilterCfg.paramsData[i].compareData[3],
                 pwdiSetRcvPktFilterReqParamsType->
                         wdiPktFilterCfg.paramsData[i].compareData[4], 
                 pwdiSetRcvPktFilterReqParamsType->
                         wdiPktFilterCfg.paramsData[i].compareData[5]);
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO, 
                 "MData: %d:%d:%d:%d:%d:%d",
                 pwdiSetRcvPktFilterReqParamsType->
                         wdiPktFilterCfg.paramsData[i].dataMask[0], 
                 pwdiSetRcvPktFilterReqParamsType->
                         wdiPktFilterCfg.paramsData[i].dataMask[1], 
                 pwdiSetRcvPktFilterReqParamsType->
                         wdiPktFilterCfg.paramsData[i].dataMask[2], 
                 pwdiSetRcvPktFilterReqParamsType->
                         wdiPktFilterCfg.paramsData[i].dataMask[3],
                 pwdiSetRcvPktFilterReqParamsType->
                         wdiPktFilterCfg.paramsData[i].dataMask[4], 
                 pwdiSetRcvPktFilterReqParamsType->
                         wdiPktFilterCfg.paramsData[i].dataMask[5]);
   }
   pwdiSetRcvPktFilterReqParamsType->wdiReqStatusCB = WDA_ReceiveFilterSetFilterReqCallback;
   pwdiSetRcvPktFilterReqParamsType->pUserData = pWdaParams;
   /*                             */
   pWdaParams->wdaWdiApiMsgParam = (void *)pwdiSetRcvPktFilterReqParamsType;
   pWdaParams->pWdaContext = pWDA;
   /*                                            */
   pWdaParams->wdaMsgParam = pRcvPktFilterCfg;
   status = WDI_ReceiveFilterSetFilterReq(pwdiSetRcvPktFilterReqParamsType,
           (WDI_ReceiveFilterSetFilterCb)WDA_ReceiveFilterSetFilterRespCallback,
           pWdaParams);
   if(IS_WDI_STATUS_FAILURE(status))
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
              "Failure in SetFilter(),free all the memory,status %d ",status);
      vos_mem_free(pWdaParams->wdaWdiApiMsgParam) ;
      vos_mem_free(pWdaParams->wdaMsgParam);
      vos_mem_free(pWdaParams);
   }
   return CONVERT_WDI2VOS_STATUS(status) ;
}
/*
                                             
   
 */ 
void WDA_FilterMatchCountRespCallback(
                            WDI_RcvFltPktMatchCntRspParamsType *pwdiRcvFltPktMatchRspParams, 
                            void * pUserData)
{
   tWDA_ReqParams *pWdaParams = (tWDA_ReqParams *)pUserData;
   tWDA_CbContext *pWDA;
   tpSirRcvFltPktMatchRsp pRcvFltPktMatchCntReq;
   tpSirRcvFltPktMatchRsp pRcvFltPktMatchCntRsp = 
                            vos_mem_malloc(sizeof(tSirRcvFltPktMatchRsp));
   tANI_U8         i;
   vos_msg_t vosMsg;

   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                                          "<------ %s " ,__func__);
   /*                                   */

   if(NULL == pRcvFltPktMatchCntRsp)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                 "%s: pRcvFltPktMatchCntRsp is NULL", __func__);
      VOS_ASSERT(0) ;
      vos_mem_free(pWdaParams);
      return ;
   }
   
   if(NULL == pWdaParams)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                 "%s: pWdaParams received NULL", __func__);
      VOS_ASSERT(0) ;
      vos_mem_free(pRcvFltPktMatchCntRsp);
      return ;
   }
   pWDA = (tWDA_CbContext *)pWdaParams->pWdaContext ;
   pRcvFltPktMatchCntReq = (tpSirRcvFltPktMatchRsp)pWdaParams->wdaMsgParam;
   //                                                      
   vos_mem_zero(pRcvFltPktMatchCntRsp,sizeof(tSirRcvFltPktMatchRsp));

   /*                */
   pRcvFltPktMatchCntRsp->mesgType = eWNI_PMC_PACKET_COALESCING_FILTER_MATCH_COUNT_RSP;
   pRcvFltPktMatchCntRsp->mesgLen = sizeof(tSirRcvFltPktMatchRsp);

   pRcvFltPktMatchCntRsp->status = pwdiRcvFltPktMatchRspParams->wdiStatus;

   for (i = 0; i < SIR_MAX_NUM_FILTERS; i++)
   {   
      pRcvFltPktMatchCntRsp->filterMatchCnt[i].filterId = pRcvFltPktMatchCntReq->filterMatchCnt[i].filterId;
      pRcvFltPktMatchCntRsp->filterMatchCnt[i].matchCnt = pRcvFltPktMatchCntReq->filterMatchCnt[i].matchCnt;
   }
   /*                     */
   vosMsg.type = eWNI_PMC_PACKET_COALESCING_FILTER_MATCH_COUNT_RSP;
   vosMsg.bodyptr = (void *)pRcvFltPktMatchCntRsp;
   vosMsg.bodyval = 0;
   if (VOS_STATUS_SUCCESS != vos_mq_post_message(VOS_MQ_ID_SME, (vos_msg_t*)&vosMsg))
   {
      /*                         */
      vos_mem_free((v_VOID_t *)pRcvFltPktMatchCntRsp);
   }

   vos_mem_free(pWdaParams->wdaMsgParam) ;
   vos_mem_free(pWdaParams->wdaWdiApiMsgParam);
   vos_mem_free(pWdaParams) ;

   return;
}
/*
                                            
                                        
                                                                                    
 */
void WDA_FilterMatchCountReqCallback(WDI_Status wdiStatus, void * pUserData)
{
   tWDA_ReqParams *pWdaParams = (tWDA_ReqParams *)pUserData;
   vos_msg_t vosMsg;

   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
              "<------ %s, wdiStatus: %d", __func__, wdiStatus);

   if(NULL == pWdaParams)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                 "%s: pWdaParams received NULL", __func__);
      VOS_ASSERT(0);
      return;
   }

   /*                     */
   vosMsg.type = eWNI_PMC_PACKET_COALESCING_FILTER_MATCH_COUNT_RSP;
   vosMsg.bodyptr = NULL;
   vosMsg.bodyval = 0;

   if(IS_WDI_STATUS_FAILURE(wdiStatus))
   {
      vos_mem_free(pWdaParams->wdaWdiApiMsgParam);
      vos_mem_free(pWdaParams->wdaMsgParam);
      vos_mem_free(pWdaParams);
      vos_mq_post_message(VOS_MQ_ID_SME, (vos_msg_t*)&vosMsg);
   }

   return;
}
/*
                                                 
                                              
 */ 
VOS_STATUS WDA_ProcessPacketFilterMatchCountReq (tWDA_CbContext *pWDA, tpSirRcvFltPktMatchRsp pRcvFltPktMatchRsp)
{
   WDI_Status status;
   WDI_RcvFltPktMatchCntReqParamsType *pwdiRcvFltPktMatchCntReqParamsType = 
      (WDI_RcvFltPktMatchCntReqParamsType *)vos_mem_malloc(sizeof(WDI_RcvFltPktMatchCntReqParamsType));
   tWDA_ReqParams *pWdaParams ;
   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                                          "------> %s " ,__func__);
   if(NULL == pwdiRcvFltPktMatchCntReqParamsType) 
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                           "%s: VOS MEM Alloc Failure", __func__); 
      VOS_ASSERT(0);
      return VOS_STATUS_E_NOMEM;
   }
   pWdaParams = (tWDA_ReqParams *)vos_mem_malloc(sizeof(tWDA_ReqParams)) ;
   if(NULL == pWdaParams)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                           "%s: VOS MEM Alloc Failure", __func__); 
      VOS_ASSERT(0);
      vos_mem_free(pwdiRcvFltPktMatchCntReqParamsType);
      return VOS_STATUS_E_NOMEM;
   }

   pwdiRcvFltPktMatchCntReqParamsType->wdiReqStatusCB = WDA_FilterMatchCountReqCallback;
   pwdiRcvFltPktMatchCntReqParamsType->pUserData = pWdaParams;

   vos_mem_copy( pwdiRcvFltPktMatchCntReqParamsType->bssId,
                 pRcvFltPktMatchRsp->bssId, 
                 sizeof(wpt_macAddr));

   /*                             */
   pWdaParams->wdaWdiApiMsgParam = (void *)pwdiRcvFltPktMatchCntReqParamsType;
   pWdaParams->pWdaContext = pWDA;
   /*                                            */
   pWdaParams->wdaMsgParam = pRcvFltPktMatchRsp;
   status = WDI_FilterMatchCountReq(pwdiRcvFltPktMatchCntReqParamsType, 
                 (WDI_FilterMatchCountCb)WDA_FilterMatchCountRespCallback,
                 pWdaParams);
   if(IS_WDI_STATUS_FAILURE(status))
   {
      /*                             */
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
              "Failure in WDI_FilterMatchCountReq(), free all the memory " );
      vos_mem_free(pWdaParams->wdaWdiApiMsgParam);
      vos_mem_free(pWdaParams) ;
      pRcvFltPktMatchRsp->status = eSIR_FAILURE ;
      WDA_SendMsg(pWDA, WDA_PACKET_COALESCING_FILTER_MATCH_COUNT_RSP, (void *)pRcvFltPktMatchRsp, 0) ;
   }
   return CONVERT_WDI2VOS_STATUS(status) ;
}
/*
                                                     
   
 */ 
void WDA_ReceiveFilterClearFilterRespCallback(
                        WDI_RcvFltPktClearRspParamsType *pwdiRcvFltPktClearRspParamsType, 
                        void * pUserData)
{
   tWDA_ReqParams *pWdaParams = (tWDA_ReqParams *)pUserData;
   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                                          "<------ %s " ,__func__);
/*                                       */
   if(NULL == pWdaParams)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                 "%s: pWdaParams received NULL", __func__);
      VOS_ASSERT(0) ;
      return ;
   }
   
   vos_mem_free(pWdaParams->wdaMsgParam) ;
   vos_mem_free(pWdaParams->wdaWdiApiMsgParam);
   vos_mem_free(pWdaParams) ;
   //                               
   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
              "WDA_ReceiveFilterClearFilterRespCallback invoked " );
   return ;
}
/*
                                                    
               
                                                                                            
 */
void WDA_ReceiveFilterClearFilterReqCallback(WDI_Status wdiStatus, void* pUserData)
{
   tWDA_ReqParams *pWdaParams = (tWDA_ReqParams *)pUserData;

   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
              "<------ %s, wdiStatus: %d", __func__, wdiStatus);

   if(NULL == pWdaParams)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                 "%s: Invalid pWdaParams pointer", __func__);
      VOS_ASSERT(0);
      return;
   }

   if(IS_WDI_STATUS_FAILURE(wdiStatus))
   {
      vos_mem_free(pWdaParams->wdaWdiApiMsgParam);
      vos_mem_free(pWdaParams->wdaMsgParam);
      vos_mem_free(pWdaParams);
   }

   return;
}
/*
                                                   
                                          
 */
VOS_STATUS WDA_ProcessReceiveFilterClearFilterReq (tWDA_CbContext *pWDA,
                                       tSirRcvFltPktClearParam *pRcvFltPktClearParam)
{
   WDI_Status status;
   WDI_RcvFltPktClearReqParamsType *pwdiRcvFltPktClearReqParamsType =
      (WDI_RcvFltPktClearReqParamsType *)vos_mem_malloc(sizeof(WDI_RcvFltPktClearReqParamsType));
   tWDA_ReqParams *pWdaParams ;
   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                                          "------> %s " ,__func__);
   if(NULL == pwdiRcvFltPktClearReqParamsType)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                           "%s: VOS MEM Alloc Failure", __func__);
      VOS_ASSERT(0);
      return VOS_STATUS_E_NOMEM;
   }
   pWdaParams = (tWDA_ReqParams *)vos_mem_malloc(sizeof(tWDA_ReqParams)) ;
   if(NULL == pWdaParams)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                           "%s: VOS MEM Alloc Failure", __func__);
      VOS_ASSERT(0);
      vos_mem_free(pwdiRcvFltPktClearReqParamsType);
      return VOS_STATUS_E_NOMEM;
   }
   pwdiRcvFltPktClearReqParamsType->filterClearParam.status = pRcvFltPktClearParam->status;
   pwdiRcvFltPktClearReqParamsType->filterClearParam.filterId = pRcvFltPktClearParam->filterId;
   vos_mem_copy(pwdiRcvFltPktClearReqParamsType->filterClearParam.selfMacAddr,
                     pRcvFltPktClearParam->selfMacAddr, sizeof(wpt_macAddr));
   vos_mem_copy(pwdiRcvFltPktClearReqParamsType->filterClearParam.bssId,
                         pRcvFltPktClearParam->bssId, sizeof(wpt_macAddr));

   pwdiRcvFltPktClearReqParamsType->wdiReqStatusCB = WDA_ReceiveFilterClearFilterReqCallback;
   pwdiRcvFltPktClearReqParamsType->pUserData = pWdaParams;
   /*                             */
   pWdaParams->wdaWdiApiMsgParam = (void *)pwdiRcvFltPktClearReqParamsType;
   pWdaParams->pWdaContext = pWDA;
   /*                                            */
   pWdaParams->wdaMsgParam = pRcvFltPktClearParam;
   status = WDI_ReceiveFilterClearFilterReq(pwdiRcvFltPktClearReqParamsType,
       (WDI_ReceiveFilterClearFilterCb)WDA_ReceiveFilterClearFilterRespCallback,
       pWdaParams);
   if(IS_WDI_STATUS_FAILURE(status))
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
              "Failure in WDA_ProcessReceiveFilterClearFilterReq(), free all the memory " );
      vos_mem_free(pWdaParams->wdaWdiApiMsgParam) ;
      vos_mem_free(pWdaParams->wdaMsgParam);
      vos_mem_free(pWdaParams);
   }
   return CONVERT_WDI2VOS_STATUS(status) ;
}
#endif //                              

/*
                                         
                                      
 */ 
VOS_STATUS WDA_ProcessSetPowerParamsReq(tWDA_CbContext *pWDA, 
                                        tSirSetPowerParamsReq *pPowerParams)
{
   WDI_Status status;
   WDI_SetPowerParamsReqParamsType *pwdiSetPowerParamsReqInfo = 
      (WDI_SetPowerParamsReqParamsType *)vos_mem_malloc(sizeof(WDI_SetPowerParamsReqParamsType)) ;
   tWDA_ReqParams *pWdaParams ;
   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                                          "------> %s " ,__func__);
   if(NULL == pwdiSetPowerParamsReqInfo) 
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                           "%s: VOS MEM Alloc Failure", __func__); 
      VOS_ASSERT(0);
      return VOS_STATUS_E_NOMEM;
   }
   pWdaParams = (tWDA_ReqParams *)vos_mem_malloc(sizeof(tWDA_ReqParams)) ;
   if(NULL == pWdaParams)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                           "%s: VOS MEM Alloc Failure", __func__); 
      VOS_ASSERT(0);
      vos_mem_free(pwdiSetPowerParamsReqInfo);
      return VOS_STATUS_E_NOMEM;
   }
   
   
   pwdiSetPowerParamsReqInfo->wdiSetPowerParamsInfo.uIgnoreDTIM       = 
      pPowerParams->uIgnoreDTIM;
   pwdiSetPowerParamsReqInfo->wdiSetPowerParamsInfo.uDTIMPeriod       = 
      pPowerParams->uDTIMPeriod;
   pwdiSetPowerParamsReqInfo->wdiSetPowerParamsInfo.uListenInterval   = 
      pPowerParams->uListenInterval;
   pwdiSetPowerParamsReqInfo->wdiSetPowerParamsInfo.uBcastMcastFilter = 
      pPowerParams->uBcastMcastFilter;
   pwdiSetPowerParamsReqInfo->wdiSetPowerParamsInfo.uEnableBET        = 
      pPowerParams->uEnableBET;
   pwdiSetPowerParamsReqInfo->wdiSetPowerParamsInfo.uBETInterval      = 
      pPowerParams->uBETInterval; 
   pwdiSetPowerParamsReqInfo->wdiSetPowerParamsInfo.uMaxLIModulatedDTIM =
      pPowerParams->uMaxLIModulatedDTIM;
   pwdiSetPowerParamsReqInfo->wdiReqStatusCB = WDA_SetPowerParamsReqCallback;
   pwdiSetPowerParamsReqInfo->pUserData = pWdaParams;

   /*                             */
   pWdaParams->wdaWdiApiMsgParam = (void *)pwdiSetPowerParamsReqInfo;
   pWdaParams->pWdaContext = pWDA;
   /*                                            */
   pWdaParams->wdaMsgParam = pPowerParams;
   status = WDI_SetPowerParamsReq( pwdiSetPowerParamsReqInfo, 
                                 (WDI_SetPowerParamsCb)WDA_SetPowerParamsRespCallback,
                                 pWdaParams);
   if(IS_WDI_STATUS_FAILURE(status))
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
              "Failure in Set power params REQ WDI API, free all the memory " );
      vos_mem_free(pWdaParams->wdaWdiApiMsgParam) ;
      vos_mem_free(pWdaParams->wdaMsgParam);
      pWdaParams->wdaWdiApiMsgParam = NULL;
      pWdaParams->wdaMsgParam = NULL;
   }
   return CONVERT_WDI2VOS_STATUS(status) ;
}

/*
                                      
                        
 */ 
void WDA_SetTmLevelRspCallback(WDI_Status status, void* pUserData)
{
   tWDA_ReqParams *pWdaParams = (tWDA_ReqParams *)pUserData; 

   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                                          "<------ %s " ,__func__);

   if(NULL == pWdaParams)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
              "%s: pWdaParams received NULL", __func__);
      VOS_ASSERT(0) ;
      return ;
   }

   /*                                                  
                                    */
   if( pWdaParams != NULL )
   {
      if( pWdaParams->wdaWdiApiMsgParam != NULL )
      {
         vos_mem_free(pWdaParams->wdaWdiApiMsgParam);
      }
      vos_mem_free(pWdaParams->wdaMsgParam) ;
      vos_mem_free(pWdaParams) ;
   }
}

/*
                                     
                       
 */
VOS_STATUS WDA_ProcessSetTmLevelReq(tWDA_CbContext *pWDA,
                                             tAniSetTmLevelReq *setTmLevelReq)
{
   WDI_Status status = WDI_STATUS_SUCCESS ;
   tWDA_ReqParams *pWdaParams ;
   WDI_SetTmLevelReqType *wdiSetTmLevelReq = 
               (WDI_SetTmLevelReqType *)vos_mem_malloc(
                                       sizeof(WDI_SetTmLevelReqType)) ;
   if(NULL == wdiSetTmLevelReq) 
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                           "%s: VOS MEM Alloc Failure", __func__); 
      VOS_ASSERT(0);
      return VOS_STATUS_E_NOMEM;
   }

   pWdaParams = (tWDA_ReqParams *)vos_mem_malloc(sizeof(tWDA_ReqParams)) ;
   if(NULL == pWdaParams)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                           "%s: VOS MEM Alloc Failure", __func__); 
      VOS_ASSERT(0);
      vos_mem_free(wdiSetTmLevelReq);
      return VOS_STATUS_E_NOMEM;
   }

   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                                          "------> %s " ,__func__);

   wdiSetTmLevelReq->tmMode  = setTmLevelReq->tmMode;
   wdiSetTmLevelReq->tmLevel = setTmLevelReq->newTmLevel;

   pWdaParams->pWdaContext = pWDA;
   pWdaParams->wdaMsgParam = setTmLevelReq;
   pWdaParams->wdaWdiApiMsgParam = wdiSetTmLevelReq;

   status = WDI_SetTmLevelReq(wdiSetTmLevelReq, 
                           (WDI_SetTmLevelCb)WDA_SetTmLevelRspCallback, pWdaParams);

   if(IS_WDI_STATUS_FAILURE(status))
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                 "Failure setting thermal mitigation level, freeing memory" );
      vos_mem_free(pWdaParams->wdaMsgParam) ;
      vos_mem_free(pWdaParams->wdaWdiApiMsgParam);
      vos_mem_free(pWdaParams) ;
   }

   return CONVERT_WDI2VOS_STATUS(status) ;
}

VOS_STATUS WDA_ProcessTxControlInd(tWDA_CbContext *pWDA,
                                   tpTxControlParams pTxCtrlParam)
{
   VOS_STATUS wdaStatus;
   
   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                                          "------> %s " ,__func__);
   if( pTxCtrlParam == NULL )
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                           "%s: Input tpTxControlParams is NULL", __func__); 
      return VOS_STATUS_E_FAILURE;
   }
   if( pTxCtrlParam->stopTx == eANI_BOOLEAN_TRUE )
   {
      wdaStatus = WDA_SuspendDataTx(pWDA);
   }
   else /*                                            */
   {
      wdaStatus = WDA_ResumeDataTx(pWDA);
   }
   return wdaStatus;
}

 /*                                     
                                                               
  */
void WDA_featureCapsExchange(v_PVOID_t pVosContext)
{
   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
      "%s:enter", __func__ );
   WDI_featureCapsExchangeReq( NULL, pVosContext);
}

/*                                         
                                                   
  */
void WDA_disableCapablityFeature(tANI_U8 feature_index)
{
   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
      "%s:enter", __func__ );
   WDI_disableCapablityFeature(feature_index);
}

 /*                                     
                                                                             
                                            
                 
                                            
                                                         
  */
tANI_U8 WDA_getHostWlanFeatCaps(tANI_U8 featEnumValue)
{
   return WDI_getHostWlanFeatCaps(featEnumValue);
}

 /*                                   
                                                                             
                                          
                 
                                            
                                                         
  */
tANI_U8 WDA_getFwWlanFeatCaps(tANI_U8 featEnumValue)
{
   return WDI_getFwWlanFeatCaps(featEnumValue);
}


/*
                         
                                                  
                        
 */
VOS_STATUS WDA_shutdown(v_PVOID_t pVosContext, wpt_boolean closeTransport)
{
   WDI_Status wdiStatus;
   //                        
   VOS_STATUS status = VOS_STATUS_SUCCESS;
   tWDA_CbContext *pWDA = (tWDA_CbContext *)VOS_GET_WDA_CTXT(pVosContext);
   if (NULL == pWDA)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
            "%s: Invoked with invalid pWDA", __func__ );
      VOS_ASSERT(0);
      return VOS_STATUS_E_FAILURE;
   }
   /*                           */
   if( (WDA_READY_STATE != pWDA->wdaState) &&
         (WDA_INIT_STATE != pWDA->wdaState) &&
         (WDA_START_STATE != pWDA->wdaState) )
   {
      VOS_ASSERT(0);
   }

   if ( (eDRIVER_TYPE_MFG != pWDA->driverMode) &&
        (VOS_TRUE == pWDA->wdaTimersCreated))
   {
      wdaDestroyTimers(pWDA);
      pWDA->wdaTimersCreated = VOS_FALSE;
   }
   else
   {
      vos_event_destroy(&pWDA->ftmStopDoneEvent);
  }

   /*                   */
   wdiStatus = WDI_Shutdown(closeTransport);
   if (IS_WDI_STATUS_FAILURE(wdiStatus) )
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                                  "error in WDA Stop" );
      status = VOS_STATUS_E_FAILURE;
   }
   /*                                                              */
   pWDA->wdaState = WDA_STOP_STATE;

   /*                                                   */
   /*                   */
   status = vos_event_destroy(&pWDA->txFrameEvent);
   if(!VOS_IS_STATUS_SUCCESS(status))
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                  "VOS Event destroy failed - status = %d", status);
      status = VOS_STATUS_E_FAILURE;
   }
   status = vos_event_destroy(&pWDA->suspendDataTxEvent);
   if(!VOS_IS_STATUS_SUCCESS(status))
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                  "VOS Event destroy failed - status = %d", status);
      status = VOS_STATUS_E_FAILURE;
   }
   status = vos_event_destroy(&pWDA->waitOnWdiIndicationCallBack);
   if(!VOS_IS_STATUS_SUCCESS(status))
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                  "VOS Event destroy failed - status = %d", status);
      status = VOS_STATUS_E_FAILURE;
   }
   /*                  */
   status = vos_free_context(pVosContext,VOS_MODULE_ID_WDA,pWDA);
   if ( !VOS_IS_STATUS_SUCCESS(status) )
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                                  "error in WDA close " );
      status = VOS_STATUS_E_FAILURE;
   }
   return status;
}

/*
                                
                                           
 */

void WDA_setNeedShutdown(v_PVOID_t pVosContext)
{
   tWDA_CbContext *pWDA = (tWDA_CbContext *)VOS_GET_WDA_CTXT(pVosContext);
   if(pWDA == NULL)
   {
       VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                         "Could not get the WDA Context pointer" );
       return;
   }
   pWDA->needShutdown  = TRUE;
}
/*
                             
                       
 */

v_BOOL_t WDA_needShutdown(v_PVOID_t pVosContext)
{
   tWDA_CbContext *pWDA = (tWDA_CbContext *)VOS_GET_WDA_CTXT(pVosContext);
   if(pWDA == NULL)
   {
       VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                         "Could not get the WDA Context pointer" );
       return 0;
   }
   return pWDA->needShutdown;
}

#ifdef WLAN_FEATURE_11AC
/*
                                           
   
 */
void WDA_SetUpdateOpModeReqCallback(WDI_Status status, void* pUserData)
{
   tWDA_ReqParams *pWdaParams = (tWDA_ReqParams *)pUserData;
   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                                          "<------ %s " ,__func__);
   if(NULL == pWdaParams)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
              "%s: pWdaParams received NULL", __func__);
      VOS_ASSERT(0) ;
      return ;
   }

   vos_mem_free(pWdaParams->wdaMsgParam) ;
   vos_mem_free(pWdaParams->wdaWdiApiMsgParam);
   vos_mem_free(pWdaParams) ;
   /* 
                                                                           
                
    */

   return ;
}

VOS_STATUS WDA_ProcessUpdateOpMode(tWDA_CbContext *pWDA, 
                                   tUpdateVHTOpMode *pData)
{
   WDI_Status status = WDI_STATUS_SUCCESS ;
   tWDA_ReqParams *pWdaParams ;
   WDI_UpdateVHTOpMode *wdiTemp = (WDI_UpdateVHTOpMode *)vos_mem_malloc(
                                             sizeof(WDI_UpdateVHTOpMode)) ;
   if(NULL == wdiTemp) 
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                           "%s: VOS MEM Alloc Failure", __func__); 
      VOS_ASSERT(0);
      return VOS_STATUS_E_NOMEM;
   }
   pWdaParams = (tWDA_ReqParams *)vos_mem_malloc(sizeof(tWDA_ReqParams)) ;
   if(NULL == pWdaParams)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                           "%s: VOS MEM Alloc Failure", __func__); 
      VOS_ASSERT(0);
      vos_mem_free(wdiTemp);
      return VOS_STATUS_E_NOMEM;
   }
   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                 "------> %s Opmode = %d and staid = %d" ,
                     __func__, pData->opMode, pData->staId);
   wdiTemp->opMode = pData->opMode;
   wdiTemp->staId  = pData->staId;
   
   pWdaParams->pWdaContext = pWDA;
   /*                                                      */
   pWdaParams->wdaMsgParam = (void *)pData;
   /*                             */
   pWdaParams->wdaWdiApiMsgParam = (void *)wdiTemp ;

   status = WDI_UpdateVHTOpModeReq( wdiTemp, (WDI_UpdateVHTOpModeCb) WDA_SetUpdateOpModeReqCallback, pWdaParams);

   if(IS_WDI_STATUS_FAILURE(status))
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
        "Failure in UPDATE VHT OP_MODE REQ Params WDI API, free all the memory " );
      vos_mem_free(pWdaParams->wdaWdiApiMsgParam) ;
      vos_mem_free(pWdaParams->wdaMsgParam);
      vos_mem_free(pWdaParams);
   }
   return CONVERT_WDI2VOS_STATUS(status) ;
} 
#endif

/*                                                                          
                                      

              
                                                   
                                                    
                                                            

            
                                    
                                                 
                                                  
                                                      
                                               
                                       

              
        

                                                                           */
void WDA_TransportChannelDebug
(
  tpAniSirGlobal pMac,
  v_BOOL_t       displaySnapshot,
  v_U8_t         debugFlags
)
{
   WDI_TransportChannelDebug(displaySnapshot, debugFlags);
   return;
}

/*                                                                          
                             

             
                                            

            
                                  

              
        

                                                                           */
void WDA_SetEnableSSR(v_BOOL_t enableSSR)
{
   WDI_SetEnableSSR(enableSSR);
}

#ifdef FEATURE_WLAN_LPHB
/*
                                    
  
 */
void WDA_LPHBconfRspCallback(WDI_Status status, void* pUserData)
{
   tWDA_ReqParams *pWdaParams = (tWDA_ReqParams *)pUserData;

   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
              "<------ %s " ,__func__);
   if (NULL == pWdaParams)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
              "%s: pWdaParams received NULL", __func__);
      VOS_ASSERT(0) ;
      return ;
   }

   /*                                                
                                    */
   if (pWdaParams != NULL)
   {
      if (pWdaParams->wdaWdiApiMsgParam != NULL)
      {
         vos_mem_free(pWdaParams->wdaWdiApiMsgParam);
      }
      vos_mem_free(pWdaParams->wdaMsgParam) ;
      vos_mem_free(pWdaParams) ;
   }

   return;
}

/*
                                   
  
 */
VOS_STATUS WDA_ProcessLPHBConfReq(tWDA_CbContext *pWDA,
                                  tSirLPHBReq *pData)
{
   WDI_Status wdiStatus;
   tWDA_ReqParams *pWdaParams ;

   VOS_TRACE(VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
             "------> %s " , __func__);

   pWdaParams = (tWDA_ReqParams *)vos_mem_malloc(sizeof(tWDA_ReqParams)) ;
   if (NULL == pWdaParams)
   {
      VOS_TRACE(VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                "%s: VOS MEM Alloc Failure", __func__);
      VOS_ASSERT(0);
      vos_mem_free(pData);
      return VOS_STATUS_E_NOMEM;
   }

   pWdaParams->pWdaContext = pWDA;
   pWdaParams->wdaMsgParam = (void *)pData;
   pWdaParams->wdaWdiApiMsgParam = NULL;

   wdiStatus = WDI_LPHBConfReq(pData, pWdaParams, WDA_LPHBconfRspCallback);
   if (WDI_STATUS_PENDING == wdiStatus)
   {
      VOS_TRACE(VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
              "Pending received for %s:%d ", __func__, __LINE__);
   }
   else if (WDI_STATUS_SUCCESS != wdiStatus)
   {
      VOS_TRACE(VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
              "Failure in %s:%d s %d", __func__, __LINE__, wdiStatus);
      vos_mem_free(pWdaParams->wdaMsgParam);
      vos_mem_free(pWdaParams);
   }

   return CONVERT_WDI2VOS_STATUS(wdiStatus);
}
#endif /*                   */

void WDA_GetBcnMissRateCallback(tANI_U8 status, tANI_U32 bcnMissRate,
                                void* pUserData)
{
   tSirBcnMissRateInfo *pBcnMissRateInfo = (tSirBcnMissRateInfo *)pUserData;

   VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
                                          "<------ %s " ,__func__);
   if (NULL == pBcnMissRateInfo)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                "%s: pWdaParams received NULL", __func__);
      VOS_ASSERT(0) ;
      return ;
   }
   if (pBcnMissRateInfo->callback)
   {
       pBcnMissRateInfo->callback(status, bcnMissRate,
                                  pBcnMissRateInfo->data);
   }
   vos_mem_free(pUserData);

   return;
}

v_VOID_t WDA_ProcessGetBcnMissRateReq(tWDA_CbContext *pWDA,
                                      tSirBcnMissRateReq *pData)
{
   WDI_Status wdiStatus;
   tSirBcnMissRateInfo *pBcnMissRateInfo;

   VOS_TRACE(VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
             "------> %s " , __func__);

   pBcnMissRateInfo =
              (tSirBcnMissRateInfo *)vos_mem_malloc(sizeof(tSirBcnMissRateInfo));
   if (NULL == pBcnMissRateInfo)
   {
      VOS_TRACE(VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                "%s: VOS MEM Alloc Failure", __func__);
      VOS_ASSERT(0);
      vos_mem_free(pData);
      return;
   }

   pBcnMissRateInfo->callback = (pGetBcnMissRateCB)(pData->callback);
   pBcnMissRateInfo->data     = pData->data;

   wdiStatus = WDI_GetBcnMissRate(pBcnMissRateInfo,
                                  WDA_GetBcnMissRateCallback,
                                  pData->bssid);
   if (WDI_STATUS_PENDING == wdiStatus)
   {
      VOS_TRACE(VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
              "Pending received for %s:%d ", __func__, __LINE__);
   }
   else if (WDI_STATUS_SUCCESS != wdiStatus)
   {
       if (pBcnMissRateInfo->callback)
       {
           pBcnMissRateInfo->callback(VOS_STATUS_E_FAILURE,
                                      -1, pBcnMissRateInfo->data);
       }
   }
   vos_mem_free(pData);
}

#ifdef WLAN_FEATURE_EXTSCAN

/*                                                                          
                                        

             
                                             

            
                                
             
                                                                           */
void WDA_EXTScanStartRspCallback(void *pEventData, void* pUserData)
{
    tWDA_ReqParams *pWdaParams = (tWDA_ReqParams *)pUserData;
    tWDA_CbContext *pWDA = NULL;
    void *pCallbackContext;
    tpAniSirGlobal pMac;

    VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
            "%s:", __func__);
    if (NULL == pWdaParams)
    {
        VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                "%s: pWdaParams received NULL", __func__);
        VOS_ASSERT(0);
        return;
    }

    pWDA = (tWDA_CbContext *) pWdaParams->pWdaContext;

    if (NULL == pWDA)
    {
        VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                "%s: pWDA received NULL", __func__);
        VOS_ASSERT(0);
        goto error;
    }

    pMac = (tpAniSirGlobal )VOS_GET_MAC_CTXT(pWDA->pVosContext);
    if (NULL == pMac)
    {
        VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                "%s:pMac is NULL", __func__);
        VOS_ASSERT(0);
        goto error;
    }

    pCallbackContext = pMac->sme.pEXTScanCallbackContext;

    if (pMac->sme.pEXTScanIndCb)
    {
        pMac->sme.pEXTScanIndCb(pCallbackContext, WDA_EXTSCAN_START_RSP,
                pEventData);
    }
    else
    {
        VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                "%s:HDD callback is null", __func__);
        VOS_ASSERT(0);
    }

error:

    if (pWdaParams->wdaWdiApiMsgParam != NULL)
    {
        vos_mem_free(pWdaParams->wdaWdiApiMsgParam);
    }
    if (pWdaParams->wdaMsgParam != NULL)
    {
        vos_mem_free(pWdaParams->wdaMsgParam);
    }
    vos_mem_free(pWdaParams) ;

    return;
}

/*                                                                          
                                       

             
                                            

            
                                
             
                                                                           */
void WDA_EXTScanStopRspCallback(void *pEventData, void* pUserData)
{
    tWDA_ReqParams *pWdaParams = (tWDA_ReqParams *)pUserData;
    tWDA_CbContext *pWDA = NULL;
    void *pCallbackContext;
    tpAniSirGlobal pMac;

    VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
            "%s:", __func__);
    if (NULL == pWdaParams)
    {
        VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                "%s: pWdaParams received NULL", __func__);
        VOS_ASSERT(0);
        return;
    }

    pWDA = (tWDA_CbContext *) pWdaParams->pWdaContext;

    if (NULL == pWDA)
    {
        VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                "%s: pWDA received NULL", __func__);
        VOS_ASSERT(0);
        goto error;
    }

    pMac = (tpAniSirGlobal )VOS_GET_MAC_CTXT(pWDA->pVosContext);
    if (NULL == pMac)
    {
        VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                "%s:pMac is NULL", __func__);
        VOS_ASSERT(0);
        goto error;
    }
    pCallbackContext = pMac->sme.pEXTScanCallbackContext;

    if (pMac->sme.pEXTScanIndCb)
    {
        VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                "%s:HDD call back function called", __func__);
        pMac->sme.pEXTScanIndCb(pCallbackContext, WDA_EXTSCAN_STOP_RSP,
                pEventData);
    }
    else
    {
        VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                "%s:HDD callback is null", __func__);
        VOS_ASSERT(0);
    }

error:

    if (pWdaParams->wdaWdiApiMsgParam != NULL)
    {
        vos_mem_free(pWdaParams->wdaWdiApiMsgParam);
    }
    if (pWdaParams->wdaMsgParam != NULL)
    {
        vos_mem_free(pWdaParams->wdaMsgParam);
    }
    vos_mem_free(pWdaParams) ;


    return;
}

/*                                                                          
                                                   

             
                                                          

            
                                
             
                                                                           */
void WDA_EXTScanGetCachedResultsRspCallback(void *pEventData, void* pUserData)
{
    tWDA_ReqParams *pWdaParams = (tWDA_ReqParams *)pUserData;
    tWDA_CbContext *pWDA = NULL;
    void *pCallbackContext;
    tpAniSirGlobal pMac;

    VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
            "%s: ", __func__);
    if (NULL == pWdaParams)
    {
        VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                "%s: pWdaParams received NULL", __func__);
        VOS_ASSERT(0);
        return;
    }

    pWDA = (tWDA_CbContext *) pWdaParams->pWdaContext;

    if (NULL == pWDA)
    {
        VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                "%s: pWDA received NULL", __func__);
        VOS_ASSERT(0);
        goto error;
    }

    pMac = (tpAniSirGlobal )VOS_GET_MAC_CTXT(pWDA->pVosContext);
    if (NULL == pMac)
    {
        VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                "%s:pMac is NULL", __func__);
        VOS_ASSERT(0);
        goto error;
    }

    pCallbackContext = pMac->sme.pEXTScanCallbackContext;

    if (pMac->sme.pEXTScanIndCb)
    {
        pMac->sme.pEXTScanIndCb(pCallbackContext,
                WDA_EXTSCAN_GET_CACHED_RESULTS_RSP,
                pEventData);
    }
    else
    {
        VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                "%s:HDD callback is null", __func__);
        VOS_ASSERT(0);
    }


error:

    if (pWdaParams->wdaWdiApiMsgParam != NULL)
    {
        vos_mem_free(pWdaParams->wdaWdiApiMsgParam);
    }
    if (pWdaParams->wdaMsgParam != NULL)
    {
        vos_mem_free(pWdaParams->wdaMsgParam);
    }
    vos_mem_free(pWdaParams) ;

    return;
}

/*                                                                          
                                                  

             
                                                        

            
                                
             
                                                                           */
void WDA_EXTScanGetCapabilitiesRspCallback(void *pEventData, void* pUserData)
{
    tWDA_ReqParams *pWdaParams = (tWDA_ReqParams *)pUserData;
    tWDA_CbContext *pWDA = NULL;
    void *pCallbackContext;
    tpAniSirGlobal pMac;

    VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
            "%s:", __func__);
    if (NULL == pWdaParams)
    {
        VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                "%s: pWdaParams received NULL", __func__);
        VOS_ASSERT(0);
        return;
    }

    pWDA = (tWDA_CbContext *) pWdaParams->pWdaContext;

    if (NULL == pWDA)
    {
        VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                "%s: pWDA received NULL", __func__);
        VOS_ASSERT(0);
        goto error;
    }

    pMac = (tpAniSirGlobal )VOS_GET_MAC_CTXT(pWDA->pVosContext);
    if (NULL == pMac)
    {
        VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                "%s:pMac is NULL", __func__);
        VOS_ASSERT(0);
        goto error;
    }

    pCallbackContext = pMac->sme.pEXTScanCallbackContext;

    if (pMac->sme.pEXTScanIndCb)
    {
        pMac->sme.pEXTScanIndCb(pCallbackContext,
                WDA_EXTSCAN_GET_CAPABILITIES_RSP,
                pEventData);
    }
    else
    {
        VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                "%s:HDD callback is null", __func__);
        VOS_ASSERT(0);
    }


error:

    if (pWdaParams->wdaWdiApiMsgParam != NULL)
    {
        vos_mem_free(pWdaParams->wdaWdiApiMsgParam);
    }
    if (pWdaParams->wdaMsgParam != NULL)
    {
        vos_mem_free(pWdaParams->wdaMsgParam);
    }
    vos_mem_free(pWdaParams) ;

    return;
}

/*                                                                          
                                                  

             
                                                         

            
                                
             
                                                                           */
void WDA_EXTScanSetBSSIDHotlistRspCallback(void *pEventData, void* pUserData)
{
    tWDA_ReqParams *pWdaParams = (tWDA_ReqParams *)pUserData;
    tWDA_CbContext *pWDA = NULL;
    void *pCallbackContext;
    tpAniSirGlobal pMac;

    VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
            "%s: ", __func__);
    if (NULL == pWdaParams)
    {
        VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                "%s: pWdaParams received NULL", __func__);
        VOS_ASSERT(0) ;
        return;
    }

    pWDA = (tWDA_CbContext *) pWdaParams->pWdaContext;

    if (NULL == pWDA)
    {
        VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                "%s: pWDA received NULL", __func__);
        VOS_ASSERT(0);
        goto error;
    }

    pMac = (tpAniSirGlobal )VOS_GET_MAC_CTXT(pWDA->pVosContext);
    if (NULL == pMac)
    {
        VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                "%s:pMac is NULL", __func__);
        VOS_ASSERT(0);
        goto error;
    }

    pCallbackContext = pMac->sme.pEXTScanCallbackContext;

    if (pMac->sme.pEXTScanIndCb)
    {
        pMac->sme.pEXTScanIndCb(pCallbackContext,
                WDA_EXTSCAN_SET_BSSID_HOTLIST_RSP,
                pEventData);
    }
    else
    {
        VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                "%s:HDD callback is null", __func__);
        VOS_ASSERT(0);
    }


error:

    if (pWdaParams->wdaWdiApiMsgParam != NULL)
    {
        vos_mem_free(pWdaParams->wdaWdiApiMsgParam);
    }
    if (pWdaParams->wdaMsgParam != NULL)
    {
        vos_mem_free(pWdaParams->wdaMsgParam);
    }
    vos_mem_free(pWdaParams) ;

    return;
}

/*                                                                          
                                                    

             
                                                           

            
                                
             
                                                                           */
void WDA_EXTScanResetBSSIDHotlistRspCallback(void *pEventData, void* pUserData)
{
    tWDA_ReqParams *pWdaParams = (tWDA_ReqParams *)pUserData;
    tWDA_CbContext *pWDA = NULL;
    void *pCallbackContext;
    tpAniSirGlobal pMac;

    VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
            "%s:", __func__);
    if (NULL == pWdaParams)
    {
        VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                "%s: pWdaParams received NULL", __func__);
        VOS_ASSERT(0) ;
        return;
    }

    pWDA = (tWDA_CbContext *) pWdaParams->pWdaContext;

    if (NULL == pWDA)
    {
        VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                "%s: pWDA received NULL", __func__);
        VOS_ASSERT(0);
        goto error;
    }

    pMac = (tpAniSirGlobal )VOS_GET_MAC_CTXT(pWDA->pVosContext);
    if (NULL == pMac)
    {
        VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                "%s:pMac is NULL", __func__);
        VOS_ASSERT(0);
        goto error;
    }

    pCallbackContext = pMac->sme.pEXTScanCallbackContext;

    if (pMac->sme.pEXTScanIndCb)
    {
        pMac->sme.pEXTScanIndCb(pCallbackContext,
                WDA_EXTSCAN_RESET_BSSID_HOTLIST_RSP,
                pEventData);
    }
    else
    {
        VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                "%s:HDD callback is null", __func__);
        VOS_ASSERT(0);
    }


error:

    if (pWdaParams->wdaWdiApiMsgParam != NULL)
    {
        vos_mem_free(pWdaParams->wdaWdiApiMsgParam);
    }
    if (pWdaParams->wdaMsgParam != NULL)
    {
        vos_mem_free(pWdaParams->wdaMsgParam);
    }
    vos_mem_free(pWdaParams) ;

    return;
}

/*                                                                          
                                                     

             
                                                              

            
                                
             
                                                                           */
void WDA_EXTScanSetSignfRSSIChangeRspCallback(void *pEventData, void* pUserData)
{
    tWDA_ReqParams *pWdaParams = (tWDA_ReqParams *)pUserData;
    tWDA_CbContext *pWDA = NULL;
    void *pCallbackContext;
    tpAniSirGlobal pMac;

    VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
            "%s:", __func__);
    if (NULL == pWdaParams)
    {
        VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                "%s: pWdaParams received NULL", __func__);
        VOS_ASSERT(0) ;
        return;
    }

    pWDA = (tWDA_CbContext *) pWdaParams->pWdaContext;

    if (NULL == pWDA)
    {
        VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                "%s: pWDA received NULL", __func__);
        VOS_ASSERT(0);
        goto error;
    }

    pMac = (tpAniSirGlobal )VOS_GET_MAC_CTXT(pWDA->pVosContext);
    if (NULL == pMac)
    {
        VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                "%s:pMac is NULL", __func__);
        VOS_ASSERT(0);
        goto error;
    }

    pCallbackContext = pMac->sme.pEXTScanCallbackContext;

    if (pMac->sme.pEXTScanIndCb)
    {
        pMac->sme.pEXTScanIndCb(pCallbackContext,
                WDA_EXTSCAN_SET_SIGNF_RSSI_CHANGE_RSP,
                pEventData);
    }
    else
    {
        VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                "%s:HDD callback is null", __func__);
        VOS_ASSERT(0);
    }


error:

    if (pWdaParams->wdaWdiApiMsgParam != NULL)
    {
        vos_mem_free(pWdaParams->wdaWdiApiMsgParam);
    }
    if (pWdaParams->wdaMsgParam != NULL)
    {
        vos_mem_free(pWdaParams->wdaMsgParam);
    }
    vos_mem_free(pWdaParams) ;

    return;
}

/*                                                                          
                                                       

             
                                                              

            
                                
             
                                                                           */
void WDA_EXTScanResetSignfRSSIChangeRspCallback(void *pEventData,
        void* pUserData)
{
    tWDA_ReqParams *pWdaParams = (tWDA_ReqParams *)pUserData;
    tWDA_CbContext *pWDA = NULL;
    void *pCallbackContext;
    tpAniSirGlobal pMac;

    VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
            "%s:", __func__);
    if (NULL == pWdaParams)
    {
        VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                "%s: pWdaParams received NULL", __func__);
        VOS_ASSERT(0) ;
        return;
    }

    pWDA = (tWDA_CbContext *) pWdaParams->pWdaContext;

    if (NULL == pWDA)
    {
        VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                "%s: pWDA received NULL", __func__);
        VOS_ASSERT(0);
        goto error;
    }

    pMac = (tpAniSirGlobal )VOS_GET_MAC_CTXT(pWDA->pVosContext);
    if (NULL == pMac)
    {
        VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                "%s:pMac is NULL", __func__);
        VOS_ASSERT(0);
        goto error;
    }

    pCallbackContext = pMac->sme.pEXTScanCallbackContext;

    if (pMac->sme.pEXTScanIndCb)
    {
        pMac->sme.pEXTScanIndCb(pCallbackContext,
                WDA_EXTSCAN_RESET_SIGNF_RSSI_CHANGE_RSP,
                pEventData);
    }
    else
    {
        VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                "%s:HDD callback is null", __func__);
        VOS_ASSERT(0);
    }


error:

    if (pWdaParams->wdaWdiApiMsgParam != NULL)
    {
        vos_mem_free(pWdaParams->wdaWdiApiMsgParam);
    }
    if (pWdaParams->wdaMsgParam != NULL)
    {
        vos_mem_free(pWdaParams->wdaMsgParam);
    }
    vos_mem_free(pWdaParams) ;

    return;
}

/*                                                                          
                                       

             
                                            

            
                                
                                                 
                                                                           */
VOS_STATUS WDA_ProcessEXTScanStartReq(tWDA_CbContext *pWDA,
        tSirEXTScanStartReqParams *wdaRequest)
{
    WDI_Status status = WDI_STATUS_SUCCESS;
    tWDA_ReqParams *pWdaParams;

    VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
            "%s: ", __func__);
    pWdaParams = (tWDA_ReqParams *)vos_mem_malloc(sizeof(tWDA_ReqParams));
    if (NULL == pWdaParams)
    {
        VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                "%s: VOS MEM Alloc Failure", __func__);
        VOS_ASSERT(0);
        return VOS_STATUS_E_NOMEM;
    }
    pWdaParams->pWdaContext = pWDA;
    pWdaParams->wdaMsgParam = wdaRequest;
    pWdaParams->wdaWdiApiMsgParam = NULL;

    status = WDI_EXTScanStartReq((void *)wdaRequest,
            (WDI_EXTScanStartRspCb)WDA_EXTScanStartRspCallback,
            (void *)pWdaParams);
    if (IS_WDI_STATUS_FAILURE(status))
    {
        VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                "Failure to request.  Free all the memory " );
        vos_mem_free(pWdaParams->wdaMsgParam);
        vos_mem_free(pWdaParams);
    }
    return CONVERT_WDI2VOS_STATUS(status);
}

/*                                                                          
                                      

             
                                            

            
                                
                                                 
                                                                           */
VOS_STATUS WDA_ProcessEXTScanStopReq(tWDA_CbContext *pWDA,
        tSirEXTScanStopReqParams *wdaRequest)
{
    WDI_Status status = WDI_STATUS_SUCCESS;
    tWDA_ReqParams *pWdaParams;

    VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
            "%s:", __func__);
    pWdaParams = (tWDA_ReqParams *)vos_mem_malloc(sizeof(tWDA_ReqParams));
    if (NULL == pWdaParams)
    {
        VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                "%s: VOS MEM Alloc Failure", __func__);
        VOS_ASSERT(0);
        return VOS_STATUS_E_NOMEM;
    }
    pWdaParams->pWdaContext = pWDA;
    pWdaParams->wdaMsgParam = wdaRequest;
    pWdaParams->wdaWdiApiMsgParam = NULL;

    status = WDI_EXTScanStopReq((void *)wdaRequest,
            (WDI_EXTScanStopRspCb)WDA_EXTScanStopRspCallback,
            (void *)pWdaParams);
    if (IS_WDI_STATUS_FAILURE(status))
    {
        VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                "Failure to request.  Free all the memory " );
        vos_mem_free(pWdaParams->wdaMsgParam);
        vos_mem_free(pWdaParams);
    }
    return CONVERT_WDI2VOS_STATUS(status);
}

/*                                                                          
                                                  

             
                                                         

            
                                
                                                 
                                                                           */
VOS_STATUS WDA_ProcessEXTScanGetCachedResultsReq(tWDA_CbContext *pWDA,
        tSirEXTScanGetCachedResultsReqParams *wdaRequest)
{
    WDI_Status status = WDI_STATUS_SUCCESS;
    tWDA_ReqParams *pWdaParams;

    VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
            "%s: ", __func__);
    pWdaParams = (tWDA_ReqParams *)vos_mem_malloc(sizeof(tWDA_ReqParams));
    if (NULL == pWdaParams)
    {
        VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                "%s: VOS MEM Alloc Failure", __func__);
        VOS_ASSERT(0);
        return VOS_STATUS_E_NOMEM;
    }
    pWdaParams->pWdaContext = pWDA;
    pWdaParams->wdaMsgParam = wdaRequest;
    pWdaParams->wdaWdiApiMsgParam = NULL;

    status = WDI_EXTScanGetCachedResultsReq((void *)wdaRequest,
     (WDI_EXTScanGetCachedResultsRspCb)WDA_EXTScanGetCachedResultsRspCallback,
            (void *)pWdaParams);
    if (IS_WDI_STATUS_FAILURE(status))
    {
        VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                "Failure to request.  Free all the memory " );
        vos_mem_free(pWdaParams->wdaMsgParam);
        vos_mem_free(pWdaParams);
    }
    return CONVERT_WDI2VOS_STATUS(status);
}

/*                                                                          
                                                 

             
                                                       

            
                                
                                                 
                                                                           */
VOS_STATUS WDA_ProcessEXTScanGetCapabilitiesReq(tWDA_CbContext *pWDA,
        tSirGetEXTScanCapabilitiesReqParams *wdaRequest)
{
    WDI_Status status = WDI_STATUS_SUCCESS;
    tWDA_ReqParams *pWdaParams;

    VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
            "%s:", __func__);
    pWdaParams = (tWDA_ReqParams *)vos_mem_malloc(sizeof(tWDA_ReqParams));
    if (NULL == pWdaParams)
    {
        VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                "%s: VOS MEM Alloc Failure", __func__);
        VOS_ASSERT(0);
        return VOS_STATUS_E_NOMEM;
    }
    pWdaParams->pWdaContext = pWDA;
    pWdaParams->wdaMsgParam = wdaRequest;
    pWdaParams->wdaWdiApiMsgParam = NULL;

    status = WDI_EXTScanGetCapabilitiesReq((void *)wdaRequest,
         (WDI_EXTScanGetCapabilitiesRspCb)WDA_EXTScanGetCapabilitiesRspCallback,
            (void *)pWdaParams);
    if (IS_WDI_STATUS_FAILURE(status))
    {
        VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                "Failure to request.  Free all the memory " );
        vos_mem_free(pWdaParams->wdaMsgParam);
        vos_mem_free(pWdaParams);
    }
    return CONVERT_WDI2VOS_STATUS(status);
}

/*                                                                          
                                                 

             
                                                

            
                                
                                                 
                                                                           */
VOS_STATUS WDA_ProcessEXTScanSetBSSIDHotlistReq(tWDA_CbContext *pWDA,
        tSirEXTScanSetBssidHotListReqParams *wdaRequest)
{
    WDI_Status status = WDI_STATUS_SUCCESS;
    tWDA_ReqParams *pWdaParams;

    VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
            "%s: ", __func__);
    pWdaParams = (tWDA_ReqParams *)vos_mem_malloc(sizeof(tWDA_ReqParams));
    if (NULL == pWdaParams)
    {
        VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                "%s: VOS MEM Alloc Failure", __func__);
        VOS_ASSERT(0);
        return VOS_STATUS_E_NOMEM;
    }
    pWdaParams->pWdaContext = pWDA;
    pWdaParams->wdaMsgParam = wdaRequest;
    pWdaParams->wdaWdiApiMsgParam = NULL;

    status = WDI_EXTScanSetBSSIDHotlistReq((void *)wdaRequest,
         (WDI_EXTScanSetBSSIDHotlistRspCb)WDA_EXTScanSetBSSIDHotlistRspCallback,
            (void *)pWdaParams);
    if (IS_WDI_STATUS_FAILURE(status))
    {
        VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                "Failure to request.  Free all the memory " );
        vos_mem_free(pWdaParams->wdaMsgParam);
        vos_mem_free(pWdaParams);
    }
    return CONVERT_WDI2VOS_STATUS(status);
}

/*                                                                          
                                                   

             
                                                  

            
                                
                                                 
                                                                           */
VOS_STATUS WDA_ProcessEXTScanResetBSSIDHotlistReq(tWDA_CbContext *pWDA,
        tSirEXTScanResetBssidHotlistReqParams *wdaRequest)
{
    WDI_Status status = WDI_STATUS_SUCCESS;
    tWDA_ReqParams *pWdaParams;

    VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
            "%s:", __func__);
    pWdaParams = (tWDA_ReqParams *)vos_mem_malloc(sizeof(tWDA_ReqParams));
    if (NULL == pWdaParams)
    {
        VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                "%s: VOS MEM Alloc Failure", __func__);
        VOS_ASSERT(0);
        return VOS_STATUS_E_NOMEM;
    }
    pWdaParams->pWdaContext = pWDA;
    pWdaParams->wdaMsgParam = wdaRequest;
    pWdaParams->wdaWdiApiMsgParam = NULL;

    status = WDI_EXTScanResetBSSIDHotlistReq((void *)wdaRequest,
     (WDI_EXTScanResetBSSIDHotlistRspCb)WDA_EXTScanResetBSSIDHotlistRspCallback,
            (void *)pWdaParams);
    if (IS_WDI_STATUS_FAILURE(status))
    {
        VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                "Failure to request.  Free all the memory " );
        vos_mem_free(pWdaParams->wdaMsgParam);
        vos_mem_free(pWdaParams);
    }
    return CONVERT_WDI2VOS_STATUS(status);
}

/*                                                                          
                                                    

             
                                                          

            
                                
                                                 
                                                                           */
VOS_STATUS WDA_ProcessEXTScanSetSignfRSSIChangeReq(tWDA_CbContext *pWDA,
        tSirEXTScanSetSignificantChangeReqParams *wdaRequest)
{
    WDI_Status status = WDI_STATUS_SUCCESS;
    tWDA_ReqParams *pWdaParams;

    VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
            "%s: ", __func__);
    pWdaParams = (tWDA_ReqParams *)vos_mem_malloc(sizeof(tWDA_ReqParams));
    if (NULL == pWdaParams)
    {
        VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                "%s: VOS MEM Alloc Failure", __func__);
        VOS_ASSERT(0);
        return VOS_STATUS_E_NOMEM;
    }
    pWdaParams->pWdaContext = pWDA;
    pWdaParams->wdaMsgParam = wdaRequest;
    pWdaParams->wdaWdiApiMsgParam = NULL;

    status = WDI_EXTScanSetSignfRSSIChangeReq((void *)wdaRequest,
    (WDI_EXTScanSetSignfRSSIChangeRspCb)WDA_EXTScanSetSignfRSSIChangeRspCallback,
            (void *)pWdaParams);
    if (IS_WDI_STATUS_FAILURE(status))
    {
        VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                "Failure to request.  Free all the memory " );
        vos_mem_free(pWdaParams->wdaMsgParam);
        vos_mem_free(pWdaParams);
    }
    return CONVERT_WDI2VOS_STATUS(status);
}

/*                                                                          
                                                      

             
                                                            

            
                                
                                                 
                                                                           */
VOS_STATUS WDA_ProcessEXTScanResetSignfRSSIChangeReq(tWDA_CbContext *pWDA,
        tSirEXTScanResetSignificantChangeReqParams *wdaRequest)
{
    WDI_Status status = WDI_STATUS_SUCCESS;
    tWDA_ReqParams *pWdaParams;

    VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_INFO,
            "%s:", __func__);
    pWdaParams = (tWDA_ReqParams *)vos_mem_malloc(sizeof(tWDA_ReqParams));
    if (NULL == pWdaParams)
    {
        VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                "%s: VOS MEM Alloc Failure", __func__);
        VOS_ASSERT(0);
        return VOS_STATUS_E_NOMEM;
    }
    pWdaParams->pWdaContext = pWDA;
    pWdaParams->wdaMsgParam = wdaRequest;
    pWdaParams->wdaWdiApiMsgParam = NULL;

    status = WDI_EXTScanResetSignfRSSIChangeReq((void *)wdaRequest,
            (WDI_EXTScanResetSignfRSSIChangeRspCb)
            WDA_EXTScanResetSignfRSSIChangeRspCallback,
            (void *)pWdaParams);
    if (IS_WDI_STATUS_FAILURE(status))
    {
        VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                "Failure to request.  Free all the memory " );
        vos_mem_free(pWdaParams->wdaMsgParam);
        vos_mem_free(pWdaParams);
    }
    return CONVERT_WDI2VOS_STATUS(status);
}
#endif /*                      */

#ifdef WLAN_FEATURE_LINK_LAYER_STATS

/*                                                                          
                                    

             
                                                             

            
                                                       
                                   

              
        

                                                                           */
void WDA_LLStatsSetRspCallback(void *pEventData, void* pUserData)
{
   tWDA_ReqParams *pWdaParams = (tWDA_ReqParams *)pUserData;


   if (NULL == pWdaParams)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
              "%s: pWdaParams received NULL", __func__);
      VOS_ASSERT(0) ;
      return ;
   }

   /*                                                
                                    */
   if (pWdaParams->wdaWdiApiMsgParam != NULL)
   {
      vos_mem_free(pWdaParams->wdaWdiApiMsgParam);
   }
   if (pWdaParams->wdaMsgParam != NULL)
   {
      vos_mem_free(pWdaParams->wdaMsgParam);
   }
   vos_mem_free(pWdaParams) ;

   return;
}

/*                                                                          
                                     

             
                                                   

            
                                
                                                              
                                                                           */
VOS_STATUS WDA_ProcessLLStatsSetReq(tWDA_CbContext *pWDA,
                                    tSirLLStatsSetReq *wdaRequest)
{
    WDI_Status status = WDI_STATUS_SUCCESS;
    tWDA_ReqParams *pWdaParams;

    pWdaParams = (tWDA_ReqParams *)vos_mem_malloc(sizeof(tWDA_ReqParams));
    if (NULL == pWdaParams)
    {
        VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                   "%s: VOS MEM Alloc Failure", __func__);
        VOS_ASSERT(0);
        return VOS_STATUS_E_NOMEM;
    }
    pWdaParams->pWdaContext = pWDA;
    pWdaParams->wdaMsgParam = wdaRequest;
    pWdaParams->wdaWdiApiMsgParam = NULL;

    status = WDI_LLStatsSetReq((void *)wdaRequest,
                               (WDI_LLStatsSetRspCb)WDA_LLStatsSetRspCallback,
                               (void *)pWdaParams);
    if (IS_WDI_STATUS_FAILURE(status))
    {
        VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                   "Failure to request.  Free all the memory " );
        vos_mem_free(pWdaParams->wdaMsgParam);
        vos_mem_free(pWdaParams);
    }
    return CONVERT_WDI2VOS_STATUS(status);
}

/*                                                                          
                                    

             
                                                             

            
                                                       
                                   

              
        

                                                                           */
void WDA_LLStatsGetRspCallback(void *pEventData, void* pUserData)
{
   tWDA_ReqParams *pWdaParams = (tWDA_ReqParams *)pUserData;

   if (NULL == pWdaParams)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
              "%s: pWdaParams received NULL", __func__);
      VOS_ASSERT(0) ;
      return ;
   }

   /*                                                
                                    */
   if (pWdaParams->wdaWdiApiMsgParam != NULL)
   {
      vos_mem_free(pWdaParams->wdaWdiApiMsgParam);
   }
   if (pWdaParams->wdaMsgParam != NULL)
   {
      vos_mem_free(pWdaParams->wdaMsgParam);
   }
   vos_mem_free(pWdaParams) ;

   return;
}

/*                                                                          
                                     

             
                                                   

            
                                
                                                              
                                                                           */
VOS_STATUS WDA_ProcessLLStatsGetReq(tWDA_CbContext *pWDA,
                                    tSirLLStatsGetReq *wdaRequest)
{
    WDI_Status status = WDI_STATUS_SUCCESS;
    tWDA_ReqParams *pWdaParams;

    pWdaParams = (tWDA_ReqParams *)vos_mem_malloc(sizeof(tWDA_ReqParams));
    if (NULL == pWdaParams)
    {
        VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                   "%s: VOS MEM Alloc Failure", __func__);
        VOS_ASSERT(0);
        return VOS_STATUS_E_NOMEM;
    }
    pWdaParams->pWdaContext = pWDA;
    pWdaParams->wdaMsgParam = wdaRequest;
    pWdaParams->wdaWdiApiMsgParam = NULL;

    status = WDI_LLStatsGetReq((void *) wdaRequest,
                               (WDI_LLStatsGetRspCb)WDA_LLStatsGetRspCallback,
                               (void *)pWdaParams);
    if (IS_WDI_STATUS_FAILURE(status))
    {
        VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                   "Failure to request.  Free all the memory " );
        vos_mem_free(pWdaParams->wdaMsgParam);
        vos_mem_free(pWdaParams);
    }
    return CONVERT_WDI2VOS_STATUS(status);
}

/*                                                                          
                                      

             
                                                               

            
                                                         
                                   

              
        

                                                                           */
void WDA_LLStatsClearRspCallback(void *pEventData, void* pUserData)
{
   tWDA_ReqParams *pWdaParams = (tWDA_ReqParams *)pUserData;


   if (NULL == pWdaParams)
   {
      VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
              "%s: pWdaParams received NULL", __func__);
      VOS_ASSERT(0) ;
      return ;
   }
   /*                                                
                                    */
   if (pWdaParams->wdaWdiApiMsgParam != NULL)
   {
      vos_mem_free(pWdaParams->wdaWdiApiMsgParam);
   }
   if (pWdaParams->wdaMsgParam != NULL)
   {
      vos_mem_free(pWdaParams->wdaMsgParam);
   }
   vos_mem_free(pWdaParams) ;
   return;
}

/*                                                                          
                                       

             
                                                     

            
                                
                                                  
                                                                           */
VOS_STATUS WDA_ProcessLLStatsClearReq(tWDA_CbContext *pWDA,
                                      tSirLLStatsClearReq *wdaRequest)
{
    WDI_Status status = WDI_STATUS_SUCCESS;
    tWDA_ReqParams *pWdaParams;

    pWdaParams = (tWDA_ReqParams *)vos_mem_malloc(sizeof(tWDA_ReqParams));
    if (NULL == pWdaParams)
    {
        VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                   "%s: VOS MEM Alloc Failure", __func__);
        VOS_ASSERT(0);
        return VOS_STATUS_E_NOMEM;
    }
    pWdaParams->pWdaContext = pWDA;
    pWdaParams->wdaMsgParam = wdaRequest;
    pWdaParams->wdaWdiApiMsgParam = NULL;

    status = WDI_LLStatsClearReq((void *)  wdaRequest,
                           (WDI_LLStatsClearRspCb)WDA_LLStatsClearRspCallback,
                           (void *)pWdaParams);
    if (IS_WDI_STATUS_FAILURE(status))
    {
        VOS_TRACE( VOS_MODULE_ID_WDA, VOS_TRACE_LEVEL_ERROR,
                   "Failure to request.  Free all the memory " );
        vos_mem_free(pWdaParams->wdaMsgParam);
        vos_mem_free(pWdaParams);
    }
    return CONVERT_WDI2VOS_STATUS(status);
}

#endif /*                               */