/* Copyright (c) 2011-2012, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef __PM8XXX_MPP_H
#define __PM8XXX_MPP_H

#include <linux/errno.h>

#define PM8XXX_MPP_DEV_NAME	"pm8xxx-mpp"

struct pm8xxx_mpp_core_data {
	int	base_addr;
	int	nmpps;
};

struct pm8xxx_mpp_platform_data {
	struct pm8xxx_mpp_core_data	core_data;
	int				mpp_base;
};

/* 
                                                                                
                                                                          
                                                                   
                                                   
                                                  
                                                    
  
                           
                                                               
                                                                   
  
                                                                          
                                                                          
                                                              
  
                                      
  
                                                                       
                                                         
                         
  
                                       
  
                                                                        
                                                       
  
                                                                 
  
                                                                           
                                 
  
                             
                                      
  
                                                                        
                                                               
  
                                       
  
                                                                       
                                          
  
                                       
  
                                                                        
                                          
  
                                      
  
                                                 
  
                                       
  
                                                                          
                                          
  
                                   
  
                                                                        
                                            
  
                                         
  
                                                                        
                                                  
  
                                           
  
                                                                          
                                          
 */
struct pm8xxx_mpp_config_data {
	unsigned	type;
	unsigned	level;
	unsigned	control;
};

/*     */
#if defined(CONFIG_GPIO_PM8XXX_MPP) || defined(CONFIG_GPIO_PM8XXX_MPP_MODULE)

/* 
                                                                               
                                                    
                                             
                     
  
                                                                            
 */
int pm8xxx_mpp_config(unsigned mpp, struct pm8xxx_mpp_config_data *config);

#else

static inline int pm8xxx_mpp_config(unsigned mpp,
				    struct pm8xxx_mpp_config_data *config)
{
	return -ENXIO;
}

#endif

/*                */
#define	PM8XXX_MPP_TYPE_D_INPUT		0
#define	PM8XXX_MPP_TYPE_D_OUTPUT	1
#define	PM8XXX_MPP_TYPE_D_BI_DIR	2
#define	PM8XXX_MPP_TYPE_A_INPUT		3
#define	PM8XXX_MPP_TYPE_A_OUTPUT	4
#define	PM8XXX_MPP_TYPE_SINK		5
#define	PM8XXX_MPP_TYPE_DTEST_SINK	6
#define	PM8XXX_MPP_TYPE_DTEST_OUTPUT	7

/*                             */
#define	PM8XXX_MPP_DIG_LEVEL_VIO_0	0
#define	PM8XXX_MPP_DIG_LEVEL_VIO_1	1
#define	PM8XXX_MPP_DIG_LEVEL_VIO_2	2
#define	PM8XXX_MPP_DIG_LEVEL_VIO_3	3
#define	PM8XXX_MPP_DIG_LEVEL_VIO_4	4
#define	PM8XXX_MPP_DIG_LEVEL_VIO_5	5
#define	PM8XXX_MPP_DIG_LEVEL_VIO_6	6
#define	PM8XXX_MPP_DIG_LEVEL_VIO_7	7

/*                                      */
#define	PM8058_MPP_DIG_LEVEL_VPH	0
#define	PM8058_MPP_DIG_LEVEL_S3		1
#define	PM8058_MPP_DIG_LEVEL_L2		2
#define	PM8058_MPP_DIG_LEVEL_L3		3

/*                                      */
#define	PM8901_MPP_DIG_LEVEL_MSMIO	0
#define	PM8901_MPP_DIG_LEVEL_DIG	1
#define	PM8901_MPP_DIG_LEVEL_L5		2
#define	PM8901_MPP_DIG_LEVEL_S4		3
#define	PM8901_MPP_DIG_LEVEL_VPH	4

/*                                      */
#define	PM8921_MPP_DIG_LEVEL_S4		1
#define	PM8921_MPP_DIG_LEVEL_L15	3
#define	PM8921_MPP_DIG_LEVEL_L17	4
#define	PM8921_MPP_DIG_LEVEL_VPH	7

/*                                      */
#define	PM8821_MPP_DIG_LEVEL_1P8	0
#define	PM8821_MPP_DIG_LEVEL_VPH	7

/*                                      */
#define	PM8018_MPP_DIG_LEVEL_L4		0
#define	PM8018_MPP_DIG_LEVEL_L14	1
#define	PM8018_MPP_DIG_LEVEL_S3		2
#define	PM8018_MPP_DIG_LEVEL_L6		3
#define	PM8018_MPP_DIG_LEVEL_L2		4
#define	PM8018_MPP_DIG_LEVEL_L5		5
#define	PM8018_MPP_DIG_LEVEL_VPH	7

