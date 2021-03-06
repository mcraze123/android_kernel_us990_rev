#ifndef _SPARC64_SFAFSR_H
#define _SPARC64_SFAFSR_H

#include <linux/const.h>

/*                                                                    */

#define SFAFSR_ME		(_AC(1,UL) << SFAFSR_ME_SHIFT)
#define SFAFSR_ME_SHIFT		32
#define SFAFSR_PRIV		(_AC(1,UL) << SFAFSR_PRIV_SHIFT)
#define SFAFSR_PRIV_SHIFT	31
#define SFAFSR_ISAP		(_AC(1,UL) << SFAFSR_ISAP_SHIFT)
#define SFAFSR_ISAP_SHIFT	30
#define SFAFSR_ETP		(_AC(1,UL) << SFAFSR_ETP_SHIFT)
#define SFAFSR_ETP_SHIFT	29
#define SFAFSR_IVUE		(_AC(1,UL) << SFAFSR_IVUE_SHIFT)
#define SFAFSR_IVUE_SHIFT	28
#define SFAFSR_TO		(_AC(1,UL) << SFAFSR_TO_SHIFT)
#define SFAFSR_TO_SHIFT		27
#define SFAFSR_BERR		(_AC(1,UL) << SFAFSR_BERR_SHIFT)
#define SFAFSR_BERR_SHIFT	26
#define SFAFSR_LDP		(_AC(1,UL) << SFAFSR_LDP_SHIFT)
#define SFAFSR_LDP_SHIFT	25
#define SFAFSR_CP		(_AC(1,UL) << SFAFSR_CP_SHIFT)
#define SFAFSR_CP_SHIFT		24
#define SFAFSR_WP		(_AC(1,UL) << SFAFSR_WP_SHIFT)
#define SFAFSR_WP_SHIFT		23
#define SFAFSR_EDP		(_AC(1,UL) << SFAFSR_EDP_SHIFT)
#define SFAFSR_EDP_SHIFT	22
#define SFAFSR_UE		(_AC(1,UL) << SFAFSR_UE_SHIFT)
#define SFAFSR_UE_SHIFT		21
#define SFAFSR_CE		(_AC(1,UL) << SFAFSR_CE_SHIFT)
#define SFAFSR_CE_SHIFT		20
#define SFAFSR_ETS		(_AC(0xf,UL) << SFAFSR_ETS_SHIFT)
#define SFAFSR_ETS_SHIFT	16
#define SFAFSR_PSYND		(_AC(0xffff,UL) << SFAFSR_PSYND_SHIFT)
#define SFAFSR_PSYND_SHIFT	0

/*                                                                   
                                                                      
 */

#define UDBE_UE			(_AC(1,UL) << 9)
#define UDBE_CE			(_AC(1,UL) << 8)
#define UDBE_E_SYNDR		(_AC(0xff,UL) << 0)

/*                                                              
                                                                
                      
  
                                                  
                                                  
                                                  
                                                 
  
                                   
 */
#define SFSTAT_UDBH_MASK	(_AC(0x3ff,UL) << SFSTAT_UDBH_SHIFT)
#define SFSTAT_UDBH_SHIFT	54
#define SFSTAT_UDBL_MASK	(_AC(0x3ff,UL) << SFSTAT_UDBH_SHIFT)
#define SFSTAT_UDBL_SHIFT	44
#define SFSTAT_TL_GT_ONE	(_AC(1,UL) << SFSTAT_TL_GT_ONE_SHIFT)
#define SFSTAT_TL_GT_ONE_SHIFT	42
#define SFSTAT_TRAP_TYPE	(_AC(0x1FF,UL) << SFSTAT_TRAP_TYPE_SHIFT)
#define SFSTAT_TRAP_TYPE_SHIFT	33
#define SFSTAT_AFSR_MASK	(_AC(0x1ffffffff,UL) << SFSTAT_AFSR_SHIFT)
#define SFSTAT_AFSR_SHIFT	0

/*                                                     */
#define ESTATE_ERR_CE		0x1 /*                                       */
#define ESTATE_ERR_NCE		0x2 /*                                       */
#define ESTATE_ERR_ISAP		0x4 /*                                       */
#define ESTATE_ERR_ALL		(ESTATE_ERR_CE | \
				 ESTATE_ERR_NCE | \
				 ESTATE_ERR_ISAP)

/*                                                           */
#define TRAP_TYPE_IAE		0x09 /*                                      */
#define TRAP_TYPE_DAE		0x32 /*                                      */
#define TRAP_TYPE_CEE		0x63 /*                                      */

#endif /*                   */
