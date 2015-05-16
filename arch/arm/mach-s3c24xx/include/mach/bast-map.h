/* arch/arm/mach-s3c2410/include/mach/bast-map.h
 *
 * Copyright (c) 2003-2004 Simtec Electronics
 *	Ben Dooks <ben@simtec.co.uk>
 *
 * Machine BAST - Memory map definitions
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

/*                                      */

/*                                                                   
                                                                       
                                                                       
       
 */

#ifndef __ASM_ARCH_BASTMAP_H
#define __ASM_ARCH_BASTMAP_H

#define BAST_IOADDR(x)	   (S3C2410_ADDR((x) + 0x01300000))

/*                                                            */

#define BAST_VA_CTRL1	    BAST_IOADDR(0x00000000)	 /*            */
#define BAST_PA_CTRL1	    (S3C2410_CS5 | 0x7800000)

#define BAST_VA_CTRL2	    BAST_IOADDR(0x00100000)	 /*            */
#define BAST_PA_CTRL2	    (S3C2410_CS1 | 0x6000000)

#define BAST_VA_CTRL3	    BAST_IOADDR(0x00200000)	 /*            */
#define BAST_PA_CTRL3	    (S3C2410_CS1 | 0x6800000)

#define BAST_VA_CTRL4	    BAST_IOADDR(0x00300000)	 /*            */
#define BAST_PA_CTRL4	    (S3C2410_CS1 | 0x7000000)

/*                                                 */

#define BAST_PA_PC104_IRQREQ  (S3C2410_CS5 | 0x6000000) /*            */
#define BAST_VA_PC104_IRQREQ  BAST_IOADDR(0x00400000)

#define BAST_PA_PC104_IRQRAW  (S3C2410_CS5 | 0x6800000) /*            */
#define BAST_VA_PC104_IRQRAW  BAST_IOADDR(0x00500000)

#define BAST_PA_PC104_IRQMASK (S3C2410_CS5 | 0x7000000) /*            */
#define BAST_VA_PC104_IRQMASK BAST_IOADDR(0x00600000)

#define BAST_PA_LCD_RCMD1     (0x8800000)
#define BAST_VA_LCD_RCMD1     BAST_IOADDR(0x00700000)

#define BAST_PA_LCD_WCMD1     (0x8000000)
#define BAST_VA_LCD_WCMD1     BAST_IOADDR(0x00800000)

#define BAST_PA_LCD_RDATA1    (0x9800000)
#define BAST_VA_LCD_RDATA1    BAST_IOADDR(0x00900000)

#define BAST_PA_LCD_WDATA1    (0x9000000)
#define BAST_VA_LCD_WDATA1    BAST_IOADDR(0x00A00000)

#define BAST_PA_LCD_RCMD2     (0xA800000)
#define BAST_VA_LCD_RCMD2     BAST_IOADDR(0x00B00000)

#define BAST_PA_LCD_WCMD2     (0xA000000)
#define BAST_VA_LCD_WCMD2     BAST_IOADDR(0x00C00000)

#define BAST_PA_LCD_RDATA2    (0xB800000)
#define BAST_VA_LCD_RDATA2    BAST_IOADDR(0x00D00000)

#define BAST_PA_LCD_WDATA2    (0xB000000)
#define BAST_VA_LCD_WDATA2    BAST_IOADDR(0x00E00000)


/*                                                            
                                                              
                             
  
                                          
  
                                        
                                        
                                       
                                        
  
                                             
  
                                             
                                                 
                                                    
                                                        
                                                      
                                                          
                                                         
                                                                   
                                                      
  
                                        
                      
                       
                      
                       
 */

#define BAST_VA_MULTISPACE (0xE0000000)

#define BAST_VA_ISAIO	   (BAST_VA_MULTISPACE + 0x00000000)
#define BAST_VA_ISAMEM	   (BAST_VA_MULTISPACE + 0x01000000)
#define BAST_VA_IDEPRI	   (BAST_VA_MULTISPACE + 0x02000000)
#define BAST_VA_IDEPRIAUX  (BAST_VA_MULTISPACE + 0x02100000)
#define BAST_VA_IDESEC	   (BAST_VA_MULTISPACE + 0x02200000)
#define BAST_VA_IDESECAUX  (BAST_VA_MULTISPACE + 0x02300000)
#define BAST_VA_ASIXNET	   (BAST_VA_MULTISPACE + 0x02400000)
#define BAST_VA_DM9000	   (BAST_VA_MULTISPACE + 0x02500000)
#define BAST_VA_SUPERIO	   (BAST_VA_MULTISPACE + 0x02600000)

#define BAST_VA_MULTISPACE (0xE0000000)

#define BAST_VAM_CS2 (0x00000000)
#define BAST_VAM_CS3 (0x04000000)
#define BAST_VAM_CS4 (0x08000000)
#define BAST_VAM_CS5 (0x0C000000)

/*                                               */

#define BAST_PA_ISAIO	  (0x00000000)
#define BAST_PA_ASIXNET	  (0x01000000)
#define BAST_PA_SUPERIO	  (0x01800000)
#define BAST_PA_IDEPRI	  (0x02000000)
#define BAST_PA_IDEPRIAUX (0x02800000)
#define BAST_PA_IDESEC	  (0x03000000)
#define BAST_PA_IDESECAUX (0x03800000)
#define BAST_PA_ISAMEM	  (0x04000000)
#define BAST_PA_DM9000	  (0x05000000)

/*                                         */

#define BAST_PCSIO (BAST_VA_SUPERIO + BAST_VAM_CS2)
/*  */

#define BAST_ASIXNET_CS  BAST_VAM_CS5
#define BAST_IDE_CS	 BAST_VAM_CS5
#define BAST_DM9000_CS	 BAST_VAM_CS4

#endif /*                      */