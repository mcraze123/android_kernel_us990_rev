#ifndef __LINUX_NL80211_H
#define __LINUX_NL80211_H
/*
 * 802.11 netlink interface public header
 *
 * Copyright 2006-2010 Johannes Berg <johannes@sipsolutions.net>
 * Copyright 2008 Michael Wu <flamingice@sourmilk.net>
 * Copyright 2008 Luis Carlos Cobo <luisca@cozybit.com>
 * Copyright 2008 Michael Buesch <m@bues.ch>
 * Copyright 2008, 2009 Luis R. Rodriguez <lrodriguez@atheros.com>
 * Copyright 2008 Jouni Malinen <jouni.malinen@atheros.com>
 * Copyright 2008 Colin McCabe <colin@cozybit.com>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 */

#include <linux/types.h>

/* 
                        
  
                                                                        
                                                                          
                                                                             
                                                                          
      
  
                        
 */

/* 
                                               
  
                                                                        
                                                                        
                                                                        
                                                                        
                       
  
                                                                        
                                                                        
                                                                     
                                                                        
                                                                        
                                                                        
                  
  
                                                                        
                                                                     
                                                  
  
                                                                       
                                                                    
                                                                       
                                                                     
                                                                      
                                                                     
  
                                                                       
                                                                     
                                                                        
                                             
  
                                                                         
         
 */

/* 
                                                    
  
                                                                    
                                                                    
                                                                    
                                                  
  
                                                                    
                                                                    
                                         
  
                                                                 
                                                                  
                                                                   
                                                                 
                                                                  
                                                                   
                                                               
  
                                                                   
                                                                    
                                                              
                                                                 
                                                                   
                                                                    
                                                                    
                                                                   
  
                                                                   
                                           
 */

/* 
                                                     
  
                                                           
  
                                                                            
                                       
                                                                             
                                                                      
                                                            
                                                                     
                                                                      
                                            
                                                                 
                                                                
                                                                       
                                                                 
                            
                                                        
                                                    
  
                                                                    
                                                                   
                                            
                                                                        
                                                  
                                                                          
                                                            
                                                                    
                                                                         
                                                                         
                        
                                                                            
                                                                       
                                                                      
                                   
  
                                                                             
                                                     
                                                                      
                                                                  
                                                                     
                                                                      
                                        
                                                                         
                        
  
                                      
                                                                          
                                                                    
                                                                       
                                                                           
                                                               
                                                                           
                                                                         
                                                                       
                                                 
                                                             
                                                          
                                                   
                                                                
                                                        
                                                                       
                                                                         
                                                               
                                                                 
                                                              
  
                                                                             
                                                                          
                                                                             
                                                                          
                                                                       
                                                     
                                                                             
                                                                         
                            
  
                                                                    
                                                                
                          
                                                                     
                                                                
                          
                                                                              
                                                      
                                                                         
                     
                                                                      
                                                     
                                                                            
                                                                           
                            
                                                                 
                         
  
                                                                           
                      
                                                                               
                                                                          
                                                                        
                                                       
                                                                    
                                                           
                                               
                                                                        
                                                 
                                          
                                                                               
                                                                      
                                                                  
  
                                                                       
                                                
  
                                                                       
                                                     
  
                                                                           
                                                                        
                                                                         
                                                                       
                                                                        
                                                        
                          
                                                                      
                                                                          
                                                                         
                                                                
                                                   
                                                            
  
                                          
                                                                          
                                                                     
                                     
                                                                  
                                                          
                                                                        
                                        
  
                                                                   
                                                                
                                                              
                                                        
                                                           
                                                                 
                                                                
                                                             
                                                                 
                                                                
                                        
                                                                        
                                    
                                                                           
                     
                                                                         
                                                                 
                                                                       
                                                                          
                                                      
                                                                         
                                                      
  
                                                                      
                      
                                                                           
                                                            
  
                                                                        
                                                                    
                                                                       
                                                
                                                                    
                                                          
                                                                
                                                                     
                                                                  
                                  
                                                                         
                                                                   
                                                                  
                                                                        
                                                                       
                                                                       
                                                                       
                                                                       
                                                                         
                                                                        
                                                                        
                                                                         
                                                              
                                                                
                                                                           
                                  
  
                                                                      
                                                                       
                                                                         
                          
                                                                        
                                                                      
                                                                        
                                                                      
                                                                        
                                                                          
                                                                          
                                                                        
                            
                                                                     
                                                                    
                                                                    
                                                                       
                                                                 
                                                                       
                                                                         
                                                                         
                                                            
                                                                        
                                     
                                                                     
                                                                 
                                                                
                                                                  
                                                                               
                                                                       
                                                                   
               
                                                                           
                                                                     
                                                                          
  
                                                                               
                                                                      
                                                                         
                                                                     
                                                                  
                                                                     
                                                                   
  
                                                                          
                                                                     
                                                                    
                                                                    
                                                                   
                                                                      
                                                                     
                                      
                                                                               
                                       
  
                                                                                
                                                                          
                 
  
                                                                          
                                                                    
                                                                    
                                                                           
                                                                           
                                                                           
                                        
                                                                     
                                 
                                                                  
                                                                        
                                                                           
                                                                       
                                                                        
                                                                         
                                                                        
                                                                
                                                             
                                           
                                             
                                                         
                                                                        
                                                                  
                                                                        
                                                                     
                                           
                                                                             
                                                          
                                                                        
                                                                    
                                                             
                                                 
  
                                                                           
                                                           
  
                                                                           
                                                                   
                                                                          
                                                                 
                                                                     
                                                                  
                                                                          
                                                            
                                                                         
                                                                     
                                                                         
                                                                     
          
                                                                      
                                                                    
                                                 
                                                                              
                                                                       
                                                                      
                                                                      
                                                                     
                                 
                                                                   
                                          
  
                                                                           
                                                                         
                                                       
  
                                                                          
                                                                     
                                                                    
                                                                      
                                                                    
                                                                     
                                                                   
                                                                   
                                                        
                                                                          
                         
                                                                            
                                                                       
                                                                        
                                                                        
                                                                      
                                                                   
                                                                      
                                                                        
                                                               
                                                                       
                                                                     
                                                                      
                                                                         
                                                
                                                                     
                                                     
                                                                             
                                                                       
                                                      
                                                                                
                                                                       
                                                                       
                                                                      
                                                                          
             
                                                                            
                          
                                                                               
                                                                          
          
                                                                         
                                                                       
           
                                                                            
                                                                       
                                              
                                                                       
                                                             
                                                                      
                                                                      
                                                                
                                 
  
                                                                                 
  
                                                                              
                                       
                                                                               
                                                  
  
                                                                         
                                                                   
                                                         
                                                                     
                                                                   
                                                       
  
                                                                      
                                                                           
                                                                            
                                                                         
                                                                            
                                                                           
                                                                               
                                                                               
                                               
  
                                                                       
                                                                       
                                                                  
                                                                  
                                                                
                                   
                                                            
  
                                                                       
                                                                   
                                                                  
                                                                
                                                                 
                                                                  
                                              
  
                                                                             
                               
  
                                                                               
                                                                         
                                                                          
                                                                       
                                                                  
                                                                  
                           
                                                        
  
                                                                          
                                                                   
                                                                    
                                                                      
               
                                                                    
                                                         
                                                                     
                                                                      
                                                          
  
                                                                            
                                                                        
                                                                        
                                                                    
  
                                                                            
                                                                     
                                                                       
                                                                     
                                                                        
                                                                      
                                                    
  
                                                                              
                                                                       
                                                                       
                                                                 
  
                                                                            
                                                    
  
                                                                           
                                                                 
                                                                   
                                        
  
                                                                           
                                                                    
                                                                        
                                                                        
                          
                                                                         
                                     
  
                                                                        
                                                                      
                                                                      
                    
  
                                                                             
                        
  
                                                                           
                                                                     
                                                                   
                                                      
                                                                         
                                                                           
                                                                          
                                                                          
                                        
  
                                                                            
                                                                          
                                                                          
                                                                         
                                   
                                                                    
         
  
                                                                            
                                                                      
                                                                      
  
                                                                            
                                         
  
                                                                           
                                                                       
                                                                       
                                                    
  
                                                                            
                                                                       
            
  
                                                                            
                         
  
                                                                     
                                                                               
  
                                                                          
                                                                  
                                                 
                                                                  
                                                                     
                                                                  
                                                                     
                                                                    
                      
  
                                                                                
                                                                
                                                                  
                             
                                                                        
                                                                       
                                                                      
                                                              
  
                                                                             
                                                                       
                                                                      
                                                                     
                                                                 
  
                                                
                                          
 */
enum nl80211_commands {
/*                                                              */
	NL80211_CMD_UNSPEC,

	NL80211_CMD_GET_WIPHY,		/*          */
	NL80211_CMD_SET_WIPHY,
	NL80211_CMD_NEW_WIPHY,
	NL80211_CMD_DEL_WIPHY,

	NL80211_CMD_GET_INTERFACE,	/*          */
	NL80211_CMD_SET_INTERFACE,
	NL80211_CMD_NEW_INTERFACE,
	NL80211_CMD_DEL_INTERFACE,

	NL80211_CMD_GET_KEY,
	NL80211_CMD_SET_KEY,
	NL80211_CMD_NEW_KEY,
	NL80211_CMD_DEL_KEY,

