/*
 * OpenRISC Linux
 *
 * SPR Definitions
 *
 * Copyright (C) 2000 Damjan Lampret
 * Copyright (C) 2003 Matjaz Breskvar <phoenix@bsemi.com>
 * Copyright (C) 2008, 2010 Embecosm Limited
 * Copyright (C) 2010-2011 Jonas Bonn <jonas@southpole.se>
 * et al.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This file is part of OpenRISC 1000 Architectural Simulator.
 */

#ifndef SPR_DEFS__H
#define SPR_DEFS__H

/*                                                 */

#define MAX_GRPS (32)
#define MAX_SPRS_PER_GRP_BITS (11)
#define MAX_SPRS_PER_GRP (1 << MAX_SPRS_PER_GRP_BITS)
#define MAX_SPRS (0x10000)

/*                               */
#define SPRGROUP_SYS	(0 << MAX_SPRS_PER_GRP_BITS)
#define SPRGROUP_DMMU	(1 << MAX_SPRS_PER_GRP_BITS)
#define SPRGROUP_IMMU	(2 << MAX_SPRS_PER_GRP_BITS)
#define SPRGROUP_DC	(3 << MAX_SPRS_PER_GRP_BITS)
#define SPRGROUP_IC	(4 << MAX_SPRS_PER_GRP_BITS)
#define SPRGROUP_MAC	(5 << MAX_SPRS_PER_GRP_BITS)
#define SPRGROUP_D	(6 << MAX_SPRS_PER_GRP_BITS)
#define SPRGROUP_PC	(7 << MAX_SPRS_PER_GRP_BITS)
#define SPRGROUP_PM	(8 << MAX_SPRS_PER_GRP_BITS)
#define SPRGROUP_PIC	(9 << MAX_SPRS_PER_GRP_BITS)
#define SPRGROUP_TT	(10 << MAX_SPRS_PER_GRP_BITS)
#define SPRGROUP_FP	(11 << MAX_SPRS_PER_GRP_BITS)

/*                                 */
#define SPR_VR		(SPRGROUP_SYS + 0)
#define SPR_UPR		(SPRGROUP_SYS + 1)
#define SPR_CPUCFGR	(SPRGROUP_SYS + 2)
#define SPR_DMMUCFGR	(SPRGROUP_SYS + 3)
#define SPR_IMMUCFGR	(SPRGROUP_SYS + 4)
#define SPR_DCCFGR	(SPRGROUP_SYS + 5)
#define SPR_ICCFGR	(SPRGROUP_SYS + 6)
#define SPR_DCFGR	(SPRGROUP_SYS + 7)
#define SPR_PCCFGR	(SPRGROUP_SYS + 8)
#define SPR_NPC         (SPRGROUP_SYS + 16)  /*             */
#define SPR_SR		(SPRGROUP_SYS + 17)  /*             */
#define SPR_PPC         (SPRGROUP_SYS + 18)  /*             */
#define SPR_FPCSR       (SPRGROUP_SYS + 20)  /*             */
#define SPR_EPCR_BASE	(SPRGROUP_SYS + 32)  /*             */
#define SPR_EPCR_LAST	(SPRGROUP_SYS + 47)  /*             */
#define SPR_EEAR_BASE	(SPRGROUP_SYS + 48)
#define SPR_EEAR_LAST	(SPRGROUP_SYS + 63)
#define SPR_ESR_BASE	(SPRGROUP_SYS + 64)
#define SPR_ESR_LAST	(SPRGROUP_SYS + 79)
#define SPR_GPR_BASE	(SPRGROUP_SYS + 1024)

/*                */
#define SPR_DMMUCR	(SPRGROUP_DMMU + 0)
#define SPR_DTLBEIR	(SPRGROUP_DMMU + 2)
#define SPR_DTLBMR_BASE(WAY)	(SPRGROUP_DMMU + 0x200 + (WAY) * 0x100)
#define SPR_DTLBMR_LAST(WAY)	(SPRGROUP_DMMU + 0x27f + (WAY) * 0x100)
#define SPR_DTLBTR_BASE(WAY)	(SPRGROUP_DMMU + 0x280 + (WAY) * 0x100)
#define SPR_DTLBTR_LAST(WAY)	(SPRGROUP_DMMU + 0x2ff + (WAY) * 0x100)

