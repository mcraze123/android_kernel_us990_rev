/**
 * eCryptfs: Linux filesystem encryption layer
 * Kernel declarations.
 *
 * Copyright (C) 1997-2003 Erez Zadok
 * Copyright (C) 2001-2003 Stony Brook University
 * Copyright (C) 2004-2008 International Business Machines Corp.
 *   Author(s): Michael A. Halcrow <mahalcro@us.ibm.com>
 *              Trevor S. Highland <trevor.highland@gmail.com>
 *              Tyler Hicks <tyhicks@ou.edu>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 */

#ifndef ECRYPTFS_KERNEL_H
#define ECRYPTFS_KERNEL_H

#include <keys/user-type.h>
#include <keys/encrypted-type.h>
#include <linux/fs.h>
#include <linux/fs_stack.h>
#include <linux/namei.h>
#include <linux/scatterlist.h>
#include <linux/hash.h>
#include <linux/nsproxy.h>
#include <linux/backing-dev.h>
#ifdef CONFIG_CRYPTO_DEV_KFIPS
#include <linux/crypto.h>
#endif
#include <linux/ecryptfs.h>

#define ECRYPTFS_DEFAULT_IV_BYTES 16
#define ECRYPTFS_DEFAULT_EXTENT_SIZE 4096
#define ECRYPTFS_MINIMUM_HEADER_EXTENT_SIZE 8192
#define ECRYPTFS_DEFAULT_MSG_CTX_ELEMS 32
#define ECRYPTFS_DEFAULT_SEND_TIMEOUT HZ
#define ECRYPTFS_MAX_MSG_CTX_TTL (HZ*3)
#define ECRYPTFS_DEFAULT_NUM_USERS 4
#define ECRYPTFS_MAX_NUM_USERS 32768
#define ECRYPTFS_XATTR_NAME "user.ecryptfs"

//                                
#define FEATURE_SDCARD_MEDIAEXN_SYSTEMCALL_ENCRYPTION
//                                

void ecryptfs_dump_auth_tok(struct ecryptfs_auth_tok *auth_tok);
extern void ecryptfs_to_hex(char *dst, char *src, size_t src_size);
extern void ecryptfs_from_hex(char *dst, char *src, int dst_size);

struct ecryptfs_key_record {
	unsigned char type;
	size_t enc_key_size;
	unsigned char sig[ECRYPTFS_SIG_SIZE];
	unsigned char enc_key[ECRYPTFS_MAX_ENCRYPTED_KEY_BYTES];
};

struct ecryptfs_auth_tok_list {
	struct ecryptfs_auth_tok *auth_tok;
	struct list_head list;
};

struct ecryptfs_crypt_stat;
struct ecryptfs_mount_crypt_stat;

struct ecryptfs_page_crypt_context {
	struct page *page;
#define ECRYPTFS_PREPARE_COMMIT_MODE 0
#define ECRYPTFS_WRITEPAGE_MODE      1
	unsigned int mode;
	union {
		struct file *lower_file;
		struct writeback_control *wbc;
	} param;
};

#if defined(CONFIG_ENCRYPTED_KEYS) || defined(CONFIG_ENCRYPTED_KEYS_MODULE)
static inline struct ecryptfs_auth_tok *
ecryptfs_get_encrypted_key_payload_data(struct key *key)
{
	if (key->type == &key_type_encrypted)
		return (struct ecryptfs_auth_tok *)
			(&((struct encrypted_key_payload *)key->payload.data)->payload_data);
	else
		return NULL;
}

static inline struct key *ecryptfs_get_encrypted_key(char *sig)
{
	return request_key(&key_type_encrypted, sig, NULL);
}

#else
static inline struct ecryptfs_auth_tok *
ecryptfs_get_encrypted_key_payload_data(struct key *key)
{
	return NULL;
}

static inline struct key *ecryptfs_get_encrypted_key(char *sig)
{
	return ERR_PTR(-ENOKEY);
}

#endif /*                       */