	NL80211_CMD_GET_BEACON,
	NL80211_CMD_SET_BEACON,
	NL80211_CMD_START_AP,
	NL80211_CMD_NEW_BEACON = NL80211_CMD_START_AP,
	NL80211_CMD_STOP_AP,
	NL80211_CMD_DEL_BEACON = NL80211_CMD_STOP_AP,

	NL80211_CMD_GET_STATION,
	NL80211_CMD_SET_STATION,
	NL80211_CMD_NEW_STATION,
	NL80211_CMD_DEL_STATION,

	NL80211_CMD_GET_MPATH,
	NL80211_CMD_SET_MPATH,
	NL80211_CMD_NEW_MPATH,
	NL80211_CMD_DEL_MPATH,

	NL80211_CMD_SET_BSS,

	NL80211_CMD_SET_REG,
	NL80211_CMD_REQ_SET_REG,

	NL80211_CMD_GET_MESH_CONFIG,
	NL80211_CMD_SET_MESH_CONFIG,

	NL80211_CMD_SET_MGMT_EXTRA_IE /*                    */,

	NL80211_CMD_GET_REG,

	NL80211_CMD_GET_SCAN,
	NL80211_CMD_TRIGGER_SCAN,
	NL80211_CMD_NEW_SCAN_RESULTS,
	NL80211_CMD_SCAN_ABORTED,

	NL80211_CMD_REG_CHANGE,

	NL80211_CMD_AUTHENTICATE,
	NL80211_CMD_ASSOCIATE,
	NL80211_CMD_DEAUTHENTICATE,
	NL80211_CMD_DISASSOCIATE,

	NL80211_CMD_MICHAEL_MIC_FAILURE,

	NL80211_CMD_REG_BEACON_HINT,

	NL80211_CMD_JOIN_IBSS,
	NL80211_CMD_LEAVE_IBSS,

	NL80211_CMD_TESTMODE,

	NL80211_CMD_CONNECT,
	NL80211_CMD_ROAM,
	NL80211_CMD_DISCONNECT,

	NL80211_CMD_SET_WIPHY_NETNS,

	NL80211_CMD_GET_SURVEY,
	NL80211_CMD_NEW_SURVEY_RESULTS,

	NL80211_CMD_SET_PMKSA,
	NL80211_CMD_DEL_PMKSA,
	NL80211_CMD_FLUSH_PMKSA,

	NL80211_CMD_REMAIN_ON_CHANNEL,
	NL80211_CMD_CANCEL_REMAIN_ON_CHANNEL,

	NL80211_CMD_SET_TX_BITRATE_MASK,

	NL80211_CMD_REGISTER_FRAME,
	NL80211_CMD_REGISTER_ACTION = NL80211_CMD_REGISTER_FRAME,
	NL80211_CMD_FRAME,
	NL80211_CMD_ACTION = NL80211_CMD_FRAME,
	NL80211_CMD_FRAME_TX_STATUS,
	NL80211_CMD_ACTION_TX_STATUS = NL80211_CMD_FRAME_TX_STATUS,

	NL80211_CMD_SET_POWER_SAVE,
	NL80211_CMD_GET_POWER_SAVE,

	NL80211_CMD_SET_CQM,
	NL80211_CMD_NOTIFY_CQM,

	NL80211_CMD_SET_CHANNEL,
	NL80211_CMD_SET_WDS_PEER,

	NL80211_CMD_FRAME_WAIT_CANCEL,

	NL80211_CMD_JOIN_MESH,
	NL80211_CMD_LEAVE_MESH,

	NL80211_CMD_UNPROT_DEAUTHENTICATE,
	NL80211_CMD_UNPROT_DISASSOCIATE,

	NL80211_CMD_NEW_PEER_CANDIDATE,

	NL80211_CMD_GET_WOWLAN,
	NL80211_CMD_SET_WOWLAN,

	NL80211_CMD_START_SCHED_SCAN,
	NL80211_CMD_STOP_SCHED_SCAN,
	NL80211_CMD_SCHED_SCAN_RESULTS,
	NL80211_CMD_SCHED_SCAN_STOPPED,

	NL80211_CMD_SET_REKEY_OFFLOAD,

	NL80211_CMD_PMKSA_CANDIDATE,

	NL80211_CMD_TDLS_OPER,
	NL80211_CMD_TDLS_MGMT,

	NL80211_CMD_UNEXPECTED_FRAME,

	NL80211_CMD_PROBE_CLIENT,

	NL80211_CMD_REGISTER_BEACONS,

	NL80211_CMD_UNEXPECTED_4ADDR_FRAME,

	NL80211_CMD_SET_NOACK_MAP,


	NL80211_CMD_CH_SWITCH_NOTIFY,

	NL80211_CMD_START_P2P_DEVICE,
	NL80211_CMD_STOP_P2P_DEVICE,

	NL80211_CMD_CONN_FAILED,

	NL80211_CMD_SET_MCAST_RATE,

	NL80211_CMD_SET_MAC_ACL,

	NL80211_CMD_RADAR_DETECT,

	NL80211_CMD_GET_PROTOCOL_FEATURES,

	NL80211_CMD_UPDATE_FT_IES,
	NL80211_CMD_FT_EVENT,

	NL80211_CMD_CRIT_PROTOCOL_START,
	NL80211_CMD_CRIT_PROTOCOL_STOP,

	NL80211_CMD_GET_COALESCE,
	NL80211_CMD_SET_COALESCE,

	NL80211_CMD_CHANNEL_SWITCH,

	NL80211_CMD_VENDOR,

	NL80211_CMD_SET_QOS_MAP,

	/*                             */

	/*                                      */
	__NL80211_CMD_AFTER_LAST,
	NL80211_CMD_MAX = __NL80211_CMD_AFTER_LAST - 1
};

/*
                                                                           
       
 */
#define NL80211_CMD_SET_BSS NL80211_CMD_SET_BSS
#define NL80211_CMD_SET_MGMT_EXTRA_IE NL80211_CMD_SET_MGMT_EXTRA_IE
#define NL80211_CMD_REG_CHANGE NL80211_CMD_REG_CHANGE
#define NL80211_CMD_AUTHENTICATE NL80211_CMD_AUTHENTICATE
#define NL80211_CMD_ASSOCIATE NL80211_CMD_ASSOCIATE
#define NL80211_CMD_DEAUTHENTICATE NL80211_CMD_DEAUTHENTICATE
#define NL80211_CMD_DISASSOCIATE NL80211_CMD_DISASSOCIATE
#define NL80211_CMD_REG_BEACON_HINT NL80211_CMD_REG_BEACON_HINT

#define NL80211_ATTR_FEATURE_FLAGS NL80211_ATTR_FEATURE_FLAGS

/*                                */
#define NL80211_CMD_GET_MESH_PARAMS NL80211_CMD_GET_MESH_CONFIG
#define NL80211_CMD_SET_MESH_PARAMS NL80211_CMD_SET_MESH_CONFIG
#define NL80211_MESH_SETUP_VENDOR_PATH_SEL_IE NL80211_MESH_SETUP_IE