/*                       */
#define SPR_IMMUCR	(SPRGROUP_IMMU + 0)
#define SPR_ITLBEIR	(SPRGROUP_IMMU + 2)
#define SPR_ITLBMR_BASE(WAY)	(SPRGROUP_IMMU + 0x200 + (WAY) * 0x100)
#define SPR_ITLBMR_LAST(WAY)	(SPRGROUP_IMMU + 0x27f + (WAY) * 0x100)
#define SPR_ITLBTR_BASE(WAY)	(SPRGROUP_IMMU + 0x280 + (WAY) * 0x100)
#define SPR_ITLBTR_LAST(WAY)	(SPRGROUP_IMMU + 0x2ff + (WAY) * 0x100)

/*                  */
#define SPR_DCCR	(SPRGROUP_DC + 0)
#define SPR_DCBPR	(SPRGROUP_DC + 1)
#define SPR_DCBFR	(SPRGROUP_DC + 2)
#define SPR_DCBIR	(SPRGROUP_DC + 3)
#define SPR_DCBWR	(SPRGROUP_DC + 4)
#define SPR_DCBLR	(SPRGROUP_DC + 5)
#define SPR_DCR_BASE(WAY)	(SPRGROUP_DC + 0x200 + (WAY) * 0x200)
#define SPR_DCR_LAST(WAY)	(SPRGROUP_DC + 0x3ff + (WAY) * 0x200)

/*                         */
#define SPR_ICCR	(SPRGROUP_IC + 0)
#define SPR_ICBPR	(SPRGROUP_IC + 1)
#define SPR_ICBIR	(SPRGROUP_IC + 2)
#define SPR_ICBLR	(SPRGROUP_IC + 3)
#define SPR_ICR_BASE(WAY)	(SPRGROUP_IC + 0x200 + (WAY) * 0x200)
#define SPR_ICR_LAST(WAY)	(SPRGROUP_IC + 0x3ff + (WAY) * 0x200)

/*           */
#define SPR_MACLO	(SPRGROUP_MAC + 1)
#define SPR_MACHI	(SPRGROUP_MAC + 2)

/*             */
#define SPR_DVR(N)	(SPRGROUP_D + (N))
#define SPR_DCR(N)	(SPRGROUP_D + 8 + (N))
#define SPR_DMR1	(SPRGROUP_D + 16)
#define SPR_DMR2	(SPRGROUP_D + 17)
#define SPR_DWCR0	(SPRGROUP_D + 18)
#define SPR_DWCR1	(SPRGROUP_D + 19)
#define SPR_DSR		(SPRGROUP_D + 20)
#define SPR_DRR		(SPRGROUP_D + 21)

/*                            */
#define SPR_PCCR(N)	(SPRGROUP_PC + (N))
#define SPR_PCMR(N)	(SPRGROUP_PC + 8 + (N))

/*                        */
#define SPR_PMR (SPRGROUP_PM + 0)

/*           */
#define SPR_PICMR (SPRGROUP_PIC + 0)
#define SPR_PICPR (SPRGROUP_PIC + 1)
#define SPR_PICSR (SPRGROUP_PIC + 2)

/*                  */
#define SPR_TTMR (SPRGROUP_TT + 0)
#define SPR_TTCR (SPRGROUP_TT + 1)

/*
                                           
  
 */
#define SPR_VR_VER	0xff000000  /*                   */
#define SPR_VR_CFG	0x00ff0000  /*                         */
#define SPR_VR_RES	0x0000ffc0  /*          */
#define SPR_VR_REV	0x0000003f  /*                    */

#define SPR_VR_VER_OFF	24
#define SPR_VR_CFG_OFF	16
#define SPR_VR_REV_OFF	0

/*
                                                
  
 */