static inline struct ecryptfs_auth_tok *
ecryptfs_get_key_payload_data(struct key *key)
{
	struct ecryptfs_auth_tok *auth_tok;

	auth_tok = ecryptfs_get_encrypted_key_payload_data(key);
	if (!auth_tok)
		return (struct ecryptfs_auth_tok *)
			(((struct user_key_payload *)key->payload.data)->data);
	else
		return auth_tok;
}

#define ECRYPTFS_MAX_KEYSET_SIZE 1024
#define ECRYPTFS_MAX_CIPHER_NAME_SIZE 32
#define ECRYPTFS_MAX_NUM_ENC_KEYS 64
#define ECRYPTFS_MAX_IV_BYTES 16	/*          */
#define ECRYPTFS_SALT_BYTES 2
#define MAGIC_ECRYPTFS_MARKER 0x3c81b7f5
#define MAGIC_ECRYPTFS_MARKER_SIZE_BYTES 8	/*     */
#define ECRYPTFS_FILE_SIZE_BYTES (sizeof(u64))
#define ECRYPTFS_SIZE_AND_MARKER_BYTES (ECRYPTFS_FILE_SIZE_BYTES \
					+ MAGIC_ECRYPTFS_MARKER_SIZE_BYTES)
#define ECRYPTFS_DEFAULT_CIPHER "aes"
#define ECRYPTFS_DEFAULT_KEY_BYTES 16
#define ECRYPTFS_DEFAULT_HASH "md5"
#ifdef CONFIG_CRYPTO_FIPS
#define ECRYPTFS_SHA256_HASH "sha256"
#endif
#define ECRYPTFS_TAG_70_DIGEST ECRYPTFS_DEFAULT_HASH
#define ECRYPTFS_TAG_1_PACKET_TYPE 0x01
#define ECRYPTFS_TAG_3_PACKET_TYPE 0x8C
#define ECRYPTFS_TAG_11_PACKET_TYPE 0xED
#define ECRYPTFS_TAG_64_PACKET_TYPE 0x40
#define ECRYPTFS_TAG_65_PACKET_TYPE 0x41
#define ECRYPTFS_TAG_66_PACKET_TYPE 0x42
#define ECRYPTFS_TAG_67_PACKET_TYPE 0x43
#define ECRYPTFS_TAG_70_PACKET_TYPE 0x46 /*                        
                        */
#define ECRYPTFS_TAG_71_PACKET_TYPE 0x47 /*                           
                  */
#define ECRYPTFS_TAG_72_PACKET_TYPE 0x48 /*                          
                     */
#define ECRYPTFS_TAG_73_PACKET_TYPE 0x49 /*                          
                  */
#define ECRYPTFS_MIN_PKT_LEN_SIZE 1 /*                                   */
#define ECRYPTFS_MAX_PKT_LEN_SIZE 2 /*                                 
                                             
                                         
         */
/*                                                          
                         */
#define ECRYPTFS_FILENAME_MIN_RANDOM_PREPEND_BYTES 16
#define ECRYPTFS_NON_NULL 0x42 /*                                  */
#define MD5_DIGEST_SIZE 16
#ifdef CONFIG_CRYPTO_FIPS
#define SHA256_HASH_SIZE 32
#endif
#define ECRYPTFS_TAG_70_DIGEST_SIZE MD5_DIGEST_SIZE
#define ECRYPTFS_TAG_70_MIN_METADATA_SIZE (1 + ECRYPTFS_MIN_PKT_LEN_SIZE \
					   + ECRYPTFS_SIG_SIZE + 1 + 1)
#define ECRYPTFS_TAG_70_MAX_METADATA_SIZE (1 + ECRYPTFS_MAX_PKT_LEN_SIZE \
					   + ECRYPTFS_SIG_SIZE + 1 + 1)
#define ECRYPTFS_FEK_ENCRYPTED_FILENAME_PREFIX "ECRYPTFS_FEK_ENCRYPTED."
#define ECRYPTFS_FEK_ENCRYPTED_FILENAME_PREFIX_SIZE 23
#define ECRYPTFS_FNEK_ENCRYPTED_FILENAME_PREFIX "ECRYPTFS_FNEK_ENCRYPTED."
#define ECRYPTFS_FNEK_ENCRYPTED_FILENAME_PREFIX_SIZE 24
#define ECRYPTFS_ENCRYPTED_DENTRY_NAME_LEN (18 + 1 + 4 + 1 + 32)

