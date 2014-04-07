/*
 * Copyright 2013 WonderMedia Technologies, Inc. All Rights Reserved. 
 *  
 * This PROPRIETARY SOFTWARE is the property of WonderMedia Technologies, Inc. 
 * and may contain trade secrets and/or other confidential information of 
 * WonderMedia Technologies, Inc. This file shall not be disclosed to any third party, 
 * in whole or in part, without prior written consent of WonderMedia. 
 *  
 * THIS PROPRIETARY SOFTWARE AND ANY RELATED DOCUMENTATION ARE PROVIDED AS IS, 
 * WITH ALL FAULTS, AND WITHOUT WARRANTY OF ANY KIND EITHER EXPRESS OR IMPLIED, 
 * AND WonderMedia TECHNOLOGIES, INC. DISCLAIMS ALL EXPRESS OR IMPLIED WARRANTIES 
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT.  
 */

//#define LOG_NDEBUG 0
#define LOG_TAG "CameraUtil"
#include <utils/Log.h>
#include <libyuv.h>

#include "CameraUtil.h"

void convertNV21toNV12(uint8_t *inputBuffer, uint8_t *outputBuffer, int width, int height)
{
    int Ysize = 0, UVsize = 0;
    uint8_t *Yin, *Uin, *Vin, *Yout, *Uout, *Vout;

    Ysize = width * height;
    UVsize = width * height >> 2;

    /* NV21: yyyy vuvu */
    Yin = inputBuffer;
    Vin = Yin + Ysize;
    Uin = Vin + 1;

    /* NV12: yyyy uvuv */
    Yout = outputBuffer;
    Uout = Yout + Ysize;
    Vout = Uout + 1;

    memcpy(Yout, Yin, Ysize);

    for (int k = 0; k < UVsize; k++) {
        *Uout = *Uin;
        *Vout = *Vin;
        Uout += 2;
        Vout += 2;
        Uin  += 2;
        Vin  += 2;
    }
}

int uvtonv21(unsigned char *src, int width, int height)
{
    int size = (width*height)>>1;
    unsigned char *ctmp, *u, *v, *cbcr, *tmp;
    int i;
    if (!src || !width || !height) {
        ALOGE("uvtonv21 error: src = %p , width = %d , height = %d", src, width, height);
        return -1;
    }
    ctmp = (unsigned char *)malloc(width*height);
    memcpy(ctmp, src, size);
    tmp = ctmp;
    size = size >> 1;
    u = ctmp;
    v = (unsigned char *)(ctmp+size);
    cbcr = src;
    //if (width%32) {
    for (i= 0; i < size; i++) {
        *cbcr = *u;
        cbcr++;
        *cbcr = *v;
        cbcr++;
        u++;
        v++;
    }
    //}
    free(tmp);
    return 0;
}

int nv21touv(void * source, int width, int height)
{
    unsigned char *src = (unsigned char *)source;

    int size = (width*height)>>1;
    unsigned char *ctmp, *u, *v, *tmp;
    int i;
    if (!src || !width || !height) {
        ALOGE("src = %p , width = %d , height = %d", src, width, height);
        return -1;
    }
    ctmp = (unsigned char *)malloc(width*height);
    tmp = ctmp;
    memcpy(ctmp, src, size);
    u = src;
    size = size >> 1;
    v = (unsigned char *)(src+size);
    //if (width%32) {
    for (i= 0; i < size; i++) {
        *u = *ctmp;
        ctmp++;
        *v = *ctmp;
        ctmp++;
        u++;
        v++;
    }
    //}
    free(tmp);
    return 0;
}

int nv21Scale(unsigned char *src, int src_w, int src_h,
              unsigned char *dst, int dst_w, int dst_h)
{
    uint8_t *uv_src, *uv_dest;
    int uv_src_size, uv_dest_size;

    uv_src = src;
    uv_src += src_w * src_h;
    uv_src_size = (src_w * src_h) / 4;
    nv21touv(uv_src, src_w, src_h);

    uv_dest = dst;
    uv_dest += (dst_w * dst_h);
    uv_dest_size = (dst_w * dst_h)/4;

    I420Scale(src, src_w,
              uv_src, src_w/2,
              uv_src+uv_src_size, src_w/2,
              src_w, src_h,
              dst, dst_w ,
              uv_dest, dst_w/2,
              uv_dest+uv_dest_size, dst_w/2,
              dst_w, dst_h,
              libyuv::kFilterNone);

    uvtonv21(uv_dest, dst_w, dst_h);

    return 0;
}