#define SPR_UPR_UP	   0x00000001  /*             */
#define SPR_UPR_DCP	   0x00000002  /*                    */
#define SPR_UPR_ICP	   0x00000004  /*                           */
#define SPR_UPR_DMP	   0x00000008  /*                  */
#define SPR_UPR_IMP	   0x00000010  /*                         */
#define SPR_UPR_MP	   0x00000020  /*             */
#define SPR_UPR_DUP	   0x00000040  /*                    */
#define SPR_UPR_PCUP	   0x00000080  /*                                   */
#define SPR_UPR_PMP	   0x00000100  /*                          */
#define SPR_UPR_PICP	   0x00000200  /*             */
#define SPR_UPR_TTP	   0x00000400  /*                    */
#define SPR_UPR_RES	   0x00fe0000  /*          */
#define SPR_UPR_CUP	   0xff000000  /*                       */

/*
                                                          
  
 */
#define SPR_CPUCFGR_NSGF   0x0000000f  /*                            */
#define SPR_CPUCFGR_CGF	   0x00000010  /*                 */
#define SPR_CPUCFGR_OB32S  0x00000020  /*                   */
#define SPR_CPUCFGR_OB64S  0x00000040  /*                   */
#define SPR_CPUCFGR_OF32S  0x00000080  /*                   */
#define SPR_CPUCFGR_OF64S  0x00000100  /*                   */
#define SPR_CPUCFGR_OV64S  0x00000200  /*                   */
#define SPR_CPUCFGR_RES	   0xfffffc00  /*          */

/*
                                                                      
             
  
 */

#define SPR_DCFGR_NDP      0x00000007  /*                            */
#define SPR_DCFGR_NDP1     0x00000000  /*                          */
#define SPR_DCFGR_NDP2     0x00000001  /*                           */
#define SPR_DCFGR_NDP3     0x00000002  /*                             */
#define SPR_DCFGR_NDP4     0x00000003  /*                            */
#define SPR_DCFGR_NDP5     0x00000004  /*                            */
#define SPR_DCFGR_NDP6     0x00000005  /*                           */
#define SPR_DCFGR_NDP7     0x00000006  /*                             */
#define SPR_DCFGR_NDP8     0x00000007  /*                             */
#define SPR_DCFGR_WPCI     0x00000008  /*                                 */

#define MATCHPOINTS_TO_NDP(n) (1 == n ? SPR_DCFGR_NDP1 : \
                               2 == n ? SPR_DCFGR_NDP2 : \
                               3 == n ? SPR_DCFGR_NDP3 : \
                               4 == n ? SPR_DCFGR_NDP4 : \
                               5 == n ? SPR_DCFGR_NDP5 : \
                               6 == n ? SPR_DCFGR_NDP6 : \
                               7 == n ? SPR_DCFGR_NDP7 : SPR_DCFGR_NDP8)
#define MAX_MATCHPOINTS  8
#define MAX_WATCHPOINTS  (MAX_MATCHPOINTS + 2)

/*
                                               
  
 */
#define SPR_SR_SM          0x00000001  /*                 */
#define SPR_SR_TEE         0x00000002  /*                             */
#define SPR_SR_IEE         0x00000004  /*                            */
#define SPR_SR_DCE         0x00000008  /*                   */
#define SPR_SR_ICE         0x00000010  /*                          */
#define SPR_SR_DME         0x00000020  /*                 */
#define SPR_SR_IME         0x00000040  /*                        */
#define SPR_SR_LEE         0x00000080  /*                      */
#define SPR_SR_CE          0x00000100  /*            */
#define SPR_SR_F           0x00000200  /*                */
#define SPR_SR_CY          0x00000400  /*            */
#define SPR_SR_OV          0x00000800  /*               */
#define SPR_SR_OVE         0x00001000  /*                         */
#define SPR_SR_DSX         0x00002000  /*                      */
#define SPR_SR_EPH         0x00004000  /*                       */
#define SPR_SR_FO          0x00008000  /*           */
#define SPR_SR_SUMRA       0x00010000  /*                            */
#define SPR_SR_RES         0x0ffe0000  /*          */
#define SPR_SR_CID         0xf0000000  /*            */

/*
                                                    
  
 */
#define SPR_DMMUCR_P2S	   0x0000003e  /*                   */
#define SPR_DMMUCR_P1S	   0x000007c0  /*                   */
#define SPR_DMMUCR_VADDR_WIDTH	0x0000f800  /*                    */
#define SPR_DMMUCR_PADDR_WIDTH	0x000f0000  /*                     */