struct ecryptfs_key_sig {
	struct list_head crypt_stat_list;
	char keysig[ECRYPTFS_SIG_SIZE_HEX + 1];
};

struct ecryptfs_filename {
	struct list_head crypt_stat_list;
#define ECRYPTFS_FILENAME_CONTAINS_DECRYPTED 0x00000001
	u32 flags;
	u32 seq_no;
	char *filename;
	char *encrypted_filename;
	size_t filename_size;
	size_t encrypted_filename_size;
	char fnek_sig[ECRYPTFS_SIG_SIZE_HEX];
	char dentry_name[ECRYPTFS_ENCRYPTED_DENTRY_NAME_LEN + 1];
};

/* 
                                                                  
  
                          
 */
struct ecryptfs_crypt_stat {
#define ECRYPTFS_STRUCT_INITIALIZED   0x00000001
#define ECRYPTFS_POLICY_APPLIED       0x00000002
#define ECRYPTFS_ENCRYPTED            0x00000004
#define ECRYPTFS_SECURITY_WARNING     0x00000008
#define ECRYPTFS_ENABLE_HMAC          0x00000010
#define ECRYPTFS_ENCRYPT_IV_PAGES     0x00000020
#define ECRYPTFS_KEY_VALID            0x00000040
#define ECRYPTFS_METADATA_IN_XATTR    0x00000080
#define ECRYPTFS_VIEW_AS_ENCRYPTED    0x00000100
#define ECRYPTFS_KEY_SET              0x00000200
#define ECRYPTFS_ENCRYPT_FILENAMES    0x00000400
#define ECRYPTFS_ENCFN_USE_MOUNT_FNEK 0x00000800
#define ECRYPTFS_ENCFN_USE_FEK        0x00001000
#define ECRYPTFS_UNLINK_SIGS          0x00002000
#define ECRYPTFS_I_SIZE_INITIALIZED   0x00004000
	u32 flags;
	unsigned int file_version;
	size_t iv_bytes;
	size_t metadata_size;
	size_t extent_size; /*                                   */
	size_t key_size;
	size_t extent_shift;
	unsigned int extent_mask;
	struct ecryptfs_mount_crypt_stat *mount_crypt_stat;
#ifndef CONFIG_CRYPTO_DEV_KFIPS
	struct crypto_blkcipher *tfm;
#else
	struct crypto_ablkcipher *tfm;
#endif
	struct crypto_hash *hash_tfm; /*                              
                                        */
	unsigned char cipher[ECRYPTFS_MAX_CIPHER_NAME_SIZE];
	unsigned char key[ECRYPTFS_MAX_KEY_BYTES];
	unsigned char root_iv[ECRYPTFS_MAX_IV_BYTES];
	struct list_head keysig_list;
	struct mutex keysig_list_mutex;
	struct mutex cs_tfm_mutex;
	struct mutex cs_hash_tfm_mutex;
	struct mutex cs_mutex;
};

/*                     */
struct ecryptfs_inode_info {
	struct inode vfs_inode;
	struct inode *wii_inode;
	struct mutex lower_file_mutex;
	atomic_t lower_file_count;
	struct file *lower_file;
	struct ecryptfs_crypt_stat crypt_stat;
};

/*                                                            
                 */
struct ecryptfs_dentry_info {
	struct path lower_path;
	struct ecryptfs_crypt_stat *crypt_stat;
};

/* 
                                                                                      
                       
                                                                  
                                                                  
                                                                      
                                                                   
                                                              
                                                                    
                                     
                           
  
                                                                      
                                                                   
                                                                 
                                                                      
                                                                
           
 */
struct ecryptfs_global_auth_tok {
#define ECRYPTFS_AUTH_TOK_INVALID 0x00000001
#define ECRYPTFS_AUTH_TOK_FNEK    0x00000002
	u32 flags;
	struct list_head mount_crypt_stat_list;
	struct key *global_auth_tok_key;
	unsigned char sig[ECRYPTFS_SIG_SIZE_HEX + 1];
};

/* 
                                        
                                         
                               
                                                                    
                                                                
                                                                  
                                                        
  
                                                                      
                                                                
                                                                   
                                                                 
 */