// for zoom function
int nv21CropScale(unsigned char *data, int pictruew, int pictrueh, int cropw, int croph)
{
    size_t size = (cropw * croph * 12) / 8;
    unsigned char *tmp = (unsigned char *)malloc(size);
    if (!tmp)
        return -ENOMEM;

    nv21Crop(data, pictruew, pictrueh, tmp, cropw, croph);
    nv21Scale(tmp, cropw, croph, data, pictruew, pictrueh);

    free(tmp);

    return 0;
}

int nv21Crop(unsigned char *src, int src_w, int src_h,
             unsigned char *dst, int dst_w, int dst_h)
{
    int i, j, spos, dpos;
    int tempCb, tempCr;
    int length, height, xofs = 0, yofs = 0;
    unsigned char *s = src;
    unsigned char *d = dst;
    unsigned char *cs = s + src_w * src_h;
    unsigned char *cd = d + dst_h * dst_w;
    int *CbCrd;

    length = src_w;
    if (dst_w > src_w) {
        xofs = (dst_w - src_w) / 2;
        d += xofs;
        cd += xofs;
        length = src_w;
    } else if (dst_w < src_w) {
        xofs = (src_w - dst_w) / 2;
        s += xofs;
        cs += xofs;
        length = dst_w;
        xofs = 0;
    }

    height = src_h;
    if (dst_h > src_h) {
        yofs = (dst_h - src_h)/2;
        d += (yofs*dst_w);
        cd += ((yofs*dst_w) >> 1);
        height = src_h; 
    } else if (src_h > dst_h) {
        yofs = (src_h - dst_h)/2;
        s += (yofs*src_w);
        cs += ((yofs*src_w) >> 1);
        height = dst_h;
        yofs = 0;
    }

    //copy Y
    for (j = 0; j < height; j++) {
        memcpy(d, s, length);
        d += dst_w;
        s += src_w;
    }

    if (xofs || yofs) {
        CbCrd = (int *)(dst+(dst_w*dst_h));
        j = dst_w * dst_h >> 3;
        for(i = 0; i < j; i++)
            CbCrd[i] = 0x80808080;
    }

    //copy CbCr
    height = height/2;
    for (j = 0; j < height; j++) {
        memcpy(cd, cs, length);
        cd += dst_w;
        cs += src_w;
    }

    return 0;
}


void YC422toNV21(const YUVBuffer * src, YUVBuffer * dst){

    int dst_w = dst->w, dst_h = dst->h;
	int src_w = src->w, src_h = src->h;
	int width = dst_w, height = dst_h;
    int i,j;
    char *src_y = src->addr;
    char *dst_y = dst->addr;
    char *src_c = src_y + src_w*src_h;
    char *dst_c = dst_y + dst_w*dst_h;

    if (src_w < dst_w){
        width = src_w;
        dst_y += (dst_w - src_w)/2;
        dst_c += (dst_w - src_w)/2;
    }else if (src_w > dst_w){
        width = dst_w;
        src_y += (src_w - dst_w)/2;
        src_c += (src_w - dst_w)/2;
    }

    if (src_h < dst_h){
        height = src_h;
        dst_y += dst_w * (dst_h - src_h)/2;
        dst_c += dst_w * (dst_h - src_h)/4;
    }else if (src_h > dst_h){
        height = dst_h;
        src_y += src_w * (src_h - dst_h)/2;
        src_c += src_w * (src_h - dst_h)/2;
    }

    if (dst_y != dst->addr){
        memset(dst+ dst_w*dst_h, 0x80, dst_w*dst_h/2);
    }

    for (j = 0; j < height; j++){
        memcpy(dst_y, src_y,width);
        dst_y+=dst_w;
        src_y+=src_w;
    }

    for (j = 0; j < height/2; j++){
        if ((width % 32) == 0){
            char *s_c, *d_c;
            s_c = src_c + j * src_w *2;
            d_c = dst_c + j * dst_w;
            asm volatile(
                "mov        r0,         %[cnt]\n\t"
                "2:\n\t"
                "subs       r0,         #32\n\t"
                "vld2.8     {d0-d3},    [%[src]]!\n\t"
                "vswp       q0,         q1\n\t"
                "vst2.8     {d0-d3},    [%[dst]]!\n\t"
                "bne        2b\n\t"
                :[src]"+r"(s_c),[dst]"+r"(d_c)
                :[cnt]"r"(width)
                :"r0","cc", "d0", "d1", "d2", "d3"
                );
           
        }else{
                int *s_c, *d_c, *r_c,temp;
                s_c = (int*)(src_c + j * src_w *2);
                d_c = (int*)(dst_c + j * dst_w);
            for (i = 0; i < width/4; i++){
                temp = s_c[i];
                d_c[i] = (((temp << 8) & 0xFF00FF00) |  ((temp >> 8) & 0x00FF00FF));
            }
        }
    }
}


