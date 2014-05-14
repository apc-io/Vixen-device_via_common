/*
 * Copyright (c) 2013 WonderMedia Technologies, Inc. All Rights Reserved.
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
#ifndef LINK_LIST_H
#define LINK_LIST_H

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct ListLinker {
    struct ListLinker *next, *prev;
    void *data;
}ListLinker;

typedef ListLinker * LinkList;

int link_list_size(LinkList ll);
int link_list_indexOfData(LinkList ll, void *data);
int link_list_insertAt(LinkList *pll, int index, void *data);
int link_list_push(LinkList *pll, void *data);
void* link_list_pop(LinkList *pll);
void* link_list_removeAt(LinkList *pll, int index);
void* link_list_editAt(LinkList ll, int index);
void* link_list_removeData(LinkList *pll, void *data);

#ifdef WMT_CONFIG_UT
inline void link_list_show(LinkList node);
int link_list_ut(void);
#endif

#if defined(__cplusplus)
}
#endif

#endif /* LINK_LIST_H */