/*
                                                           
  
 */
#define SPR_IMMUCR_P2S	   0x0000003e  /*                   */
#define SPR_IMMUCR_P1S	   0x000007c0  /*                   */
#define SPR_IMMUCR_VADDR_WIDTH	0x0000f800  /*                    */
#define SPR_IMMUCR_PADDR_WIDTH	0x000f0000  /*                     */

/*
                                                  
  
 */
#define SPR_DTLBMR_V	   0x00000001  /*       */
#define SPR_DTLBMR_PL1	   0x00000002  /*                              */
#define SPR_DTLBMR_CID	   0x0000003c  /*            */
#define SPR_DTLBMR_LRU	   0x000000c0  /*                     */
#define SPR_DTLBMR_VPN	   0xfffff000  /*                     */

/*
                                                      
  
 */
#define SPR_DTLBTR_CC	   0x00000001  /*                 */
#define SPR_DTLBTR_CI	   0x00000002  /*               */
#define SPR_DTLBTR_WBC	   0x00000004  /*                  */
#define SPR_DTLBTR_WOM	   0x00000008  /*                       */
#define SPR_DTLBTR_A	   0x00000010  /*          */
#define SPR_DTLBTR_D	   0x00000020  /*       */
#define SPR_DTLBTR_URE	   0x00000040  /*                  */
#define SPR_DTLBTR_UWE	   0x00000080  /*                   */
#define SPR_DTLBTR_SRE	   0x00000100  /*                        */
#define SPR_DTLBTR_SWE	   0x00000200  /*                         */
#define SPR_DTLBTR_PPN	   0xfffff000  /*                      */

/*
                                                         
  
 */
#define SPR_ITLBMR_V	   0x00000001  /*       */
#define SPR_ITLBMR_PL1	   0x00000002  /*                              */
#define SPR_ITLBMR_CID	   0x0000003c  /*            */
#define SPR_ITLBMR_LRU	   0x000000c0  /*                     */
#define SPR_ITLBMR_VPN	   0xfffff000  /*                     */

/*
                                                             
  
 */
#define SPR_ITLBTR_CC	   0x00000001  /*                 */
#define SPR_ITLBTR_CI	   0x00000002  /*               */
#define SPR_ITLBTR_WBC	   0x00000004  /*                  */
#define SPR_ITLBTR_WOM	   0x00000008  /*                       */
#define SPR_ITLBTR_A	   0x00000010  /*          */
#define SPR_ITLBTR_D	   0x00000020  /*       */
#define SPR_ITLBTR_SXE	   0x00000040  /*                  */
#define SPR_ITLBTR_UXE	   0x00000080  /*                   */
#define SPR_ITLBTR_PPN	   0xfffff000  /*                      */

/*
                                                  
  
 */
#define SPR_DCCR_EW	   0x000000ff  /*             */

/*
                                                  
  
 */
#define SPR_ICCR_EW	   0x000000ff  /*             */

/*
                                                        
  
 */

#define SPR_DCCFGR_NCW		0x00000007
#define SPR_DCCFGR_NCS		0x00000078
#define SPR_DCCFGR_CBS		0x00000080
#define SPR_DCCFGR_CWS		0x00000100
#define SPR_DCCFGR_CCRI		0x00000200
#define SPR_DCCFGR_CBIRI	0x00000400
#define SPR_DCCFGR_CBPRI	0x00000800
#define SPR_DCCFGR_CBLRI	0x00001000
#define SPR_DCCFGR_CBFRI	0x00002000
#define SPR_DCCFGR_CBWBRI	0x00004000

#define SPR_DCCFGR_NCW_OFF      0
#define SPR_DCCFGR_NCS_OFF      3
#define SPR_DCCFGR_CBS_OFF	7

/*
                                                               
  
 */
#define SPR_ICCFGR_NCW		0x00000007
#define SPR_ICCFGR_NCS		0x00000078
#define SPR_ICCFGR_CBS		0x00000080
#define SPR_ICCFGR_CCRI		0x00000200
#define SPR_ICCFGR_CBIRI	0x00000400
#define SPR_ICCFGR_CBPRI	0x00000800
#define SPR_ICCFGR_CBLRI	0x00001000