void YC422toNV12(const YUVBuffer * src, YUVBuffer * dst){

    int dst_w = dst->w, dst_h = dst->h;
	int src_w = src->w, src_h = src->h;
	int width = dst_w, height = dst_h;
    int i,j;
    char *src_y = src->addr;
    char *dst_y = dst->addr;
    char *src_c = src_y + src_w*src_h;
    char *dst_c = dst_y + dst_w*dst_h;

    if (src_w < dst_w){
        width = src_w;
        dst_y += (dst_w - src_w)/2;
        dst_c += (dst_w - src_w)/2;
    }else if (src_w > dst_w){
        width = dst_w;
        src_y += (src_w - dst_w)/2;
        src_c += (src_w - dst_w)/2;
    }

    if (src_h < dst_h){
        height = src_h;
        dst_y += dst_w * (dst_h - src_h)/2;
        dst_c += dst_w * (dst_h - src_h)/4;
    }else if (src_h > dst_h){
        height = dst_h;
        src_y += src_w * (src_h - dst_h)/2;
        src_c += src_w * (src_h - dst_h)/2;
    }

    if (dst_y != dst->addr){
        memset(dst+ dst_w*dst_h, 0x80, dst_w*dst_h/2);
    }

    for (j = 0; j < height; j++){
        memcpy(dst_y, src_y,width);
        dst_y+=dst_w;
        src_y+=src_w;
    }

    for (j = 0; j < height/2; j++){
        if ((width % 32) == 0){
            char *s_c, *d_c;
            s_c = src_c + j * src_w *2;
            d_c = dst_c + j * dst_w;
            asm volatile(
                "mov        r0,         %[cnt]\n\t"
                "2:\n\t"
                "subs       r0,         #32\n\t"
                "vld2.8     {d0-d3},    [%[src]]!\n\t"
                "vst2.8     {d0-d3},    [%[dst]]!\n\t"
                "bne        2b\n\t"
                :[src]"+r"(s_c),[dst]"+r"(d_c)
                :[cnt]"r"(width)
                :"r0","cc", "d0", "d1", "d2", "d3"
                );
           
        }else{
                int *s_c, *d_c, *r_c,temp;
                s_c = (int*)(src_c + j * src_w *2);
                d_c = (int*)(dst_c + j * dst_w);
            for (i = 0; i < width/4; i++){
                d_c[i] = s_c[i];
                //d_c[i] = (((temp << 8) & 0xFF00FF00) |  ((temp >> 8) & 0x00FF00FF));
            }
        }
    }
}