/* 
                                                  
  
                                                              
  
                                                         
                                       
                                                           
                                                                        
                                                                     
                                                                          
                                                                        
                                                                   
                   
                                
                                                                          
                                                                         
                                                                             
                                                                  
                           
                                                                            
                                                         
                          
                                                                             
                                                                 
                                                               
                                                                          
                                                                      
                                                         
                                                                               
                                          
  
                                                                             
                                               
                                                                            
  
                                                
  
                                                                         
                                                                     
       
                                          
                                                                             
                                       
                                                                           
                                             
  
                                                       
                                                       
                                                                     
                                                                    
  
                                                              
                                                                             
                                                                     
                                                                   
                             
                                                                         
                                                                 
                                                 
                                                                            
                                                                  
                                                                            
                                                                  
                                                
  
                                                                   
                                
  
                                               
                                                                           
                                                                             
                                                                             
                                                                        
                            
  
                                                                              
                                 
  
                                                                          
                                                                    
                                                                           
                                                                           
                                                                       
                                                                     
                                                                   
                                                                        
                         
                                                                          
         
  
                                                                             
                                                                      
               
                                                                     
               
                                                             
                                                                        
                                                 
  
                                                                       
                                                              
  
                                                                   
                                                                   
                         
  
                                                           
                                  
  
                                                                  
                                   
  
                                                                      
                                            
                                                                  
                                                                
                                                                        
                                      
                                                                     
                                                         
                                                                   
                                                               
  
                                                                             
                                                                               
                                                                       
                                     
  
                                                                             
                                                                          
                                                                                
                                                                         
  
                                                                   
                                                                       
                                                
  
                                                                             
                                                                       
                               
                                                            
                                                                            
                       
                                                                            
                                 
  
                                                                               
        
  
                                                                              
                                                                     
                                                                      
                                                        
                            
                                                                             
                                                                     
                                                                     
                                                        
                            
  
                                                                            
                
  
                                                                              
                                           
  
                                                                               
                                                      
  
                                                                               
                                                                      
                             
                                                                
  
                                                   
                                   
  
                                                                            
                                                                       
                                                                  
                                                                      
                                                                    
                           
                                                                      
                                                             
                                                                  
                                                              
                                                               
                                                        
                                                                  
                                                                  
                                                                   
  
                                                                            
                                                                     
  
                                                                          
                                                                    
                              
                                                                     
              
                                                                         
                                                                          
                                              
  
                                                                               
                                                                         
                     
                                                                              
                                                                      
        
                                                                              
                                                                          
                                                      
                                                                            
                                                                       
  
                                                                        
                                                                
                                                                          
                                                        
  
                                                                       
                                                
  
                                                                
                                
                                                                      
                                                                 
                                     
  
                                                        
  
                                                                      
                                                              
                                                               
                                                                
                                                              
                                                                
                           
  
                                                                   
  
                                                                         
                                                                         
                                                           
  
                                                       
                                                                        
                            
  
                                                                         
                                                                      
                                                                
                                                     
  
                                                                   
  
                                                   
                                                                      
                                                                         
                                                                      
                                                                        
                                                 
  
                                                                             
                                                                      
                                                                            
                                       
                                                                       
                                                                      
                                                              
                      
                                                                       
                                                                      
                                                                
  
                                                                  
                                 
  
                                                                   
                                                            
  
                                                                              
                                                                        
                                                                   
                                                        
                            
  
                                                                              
                         
  
                                                                         
                                                           
                                                                                
                                                                             
                                   
  
                                                                             
                                      
  
                                                                               
                                                                         
                                                                      
                                                                  
  
                                                                        
                                                                       
                                                                           
                                                                        
                                                                         
                                                                   
                                                                     
                                                                 
                                                                           
                                
  
                                                                            
                                                                         
                                                                          
                                                                    
                                                                      
  
                                                                               
                                                             
  
                                                                               
                                                             
  
                                                                     
  
                                                                            
                                                                      
                                                                     
                                                                      
                           
  
                                                      
  
                                                                       
                                                                 
                                       
  
                                                                             
                                   
                                                                               
                                                            
                                                                               
                                                                            
                                               
                                                                  
                                                               
                                                  
                                                      
  
                                                                           
                                              
                                                                    
                                                              
                                                                      
            

                                                                     
                    

                                                                    
                                                                 
                                                          
                          
                                                                 
                                                                
                                                               
                                                                
                                 
                                                               
                                                             
                                                       
                                                              
                                                                
                                                             
                                                          
                                    
  
                                                                               
                                                                      
                                                 
                                                              
                                                                       
                                                                       
                                                    
  
                                                                         
                                                                          
  
                                                                                  
                                                                           
                                                                        
                                                                    
  
                                                                                
                                                                    
                                                  
  
                                                                                
                                                                           
                                                                           
                                                        
                                                                          
                                                                 
                                                                       
                                                                           
                                  
  
                                                                           
                                                  
                                                                         
         
  
                                                                           
                                                                    
  
                                                                               
                                                                 
  
                                                                         
                                                                         
                                                                        
                                   
                                                            
                               
  
                                                                         
                                                                
                                                 
                                                                             
                                         
                                                               
                                                     
                                                                       
                      
                                                                           
                                                             
                                                                     
                                                          
  
                                                                            
                                                                        
                                                              
                                 
  
                                                                            
                                                                        
                                                                       
                                                     
  
                                                                      
                                                                      
                                                                            
  
                                       
                                                               
                                                                  
  
                                                                              
                                                                          
                          
                                                                    
                                                                     
                                                              
                                                             
                                                           
                                              
                                                         
                                        
                                                                
                                                                    
                                         
  
                                                                           
                                                                         
                        
  
                                                                         
                      
  
                                                                          
                                                                           
                                                                        
                                                                 
                                      
  
                                                                         
                                                                    
                                                     
  
                                                                  
                                        
  
                                                                        
                                                                 
                                                                
                                                           
                                                                
  
                                                                         
                                                                       
                                                    
  
                                                                             
                                                             
  
                                                                         
                                                              
  
                                                             
  
                                                                           
                                    
                                                                       
                                                                  
                                                                 
                     
  
                                                                               
                                            
  
                                                                      
                             
  
                                                                   
           
  
                                                                    
                                                            
       
  
                                                                                
                                                      
  
                                                                              
                                                                  
                                             
                                                                            
                                                                    
  
                                                                             
                                                         
  
                                                                      
                                                                        
               
  
                                                                     
                                                           
  
                                                                     
                                                              
                                          
  
                                                 
  
                                                                   
          
  
                                                                               
                                                      
                                                                          
                                                               
  
                                                                          
                                                                          
                                                                        
                                
  
                                                          
  
                                                                               
                                  
                                                                                
                                                                    
              
                                                                                
                                                  
                                                                       
                                                         
                                                                      
                                                          
  
                                                                  
                                                  
  
                                                                     
  
                                                               
                                    
  
                                                                 
                                                                  
                                                                   
                                                                        
                                                                        
                
  
                                                                     
                                                                        
                                                  
                                                                       
                                                                  
                                                                            
                                                     
  
                                                                            
                                                                           
                                     
  
                                                                    
                                                                          
  
                                                                           
                                                                        
                                                                        
                                                                       
                                                                        
                                                                     
  
                                                                             
                                                          
  
                                                                
                                           
 */
enum nl80211_attrs {
/*                                                              */
	NL80211_ATTR_UNSPEC,

	NL80211_ATTR_WIPHY,
	NL80211_ATTR_WIPHY_NAME,

	NL80211_ATTR_IFINDEX,
	NL80211_ATTR_IFNAME,
	NL80211_ATTR_IFTYPE,

	NL80211_ATTR_MAC,

	NL80211_ATTR_KEY_DATA,
	NL80211_ATTR_KEY_IDX,
	NL80211_ATTR_KEY_CIPHER,
	NL80211_ATTR_KEY_SEQ,
	NL80211_ATTR_KEY_DEFAULT,

	NL80211_ATTR_BEACON_INTERVAL,
	NL80211_ATTR_DTIM_PERIOD,
	NL80211_ATTR_BEACON_HEAD,
	NL80211_ATTR_BEACON_TAIL,

	NL80211_ATTR_STA_AID,
	NL80211_ATTR_STA_FLAGS,
	NL80211_ATTR_STA_LISTEN_INTERVAL,
	NL80211_ATTR_STA_SUPPORTED_RATES,
	NL80211_ATTR_STA_VLAN,
	NL80211_ATTR_STA_INFO,

	NL80211_ATTR_WIPHY_BANDS,

	NL80211_ATTR_MNTR_FLAGS,

	NL80211_ATTR_MESH_ID,
	NL80211_ATTR_STA_PLINK_ACTION,
	NL80211_ATTR_MPATH_NEXT_HOP,
	NL80211_ATTR_MPATH_INFO,

	NL80211_ATTR_BSS_CTS_PROT,
	NL80211_ATTR_BSS_SHORT_PREAMBLE,
	NL80211_ATTR_BSS_SHORT_SLOT_TIME,

	NL80211_ATTR_HT_CAPABILITY,

	NL80211_ATTR_SUPPORTED_IFTYPES,

	NL80211_ATTR_REG_ALPHA2,
	NL80211_ATTR_REG_RULES,

	NL80211_ATTR_MESH_CONFIG,

	NL80211_ATTR_BSS_BASIC_RATES,

	NL80211_ATTR_WIPHY_TXQ_PARAMS,
	NL80211_ATTR_WIPHY_FREQ,
	NL80211_ATTR_WIPHY_CHANNEL_TYPE,

	NL80211_ATTR_KEY_DEFAULT_MGMT,

	NL80211_ATTR_MGMT_SUBTYPE,
	NL80211_ATTR_IE,

	NL80211_ATTR_MAX_NUM_SCAN_SSIDS,

	NL80211_ATTR_SCAN_FREQUENCIES,
	NL80211_ATTR_SCAN_SSIDS,
	NL80211_ATTR_GENERATION, /*                              */
	NL80211_ATTR_BSS,

	NL80211_ATTR_REG_INITIATOR,
	NL80211_ATTR_REG_TYPE,

	NL80211_ATTR_SUPPORTED_COMMANDS,

	NL80211_ATTR_FRAME,
	NL80211_ATTR_SSID,
	NL80211_ATTR_AUTH_TYPE,
	NL80211_ATTR_REASON_CODE,

	NL80211_ATTR_KEY_TYPE,

	NL80211_ATTR_MAX_SCAN_IE_LEN,
	NL80211_ATTR_CIPHER_SUITES,

	NL80211_ATTR_FREQ_BEFORE,
	NL80211_ATTR_FREQ_AFTER,

	NL80211_ATTR_FREQ_FIXED,


	NL80211_ATTR_WIPHY_RETRY_SHORT,
	NL80211_ATTR_WIPHY_RETRY_LONG,
	NL80211_ATTR_WIPHY_FRAG_THRESHOLD,
	NL80211_ATTR_WIPHY_RTS_THRESHOLD,

	NL80211_ATTR_TIMED_OUT,

	NL80211_ATTR_USE_MFP,

	NL80211_ATTR_STA_FLAGS2,

	NL80211_ATTR_CONTROL_PORT,

	NL80211_ATTR_TESTDATA,

	NL80211_ATTR_PRIVACY,

	NL80211_ATTR_DISCONNECTED_BY_AP,
	NL80211_ATTR_STATUS_CODE,

	NL80211_ATTR_CIPHER_SUITES_PAIRWISE,
	NL80211_ATTR_CIPHER_SUITE_GROUP,
	NL80211_ATTR_WPA_VERSIONS,
	NL80211_ATTR_AKM_SUITES,

	NL80211_ATTR_REQ_IE,
	NL80211_ATTR_RESP_IE,

	NL80211_ATTR_PREV_BSSID,

	NL80211_ATTR_KEY,
	NL80211_ATTR_KEYS,

	NL80211_ATTR_PID,

	NL80211_ATTR_4ADDR,

	NL80211_ATTR_SURVEY_INFO,

	NL80211_ATTR_PMKID,
	NL80211_ATTR_MAX_NUM_PMKIDS,

