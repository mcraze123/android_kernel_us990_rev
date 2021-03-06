/*
 *  arch/arm/include/asm/pgtable-nommu.h
 *
 *  Copyright (C) 1995-2002 Russell King
 *  Copyright (C) 2004  Hyok S. Choi
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#ifndef _ASMARM_PGTABLE_NOMMU_H
#define _ASMARM_PGTABLE_NOMMU_H

#ifndef __ASSEMBLY__

#include <linux/slab.h>
#include <asm/processor.h>
#include <asm/page.h>

/*
                                
 */
#define pgd_present(pgd)	(1)
#define pgd_none(pgd)		(0)
#define pgd_bad(pgd)		(0)
#define pgd_clear(pgdp)
#define kern_addr_valid(addr)	(1)
#define	pmd_offset(a, b)	((void *)0)
/*       */
/*
                                                                              
                                                                     
 */
#define PGDIR_SHIFT		21

#define PGDIR_SIZE		(1UL << PGDIR_SHIFT)
#define PGDIR_MASK		(~(PGDIR_SIZE-1))
/*       */

#define PAGE_NONE	__pgprot(0)
#define PAGE_SHARED	__pgprot(0)
#define PAGE_COPY	__pgprot(0)
#define PAGE_READONLY	__pgprot(0)
#define PAGE_KERNEL	__pgprot(0)

#define swapper_pg_dir ((pgd_t *) 0)

#define __swp_type(x)		(0)
#define __swp_offset(x)		(0)
#define __swp_entry(typ,off)	((swp_entry_t) { ((typ) | ((off) << 7)) })
#define __pte_to_swp_entry(pte)	((swp_entry_t) { pte_val(pte) })
#define __swp_entry_to_pte(x)	((pte_t) { (x).val })


typedef pte_t *pte_addr_t;

static inline int pte_file(pte_t pte) { return 0; }

/*
                                                              
                                     
 */
#define ZERO_PAGE(vaddr)	(virt_to_page(0))

/*
                                                       
 */
#define pgprot_noncached(prot)	__pgprot(0)
#define pgprot_writecombine(prot) __pgprot(0)
#define pgprot_dmacoherent(prot) __pgprot(0)


/*
                                                                         
 */
extern unsigned int kobjsize(const void *objp);

/*
                                      
 */
#define pgtable_cache_init()	do { } while (0)
#define io_remap_pfn_range	remap_pfn_range


/*
                                                           
                                          
 */
#define	VMALLOC_START	0UL
#define	VMALLOC_END	0xffffffffUL

#define FIRST_USER_ADDRESS      (0)

#include <asm-generic/pgtable.h>

#else 

/*
                                 
 */
#define v3_tlb_fns	(0)
#define v4_tlb_fns	(0)
#define v4wb_tlb_fns	(0)
#define v4wbi_tlb_fns	(0)
#define v6wbi_tlb_fns	(0)
#define v7wbi_tlb_fns	(0)

#define v3_user_fns	(0)
#define v4_user_fns	(0)
#define v4_mc_user_fns	(0)
#define v4wb_user_fns	(0)
#define v4wt_user_fns	(0)
#define v6_user_fns	(0)
#define xscale_mc_user_fns (0)

#endif /*            */

#endif /*                   */