void YC422toNV21_RecNV12(char *src, char *dst, char *rec, int src_w, int src_h, int dst_w, int dst_h){
    int width = dst_w, height = dst_h;
    int i,j;
    char *src_y = src;
    char *dst_y = dst;
    char *src_c = src + src_w*src_h;
    char *dst_c = dst + dst_w*dst_h;
    char *rec_y = rec;
    char *rec_c = rec + dst_w*dst_h;

    if (src_w < dst_w){
        width = src_w;
        dst_y += (dst_w - src_w)/2;
        dst_c += (dst_w - src_w)/2;
        rec_y += (dst_w - src_w)/2;
        rec_c += (dst_w - src_w)/2;
    }else if (src_w > dst_w){
        width = dst_w;
        src_y += (src_w - dst_w)/2;
        src_c += (src_w - dst_w)/2;
    }

    if (src_h < dst_h){
        height = src_h;
        dst_y += dst_w * (dst_h - src_h)/2;
        rec_y += dst_w * (dst_h - src_h)/2;
        dst_c += dst_w * (dst_h - src_h)/4;
        rec_c += dst_w * (dst_h - src_h)/4;
    }else if (src_h > dst_h){
        height = dst_h;
        src_y += src_w * (src_h - dst_h)/2;
        src_c += src_w * (src_h - dst_h)/2;
    }

    if (dst_y != dst){
        memset(dst+ dst_w*dst_h, 0x80, dst_w*dst_h/2);
        memset(rec + dst_w*dst_h, 0x80, dst_w*dst_h/2);

    }

    for (j = 0; j < height; j++){
        memcpy(dst_y, src_y,width);
        memcpy(rec_y, src_y, width);
        rec_y += dst_w;
        dst_y+=dst_w;
        src_y+=src_w;
    }


    for (j = 0; j < height/2; j++){
        if ((width % 32) == 0){
            char *s_c, *d_c, *r_c;
            s_c = src_c + j * src_w *2;
            d_c = dst_c + j * dst_w;
            r_c = rec_c + j * dst_w;
            asm volatile(
                "mov        r0,         %[cnt]\n\t"
                "1:\n\t"
                "subs       r0,         #32\n\t"
                "vld2.8     {d0-d3},    [%[src]]!\n\t"
                "vst2.8     {d0-d3},    [%[rec]]!\n\t"
                "vswp       q0,         q1\n\t"
                "vst2.8     {d0-d3},    [%[dst]]!\n\t"
                "bne        1b\n\t"
                :[src]"+r"(s_c),[dst]"+r"(d_c),[rec]"+r"(r_c)
                :[cnt]"r"(width)
                :"r0","cc", "d0", "d1", "d2", "d3"
                );
        }else{
	        int *s_c, *d_c, *r_c,temp;
	        s_c = (int*)(src_c + j * src_w *2);
	        d_c = (int*)(dst_c + j * dst_w);
	        r_c = (int*)(rec_c + j * dst_w);
	    	for (i = 0; i < width/4; i++){
	        temp = s_c[i];
	        r_c[i] = temp;
	        d_c[i] = (((temp << 8) & 0xFF00FF00) |  ((temp >> 8) & 0x00FF00FF));
	    	}
        }
    }
}

void NV21_Crop(const YUVBuffer * src, YUVBuffer * dst){
    int dst_w = dst->w, dst_h = dst->h;
	int src_w = src->w, src_h = src->h;
    int width = dst_w, height = dst_h;
    int i,j;
    char *src_y = src->addr;
    char *dst_y = dst->addr;
    char *src_c = src_y + src_w*src_h;
    char *dst_c = dst_y + dst_w*dst_h;

    if (src_w < dst_w){
        width = src_w;
        dst_y += (dst_w - src_w)/2;
        dst_c += (dst_w - src_w)/2;
    }else if (src_w > dst_w){
        width = dst_w;
        src_y += (src_w - dst_w)/2;
        src_c += (src_w - dst_w)/2;
    }

    if (src_h < dst_h){
        height = src_h;
        dst_y += dst_w * (dst_h - src_h)/2;
        dst_c += dst_w * (dst_h - src_h)/4;
    }else if (src_h > dst_h){
        height = dst_h;
        src_y += src_w * (src_h - dst_h)/2;
        src_c += src_w * (src_h - dst_h)/4;
    }

    if (dst_y != dst->addr){
        memset(dst+ dst_w*dst_h, 0x80, dst_w*dst_h/2);
    }

    for (j = 0; j < height; j++){
        memcpy(dst_y, src_y,width);
        dst_y+=dst_w;
        src_y+=src_w;
    }


    for (j = 0; j < height/2; j++){
        memcpy(dst_c, src_c, width);
        dst_c += dst_w;
        src_c += src_w;
    }
}

