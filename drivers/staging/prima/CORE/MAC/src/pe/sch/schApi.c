/*
 * Copyright (c) 2012-2013 The Linux Foundation. All rights reserved.
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

/*
  
                                                                    
                      
  
                            
                        
            
                                                          
                                                                       
  
 */
#include "palTypes.h"
#include "aniGlobal.h"
#include "wniCfgSta.h"

#include "sirMacProtDef.h"
#include "sirMacPropExts.h"
#include "sirCommon.h"


#include "cfgApi.h"
#include "pmmApi.h"

#include "limApi.h"

#include "schApi.h"
#include "schDebug.h"

#include "schSysParams.h"
#include "limTrace.h"
#include "limTypes.h"

#include "wlan_qct_wda.h"

//                                                                    
//
//                                          
//
//                                                                   
static tANI_U8 gSchProbeRspTemplate[SCH_MAX_PROBE_RESP_SIZE];
static tANI_U8 gSchBeaconFrameBegin[SCH_MAX_BEACON_SIZE];
static tANI_U8 gSchBeaconFrameEnd[SCH_MAX_BEACON_SIZE];

//                                                                     
/* 
                 
  
            
                                                         
  
         
  
               
  
        
  
              
               
 */

tANI_U8
schGetCFPCount(tpAniSirGlobal pMac)
{
    return pMac->sch.schObject.gSchCFPCount;
}

//                                                                     
/* 
                        
  
            
                                                                      
  
         
  
               
  
        
  
              
               
 */

tANI_U16
schGetCFPDurRemaining(tpAniSirGlobal pMac)
{
    return pMac->sch.schObject.gSchCFPDurRemaining;
}


//                                                                     
/* 
                
  
            
             
  
         
  
               
  
        
  
              
               
 */

void
schInitialize(tpAniSirGlobal pMac)
{
    pmmInitialize(pMac);
}

//                                                                     
/* 
                 
  
            
                     
  
         
  
               
  
        
  
              
               
 */

void
schInitGlobals(tpAniSirGlobal pMac)
{
    pMac->sch.gSchHcfEnabled = false;

    pMac->sch.gSchScanRequested = false;
    pMac->sch.gSchScanReqRcvd = false;

    pMac->sch.gSchGenBeacon = 1;
    pMac->sch.gSchBeaconsSent = 0;
    pMac->sch.gSchBeaconsWritten = 0;
    pMac->sch.gSchBcnParseErrorCnt = 0;
    pMac->sch.gSchBcnIgnored = 0;
    pMac->sch.gSchBBXportRcvCnt = 0;
    pMac->sch.gSchUnknownRcvCnt = 0;
    pMac->sch.gSchBcnRcvCnt = 0;
    pMac->sch.gSchRRRcvCnt = 0;
    pMac->sch.qosNullCnt = 0;
    pMac->sch.numData = 0;
    pMac->sch.numPoll = 0;
    pMac->sch.numCorrupt = 0;
    pMac->sch.numBogusInt = 0;
    pMac->sch.numTxAct0 = 0;
    pMac->sch.rrTimeout = SCH_RR_TIMEOUT;
    pMac->sch.pollPeriod = SCH_POLL_PERIOD;
    pMac->sch.keepAlive = 0;
    pMac->sch.multipleSched = 1;
    pMac->sch.maxPollTimeouts = 20;
    pMac->sch.checkCfbFlagStuck = 0;

    pMac->sch.schObject.gSchProbeRspTemplate = gSchProbeRspTemplate;
    pMac->sch.schObject.gSchBeaconFrameBegin = gSchBeaconFrameBegin;
    pMac->sch.schObject.gSchBeaconFrameEnd   = gSchBeaconFrameEnd;

}

//                                                                     
/* 
                 
  
            
                                                         
  
         
  
               
  
        
  
                                 
               
 */

tSirRetStatus
schPostMessage(tpAniSirGlobal pMac, tpSirMsgQ pMsg)
{
    schProcessMessage(pMac, pMsg);

    return eSIR_SUCCESS;
}





//                                                                            
/* 
                      
  
            
  
         
  
               
  
        
  
              
               
 */

void
schSendStartScanRsp(tpAniSirGlobal pMac)
{
    tSirMsgQ        msgQ;
    tANI_U32        retCode;

    PELOG1(schLog(pMac, LOG1, FL("Sending LIM message to go into scan"));)
    msgQ.type = SIR_SCH_START_SCAN_RSP;
    if ((retCode = limPostMsgApi(pMac, &msgQ)) != eSIR_SUCCESS)
        schLog(pMac, LOGE,
               FL("Posting START_SCAN_RSP to LIM failed, reason=%X"), retCode);
}

/* 
                   
  
            
  
         
                                         
                                                      
                                                                      
                        
  
               
                                                                           
                                                      
  
        
  
                     
  
                       
  
                                     
  
                     
 */