struct ecryptfs_key_tfm {
	struct crypto_blkcipher *key_tfm;
	size_t key_size;
	struct mutex key_tfm_mutex;
	struct list_head key_tfm_list;
	unsigned char cipher_name[ECRYPTFS_MAX_CIPHER_NAME_SIZE + 1];
};

extern struct mutex key_tfm_list_mutex;

/* 
                                                                    
                                                                      
                                                                    
                           
 */
struct ecryptfs_mount_crypt_stat {
	/*                                                     */
#define ECRYPTFS_PLAINTEXT_PASSTHROUGH_ENABLED 0x00000001
#define ECRYPTFS_XATTR_METADATA_ENABLED        0x00000002
#define ECRYPTFS_ENCRYPTED_VIEW_ENABLED        0x00000004
#define ECRYPTFS_MOUNT_CRYPT_STAT_INITIALIZED  0x00000008
#define ECRYPTFS_GLOBAL_ENCRYPT_FILENAMES      0x00000010
#define ECRYPTFS_GLOBAL_ENCFN_USE_MOUNT_FNEK   0x00000020
#define ECRYPTFS_GLOBAL_ENCFN_USE_FEK          0x00000040
#define ECRYPTFS_GLOBAL_MOUNT_AUTH_TOK_ONLY    0x00000080
#define ECRYPTFS_DECRYPTION_ONLY	       0x00000100 //                          
	u32 flags;
	struct list_head global_auth_tok_list;
	struct mutex global_auth_tok_list_mutex;
	size_t global_default_cipher_key_size;
	size_t global_default_fn_cipher_key_bytes;
	unsigned char global_default_cipher_name[ECRYPTFS_MAX_CIPHER_NAME_SIZE
						 + 1];
	unsigned char global_default_fn_cipher_name[
		ECRYPTFS_MAX_CIPHER_NAME_SIZE + 1];
	char global_default_fnek_sig[ECRYPTFS_SIG_SIZE_HEX + 1];
};

/*                          */
struct ecryptfs_sb_info {
	struct super_block *wsi_sb;
	struct ecryptfs_mount_crypt_stat mount_crypt_stat;
	struct backing_dev_info bdi;
};

/*                    */
struct ecryptfs_file_info {
	struct file *wfi_file;
	struct ecryptfs_crypt_stat *crypt_stat;
};

/*                                             */
struct ecryptfs_auth_tok_list_item {
	unsigned char encrypted_session_key[ECRYPTFS_MAX_KEY_BYTES];
	struct list_head list;
	struct ecryptfs_auth_tok auth_tok;
};

struct ecryptfs_message {
	/*                                                    */
	/*                                 */
	/*                              */
	u32 index;
	u32 data_len;
	u8 data[];
};

struct ecryptfs_msg_ctx {
#define ECRYPTFS_MSG_CTX_STATE_FREE     0x01
#define ECRYPTFS_MSG_CTX_STATE_PENDING  0x02
#define ECRYPTFS_MSG_CTX_STATE_DONE     0x03
#define ECRYPTFS_MSG_CTX_STATE_NO_REPLY 0x04
	u8 state;
#define ECRYPTFS_MSG_HELO 100
#define ECRYPTFS_MSG_QUIT 101
#define ECRYPTFS_MSG_REQUEST 102
#define ECRYPTFS_MSG_RESPONSE 103
	u8 type;
	u32 index;
	/*                                                         
                                                        
                                                             
                                                               
           */
	u32 counter;
	size_t msg_size;
	struct ecryptfs_message *msg;
	struct task_struct *task;
	struct list_head node;
	struct list_head daemon_out_list;
	struct mutex mux;
};

struct ecryptfs_daemon;

struct ecryptfs_daemon {
#define ECRYPTFS_DAEMON_IN_READ      0x00000001
#define ECRYPTFS_DAEMON_IN_POLL      0x00000002
#define ECRYPTFS_DAEMON_ZOMBIE       0x00000004
#define ECRYPTFS_DAEMON_MISCDEV_OPEN 0x00000008
	u32 flags;
	u32 num_queued_msg_ctx;
	struct pid *pid;
	uid_t euid;
	struct user_namespace *user_ns;
	struct task_struct *task;
	struct mutex mux;
	struct list_head msg_ctx_out_queue;
	wait_queue_head_t wait;
	struct hlist_node euid_chain;
};