	NL80211_ATTR_DURATION,

	NL80211_ATTR_COOKIE,

	NL80211_ATTR_WIPHY_COVERAGE_CLASS,

	NL80211_ATTR_TX_RATES,

	NL80211_ATTR_FRAME_MATCH,

	NL80211_ATTR_ACK,

	NL80211_ATTR_PS_STATE,

	NL80211_ATTR_CQM,

	NL80211_ATTR_LOCAL_STATE_CHANGE,

	NL80211_ATTR_AP_ISOLATE,

	NL80211_ATTR_WIPHY_TX_POWER_SETTING,
	NL80211_ATTR_WIPHY_TX_POWER_LEVEL,

	NL80211_ATTR_TX_FRAME_TYPES,
	NL80211_ATTR_RX_FRAME_TYPES,
	NL80211_ATTR_FRAME_TYPE,

	NL80211_ATTR_CONTROL_PORT_ETHERTYPE,
	NL80211_ATTR_CONTROL_PORT_NO_ENCRYPT,

	NL80211_ATTR_SUPPORT_IBSS_RSN,

	NL80211_ATTR_WIPHY_ANTENNA_TX,
	NL80211_ATTR_WIPHY_ANTENNA_RX,

	NL80211_ATTR_MCAST_RATE,

	NL80211_ATTR_OFFCHANNEL_TX_OK,

	NL80211_ATTR_BSS_HT_OPMODE,

	NL80211_ATTR_KEY_DEFAULT_TYPES,

	NL80211_ATTR_MAX_REMAIN_ON_CHANNEL_DURATION,

	NL80211_ATTR_MESH_SETUP,

	NL80211_ATTR_WIPHY_ANTENNA_AVAIL_TX,
	NL80211_ATTR_WIPHY_ANTENNA_AVAIL_RX,

	NL80211_ATTR_SUPPORT_MESH_AUTH,
	NL80211_ATTR_STA_PLINK_STATE,

	NL80211_ATTR_WOWLAN_TRIGGERS,
	NL80211_ATTR_WOWLAN_TRIGGERS_SUPPORTED,

	NL80211_ATTR_SCHED_SCAN_INTERVAL,

	NL80211_ATTR_INTERFACE_COMBINATIONS,
	NL80211_ATTR_SOFTWARE_IFTYPES,

	NL80211_ATTR_REKEY_DATA,

	NL80211_ATTR_MAX_NUM_SCHED_SCAN_SSIDS,
	NL80211_ATTR_MAX_SCHED_SCAN_IE_LEN,

	NL80211_ATTR_SCAN_SUPP_RATES,

	NL80211_ATTR_HIDDEN_SSID,

	NL80211_ATTR_IE_PROBE_RESP,
	NL80211_ATTR_IE_ASSOC_RESP,

	NL80211_ATTR_STA_WME,
	NL80211_ATTR_SUPPORT_AP_UAPSD,

	NL80211_ATTR_ROAM_SUPPORT,

	NL80211_ATTR_SCHED_SCAN_MATCH,
	NL80211_ATTR_MAX_MATCH_SETS,

	NL80211_ATTR_PMKSA_CANDIDATE,

	NL80211_ATTR_TX_NO_CCK_RATE,

	NL80211_ATTR_TDLS_ACTION,
	NL80211_ATTR_TDLS_DIALOG_TOKEN,
	NL80211_ATTR_TDLS_OPERATION,
	NL80211_ATTR_TDLS_SUPPORT,
	NL80211_ATTR_TDLS_EXTERNAL_SETUP,

	NL80211_ATTR_DEVICE_AP_SME,

	NL80211_ATTR_DONT_WAIT_FOR_ACK,

	NL80211_ATTR_FEATURE_FLAGS,

	NL80211_ATTR_PROBE_RESP_OFFLOAD,

	NL80211_ATTR_PROBE_RESP,

	NL80211_ATTR_DFS_REGION,

	NL80211_ATTR_DISABLE_HT,
	NL80211_ATTR_HT_CAPABILITY_MASK,

	NL80211_ATTR_NOACK_MAP,

	NL80211_ATTR_INACTIVITY_TIMEOUT,

	NL80211_ATTR_RX_SIGNAL_DBM,

	NL80211_ATTR_BG_SCAN_PERIOD,

	NL80211_ATTR_WDEV,

	NL80211_ATTR_USER_REG_HINT_TYPE,

	NL80211_ATTR_CONN_FAILED_REASON,

	NL80211_ATTR_SAE_DATA,

	NL80211_ATTR_VHT_CAPABILITY,

	NL80211_ATTR_SCAN_FLAGS,

	NL80211_ATTR_CHANNEL_WIDTH,
	NL80211_ATTR_CENTER_FREQ1,
	NL80211_ATTR_CENTER_FREQ2,

	NL80211_ATTR_P2P_CTWINDOW,
	NL80211_ATTR_P2P_OPPPS,

	NL80211_ATTR_LOCAL_MESH_POWER_MODE,

	NL80211_ATTR_ACL_POLICY,

	NL80211_ATTR_MAC_ADDRS,

	NL80211_ATTR_MAC_ACL_MAX,

	NL80211_ATTR_RADAR_EVENT,

	NL80211_ATTR_EXT_CAPA,
	NL80211_ATTR_EXT_CAPA_MASK,

	NL80211_ATTR_STA_CAPABILITY,
	NL80211_ATTR_STA_EXT_CAPABILITY,

	NL80211_ATTR_PROTOCOL_FEATURES,
	NL80211_ATTR_SPLIT_WIPHY_DUMP,

	NL80211_ATTR_DISABLE_VHT,
	NL80211_ATTR_VHT_CAPABILITY_MASK,

	NL80211_ATTR_MDID,
	NL80211_ATTR_IE_RIC,

	NL80211_ATTR_CRIT_PROT_ID,
	NL80211_ATTR_MAX_CRIT_PROT_DURATION,

	NL80211_ATTR_PEER_AID,

	NL80211_ATTR_COALESCE_RULE,

	NL80211_ATTR_CH_SWITCH_COUNT,
	NL80211_ATTR_CH_SWITCH_BLOCK_TX,
	NL80211_ATTR_CSA_IES,
	NL80211_ATTR_CSA_C_OFF_BEACON,
	NL80211_ATTR_CSA_C_OFF_PRESP,

	NL80211_ATTR_RXMGMT_FLAGS,

	NL80211_ATTR_STA_SUPPORTED_CHANNELS,

	NL80211_ATTR_STA_SUPPORTED_OPER_CLASSES,

	NL80211_ATTR_HANDLE_DFS,

	NL80211_ATTR_SUPPORT_5_MHZ,
	NL80211_ATTR_SUPPORT_10_MHZ,

	NL80211_ATTR_OPMODE_NOTIF,

	NL80211_ATTR_VENDOR_ID,
	NL80211_ATTR_VENDOR_SUBCMD,
	NL80211_ATTR_VENDOR_DATA,

	NL80211_ATTR_VENDOR_EVENTS,

	NL80211_ATTR_QOS_MAP,

	NL80211_ATTR_MAC_HINT,
	NL80211_ATTR_WIPHY_FREQ_HINT,

	NL80211_ATTR_MAX_AP_ASSOC_STA,

	NL80211_ATTR_TDLS_PEER_CAPABILITY,

	/*                                                     */

	__NL80211_ATTR_AFTER_LAST,
	NL80211_ATTR_MAX = __NL80211_ATTR_AFTER_LAST - 1
};

/*                                */
#define NL80211_ATTR_SCAN_GENERATION NL80211_ATTR_GENERATION
#define	NL80211_ATTR_MESH_PARAMS NL80211_ATTR_MESH_CONFIG

/*
                                                                             
       
 */
#define NL80211_CMD_CONNECT NL80211_CMD_CONNECT
#define NL80211_ATTR_HT_CAPABILITY NL80211_ATTR_HT_CAPABILITY
#define NL80211_ATTR_BSS_BASIC_RATES NL80211_ATTR_BSS_BASIC_RATES
#define NL80211_ATTR_WIPHY_TXQ_PARAMS NL80211_ATTR_WIPHY_TXQ_PARAMS
#define NL80211_ATTR_WIPHY_FREQ NL80211_ATTR_WIPHY_FREQ
#define NL80211_ATTR_WIPHY_CHANNEL_TYPE NL80211_ATTR_WIPHY_CHANNEL_TYPE
#define NL80211_ATTR_MGMT_SUBTYPE NL80211_ATTR_MGMT_SUBTYPE
#define NL80211_ATTR_IE NL80211_ATTR_IE
#define NL80211_ATTR_REG_INITIATOR NL80211_ATTR_REG_INITIATOR
#define NL80211_ATTR_REG_TYPE NL80211_ATTR_REG_TYPE
#define NL80211_ATTR_FRAME NL80211_ATTR_FRAME
#define NL80211_ATTR_SSID NL80211_ATTR_SSID
#define NL80211_ATTR_AUTH_TYPE NL80211_ATTR_AUTH_TYPE
#define NL80211_ATTR_REASON_CODE NL80211_ATTR_REASON_CODE
#define NL80211_ATTR_CIPHER_SUITES_PAIRWISE NL80211_ATTR_CIPHER_SUITES_PAIRWISE
#define NL80211_ATTR_CIPHER_SUITE_GROUP NL80211_ATTR_CIPHER_SUITE_GROUP
#define NL80211_ATTR_WPA_VERSIONS NL80211_ATTR_WPA_VERSIONS
#define NL80211_ATTR_AKM_SUITES NL80211_ATTR_AKM_SUITES
#define NL80211_ATTR_KEY NL80211_ATTR_KEY
#define NL80211_ATTR_KEYS NL80211_ATTR_KEYS
#define NL80211_ATTR_FEATURE_FLAGS NL80211_ATTR_FEATURE_FLAGS