tSirRetStatus schSendBeaconReq( tpAniSirGlobal pMac, tANI_U8 *beaconPayload, tANI_U16 size, tpPESession psessionEntry)
{
  tSirMsgQ msgQ;
  tpSendbeaconParams beaconParams = NULL;
  tSirRetStatus retCode;

  schLog( pMac, LOG2,
         FL( "Indicating HAL to copy the beacon template [%d bytes] to memory" ),
         size );

  beaconParams = vos_mem_malloc(sizeof(tSendbeaconParams));
  if ( NULL == beaconParams )
      return eSIR_FAILURE;

  msgQ.type = WDA_SEND_BEACON_REQ;

  //                                                     
  msgQ.reserved = 0;

  //                                  
  /*                                     */
  //                                         
  vos_mem_copy(beaconParams->bssId, psessionEntry->bssId, sizeof(psessionEntry->bssId));

  beaconParams->timIeOffset = pMac->sch.schObject.gSchBeaconOffsetBegin;
  /*                                                        */
  if ((pMac->sch.schObject.p2pIeOffset != 0) &&
          (pMac->sch.schObject.p2pIeOffset <
           pMac->sch.schObject.gSchBeaconOffsetBegin))
  {
      schLog(pMac, LOGE,FL("Invalid p2pIeOffset:[%d]"),
              pMac->sch.schObject.p2pIeOffset);
      VOS_ASSERT( 0 );
      return eSIR_FAILURE;
  }
  beaconParams->p2pIeOffset = pMac->sch.schObject.p2pIeOffset;
#ifdef WLAN_SOFTAP_FW_BEACON_TX_PRNT_LOG
  schLog(pMac, LOGE,FL("TimIeOffset:[%d]"),beaconParams->TimIeOffset );
#endif

  beaconParams->beacon = beaconPayload;
  beaconParams->beaconLength = (tANI_U32) size;
  msgQ.bodyptr = beaconParams;
  msgQ.bodyval = 0;

  //                                        

  //                                 
  if (psessionEntry->beacon )
  {
    vos_mem_free(psessionEntry->beacon);
  }

  psessionEntry->bcnLen = 0;
  psessionEntry->beacon = NULL;

  psessionEntry->beacon = vos_mem_malloc(size);
  if ( psessionEntry->beacon != NULL )
  {
    vos_mem_copy(psessionEntry->beacon, beaconPayload, size);
    psessionEntry->bcnLen = size;
  }

  MTRACE(macTraceMsgTx(pMac, psessionEntry->peSessionId, msgQ.type));
  if( eSIR_SUCCESS != (retCode = wdaPostCtrlMsg( pMac, &msgQ )))
  {
    schLog( pMac, LOGE,
        FL("Posting SEND_BEACON_REQ to HAL failed, reason=%X"),
        retCode );
  } else
  {
    schLog( pMac, LOG2,
        FL("Successfully posted WDA_SEND_BEACON_REQ to HAL"));

    if( (psessionEntry->limSystemRole == eLIM_AP_ROLE ) 
        && (pMac->sch.schObject.fBeaconChanged)
        && ((psessionEntry->proxyProbeRspEn)
        || (IS_FEATURE_SUPPORTED_BY_FW(WPS_PRBRSP_TMPL)))
      )

    {
        schLog(pMac, LOG1, FL("Sending probeRsp Template to HAL"));
        if(eSIR_SUCCESS != (retCode = limSendProbeRspTemplateToHal(pMac,psessionEntry,
                                    &psessionEntry->DefProbeRspIeBitmap[0])))
        {
            /*                                          */
            schLog(pMac, LOGE, FL("FAILED to send probe response template with retCode %d"), retCode);
        }
    }
  }

  return retCode;
}