extern struct mutex ecryptfs_daemon_hash_mux;

static inline size_t
ecryptfs_lower_header_size(struct ecryptfs_crypt_stat *crypt_stat)
{
	if (crypt_stat->flags & ECRYPTFS_METADATA_IN_XATTR)
		return 0;
	return crypt_stat->metadata_size;
}

static inline struct ecryptfs_file_info *
ecryptfs_file_to_private(struct file *file)
{
	return file->private_data;
}

static inline void
ecryptfs_set_file_private(struct file *file,
			  struct ecryptfs_file_info *file_info)
{
	file->private_data = file_info;
}

static inline struct file *ecryptfs_file_to_lower(struct file *file)
{
	return ((struct ecryptfs_file_info *)file->private_data)->wfi_file;
}

static inline void
ecryptfs_set_file_lower(struct file *file, struct file *lower_file)
{
	((struct ecryptfs_file_info *)file->private_data)->wfi_file =
		lower_file;
}

static inline struct ecryptfs_inode_info *
ecryptfs_inode_to_private(struct inode *inode)
{
	return container_of(inode, struct ecryptfs_inode_info, vfs_inode);
}

static inline struct inode *ecryptfs_inode_to_lower(struct inode *inode)
{
	return ecryptfs_inode_to_private(inode)->wii_inode;
}

static inline void
ecryptfs_set_inode_lower(struct inode *inode, struct inode *lower_inode)
{
	ecryptfs_inode_to_private(inode)->wii_inode = lower_inode;
}

static inline struct ecryptfs_sb_info *
ecryptfs_superblock_to_private(struct super_block *sb)
{
	return (struct ecryptfs_sb_info *)sb->s_fs_info;
}

static inline void
ecryptfs_set_superblock_private(struct super_block *sb,
				struct ecryptfs_sb_info *sb_info)
{
	sb->s_fs_info = sb_info;
}

static inline struct super_block *
ecryptfs_superblock_to_lower(struct super_block *sb)
{
	return ((struct ecryptfs_sb_info *)sb->s_fs_info)->wsi_sb;
}

static inline void
ecryptfs_set_superblock_lower(struct super_block *sb,
			      struct super_block *lower_sb)
{
	((struct ecryptfs_sb_info *)sb->s_fs_info)->wsi_sb = lower_sb;
}

static inline struct ecryptfs_dentry_info *
ecryptfs_dentry_to_private(struct dentry *dentry)
{
	return (struct ecryptfs_dentry_info *)dentry->d_fsdata;
}

static inline void
ecryptfs_set_dentry_private(struct dentry *dentry,
			    struct ecryptfs_dentry_info *dentry_info)
{
	dentry->d_fsdata = dentry_info;
}

static inline struct dentry *
ecryptfs_dentry_to_lower(struct dentry *dentry)
{
	return ((struct ecryptfs_dentry_info *)dentry->d_fsdata)->lower_path.dentry;
}

static inline void
ecryptfs_set_dentry_lower(struct dentry *dentry, struct dentry *lower_dentry)
{
	((struct ecryptfs_dentry_info *)dentry->d_fsdata)->lower_path.dentry =
		lower_dentry;
}

static inline struct vfsmount *
ecryptfs_dentry_to_lower_mnt(struct dentry *dentry)
{
	return ((struct ecryptfs_dentry_info *)dentry->d_fsdata)->lower_path.mnt;
}

static inline void
ecryptfs_set_dentry_lower_mnt(struct dentry *dentry, struct vfsmount *lower_mnt)
{
	((struct ecryptfs_dentry_info *)dentry->d_fsdata)->lower_path.mnt =
		lower_mnt;
}