#define SPR_ICCFGR_NCW_OFF      0
#define SPR_ICCFGR_NCS_OFF      3
#define SPR_ICCFGR_CBS_OFF	7

/*
                                                      
  
 */

#define SPR_DMMUCFGR_NTW	0x00000003
#define SPR_DMMUCFGR_NTS	0x0000001C
#define SPR_DMMUCFGR_NAE	0x000000E0
#define SPR_DMMUCFGR_CRI	0x00000100
#define SPR_DMMUCFGR_PRI        0x00000200
#define SPR_DMMUCFGR_TEIRI	0x00000400
#define SPR_DMMUCFGR_HTR	0x00000800

#define SPR_DMMUCFGR_NTW_OFF	0
#define SPR_DMMUCFGR_NTS_OFF	2

/*
                                                             
  
 */

#define SPR_IMMUCFGR_NTW	0x00000003
#define SPR_IMMUCFGR_NTS	0x0000001C
#define SPR_IMMUCFGR_NAE	0x000000E0
#define SPR_IMMUCFGR_CRI	0x00000100
#define SPR_IMMUCFGR_PRI	0x00000200
#define SPR_IMMUCFGR_TEIRI	0x00000400
#define SPR_IMMUCFGR_HTR	0x00000800

#define SPR_IMMUCFGR_NTW_OFF	0
#define SPR_IMMUCFGR_NTS_OFF	2

/*
                                              
  
 */
#define SPR_DCR_DP	0x00000001  /*                 */
#define SPR_DCR_CC	0x0000000e  /*                   */
#define SPR_DCR_SC	0x00000010  /*                */
#define SPR_DCR_CT	0x000000e0  /*            */

/*                                  */
#define SPR_DCR_CC_MASKED 0x00000000
#define SPR_DCR_CC_EQUAL  0x00000002
#define SPR_DCR_CC_LESS   0x00000004
#define SPR_DCR_CC_LESSE  0x00000006
#define SPR_DCR_CC_GREAT  0x00000008
#define SPR_DCR_CC_GREATE 0x0000000a
#define SPR_DCR_CC_NEQUAL 0x0000000c

/*                                  */
#define SPR_DCR_CT_DISABLED 0x00000000
#define SPR_DCR_CT_IFEA     0x00000020
#define SPR_DCR_CT_LEA      0x00000040
#define SPR_DCR_CT_SEA      0x00000060
#define SPR_DCR_CT_LD       0x00000080
#define SPR_DCR_CT_SD       0x000000a0
#define SPR_DCR_CT_LSEA     0x000000c0
#define SPR_DCR_CT_LSD	    0x000000e0
/*                                                                                 */

/*
                                            
  
 */
#define SPR_DMR1_CW       0x000fffff  /*                          */
#define SPR_DMR1_CW0_AND  0x00000001
#define SPR_DMR1_CW0_OR   0x00000002
#define SPR_DMR1_CW0      (SPR_DMR1_CW0_AND | SPR_DMR1_CW0_OR)
#define SPR_DMR1_CW1_AND  0x00000004
#define SPR_DMR1_CW1_OR   0x00000008
#define SPR_DMR1_CW1      (SPR_DMR1_CW1_AND | SPR_DMR1_CW1_OR)
#define SPR_DMR1_CW2_AND  0x00000010
#define SPR_DMR1_CW2_OR   0x00000020
#define SPR_DMR1_CW2      (SPR_DMR1_CW2_AND | SPR_DMR1_CW2_OR)
#define SPR_DMR1_CW3_AND  0x00000040
#define SPR_DMR1_CW3_OR   0x00000080
#define SPR_DMR1_CW3      (SPR_DMR1_CW3_AND | SPR_DMR1_CW3_OR)
#define SPR_DMR1_CW4_AND  0x00000100
#define SPR_DMR1_CW4_OR   0x00000200
#define SPR_DMR1_CW4      (SPR_DMR1_CW4_AND | SPR_DMR1_CW4_OR)
#define SPR_DMR1_CW5_AND  0x00000400
#define SPR_DMR1_CW5_OR   0x00000800
#define SPR_DMR1_CW5      (SPR_DMR1_CW5_AND | SPR_DMR1_CW5_OR)
#define SPR_DMR1_CW6_AND  0x00001000
#define SPR_DMR1_CW6_OR   0x00002000
#define SPR_DMR1_CW6      (SPR_DMR1_CW6_AND | SPR_DMR1_CW6_OR)
#define SPR_DMR1_CW7_AND  0x00004000
#define SPR_DMR1_CW7_OR   0x00008000
#define SPR_DMR1_CW7      (SPR_DMR1_CW7_AND | SPR_DMR1_CW7_OR)
#define SPR_DMR1_CW8_AND  0x00010000
#define SPR_DMR1_CW8_OR   0x00020000
#define SPR_DMR1_CW8      (SPR_DMR1_CW8_AND | SPR_DMR1_CW8_OR)
#define SPR_DMR1_CW9_AND  0x00040000
#define SPR_DMR1_CW9_OR   0x00080000
#define SPR_DMR1_CW9      (SPR_DMR1_CW9_AND | SPR_DMR1_CW9_OR)
#define SPR_DMR1_RES1      0x00300000  /*          */
#define SPR_DMR1_ST	  0x00400000  /*                  */
#define SPR_DMR1_BT	  0x00800000  /*              */
#define SPR_DMR1_RES2	  0xff000000  /*          */