#define NL80211_MAX_SUPP_RATES			32
#define NL80211_MAX_SUPP_HT_RATES		77
#define NL80211_MAX_SUPP_REG_RULES		32
#define NL80211_TKIP_DATA_OFFSET_ENCR_KEY	0
#define NL80211_TKIP_DATA_OFFSET_TX_MIC_KEY	16
#define NL80211_TKIP_DATA_OFFSET_RX_MIC_KEY	24
#define NL80211_HT_CAPABILITY_LEN		26
#define NL80211_VHT_CAPABILITY_LEN		12

//     
#ifdef CONFIG_BRCM_WAPI
#define NL80211_MAX_NR_CIPHER_SUITES		6
#define NL80211_MAX_NR_AKM_SUITES		4
#else
#define NL80211_MAX_NR_CIPHER_SUITES		5
#define NL80211_MAX_NR_AKM_SUITES		2
#endif
//     

/* 
                                                  
  
                                                                
                                                
                                              
                                   
                                                                             
                                                                       
                     
                                                       
                                                                  
                                         
                                         
                                          
                                                                       
                                                          
  
                                                      
                                   
  
 */
enum nl80211_iftype {
	NL80211_IFTYPE_UNSPECIFIED,
	NL80211_IFTYPE_ADHOC,
	NL80211_IFTYPE_STATION,
	NL80211_IFTYPE_AP,
	NL80211_IFTYPE_AP_VLAN,
	NL80211_IFTYPE_WDS,
	NL80211_IFTYPE_MONITOR,
	NL80211_IFTYPE_MESH_POINT,
	NL80211_IFTYPE_P2P_CLIENT,
	NL80211_IFTYPE_P2P_GO,

	/*           */
	NUM_NL80211_IFTYPES,
	NL80211_IFTYPE_MAX = NUM_NL80211_IFTYPES - 1
};

/* 
                                         
  
                                                                   
                                                              
  
                                                              
                                                               
                                                                           
                             
                                                    
                                                                  
                                                            
                                                                          
                                                                       
                                                                      
                                                                       
              
                                                                       
                                               
 */
enum nl80211_sta_flags {
	__NL80211_STA_FLAG_INVALID,
	NL80211_STA_FLAG_AUTHORIZED,
	NL80211_STA_FLAG_SHORT_PREAMBLE,
	NL80211_STA_FLAG_WME,
	NL80211_STA_FLAG_MFP,
	NL80211_STA_FLAG_AUTHENTICATED,
	NL80211_STA_FLAG_TDLS_PEER,

	/*           */
	__NL80211_STA_FLAG_AFTER_LAST,
	NL80211_STA_FLAG_MAX = __NL80211_STA_FLAG_AFTER_LAST - 1
};

/* 
                                                          
                                      
                                    
  
                                                                 
 */
struct nl80211_sta_flag_update {
	__u32 mask;
	__u32 set;
} __attribute__((packed));

/* 
                                               
  
                                                               
                                                           
  
                                                               
                                                             
                                                     
                                                              
                                                    
                                                                     
                                                     
                                                            
                                                   
                                                         
                                                     
                                                
 */
enum nl80211_rate_info {
	__NL80211_RATE_INFO_INVALID,
	NL80211_RATE_INFO_BITRATE,
	NL80211_RATE_INFO_MCS,
	NL80211_RATE_INFO_40_MHZ_WIDTH,
	NL80211_RATE_INFO_SHORT_GI,
	NL80211_RATE_INFO_VHT_MCS,
	NL80211_RATE_INFO_VHT_NSS,
	NL80211_RATE_INFO_80_MHZ_WIDTH,
	NL80211_RATE_INFO_80P80_MHZ_WIDTH,
	NL80211_RATE_INFO_160_MHZ_WIDTH,

	/*           */
	__NL80211_RATE_INFO_AFTER_LAST,
	NL80211_RATE_INFO_MAX = __NL80211_RATE_INFO_AFTER_LAST - 1
};

/* 
                                                                
  
                                                                  
                                                           
  
                                                                   
                                                                            
                                                                            
         
                                                                              
         
                                                                     
                                                                
                                                                             
                                                    
 */
enum nl80211_sta_bss_param {
	__NL80211_STA_BSS_PARAM_INVALID,
	NL80211_STA_BSS_PARAM_CTS_PROT,
	NL80211_STA_BSS_PARAM_SHORT_PREAMBLE,
	NL80211_STA_BSS_PARAM_SHORT_SLOT_TIME,
	NL80211_STA_BSS_PARAM_DTIM_PERIOD,
	NL80211_STA_BSS_PARAM_BEACON_INTERVAL,

	/*           */
	__NL80211_STA_BSS_PARAM_AFTER_LAST,
	NL80211_STA_BSS_PARAM_MAX = __NL80211_STA_BSS_PARAM_AFTER_LAST - 1
};

/* 
                                              
  
                                                             
                                            
  
                                                              
                                                                         
                                                                            
                                                                             
                                                                            
                                                                          
                                                            
                                                                               
                                                                        
           
                                                                     
                                                                           
                                                                  
                                                  
                                                  
                                                                 
                                  
                                                                        
                                               
                                                                               
                                                                   
                                                                             
                                                                          
                                                                               
                                           
                                                                 
 */
enum nl80211_sta_info {
	__NL80211_STA_INFO_INVALID,
	NL80211_STA_INFO_INACTIVE_TIME,
	NL80211_STA_INFO_RX_BYTES,
	NL80211_STA_INFO_TX_BYTES,
	NL80211_STA_INFO_LLID,
	NL80211_STA_INFO_PLID,
	NL80211_STA_INFO_PLINK_STATE,
	NL80211_STA_INFO_SIGNAL,
	NL80211_STA_INFO_TX_BITRATE,
	NL80211_STA_INFO_RX_PACKETS,
	NL80211_STA_INFO_TX_PACKETS,
	NL80211_STA_INFO_TX_RETRIES,
	NL80211_STA_INFO_TX_FAILED,
	NL80211_STA_INFO_SIGNAL_AVG,
	NL80211_STA_INFO_RX_BITRATE,
	NL80211_STA_INFO_BSS_PARAM,
	NL80211_STA_INFO_CONNECTED_TIME,
	NL80211_STA_INFO_STA_FLAGS,
	NL80211_STA_INFO_BEACON_LOSS,

	/*           */
	__NL80211_STA_INFO_AFTER_LAST,
	NL80211_STA_INFO_MAX = __NL80211_STA_INFO_AFTER_LAST - 1
};

/* 
                                                     
  
                                                      
                                                                            
                                                                  
                                                                 
                                                                          
 */
enum nl80211_mpath_flags {
	NL80211_MPATH_FLAG_ACTIVE =	1<<0,
	NL80211_MPATH_FLAG_RESOLVING =	1<<1,
	NL80211_MPATH_FLAG_SN_VALID =	1<<2,
	NL80211_MPATH_FLAG_FIXED =	1<<3,
	NL80211_MPATH_FLAG_RESOLVED =	1<<4,
};

/* 
                                                  
  
                                                                            
                                 
  
                                                                
                                                                               
                                                      
                                                              
                                                                              
                                                            
                              
                                                                               
                                                                     
                                                                          
                   
                                                 
 */
enum nl80211_mpath_info {
	__NL80211_MPATH_INFO_INVALID,
	NL80211_MPATH_INFO_FRAME_QLEN,
	NL80211_MPATH_INFO_SN,
	NL80211_MPATH_INFO_METRIC,
	NL80211_MPATH_INFO_EXPTIME,
	NL80211_MPATH_INFO_FLAGS,
	NL80211_MPATH_INFO_DISCOVERY_TIMEOUT,
	NL80211_MPATH_INFO_DISCOVERY_RETRIES,

	/*           */
	__NL80211_MPATH_INFO_AFTER_LAST,
	NL80211_MPATH_INFO_MAX = __NL80211_MPATH_INFO_AFTER_LAST - 1
};

/* 
                                           
                                                               
                                                                
                                          
                                                             
                                        
                                                                             
                     
                                                                           
                                                               
                                                                 
                                                                              
                      
                                                                             
                                                                   
                                                
 */
enum nl80211_band_attr {
	__NL80211_BAND_ATTR_INVALID,
	NL80211_BAND_ATTR_FREQS,
	NL80211_BAND_ATTR_RATES,

	NL80211_BAND_ATTR_HT_MCS_SET,
	NL80211_BAND_ATTR_HT_CAPA,
	NL80211_BAND_ATTR_HT_AMPDU_FACTOR,
	NL80211_BAND_ATTR_HT_AMPDU_DENSITY,

	NL80211_BAND_ATTR_VHT_MCS_SET,
	NL80211_BAND_ATTR_VHT_CAPA,

	/*           */
	__NL80211_BAND_ATTR_AFTER_LAST,
	NL80211_BAND_ATTR_MAX = __NL80211_BAND_ATTR_AFTER_LAST - 1
};

#define NL80211_BAND_ATTR_HT_CAPA NL80211_BAND_ATTR_HT_CAPA

/* 
                                                     
                                                                    
                                                 
                                                                   
                     
                                                                 
                                                          
                                                                   
                                                
                                                              
                                                
                                                                          
               
                                                                  
                    
                                                     
 */