tANI_U32 limSendProbeRspTemplateToHal(tpAniSirGlobal pMac,tpPESession psessionEntry
                                  ,tANI_U32* IeBitmap)
{
    tSirMsgQ  msgQ;
    tANI_U8 *pFrame2Hal = pMac->sch.schObject.gSchProbeRspTemplate;
    tpSendProbeRespParams pprobeRespParams=NULL;
    tANI_U32  retCode = eSIR_FAILURE;
    tANI_U32             nPayload,nBytes,nStatus;
    tpSirMacMgmtHdr      pMacHdr;
    tANI_U32             addnIEPresent;
    tANI_U32             addnIELen=0;
    tSirRetStatus        nSirStatus;
    tANI_U8              *addIE = NULL;

    nStatus = dot11fGetPackedProbeResponseSize( pMac, &psessionEntry->probeRespFrame, &nPayload );
    if ( DOT11F_FAILED( nStatus ) )
    {
        schLog( pMac, LOGE, FL("Failed to calculate the packed size f"
                               "or a Probe Response (0x%08x)."),
                nStatus );
        //                                            
        nPayload = sizeof( tDot11fProbeResponse );
    }
    else if ( DOT11F_WARNED( nStatus ) )
    {
        schLog( pMac, LOGE, FL("There were warnings while calculating"
                               "the packed size for a Probe Response "
                               "(0x%08x)."), nStatus );
    }

    nBytes = nPayload + sizeof( tSirMacMgmtHdr );

    //                                            
    if (wlan_cfgGetInt(pMac, WNI_CFG_PROBE_RSP_ADDNIE_FLAG, &addnIEPresent) != eSIR_SUCCESS)
    {
        schLog(pMac, LOGE, FL("Unable to get WNI_CFG_PROBE_RSP_ADDNIE_FLAG"));
        return retCode;
    }

    if (addnIEPresent)
    {
        //                      
        addIE = vos_mem_malloc(WNI_CFG_PROBE_RSP_ADDNIE_DATA1_LEN);
        if ( NULL == addIE )
        {
             schLog(pMac, LOGE,
                 FL("Unable to get WNI_CFG_PROBE_RSP_ADDNIE_DATA1 length"));
             return retCode;
        }

        if (wlan_cfgGetStrLen(pMac, WNI_CFG_PROBE_RSP_ADDNIE_DATA1,
                                               &addnIELen) != eSIR_SUCCESS)
        {
            schLog(pMac, LOGE,
                FL("Unable to get WNI_CFG_PROBE_RSP_ADDNIE_DATA1 length"));

            vos_mem_free(addIE);
            return retCode;
        }

        if (addnIELen <= WNI_CFG_PROBE_RSP_ADDNIE_DATA1_LEN && addnIELen &&
                                 (nBytes + addnIELen) <= SIR_MAX_PACKET_SIZE)
        {
            if ( eSIR_SUCCESS != wlan_cfgGetStr(pMac,
                                    WNI_CFG_PROBE_RSP_ADDNIE_DATA1, &addIE[0],
                                    &addnIELen) )
            {
               schLog(pMac, LOGE,
                   FL("Unable to get WNI_CFG_PROBE_RSP_ADDNIE_DATA1 String"));

               vos_mem_free(addIE);
               return retCode;
            }
        }
    }

    if (addnIEPresent)
    {
        if ((nBytes + addnIELen) <= SIR_MAX_PACKET_SIZE )
            nBytes += addnIELen;
        else
            addnIEPresent = false; //                    
    }

    //          
    vos_mem_set(pFrame2Hal, nBytes, 0);

    //                                         
    nSirStatus = limPopulateMacHeader( pMac, pFrame2Hal, SIR_MAC_MGMT_FRAME,
                                SIR_MAC_MGMT_PROBE_RSP, psessionEntry->selfMacAddr,psessionEntry->selfMacAddr);

    if ( eSIR_SUCCESS != nSirStatus )
    {
        schLog( pMac, LOGE, FL("Failed to populate the buffer descrip"
                               "tor for a Probe Response (%d)."),
                nSirStatus );

        vos_mem_free(addIE);
        return retCode;
    }

    pMacHdr = ( tpSirMacMgmtHdr ) pFrame2Hal;

    sirCopyMacAddr(pMacHdr->bssId,psessionEntry->bssId);

    //                                    
    nStatus = dot11fPackProbeResponse( pMac, &psessionEntry->probeRespFrame, pFrame2Hal + sizeof(tSirMacMgmtHdr),
                                       nPayload, &nPayload );

    if ( DOT11F_FAILED( nStatus ) )
    {
        schLog( pMac, LOGE, FL("Failed to pack a Probe Response (0x%08x)."),
                nStatus );

        vos_mem_free(addIE);
        return retCode;                 //           
    }
    else if ( DOT11F_WARNED( nStatus ) )
    {
        schLog( pMac, LOGE, FL("There were warnings while packing a P"
                               "robe Response (0x%08x)."), nStatus );
    }

    if (addnIEPresent)
    {
        vos_mem_copy ( &pFrame2Hal[nBytes - addnIELen],
                             &addIE[0], addnIELen);
    }

    /*                           */
    vos_mem_free(addIE);

    pprobeRespParams = vos_mem_malloc(sizeof( tSendProbeRespParams ));
    if ( NULL == pprobeRespParams )
    {
        schLog( pMac, LOGE, FL("limSendProbeRspTemplateToHal: HAL probe response params malloc failed for bytes %d"), nBytes );
    }
    else
    {
        /*
                                                        
                                       
                                     
        */

        sirCopyMacAddr( pprobeRespParams->bssId,  psessionEntry->bssId);
        pprobeRespParams->pProbeRespTemplate   = pFrame2Hal;
        pprobeRespParams->probeRespTemplateLen = nBytes;
        vos_mem_copy(pprobeRespParams->ucProxyProbeReqValidIEBmap,IeBitmap,(sizeof(tANI_U32) * 8));
        msgQ.type     = WDA_UPDATE_PROBE_RSP_TEMPLATE_IND;
        msgQ.reserved = 0;
        msgQ.bodyptr  = pprobeRespParams;
        msgQ.bodyval  = 0;

        if( eSIR_SUCCESS != (retCode = wdaPostCtrlMsg( pMac, &msgQ )))
        {
            /*                           */
            schLog( pMac,LOGE, FL("limSendProbeRspTemplateToHal: FAIL bytes %d retcode[%X]"), nBytes, retCode );
            vos_mem_free(pprobeRespParams);
        }
        else
        {
            schLog( pMac,LOG1, FL("limSendProbeRspTemplateToHal: Probe response template msg posted to HAL of bytes %d"),nBytes );
        }
    }

    return retCode;
}

