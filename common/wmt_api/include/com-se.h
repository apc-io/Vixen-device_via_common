/*
 * Copyright (c) 2008-2013 WonderMedia Technologies, Inc. All Rights Reserved.
 *
 * This PROPRIETARY SOFTWARE is the property of WonderMedia Technologies, Inc.
 * and may contain trade secrets and/or other confidential information of 
 * WonderMedia Technologies, Inc. This file shall not be disclosed to any
 * third party, in whole or in part, without prior written consent of
 * WonderMedia.
 *
 * THIS PROPRIETARY SOFTWARE AND ANY RELATED DOCUMENTATION ARE PROVIDED
 * AS IS, WITH ALL FAULTS, AND WITHOUT WARRANTY OF ANY KIND EITHER EXPRESS
 * OR IMPLIED, AND WONDERMEDIA TECHNOLOGIES, INC. DISCLAIMS ALL EXPRESS
 * OR IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE,
 * QUIET ENJOYMENT OR NON-INFRINGEMENT.
 */
#ifndef COM_SE_H
/* To assert that only one occurrence is included */
#define COM_SE_H

/*-------------------- MODULE DEPENDENCY -------------------------------------*/


/*-------------------- EXPORTED PRIVATE CONSTANTS ----------------------------*/

// Global definition
#ifndef u8
#define u8  unsigned char
#endif
#ifndef u16
#define u16 unsigned short
#endif
#ifndef u32
#define u32 unsigned int
#endif

#ifndef IN
#define IN
#endif

#ifndef OUT
#define OUT
#endif

/*------------------------------------------------------------------------------
    Common members in structure for all kinds of encrypt/descrypt
------------------------------------------------------------------------------*/
#define CYPHER_COMMON_CFG() \
    int fd;  /* file handle from driver */ \
    cypher_algo_t algo_mode; \
    u32 input_addr;   \
    u32 output_addr;	\
    u32 text_length
/* End of CYPHER_COMMON_CFG() */


//#define CIPHER_IOC_MAGIC	'k'
#define CIPHER_IOC_MAGIC	238

#define CIPHER_ENCRYPT              _IOWR(CIPHER_IOC_MAGIC, 0, int)
#define CIPHER_DECRYPT              _IOWR(CIPHER_IOC_MAGIC, 1, int)
#define CIPHER_GET_ADDR_INPUT       _IOWR(CIPHER_IOC_MAGIC, 2, int)
#define CIPHER_GET_ADDR_OUTPUT      _IOWR(CIPHER_IOC_MAGIC, 3, int)
#define CIPHER_LOCK                 _IO(CIPHER_IOC_MAGIC, 4)
#define CIPHER_UNLOCK               _IO(CIPHER_IOC_MAGIC, 5)
#define CIPHER_IOC_MAXNR            6

/*-------------------- EXPORTED PRIVATE TYPES---------------------------------*/
typedef enum cypher_algo_e
{
    CYPHER_ALGO_AES = 0,        // Advanced Encryption Standard      
    CYPHER_ALGO_RC4,            // Rivest Cipher
    CYPHER_ALGO_DES,            // Data Encryption Standard
    CYPHER_ALGO_TDES,           // Three Data Encryption Standard
    CYPHER_ALGO_SHA1,           // Secure Hash Algorithm 
    CYPHER_ALGO_SHA256,         // Secure Hash Algorithm 256
    CYPHER_ALGO_SHA224, 		    // Secure Hash Algorithm 224
    CYPHER_ALGO_RNG,			      //Random Generation
    CYPHER_ALGO_MAX
} cypher_algo_t;	// Cryptographic algorithm mode

// http://www-128.ibm.com/developerworks/tw/library/s-crypt02.html
typedef enum cypher_op_e
{
    CYPHER_OP_ECB = 0,      // Electronic Codebook mode
    CYPHER_OP_CBC,          // Cipher Block Chaining mode
    CYPHER_OP_CTR,          // Counter mode
    CYPHER_OP_OFB,          // Output Feedback mode
    CYPHER_OP_RC4_HW_KEY,   // Key calculate by HW
    CYPHER_OP_RC4_SW_KEY,   // Key calculate by SW
    CYPHER_OP_ECB_HW_KEY,	// Electronic Codebook mode HW key
    CYPHER_OP_CBC_HW_KEY,	// Electronic Codebook mode HW key
    CYPHER_OP_CTR_HW_KEY,	// Electronic Codebook mode HW key
    CYPHER_OP_OFB_HW_KEY,	// Electronic Codebook mode HW key
    CYPHER_OP_PCBC, 		    // Output Feedback mode
    CYPHER_OP_CFB_dec,		
    CYPHER_OP_CFB_enc,		
    CYPHER_OP_ECB_CTS,		
    CYPHER_OP_CBC_CTS,		
    CYPHER_OP_PCBC_CTS, 

    CYPHER_OP_MAX
} cypher_op_t;

typedef struct cypher_base_cfg_s
{
    CYPHER_COMMON_CFG();

    cypher_op_t op_mode;
    u32 dec_enc;   /* Used in driver only */
    u32 key_addr;  // address
    u32 IV_addr;   // address: Used in both CBC and CTR modes 
    u32 INC;       // Used in CTR mode only
    u32 key_len;   // Used in RC4 mode only
    u32 sha1_data_length;	// for SHA1 only
    u32 sha256_data_length;	// for SHA256 only
    u32 reseed_interval; /*Used in RNG only*/
} cypher_base_cfg_t;


/*-------------------- EXPORTED PRIVATE VARIABLES -----------------------------*/
#ifdef WMT_SE_C /* allocate memory for variables only in wmt-se.c */
#       define EXTERN
#else
#       define EXTERN   extern
#endif /* ifdef WMT_SE_C */

#undef EXTERN

/*--------------------- EXPORTED PRIVATE MACROS -------------------------------*/
// Encrypt/Decrypt mode
#define CYPHER_ENCRYPT      1
#define CYPHER_DECRYPT      0

#define CYPHER_ZERO         0x0L

#define BUFFER_MULTIPLE     256
#define DMA_BOUNDARY        16  // 16 bytes

// is 4096 ==> total_byes:4096*sizeof(u32)=16384
#define BUFFER_MAX (BUFFER_MULTIPLE * DMA_BOUNDARY * 64 ) //add size to 1M by Brad 2011.08.26

#define AES_KEY_BYTE_SIZE           16  //  = 128 bits = 16 char = 32 hex int
#define DES_KEY_BYTE_SIZE           8   //  8B = 64 bits = 8 char = 16 hex int
#define TDES_KEY_BYTE_SIZE          24  //  24B = 192 bits = 24 char = 48 hex int

/*--------------------- EXPORTED PRIVATE FUNCTIONS  ---------------------------*/


#endif /* ifndef COM_SE_H */

/*=== END com-se.h ==========================================================*/