enum nl80211_frequency_attr {
	__NL80211_FREQUENCY_ATTR_INVALID,
	NL80211_FREQUENCY_ATTR_FREQ,
	NL80211_FREQUENCY_ATTR_DISABLED,
	NL80211_FREQUENCY_ATTR_PASSIVE_SCAN,
	NL80211_FREQUENCY_ATTR_NO_IBSS,
	NL80211_FREQUENCY_ATTR_RADAR,
	NL80211_FREQUENCY_ATTR_MAX_TX_POWER,

	/*           */
	__NL80211_FREQUENCY_ATTR_AFTER_LAST,
	NL80211_FREQUENCY_ATTR_MAX = __NL80211_FREQUENCY_ATTR_AFTER_LAST - 1
};

#define NL80211_FREQUENCY_ATTR_MAX_TX_POWER NL80211_FREQUENCY_ATTR_MAX_TX_POWER

/* 
                                                 
                                                                  
                                                           
                                                                     
                   
                                                              
                    
                                                   
 */
enum nl80211_bitrate_attr {
	__NL80211_BITRATE_ATTR_INVALID,
	NL80211_BITRATE_ATTR_RATE,
	NL80211_BITRATE_ATTR_2GHZ_SHORTPREAMBLE,

	/*           */
	__NL80211_BITRATE_ATTR_AFTER_LAST,
	NL80211_BITRATE_ATTR_MAX = __NL80211_BITRATE_ATTR_AFTER_LAST - 1
};

/* 
                                                                           
                                                                     
                      
                                                                       
                      
                                                                      
                                                                            
                                                                       
                                                                     
                                                                  
                                                                    
                                                                
                                                                   
                                                                           
 */
enum nl80211_reg_initiator {
	NL80211_REGDOM_SET_BY_CORE,
	NL80211_REGDOM_SET_BY_USER,
	NL80211_REGDOM_SET_BY_DRIVER,
	NL80211_REGDOM_SET_BY_COUNTRY_IE,
};

/* 
                                                                  
                                                                               
                                                               
                                                  
                                                                                
           
                                                                           
                                                                           
                                                                          
                       
                                                                              
                                                                      
                                                                       
                                  
 */
enum nl80211_reg_type {
	NL80211_REGDOM_TYPE_COUNTRY,
	NL80211_REGDOM_TYPE_WORLD,
	NL80211_REGDOM_TYPE_CUSTOM_WORLD,
	NL80211_REGDOM_TYPE_INTERSECTION,
};

/* 
                                                          
                                                                   
                                                                        
                                                             
                                 
                                                                         
                                                                           
              
                                                                         
                                                                     
              
                                                                      
                            
                                                                          
                                                                 
                                               
                                                                  
                                                             
                                                                       
                    
                                                    
 */
enum nl80211_reg_rule_attr {
	__NL80211_REG_RULE_ATTR_INVALID,
	NL80211_ATTR_REG_RULE_FLAGS,

	NL80211_ATTR_FREQ_RANGE_START,
	NL80211_ATTR_FREQ_RANGE_END,
	NL80211_ATTR_FREQ_RANGE_MAX_BW,

	NL80211_ATTR_POWER_RULE_MAX_ANT_GAIN,
	NL80211_ATTR_POWER_RULE_MAX_EIRP,

	/*           */
	__NL80211_REG_RULE_ATTR_AFTER_LAST,
	NL80211_REG_RULE_ATTR_MAX = __NL80211_REG_RULE_ATTR_AFTER_LAST - 1
};

/* 
                                                                       
                                                                           
                                                                     
                                      
                                                                    
                                     
                                                            
 */
enum nl80211_sched_scan_match_attr {
	__NL80211_SCHED_SCAN_MATCH_ATTR_INVALID,

	NL80211_ATTR_SCHED_SCAN_MATCH_SSID,

	/*           */
	__NL80211_SCHED_SCAN_MATCH_ATTR_AFTER_LAST,
	NL80211_SCHED_SCAN_MATCH_ATTR_MAX =
		__NL80211_SCHED_SCAN_MATCH_ATTR_AFTER_LAST - 1
};

/* 
                                                      
  
                                                    
                                                  
                                                       
                                                         
                                                       
                                                               
                                                                      
                                                      
                                           
 */
enum nl80211_reg_rule_flags {
	NL80211_RRF_NO_OFDM		= 1<<0,
	NL80211_RRF_NO_CCK		= 1<<1,
	NL80211_RRF_NO_INDOOR		= 1<<2,
	NL80211_RRF_NO_OUTDOOR		= 1<<3,
	NL80211_RRF_DFS			= 1<<4,
	NL80211_RRF_PTP_ONLY		= 1<<5,
	NL80211_RRF_PTMP_ONLY		= 1<<6,
	NL80211_RRF_PASSIVE_SCAN	= 1<<7,
	NL80211_RRF_NO_IBSS		= 1<<8,
};

/* 
                                                    
  
                                                                 
                                                               
                                                                
                                                                       
 */
enum nl80211_dfs_regions {
	NL80211_DFS_UNSET	= 0,
	NL80211_DFS_FCC		= 1,
	NL80211_DFS_ETSI	= 2,
	NL80211_DFS_JP		= 3,
};

/* 
                                                
  
                                                                
                                           
  
                                                                 
                                                              
                                                               
                                                               
                                                                           
                        
                                                                         
                                                                    
                                                                           
                          
                                                                       
                 
                                                                       
                    
                                                                 
                    
                                                  
 */
enum nl80211_survey_info {
	__NL80211_SURVEY_INFO_INVALID,
	NL80211_SURVEY_INFO_FREQUENCY,
	NL80211_SURVEY_INFO_NOISE,
	NL80211_SURVEY_INFO_IN_USE,
	NL80211_SURVEY_INFO_CHANNEL_TIME,
	NL80211_SURVEY_INFO_CHANNEL_TIME_BUSY,
	NL80211_SURVEY_INFO_CHANNEL_TIME_EXT_BUSY,
	NL80211_SURVEY_INFO_CHANNEL_TIME_RX,
	NL80211_SURVEY_INFO_CHANNEL_TIME_TX,

	/*           */
	__NL80211_SURVEY_INFO_AFTER_LAST,
	NL80211_SURVEY_INFO_MAX = __NL80211_SURVEY_INFO_AFTER_LAST - 1
};

/* 
                                                        
  
                               
  
                                         
  
                                                       
                                                         
                                                  
                                                        
                                                                  
                             
  
                                                
                                                        
 */
enum nl80211_mntr_flags {
	__NL80211_MNTR_FLAG_INVALID,
	NL80211_MNTR_FLAG_FCSFAIL,
	NL80211_MNTR_FLAG_PLCPFAIL,
	NL80211_MNTR_FLAG_CONTROL,
	NL80211_MNTR_FLAG_OTHER_BSS,
	NL80211_MNTR_FLAG_COOK_FRAMES,

	/*           */
	__NL80211_MNTR_FLAG_AFTER_LAST,
	NL80211_MNTR_FLAG_MAX = __NL80211_MNTR_FLAG_AFTER_LAST - 1
};

/* 
                                                               
  
                                                                        
          
  
                                            
  
                                                                          
                                                        
  
                                                                               
                                                                           
  
                                                                       
                    
  
                                                                         
                         
  
                                                                           
                                                                           
       
  
                                                                               
         
  
                                                                      
                                                        
  
                                                                       
                                                                          
          
  
                                                                            
                    
  
                                                                          
                                                        
  
                                                                               
                                                                             
                                     
  
                                                                             
                                                                           
                    
  
                                                                           
                                                                             
  
                                                                       
  
                                                                           
                                                 
  
                                                                               
                                      
  
                                                                             
                                                                      
                       
  
                                                                             
                                                                           
                
  
                                                                             
                                                             
  
                                                                              
                                                                          
               
  
                                                                            
  
                                                    
 */
enum nl80211_meshconf_params {
	__NL80211_MESHCONF_INVALID,
	NL80211_MESHCONF_RETRY_TIMEOUT,
	NL80211_MESHCONF_CONFIRM_TIMEOUT,
	NL80211_MESHCONF_HOLDING_TIMEOUT,
	NL80211_MESHCONF_MAX_PEER_LINKS,
	NL80211_MESHCONF_MAX_RETRIES,
	NL80211_MESHCONF_TTL,
	NL80211_MESHCONF_AUTO_OPEN_PLINKS,
	NL80211_MESHCONF_HWMP_MAX_PREQ_RETRIES,
	NL80211_MESHCONF_PATH_REFRESH_TIME,
	NL80211_MESHCONF_MIN_DISCOVERY_TIMEOUT,
	NL80211_MESHCONF_HWMP_ACTIVE_PATH_TIMEOUT,
	NL80211_MESHCONF_HWMP_PREQ_MIN_INTERVAL,
	NL80211_MESHCONF_HWMP_NET_DIAM_TRVS_TIME,
	NL80211_MESHCONF_HWMP_ROOTMODE,
	NL80211_MESHCONF_ELEMENT_TTL,
	NL80211_MESHCONF_HWMP_RANN_INTERVAL,
	NL80211_MESHCONF_GATE_ANNOUNCEMENTS,
	NL80211_MESHCONF_HWMP_PERR_MIN_INTERVAL,
	NL80211_MESHCONF_FORWARDING,
	NL80211_MESHCONF_RSSI_THRESHOLD,