void YUY2toNV21_RecNV12(char *src, char *dst, char *rec,int src_w, int src_h,int dst_w, int dst_h){
    int width = dst_w, height = dst_h;
    int i,j;
    char *src_y = src;
    char *dst_y = dst;
    char *dst_c = dst + dst_w*dst_h;
    char *rec_y = rec;
    char *rec_c = rec + dst_w*dst_h;


    if (src_w < dst_w){
        width = src_w;
        dst_y += (dst_w - src_w)/2;
        dst_c += (dst_w - src_w)/2;
        rec_y += (dst_w - src_w)/2;
        rec_c += (dst_w - src_w)/2;
    }else if (src_w > dst_w){
        width = dst_w;
        src_y += (src_w - dst_w);
    }

    if (src_h < dst_h){
        height = src_h;
        dst_y += dst_w * (dst_h - src_h)/2;
        rec_y += dst_w * (dst_h - src_h)/2;
        dst_c += dst_w * (dst_h - src_h)/4;
        rec_c += dst_w * (dst_h - src_h)/4;
    }else if (src_h > dst_h){
        height = dst_h;
        src_y += src_w * (src_h - dst_h);
    }

    if (dst_y != dst){
        memset(dst + dst_w*dst_h, 0x80, dst_w*dst_h/2);
        memset(rec + dst_w*dst_h, 0x80, dst_w*dst_h/2);
    }


    if ((width % 16) == 0){
        for (j = 0; j < height; j+=2){
            char *s_y, *d_y, *r_y, *d_c, *r_c;
            s_y = src_y + j * src_w *2;
            d_y = dst_y + j * dst_w;
            r_y = rec_y + j * dst_w;
            d_c = dst_c + (j/2) * dst_w;
            r_c = rec_c + (j/2) * dst_w;
            asm volatile(
                "mov        r0,         %[cnt]\n\t"
                "1:\n\t"
                "subs       r0,         #16\n\t"
                "vld4.8     {d0-d3},    [%[src]]!\n\t"
                "vst2.8     {d0,d2},    [%[dsty]]!\n\t"
                "vst2.8     {d0,d2},    [%[recy]]!\n\t"
                "vst2.8     {d1,d3},    [%[recc]]!\n\t"
                "vswp       d1,         d3\n\t"
                "vst2.8     {d1,d3},    [%[dstc]]!\n\t"
                "bne        1b\n\t"
                :[src]"+r"(s_y),[dsty]"+r"(d_y),[recy]"+r"(r_y),[dstc]"+r"(d_c),[recc]"+r"(r_c)
                :[cnt]"r"(width)
                :"memory","d0","d1","d2","d3","r0", "cc"
                );

            s_y = src_y + (j+1) * src_w *2;
            d_y = dst_y + (j+1) * dst_w;
            r_y = rec_y + (j+1) * dst_w;
            asm volatile(
                "mov        r0,         %[cnt]\n\t"
                "1:\n\t"
                "subs       r0,         #16\n\t"
                "vld4.8     {d0-d3},    [%[src]]!\n\t"
                "vst2.8     {d0,d2},    [%[dsty]]!\n\t"
                "vst2.8     {d0,d2},    [%[recy]]!\n\t"
                "bne        1b\n\t"
                :[src]"+r"(s_y),[dsty]"+r"(d_y),[recy]"+r"(r_y)
                :[cnt]"r"(width)
                :"memory","d0","d1","d2","d3","r0", "cc"
                );
        }
    }
}