/*
                                                                           
  
 */
#define SPR_DMR2_WCE0	   0x00000001  /*                             */
#define SPR_DMR2_WCE1	   0x00000002  /*                             */
#define SPR_DMR2_AWTC	   0x00000ffc  /*                                */
#define SPR_DMR2_AWTC_OFF           2  /*                          */
#define SPR_DMR2_WGB	   0x003ff000  /*                                   */
#define SPR_DMR2_WGB_OFF           12  /*                         */
#define SPR_DMR2_WBS	   0xffc00000  /*                        */
#define SPR_DMR2_WBS_OFF           22  /*                         */

/*
                                                         
  
 */
#define SPR_DWCR_COUNT	    0x0000ffff  /*       */
#define SPR_DWCR_MATCH	    0xffff0000  /*       */
#define SPR_DWCR_MATCH_OFF          16  /*                  */

/*
                                          
  
 */
#define SPR_DSR_RSTE	0x00000001  /*                 */
#define SPR_DSR_BUSEE	0x00000002  /*                     */
#define SPR_DSR_DPFE	0x00000004  /*                           */
#define SPR_DSR_IPFE	0x00000008  /*                           */
#define SPR_DSR_TTE	0x00000010  /*                      */
#define SPR_DSR_AE	0x00000020  /*                     */
#define SPR_DSR_IIE	0x00000040  /*                               */
#define SPR_DSR_IE	0x00000080  /*                     */
#define SPR_DSR_DME	0x00000100  /*                     */
#define SPR_DSR_IME	0x00000200  /*                     */
#define SPR_DSR_RE	0x00000400  /*                 */
#define SPR_DSR_SCE	0x00000800  /*                       */
#define SPR_DSR_FPE     0x00001000  /*                          */
#define SPR_DSR_TE	0x00002000  /*                */

/*
                                            
  
 */
#define SPR_DRR_RSTE	0x00000001  /*                 */
#define SPR_DRR_BUSEE	0x00000002  /*                     */
#define SPR_DRR_DPFE	0x00000004  /*                           */
#define SPR_DRR_IPFE	0x00000008  /*                           */
#define SPR_DRR_TTE	0x00000010  /*                      */
#define SPR_DRR_AE	0x00000020  /*                     */
#define SPR_DRR_IIE	0x00000040  /*                               */
#define SPR_DRR_IE	0x00000080  /*                     */
#define SPR_DRR_DME	0x00000100  /*                     */
#define SPR_DRR_IME	0x00000200  /*                     */
#define SPR_DRR_RE	0x00000400  /*                 */
#define SPR_DRR_SCE	0x00000800  /*                       */
#define SPR_DRR_FPE     0x00001000  /*                          */
#define SPR_DRR_TE	0x00002000  /*                */

/*
                                                          
  
 */