/*                                      */
#define	PM8038_MPP_DIG_LEVEL_L20	0
#define	PM8038_MPP_DIG_LEVEL_L11	1
#define	PM8038_MPP_DIG_LEVEL_L5		2
#define	PM8038_MPP_DIG_LEVEL_L15	3
#define	PM8038_MPP_DIG_LEVEL_L17	4
#define	PM8038_MPP_DIG_LEVEL_VPH	7

/*                        */
#define	PM8XXX_MPP_DIN_TO_INT		0
#define	PM8XXX_MPP_DIN_TO_DBUS1		1
#define	PM8XXX_MPP_DIN_TO_DBUS2		2
#define	PM8XXX_MPP_DIN_TO_DBUS3		3

/*                         */
#define	PM8XXX_MPP_DOUT_CTRL_LOW	0
#define	PM8XXX_MPP_DOUT_CTRL_HIGH	1
#define	PM8XXX_MPP_DOUT_CTRL_MPP	2
#define	PM8XXX_MPP_DOUT_CTRL_INV_MPP	3

/*                        */
#define	PM8XXX_MPP_BI_PULLUP_1KOHM	0
#define	PM8XXX_MPP_BI_PULLUP_OPEN	1
#define	PM8XXX_MPP_BI_PULLUP_10KOHM	2
#define	PM8XXX_MPP_BI_PULLUP_30KOHM	3

/*                     */
#define	PM8XXX_MPP_AIN_AMUX_CH5		0
#define	PM8XXX_MPP_AIN_AMUX_CH6		1
#define	PM8XXX_MPP_AIN_AMUX_CH7		2
#define	PM8XXX_MPP_AIN_AMUX_CH8		3
#define	PM8XXX_MPP_AIN_AMUX_CH9		4
#define	PM8XXX_MPP_AIN_AMUX_ABUS1	5
#define	PM8XXX_MPP_AIN_AMUX_ABUS2	6
#define	PM8XXX_MPP_AIN_AMUX_ABUS3	7

/*                      */
#define	PM8XXX_MPP_AOUT_LVL_1V25	0
#define	PM8XXX_MPP_AOUT_LVL_1V25_2	1
#define	PM8XXX_MPP_AOUT_LVL_0V625	2
#define	PM8XXX_MPP_AOUT_LVL_0V3125	3
#define	PM8XXX_MPP_AOUT_LVL_MPP		4
#define	PM8XXX_MPP_AOUT_LVL_ABUS1	5
#define	PM8XXX_MPP_AOUT_LVL_ABUS2	6
#define	PM8XXX_MPP_AOUT_LVL_ABUS3	7

/*                        */
#define	PM8XXX_MPP_AOUT_CTRL_DISABLE		0
#define	PM8XXX_MPP_AOUT_CTRL_ENABLE		1
#define	PM8XXX_MPP_AOUT_CTRL_MPP_HIGH_EN	2
#define	PM8XXX_MPP_AOUT_CTRL_MPP_LOW_EN		3

/*                     */
#define	PM8XXX_MPP_CS_OUT_5MA		0
#define	PM8XXX_MPP_CS_OUT_10MA		1
#define	PM8XXX_MPP_CS_OUT_15MA		2
#define	PM8XXX_MPP_CS_OUT_20MA		3
#define	PM8XXX_MPP_CS_OUT_25MA		4
#define	PM8XXX_MPP_CS_OUT_30MA		5
#define	PM8XXX_MPP_CS_OUT_35MA		6
#define	PM8XXX_MPP_CS_OUT_40MA		7

/*                       */
#define	PM8XXX_MPP_CS_CTRL_DISABLE	0
#define	PM8XXX_MPP_CS_CTRL_ENABLE	1
#define	PM8XXX_MPP_CS_CTRL_MPP_HIGH_EN	2
#define	PM8XXX_MPP_CS_CTRL_MPP_LOW_EN	3

/*                             */
#define	PM8XXX_MPP_DTEST_CS_CTRL_EN1	0
#define	PM8XXX_MPP_DTEST_CS_CTRL_EN2	1
#define	PM8XXX_MPP_DTEST_CS_CTRL_EN3	2
#define	PM8XXX_MPP_DTEST_CS_CTRL_EN4	3

/*                               */
#define	PM8XXX_MPP_DTEST_DBUS1		0
#define	PM8XXX_MPP_DTEST_DBUS2		1
#define	PM8XXX_MPP_DTEST_DBUS3		2
#define	PM8XXX_MPP_DTEST_DBUS4		3

#endif