void YUY2toNV21(const YUVBuffer * src, YUVBuffer * dst){
    int dst_w = dst->w, dst_h = dst->h;
	int src_w = src->w, src_h = src->h;
	int width = dst_w, height = dst_h;
    int i,j;
    char *src_y = src->addr;
    char *dst_y = dst->addr;
    char *dst_c = dst_y + width*height;


    if (src_w < dst_w){
        width = src_w;
        dst_y += (dst_w - src_w)/2;
        dst_c += (dst_w - src_w)/2;
    }else if (src_w > dst_w){
        width = dst_w;
        src_y += (src_w - dst_w);
    }

    if (src_h < dst_h){
        height = src_h;
        dst_y += dst_w * (dst_h - src_h)/2;
        dst_c += dst_w * (dst_h - src_h)/4;
    }else if (src_h > dst_h){
        height = dst_h;
        src_y += src_w * (src_h - dst_h);
    }

    if (dst_y != dst->addr){
        memset(dst + dst_w*dst_h, 0x80, dst_w*dst_h/2);
    }

    if ((width % 16) == 0){
        for (j = 0; j < height; j+=2){
            char *s_y, *d_y, *d_c;
            s_y = src_y + j * src_w *2;
            d_y = dst_y + j * dst_w;
            d_c = dst_c + (j/2) * dst_w;
            asm volatile(
                "mov        r0,         %[cnt]\n\t"
                "1:\n\t"
                "subs       r0,         #16\n\t"
                "vld4.8     {d0-d3},    [%[src]]!\n\t"
                "vst2.8     {d0,d2},    [%[dsty]]!\n\t"
                "vswp       d1,         d3\n\t"
                "vst2.8     {d1,d3},    [%[dstc]]!\n\t"
                "bne        1b\n\t"
                :[src]"+r"(s_y),[dsty]"+r"(d_y),[dstc]"+r"(d_c)
                :[cnt]"r"(width)
                :"memory","d0","d1","d2","d3","r0", "cc"
                );

            s_y = src_y + (j+1) * src_w *2;
            d_y = dst_y + (j+1) * dst_w;
            asm volatile(
                "mov        r0,         %[cnt]\n\t"
                "1:\n\t"
                "subs       r0,         #16\n\t"
                "vld4.8     {d0-d3},    [%[src]]!\n\t"
                "vst2.8     {d0,d2},    [%[dsty]]!\n\t"
                "bne        1b\n\t"
                :[src]"+r"(s_y),[dsty]"+r"(d_y)
                :[cnt]"r"(width)
                :"memory","d0","d1","d2","d3","r0", "cc"
                );
        }
    }
}


void YUY2toNV12(const YUVBuffer * src, YUVBuffer * dst){
    int dst_w = dst->w, dst_h = dst->h;
	int src_w = src->w, src_h = src->h;
	int width = dst_w, height = dst_h;
    int i,j;
    char *src_y = src->addr;
    char *dst_y = dst->addr;
    char *dst_c = dst_y + width*height;


    if (src_w < dst_w){
        width = src_w;
        dst_y += (dst_w - src_w)/2;
        dst_c += (dst_w - src_w)/2;
    }else if (src_w > dst_w){
        width = dst_w;
        src_y += (src_w - dst_w);
    }

    if (src_h < dst_h){
        height = src_h;
        dst_y += dst_w * (dst_h - src_h)/2;
        dst_c += dst_w * (dst_h - src_h)/4;
    }else if (src_h > dst_h){
        height = dst_h;
        src_y += src_w * (src_h - dst_h);
    }

    if (dst_y != dst->addr){
        memset(dst + dst_w*dst_h, 0x80, dst_w*dst_h/2);
    }

    if ((width % 16) == 0){
        for (j = 0; j < height; j+=2){
            char *s_y, *d_y, *d_c;
            s_y = src_y + j * src_w *2;
            d_y = dst_y + j * dst_w;
            d_c = dst_c + (j/2) * dst_w;
            asm volatile(
                "mov        r0,         %[cnt]\n\t"
                "1:\n\t"
                "subs       r0,         #16\n\t"
                "vld4.8     {d0-d3},    [%[src]]!\n\t"
                "vst2.8     {d0,d2},    [%[dsty]]!\n\t"
                "vst2.8     {d1,d3},    [%[dstc]]!\n\t"
                "bne        1b\n\t"
                :[src]"+r"(s_y),[dsty]"+r"(d_y),[dstc]"+r"(d_c)
                :[cnt]"r"(width)
                :"memory","d0","d1","d2","d3","r0", "cc"
                );

            s_y = src_y + (j+1) * src_w *2;
            d_y = dst_y + (j+1) * dst_w;
            asm volatile(
                "mov        r0,         %[cnt]\n\t"
                "1:\n\t"
                "subs       r0,         #16\n\t"
                "vld4.8     {d0-d3},    [%[src]]!\n\t"
                "vst2.8     {d0,d2},    [%[dsty]]!\n\t"
                "bne        1b\n\t"
                :[src]"+r"(s_y),[dsty]"+r"(d_y)
                :[cnt]"r"(width)
                :"memory","d0","d1","d2","d3","r0", "cc"
                );
        }
    }
}