#define ecryptfs_printk(type, fmt, arg...) \
        __ecryptfs_printk(type "%s: " fmt, __func__, ## arg);
__printf(1, 2)
void __ecryptfs_printk(const char *fmt, ...);

extern const struct file_operations ecryptfs_main_fops;
extern const struct file_operations ecryptfs_dir_fops;
extern const struct inode_operations ecryptfs_main_iops;
extern const struct inode_operations ecryptfs_dir_iops;
extern const struct inode_operations ecryptfs_symlink_iops;
extern const struct super_operations ecryptfs_sops;
extern const struct dentry_operations ecryptfs_dops;
extern const struct address_space_operations ecryptfs_aops;
extern int ecryptfs_verbosity;
extern unsigned int ecryptfs_message_buf_len;
extern signed long ecryptfs_message_wait_timeout;
extern unsigned int ecryptfs_number_of_users;

extern struct kmem_cache *ecryptfs_auth_tok_list_item_cache;
extern struct kmem_cache *ecryptfs_file_info_cache;
extern struct kmem_cache *ecryptfs_dentry_info_cache;
extern struct kmem_cache *ecryptfs_inode_info_cache;
extern struct kmem_cache *ecryptfs_sb_info_cache;
extern struct kmem_cache *ecryptfs_header_cache;
extern struct kmem_cache *ecryptfs_xattr_cache;
extern struct kmem_cache *ecryptfs_key_record_cache;
extern struct kmem_cache *ecryptfs_key_sig_cache;
extern struct kmem_cache *ecryptfs_global_auth_tok_cache;
extern struct kmem_cache *ecryptfs_key_tfm_cache;
extern struct kmem_cache *ecryptfs_open_req_cache;
#ifdef CONFIG_CRYPTO_DEV_KFIPS
extern struct kmem_cache *ecryptfs_page_crypt_req_cache;
extern struct kmem_cache *ecryptfs_extent_crypt_req_cache;
#endif

struct ecryptfs_open_req {
#define ECRYPTFS_REQ_PROCESSED 0x00000001
#define ECRYPTFS_REQ_DROPPED   0x00000002
#define ECRYPTFS_REQ_ZOMBIE    0x00000004
	u32 flags;
	struct file **lower_file;
	struct dentry *lower_dentry;
	struct vfsmount *lower_mnt;
	wait_queue_head_t wait;
	struct mutex mux;
	struct list_head kthread_ctl_list;
};

#ifdef CONFIG_CRYPTO_DEV_KFIPS
struct ecryptfs_page_crypt_req;
typedef void (*page_crypt_completion)(
	struct ecryptfs_page_crypt_req *page_crypt_req);

struct ecryptfs_page_crypt_req {
	struct page *page;
	atomic_t num_refs;
	atomic_t rc;
	page_crypt_completion completion_func;
	struct completion completion;
};

struct ecryptfs_extent_crypt_req {
	struct ecryptfs_page_crypt_req *page_crypt_req;
	struct ablkcipher_request *req;
	struct ecryptfs_crypt_stat *crypt_stat;
	struct inode *inode;
	struct page *enc_extent_page;
	char extent_iv[ECRYPTFS_MAX_IV_BYTES];
	unsigned long extent_offset;
	struct scatterlist src_sg;
	struct scatterlist dst_sg;
};

#endif
struct inode *ecryptfs_get_inode(struct inode *lower_inode,
				 struct super_block *sb);
void ecryptfs_i_size_init(const char *page_virt, struct inode *inode);
int ecryptfs_decode_and_decrypt_filename(char **decrypted_name,
					 size_t *decrypted_name_size,
					 struct dentry *ecryptfs_dentry,
					 const char *name, size_t name_size);
int ecryptfs_fill_zeros(struct file *file, loff_t new_length);
int ecryptfs_encrypt_and_encode_filename(
	char **encoded_name,
	size_t *encoded_name_size,
	struct ecryptfs_crypt_stat *crypt_stat,
	struct ecryptfs_mount_crypt_stat *mount_crypt_stat,
	const char *name, size_t name_size);
struct dentry *ecryptfs_lower_dentry(struct dentry *this_dentry);
void ecryptfs_dump_hex(char *data, int bytes);
int virt_to_scatterlist(const void *addr, int size, struct scatterlist *sg,
			int sg_size);
int ecryptfs_compute_root_iv(struct ecryptfs_crypt_stat *crypt_stat);
void ecryptfs_rotate_iv(unsigned char *iv);
void ecryptfs_init_crypt_stat(struct ecryptfs_crypt_stat *crypt_stat);
void ecryptfs_destroy_crypt_stat(struct ecryptfs_crypt_stat *crypt_stat);
void ecryptfs_destroy_mount_crypt_stat(
	struct ecryptfs_mount_crypt_stat *mount_crypt_stat);
int ecryptfs_init_crypt_ctx(struct ecryptfs_crypt_stat *crypt_stat);
int ecryptfs_write_inode_size_to_metadata(struct inode *ecryptfs_inode);
#ifdef CONFIG_CRYPTO_DEV_KFIPS
struct ecryptfs_page_crypt_req *ecryptfs_alloc_page_crypt_req(
	struct page *page,
	page_crypt_completion completion_func);
void ecryptfs_free_page_crypt_req(
	struct ecryptfs_page_crypt_req *page_crypt_req);
#endif
int ecryptfs_encrypt_page(struct page *page);
#ifdef CONFIG_CRYPTO_DEV_KFIPS
void ecryptfs_encrypt_page_async(
	struct ecryptfs_page_crypt_req *page_crypt_req);
#endif
int ecryptfs_decrypt_page(struct page *page);
#ifdef CONFIG_CRYPTO_DEV_KFIPS
void ecryptfs_decrypt_page_async(
	struct ecryptfs_page_crypt_req *page_crypt_req);
#endif
int ecryptfs_write_metadata(struct dentry *ecryptfs_dentry,
			    struct inode *ecryptfs_inode);
int ecryptfs_read_metadata(struct dentry *ecryptfs_dentry);
int ecryptfs_new_file_context(struct inode *ecryptfs_inode);
void ecryptfs_write_crypt_stat_flags(char *page_virt,
				     struct ecryptfs_crypt_stat *crypt_stat,
				     size_t *written);
int ecryptfs_read_and_validate_header_region(struct inode *inode);
int ecryptfs_read_and_validate_xattr_region(struct dentry *dentry,
					    struct inode *inode);
u8 ecryptfs_code_for_cipher_string(char *cipher_name, size_t key_bytes);
int ecryptfs_cipher_code_to_string(char *str, u8 cipher_code);
void ecryptfs_set_default_sizes(struct ecryptfs_crypt_stat *crypt_stat);
int ecryptfs_generate_key_packet_set(char *dest_base,
				     struct ecryptfs_crypt_stat *crypt_stat,
				     struct dentry *ecryptfs_dentry,
				     size_t *len, size_t max);
int
ecryptfs_parse_packet_set(struct ecryptfs_crypt_stat *crypt_stat,
			  unsigned char *src, struct dentry *ecryptfs_dentry);
int ecryptfs_truncate(struct dentry *dentry, loff_t new_length);
ssize_t
ecryptfs_getxattr_lower(struct dentry *lower_dentry, const char *name,
			void *value, size_t size);
int
ecryptfs_setxattr(struct dentry *dentry, const char *name, const void *value,
		  size_t size, int flags);
int ecryptfs_read_xattr_region(char *page_virt, struct inode *ecryptfs_inode);
int ecryptfs_process_helo(uid_t euid, struct user_namespace *user_ns,
			  struct pid *pid);
int ecryptfs_process_quit(uid_t euid, struct user_namespace *user_ns,
			  struct pid *pid);
int ecryptfs_process_response(struct ecryptfs_message *msg, uid_t euid,
			      struct user_namespace *user_ns, struct pid *pid,
			      u32 seq);
int ecryptfs_send_message(char *data, int data_len,
			  struct ecryptfs_msg_ctx **msg_ctx);
int ecryptfs_wait_for_response(struct ecryptfs_msg_ctx *msg_ctx,
			       struct ecryptfs_message **emsg);
int ecryptfs_init_messaging(void);
void ecryptfs_release_messaging(void);

void
ecryptfs_write_header_metadata(char *virt,
			       struct ecryptfs_crypt_stat *crypt_stat,
			       size_t *written);
int ecryptfs_add_keysig(struct ecryptfs_crypt_stat *crypt_stat, char *sig);
int
ecryptfs_add_global_auth_tok(struct ecryptfs_mount_crypt_stat *mount_crypt_stat,
			   char *sig, u32 global_auth_tok_flags);
int ecryptfs_get_global_auth_tok_for_sig(
	struct ecryptfs_global_auth_tok **global_auth_tok,
	struct ecryptfs_mount_crypt_stat *mount_crypt_stat, char *sig);
int
ecryptfs_add_new_key_tfm(struct ecryptfs_key_tfm **key_tfm, char *cipher_name,
			 size_t key_size);
int ecryptfs_init_crypto(void);
int ecryptfs_destroy_crypto(void);
int ecryptfs_tfm_exists(char *cipher_name, struct ecryptfs_key_tfm **key_tfm);
int ecryptfs_get_tfm_and_mutex_for_cipher_name(struct crypto_blkcipher **tfm,
					       struct mutex **tfm_mutex,
					       char *cipher_name);
int ecryptfs_keyring_auth_tok_for_sig(struct key **auth_tok_key,
				      struct ecryptfs_auth_tok **auth_tok,
				      char *sig);
int ecryptfs_write_lower(struct inode *ecryptfs_inode, char *data,
			 loff_t offset, size_t size);
int ecryptfs_write_lower_page_segment(struct inode *ecryptfs_inode,
				      struct page *page_for_lower,
				      size_t offset_in_page, size_t size);
int ecryptfs_write(struct inode *inode, char *data, loff_t offset, size_t size);
int ecryptfs_read_lower(char *data, loff_t offset, size_t size,
			struct inode *ecryptfs_inode);
int ecryptfs_read_lower_page_segment(struct page *page_for_ecryptfs,
				     pgoff_t page_index,
				     size_t offset_in_page, size_t size,
				     struct inode *ecryptfs_inode);
struct page *ecryptfs_get_locked_page(struct inode *inode, loff_t index);
int ecryptfs_exorcise_daemon(struct ecryptfs_daemon *daemon);
int ecryptfs_find_daemon_by_euid(struct ecryptfs_daemon **daemon, uid_t euid,
				 struct user_namespace *user_ns);
int ecryptfs_parse_packet_length(unsigned char *data, size_t *size,
				 size_t *length_size);
int ecryptfs_write_packet_length(char *dest, size_t size,
				 size_t *packet_size_length);
int ecryptfs_init_ecryptfs_miscdev(void);
void ecryptfs_destroy_ecryptfs_miscdev(void);
int ecryptfs_send_miscdev(char *data, size_t data_size,
			  struct ecryptfs_msg_ctx *msg_ctx, u8 msg_type,
			  u16 msg_flags, struct ecryptfs_daemon *daemon);
void ecryptfs_msg_ctx_alloc_to_free(struct ecryptfs_msg_ctx *msg_ctx);
int
ecryptfs_spawn_daemon(struct ecryptfs_daemon **daemon, uid_t euid,
		      struct user_namespace *user_ns, struct pid *pid);
int ecryptfs_init_kthread(void);
void ecryptfs_destroy_kthread(void);
int ecryptfs_privileged_open(struct file **lower_file,
			     struct dentry *lower_dentry,
			     struct vfsmount *lower_mnt,
			     const struct cred *cred);
int ecryptfs_get_lower_file(struct dentry *dentry, struct inode *inode);
void ecryptfs_put_lower_file(struct inode *inode);
int
ecryptfs_write_tag_70_packet(char *dest, size_t *remaining_bytes,
			     size_t *packet_size,
			     struct ecryptfs_mount_crypt_stat *mount_crypt_stat,
			     char *filename, size_t filename_size);
int
ecryptfs_parse_tag_70_packet(char **filename, size_t *filename_size,
			     size_t *packet_size,
			     struct ecryptfs_mount_crypt_stat *mount_crypt_stat,
			     char *data, size_t max_packet_size);
int ecryptfs_set_f_namelen(long *namelen, long lower_namelen,
			   struct ecryptfs_mount_crypt_stat *mount_crypt_stat);
int ecryptfs_derive_iv(char *iv, struct ecryptfs_crypt_stat *crypt_stat,
		       loff_t offset);

#endif /*                           */