#define SPR_PCMR_CP	0x00000001  /*                 */
#define SPR_PCMR_UMRA	0x00000002  /*                       */
#define SPR_PCMR_CISM	0x00000004  /*                          */
#define SPR_PCMR_CIUM	0x00000008  /*                    */
#define SPR_PCMR_LA	0x00000010  /*                   */
#define SPR_PCMR_SA	0x00000020  /*                    */
#define SPR_PCMR_IF	0x00000040  /*                        */
#define SPR_PCMR_DCM	0x00000080  /*                       */
#define SPR_PCMR_ICM	0x00000100  /*                       */
#define SPR_PCMR_IFS	0x00000200  /*                        */
#define SPR_PCMR_LSUS	0x00000400  /*                 */
#define SPR_PCMR_BS	0x00000800  /*                    */
#define SPR_PCMR_DTLBM	0x00001000  /*                 */
#define SPR_PCMR_ITLBM	0x00002000  /*                 */
#define SPR_PCMR_DDS	0x00004000  /*                             */
#define SPR_PCMR_WPE	0x03ff8000  /*                   */

/*
                                                    
  
 */
#define SPR_PMR_SDF	0x0000000f  /*                  */
#define SPR_PMR_DME	0x00000010  /*                  */
#define SPR_PMR_SME	0x00000020  /*                   */
#define SPR_PMR_DCGE	0x00000040  /*                             */
#define SPR_PMR_SUME	0x00000080  /*                     */

/*
                            
  
 */
#define SPR_PICMR_IUM	0xfffffffc  /*                  */

/*
                            
  
 */
#define SPR_PICPR_IPRIO	0xfffffffc  /*                    */

/*
                            
  
 */
#define SPR_PICSR_IS	0xffffffff  /*                  */

/*
                                                  
  
 */

#define SPR_TTCR_CNT	0xffffffff  /*                    */
#define SPR_TTMR_TP	0x0fffffff  /*             */
#define SPR_TTMR_IP	0x10000000  /*                   */
#define SPR_TTMR_IE	0x20000000  /*                  */
#define SPR_TTMR_DI	0x00000000  /*          */
#define SPR_TTMR_RT	0x40000000  /*              */
#define SPR_TTMR_SR     0x80000000  /*            */
#define SPR_TTMR_CR     0xc0000000  /*                */
#define SPR_TTMR_M      0xc0000000  /*           */

/*
                                                     
  
 */
#define SPR_FPCSR_FPEE  0x00000001  /*                                 */
#define SPR_FPCSR_RM    0x00000006  /*               */
#define SPR_FPCSR_OVF   0x00000008  /*               */
#define SPR_FPCSR_UNF   0x00000010  /*                */
#define SPR_FPCSR_SNF   0x00000020  /*           */
#define SPR_FPCSR_QNF   0x00000040  /*           */
#define SPR_FPCSR_ZF    0x00000080  /*           */
#define SPR_FPCSR_IXF   0x00000100  /*              */
#define SPR_FPCSR_IVF   0x00000200  /*              */
#define SPR_FPCSR_INF   0x00000400  /*               */
#define SPR_FPCSR_DZF   0x00000800  /*                     */
#define SPR_FPCSR_ALLF (SPR_FPCSR_OVF | SPR_FPCSR_UNF | SPR_FPCSR_SNF | \
			SPR_FPCSR_QNF | SPR_FPCSR_ZF | SPR_FPCSR_IXF |  \
			SPR_FPCSR_IVF | SPR_FPCSR_INF | SPR_FPCSR_DZF)

#define FPCSR_RM_RN (0<<1)
#define FPCSR_RM_RZ (1<<1)
#define FPCSR_RM_RIP (2<<1)
#define FPCSR_RM_RIN (3<<1)

/*
                  
  
 */
#define NOP_NOP          0x0000      /*                        */
#define NOP_EXIT         0x0001      /*                   */
#define NOP_REPORT       0x0002      /*               */
/*                                                                      */
#define NOP_PUTC         0x0004      /*                          */
#define NOP_CNT_RESET    0x0005	     /*                           */
#define NOP_GET_TICKS    0x0006	     /*                          */
#define NOP_GET_PS       0x0007      /*                         */
#define NOP_REPORT_FIRST 0x0400      /*                    */
#define NOP_REPORT_LAST  0x03ff      /*                    */

#endif	/*             */