void YUVConvert(const YUVBuffer * src, YUVBuffer * dst){
	switch (src->format){
		case YUV_PIX_FMT_YUYV	:
			if (dst->format == YUV_PIX_FMT_NV21)
				YUY2toNV21(src, dst);
			else if (dst->format == YUV_PIX_FMT_NV12)
				YUY2toNV12(src, dst);
			else
				ALOGW("format convert not support yet! at line %d",__LINE__);
			break;
		case YUV_PIX_FMT_YC422	:
			if (dst->format == YUV_PIX_FMT_NV21)
				YC422toNV21(src, dst);
			else if (dst->format == YUV_PIX_FMT_NV12)
				YC422toNV12(src, dst);
			else
				ALOGW("format convert not support yet! at line %d",__LINE__);
			break;
		case YUV_PIX_FMT_NV21	:
			if (dst->format == YUV_PIX_FMT_NV21)
				NV21_Crop(src, dst);
			else
				ALOGW("format convert not support yet! at line %d",__LINE__);
			break;
		default	:
			ALOGW("format convert not support yet! at line %d",__LINE__);
			break;
	}
}

void YUVSacle(const YUVBuffer * src, YUVBuffer * dst){

}


void YUVMirror(const YUVBuffer * src, YUVBuffer * dst){

}
void yuvBufferConvert(const YUVBuffer * src, YUVBuffer * dst, int convertFlag){

	switch (convertFlag){
		case	YUV_CONVERT_ROTATE0 :
			YUVConvert(src, dst);
			break;
		case	YUV_CONVERT_SCALE :
			YUVSacle(src, dst);
			break;

		case	YUV_CONVERT_MIRROR	:
			YUVMirror(src, dst);
			break;
		default	:
			ALOGW("format convert not support yet! at line %d",__LINE__);
			break;
	}
}

#define ALIGN(x, a)       (((x) + (a) - 1) & ~((a) - 1))
#define ALIGN_TO_32B(x)   ((((x) + (1 <<  5) - 1) >>  5) <<  5)
#define ALIGN_TO_128B(x)  ((((x) + (1 <<  7) - 1) >>  7) <<  7)
#define ALIGN_TO_8KB(x)   ((((x) + (1 << 13) - 1) >> 13) << 13)

#define GET_32BPP_FRAME_SIZE(w, h)  (((w) * (h)) << 2)
#define GET_24BPP_FRAME_SIZE(w, h)  (((w) * (h)) * 3)
#define GET_16BPP_FRAME_SIZE(w, h)  (((w) * (h)) << 1)

unsigned int FRAME_SIZE(int hal_pixel_format, int width, int height)
{
    unsigned int expectedBytes;

    switch (hal_pixel_format) {
    case V4L2_PIX_FMT_YVU420:
        {
            int yStride = ALIGN(width, 16);
            int uvStride = ALIGN(yStride, 32) / 2;
            int ySize = yStride * height;
            int uvSize = uvStride * height / 2;
            expectedBytes = ySize + uvSize * 2;
        }
        break;

    case V4L2_PIX_FMT_NV21:
    default:
        expectedBytes = width * height * 12 / 8;
        break;

    }

    return expectedBytes;
}

// vim: et ts=4 shiftwidth=4