	/*           */
	__NL80211_MESHCONF_ATTR_AFTER_LAST,
	NL80211_MESHCONF_ATTR_MAX = __NL80211_MESHCONF_ATTR_AFTER_LAST - 1
};

/* 
                                                         
  
                                                                            
                                    
  
                                              
  
                                                                          
                                                                            
        
  
                                                                        
                                                                       
          
  
                                                                              
                                                                            
                                                                              
  
                                                                              
                                                 
  
                                                                              
                                                                               
                                                                       
                                                                      
                                                                   
                                                                            
                                                                              
                                                                            
  
                                                                             
                                                      
 */
enum nl80211_mesh_setup_params {
	__NL80211_MESH_SETUP_INVALID,
	NL80211_MESH_SETUP_ENABLE_VENDOR_PATH_SEL,
	NL80211_MESH_SETUP_ENABLE_VENDOR_METRIC,
	NL80211_MESH_SETUP_IE,
	NL80211_MESH_SETUP_USERSPACE_AUTH,
	NL80211_MESH_SETUP_USERSPACE_AMPE,

	/*           */
	__NL80211_MESH_SETUP_ATTR_AFTER_LAST,
	NL80211_MESH_SETUP_ATTR_MAX = __NL80211_MESH_SETUP_ATTR_AFTER_LAST - 1
};

/* 
                                                        
                                                              
                                                                 
                                                                             
           
                                                                          
                               
                                                                          
                               
                                                                
                                           
                                                      
 */
enum nl80211_txq_attr {
	__NL80211_TXQ_ATTR_INVALID,
	NL80211_TXQ_ATTR_QUEUE,
	NL80211_TXQ_ATTR_TXOP,
	NL80211_TXQ_ATTR_CWMIN,
	NL80211_TXQ_ATTR_CWMAX,
	NL80211_TXQ_ATTR_AIFS,

	/*           */
	__NL80211_TXQ_ATTR_AFTER_LAST,
	NL80211_TXQ_ATTR_MAX = __NL80211_TXQ_ATTR_AFTER_LAST - 1
};

enum nl80211_txq_q {
	NL80211_TXQ_Q_VO,
	NL80211_TXQ_Q_VI,
	NL80211_TXQ_Q_BE,
	NL80211_TXQ_Q_BK
};

enum nl80211_channel_type {
	NL80211_CHAN_NO_HT,
	NL80211_CHAN_HT20,
	NL80211_CHAN_HT40MINUS,
	NL80211_CHAN_HT40PLUS
};

/* 
                                                  
  
                                  
                                                  
                                                 
                                                                    
                                                                  
                                                                  
                                                
                                                                    
                                                             
                                                                     
                                                                 
                                                                      
                                                                         
                                
                                                                        
                                        
                                                                       
                                                                          
                                                                    
                           
                                                                           
                                              
                                                     
                                                        
                                                                           
                                                                         
                    
                                                                
                                     
                                                                 
                                                         
                                                                             
                                                                         
                                      
                                          
 */
enum nl80211_bss {
	__NL80211_BSS_INVALID,
	NL80211_BSS_BSSID,
	NL80211_BSS_FREQUENCY,
	NL80211_BSS_TSF,
	NL80211_BSS_BEACON_INTERVAL,
	NL80211_BSS_CAPABILITY,
	NL80211_BSS_INFORMATION_ELEMENTS,
	NL80211_BSS_SIGNAL_MBM,
	NL80211_BSS_SIGNAL_UNSPEC,
	NL80211_BSS_STATUS,
	NL80211_BSS_SEEN_MS_AGO,
	NL80211_BSS_BEACON_IES,
	NL80211_BSS_CHAN_WIDTH,
	NL80211_BSS_BEACON_TSF,
	NL80211_BSS_PRESP_DATA,

	/*           */
	__NL80211_BSS_AFTER_LAST,
	NL80211_BSS_MAX = __NL80211_BSS_AFTER_LAST - 1
};

/* 
                                         
                                                                  
                                                            
                                                        
  
                                                         
                                                        
 */
enum nl80211_bss_status {
	NL80211_BSS_STATUS_AUTHENTICATED,
	NL80211_BSS_STATUS_ASSOCIATED,
	NL80211_BSS_STATUS_IBSS_JOINED,
};

/* 
                                              
  
                                                            
                                                                     
                                                           
                                                                              
                                    
                                                      
                                                                        
                                                                  
                                              
 */
enum nl80211_auth_type {
	NL80211_AUTHTYPE_OPEN_SYSTEM,
	NL80211_AUTHTYPE_SHARED_KEY,
	NL80211_AUTHTYPE_FT,
	NL80211_AUTHTYPE_NETWORK_EAP,

	/*           */
	__NL80211_AUTHTYPE_NUM,
	NL80211_AUTHTYPE_MAX = __NL80211_AUTHTYPE_NUM - 1,
	NL80211_AUTHTYPE_AUTOMATIC
};

/* 
                                   
                                                          
                                                               
                                          
                                                     
 */
enum nl80211_key_type {
	NL80211_KEYTYPE_GROUP,
	NL80211_KEYTYPE_PAIRWISE,
	NL80211_KEYTYPE_PEERKEY,

	NUM_NL80211_KEYTYPES
};

/* 
                                                       
                                                        
                                                              
 */
enum nl80211_mfp {
	NL80211_MFP_NO,
	NL80211_MFP_REQUIRED,
};

enum nl80211_wpa_versions {
	NL80211_WPA_VERSION_1 = 1 << 0,
	NL80211_WPA_VERSION_2 = 1 << 1,
//     
#ifdef CONFIG_BRCM_WAPI
	NL80211_WAPI_VERSION_1 = 1 << 2,
#endif
};

/* 
                                                     
                                               
                                                                   
              
                                                                     
                
                                                          
 */
enum nl80211_key_default_types {
	__NL80211_KEY_DEFAULT_TYPE_INVALID,
	NL80211_KEY_DEFAULT_TYPE_UNICAST,
	NL80211_KEY_DEFAULT_TYPE_MULTICAST,

	NUM_NL80211_KEY_DEFAULT_TYPES
};

/* 
                                               
                                  
                                                                    
                                                                     
       
                                     
                                                                        
                                       
                                                                      
                                             
                                                    
                                                                    
                                                                     
                                                             
                                                    
                                                                  
                                                                 
                                       
                                      
                                          
 */
enum nl80211_key_attributes {
	__NL80211_KEY_INVALID,
	NL80211_KEY_DATA,
	NL80211_KEY_IDX,
	NL80211_KEY_CIPHER,
	NL80211_KEY_SEQ,
	NL80211_KEY_DEFAULT,
	NL80211_KEY_DEFAULT_MGMT,
	NL80211_KEY_TYPE,
	NL80211_KEY_DEFAULT_TYPES,

	/*           */
	__NL80211_KEY_AFTER_LAST,
	NL80211_KEY_MAX = __NL80211_KEY_AFTER_LAST - 1
};

/* 
                                                           
                                     
                                                                               
                                                                         
                                                               
                                              
                                                                    
                              
                                         
                                                 
 */
enum nl80211_tx_rate_attributes {
	__NL80211_TXRATE_INVALID,
	NL80211_TXRATE_LEGACY,
	NL80211_TXRATE_MCS,

	/*           */
	__NL80211_TXRATE_AFTER_LAST,
	NL80211_TXRATE_MAX = __NL80211_TXRATE_AFTER_LAST - 1
};

/* 
                                     
                                       
                                                        
 */
enum nl80211_band {
	NL80211_BAND_2GHZ,
	NL80211_BAND_5GHZ,
};

enum nl80211_ps_state {
	NL80211_PS_DISABLED,
	NL80211_PS_ENABLED,
};

/* 
                                                                
                                       
                                                                            
                                                                        
              
                                                                            
                                                                        
                                                                   
                                                               
                                                                          
                                                        
                                           
                                               
 */
enum nl80211_attr_cqm {
	__NL80211_ATTR_CQM_INVALID,
	NL80211_ATTR_CQM_RSSI_THOLD,
	NL80211_ATTR_CQM_RSSI_HYST,
	NL80211_ATTR_CQM_RSSI_THRESHOLD_EVENT,
	NL80211_ATTR_CQM_PKT_LOSS_EVENT,

	/*           */
	__NL80211_ATTR_CQM_AFTER_LAST,
	NL80211_ATTR_CQM_MAX = __NL80211_ATTR_CQM_AFTER_LAST - 1
};

/* 
                                                               
                                                                          
                            
                                                                      
                            
 */
enum nl80211_cqm_rssi_threshold_event {
	NL80211_CQM_RSSI_THRESHOLD_EVENT_LOW,
	NL80211_CQM_RSSI_THRESHOLD_EVENT_HIGH,
};


/* 
                                                      
                                                                      
                                                                 
                                                             
 */
enum nl80211_tx_power_setting {
	NL80211_TX_POWER_AUTOMATIC,
	NL80211_TX_POWER_LIMITED,
	NL80211_TX_POWER_FIXED,
};

/* 
                                                                            
                                                                        
                                                                         
                         
                                                                         
                                                                       
                                                                     
                                                                   
                                                                      
                                                                    
                                                                  
                                                
                                                                   
                                                                   
                                                            
                                                   
                                                   
 */
enum nl80211_wowlan_packet_pattern_attr {
	__NL80211_WOWLAN_PKTPAT_INVALID,
	NL80211_WOWLAN_PKTPAT_MASK,
	NL80211_WOWLAN_PKTPAT_PATTERN,

	NUM_NL80211_WOWLAN_PKTPAT,
	MAX_NL80211_WOWLAN_PKTPAT = NUM_NL80211_WOWLAN_PKTPAT - 1,
};

/* 
                                                                      
                                                      
                                                   
                                                   
  
                                                                  
                                                                 
                                                           
 */
struct nl80211_wowlan_pattern_support {
	__u32 max_patterns;
	__u32 min_pattern_len;
	__u32 max_pattern_len;
} __attribute__((packed));

/* 
                                                            
                                                                       
                                                                       
                                                                   
                                                 
                                                                             
                                                
                                                                             
                                                             
                                                                             
                                                                           
                                                                           
                                                                          
                                                                         
                                                     
  
                                                                       
                                                     
                                                                              
                                                                          
                       
                                                                           
                             
                                                                          
                
                                                                         
                                                                       
                                                     
                                                                
                                                                    
 */
enum nl80211_wowlan_triggers {
	__NL80211_WOWLAN_TRIG_INVALID,
	NL80211_WOWLAN_TRIG_ANY,
	NL80211_WOWLAN_TRIG_DISCONNECT,
	NL80211_WOWLAN_TRIG_MAGIC_PKT,
	NL80211_WOWLAN_TRIG_PKT_PATTERN,
	NL80211_WOWLAN_TRIG_GTK_REKEY_SUPPORTED,
	NL80211_WOWLAN_TRIG_GTK_REKEY_FAILURE,
	NL80211_WOWLAN_TRIG_EAP_IDENT_REQUEST,
	NL80211_WOWLAN_TRIG_4WAY_HANDSHAKE,
	NL80211_WOWLAN_TRIG_RFKILL_RELEASE,

	/*           */
	NUM_NL80211_WOWLAN_TRIG,
	MAX_NL80211_WOWLAN_TRIG = NUM_NL80211_WOWLAN_TRIG - 1
};

/* 
                                                    
                                          
                                                              
                                                       
                                                            
                                                     
                                                 
                                                     
 */
enum nl80211_iface_limit_attrs {
	NL80211_IFACE_LIMIT_UNSPEC,
	NL80211_IFACE_LIMIT_MAX,
	NL80211_IFACE_LIMIT_TYPES,

	/*           */
	NUM_NL80211_IFACE_LIMIT,
	MAX_NL80211_IFACE_LIMIT = NUM_NL80211_IFACE_LIMIT - 1
};

/* 
                                                                        
  
                                         
                                                                      
                                                                  
                                                                       
                                                                    
                                                                   
                                                             
                                                                      
                                                                   
                                                                   
                                                
                                                                      
                                                    
                                                
                                                    
  
            
                                                                           
                                                
  
                                                          
                       
  
                                                   
                                           
  
                                                                
                                            
  
                                                                    
                                                                        
                                       
  
                                                                     
                                                                      
                                                                    
                                                     
                                                      
 */
enum nl80211_if_combination_attrs {
	NL80211_IFACE_COMB_UNSPEC,
	NL80211_IFACE_COMB_LIMITS,
	NL80211_IFACE_COMB_MAXNUM,
	NL80211_IFACE_COMB_STA_AP_BI_MATCH,
	NL80211_IFACE_COMB_NUM_CHANNELS,

	/*           */
	NUM_NL80211_IFACE_COMB,
	MAX_NL80211_IFACE_COMB = NUM_NL80211_IFACE_COMB - 1
};


/* 
                                                                            
  
                                                                
                                        
                                                                 
                 
                                                                   
                      
                                                             
                               
                                                      
                                                                      
                                                                
                      
                                                        
                                                                     
 */
enum nl80211_plink_state {
	NL80211_PLINK_LISTEN,
	NL80211_PLINK_OPN_SNT,
	NL80211_PLINK_OPN_RCVD,
	NL80211_PLINK_CNF_RCVD,
	NL80211_PLINK_ESTAB,
	NL80211_PLINK_HOLDING,
	NL80211_PLINK_BLOCKED,

	/*           */
	NUM_NL80211_PLINK_STATES,
	MAX_NL80211_PLINK_STATES = NUM_NL80211_PLINK_STATES - 1
};

#define NL80211_KCK_LEN			16
#define NL80211_KEK_LEN			16
#define NL80211_REPLAY_CTR_LEN		8

/* 
                                                             
                                                                      
                                                       
                                                         
                                                          
                                                                 
                                                              
 */
enum nl80211_rekey_data {
	__NL80211_REKEY_DATA_INVALID,
	NL80211_REKEY_DATA_KEK,
	NL80211_REKEY_DATA_KCK,
	NL80211_REKEY_DATA_REPLAY_CTR,

	/*           */
	NUM_NL80211_REKEY_DATA,
	MAX_NL80211_REKEY_DATA = NUM_NL80211_REKEY_DATA - 1
};

/* 
                                                                  
                                                                           
                 
                                                                             
                   
                                                                                
                                                              
 */
enum nl80211_hidden_ssid {
	NL80211_HIDDEN_SSID_NOT_IN_USE,
	NL80211_HIDDEN_SSID_ZERO_LEN,
	NL80211_HIDDEN_SSID_ZERO_CONTENTS
};

/* 
                                                     
                                                                  
                                                                    
                                                      
                                                                      
                                                                        
                                          
                                                      
 */
enum nl80211_sta_wme_attr {
	__NL80211_STA_WME_INVALID,
	NL80211_STA_WME_UAPSD_QUEUES,
	NL80211_STA_WME_MAX_SP,

	/*           */
	__NL80211_STA_WME_AFTER_LAST,
	NL80211_STA_WME_MAX = __NL80211_STA_WME_AFTER_LAST - 1
};

/* 
                                                                              
                                                                           
                                                                                
            
                                                             
                                                                            
                                                                             
             
                                                                          
             
 */
enum nl80211_pmksa_candidate_attr {
	__NL80211_PMKSA_CANDIDATE_INVALID,
	NL80211_PMKSA_CANDIDATE_INDEX,
	NL80211_PMKSA_CANDIDATE_BSSID,
	NL80211_PMKSA_CANDIDATE_PREAUTH,

	/*           */
	NUM_NL80211_PMKSA_CANDIDATE,
	MAX_NL80211_PMKSA_CANDIDATE = NUM_NL80211_PMKSA_CANDIDATE - 1
};

/* 
                                                                        
                                                             
                                       
                                                                            
                                              
                                                
 */
enum nl80211_tdls_operation {
	NL80211_TDLS_DISCOVERY_REQ,
	NL80211_TDLS_SETUP,
	NL80211_TDLS_TEARDOWN,
	NL80211_TDLS_ENABLE_LINK,
	NL80211_TDLS_DISABLE_LINK,
};

/*
                                                               
                                                  
                                  
                              
  
 */

/* 
                                                      
                                                                      
                                                              
                 
                                                                         
                                                                          
                                              
 */
enum nl80211_feature_flags {
	NL80211_FEATURE_SK_TX_STATUS	= 1 << 0,
	NL80211_FEATURE_HT_IBSS		= 1 << 1,
	NL80211_FEATURE_INACTIVITY_TIMER = 1 << 2,
};

/* 
                                                                    
                                                            
                                                                  
                                                              
                                                                 
                                                                
               
  
                                                                  
                                                                   
                                                           
                                                                  
 */
enum nl80211_probe_resp_offload_support_attr {
	NL80211_PROBE_RESP_OFFLOAD_SUPPORT_WPS =	1<<0,
	NL80211_PROBE_RESP_OFFLOAD_SUPPORT_WPS2 =	1<<1,
	NL80211_PROBE_RESP_OFFLOAD_SUPPORT_P2P =	1<<2,
	NL80211_PROBE_RESP_OFFLOAD_SUPPORT_80211U =	1<<3,
};

/* 
                                                                         
                                                                        
                                 
                                                                                
 */
enum nl80211_connect_failed_reason {
	NL80211_CONN_FAIL_MAX_CLIENTS,
	NL80211_CONN_FAIL_BLOCKED_CLIENT,
};

/* 
                                                  
  
                                                        
                                                         
                                         
  
                                                                    
                                                                    
                           
                                                                              
                                                                     
 */
enum nl80211_acl_policy {
	NL80211_ACL_POLICY_ACCEPT_UNLESS_LISTED,
	NL80211_ACL_POLICY_DENY_UNLESS_LISTED,
};
/* 
                                                                     
  
                                                    
                                                      
                                             
                                             
                                              
 */
enum nl80211_crit_proto_id {
	NL80211_CRIT_PROTO_UNSPEC,
	NL80211_CRIT_PROTO_DHCP,
	NL80211_CRIT_PROTO_EAPOL,
	NL80211_CRIT_PROTO_APIPA,
	/*                                     */
	NUM_NL80211_CRIT_PROTO
};

/*                                                 */
#define NL80211_CRIT_PROTO_MAX_DURATION		5000 /*      */

/*
                                                              
                                                               
                                   
 */
#define NL80211_VENDOR_ID_IS_LINUX	0x80000000

/* 
                                                       
                                                                         
                                                                     
                                                                    
                                  
                                          
 */
struct nl80211_vendor_cmd_info {
	__u32 vendor_id;
	__u32 subcmd;
};

/* 
                                                       
  
                                                                   
                                    
  
                                                  
                                                    
                                                    
 */
enum nl80211_tdls_peer_capability {
	NL80211_TDLS_PEER_HT = 1<<0,
	NL80211_TDLS_PEER_VHT = 1<<1,
	NL80211_TDLS_PEER_WMM = 1<<2,
};

#endif /*                   */
