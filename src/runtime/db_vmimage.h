/* db_vmimage.h - compiled image definitions
 *
 * Copyright (c) 2011 by David Michael Betz.  All rights reserved.
 *
 */

#ifndef __DB_VMIMAGE_H__
#define __DB_VMIMAGE_H__

#include "db_system.h"
#include "db_image.h"

/* image file section */
typedef struct {
    ImageFileSection *fileSection;
    uint8_t *data;
} ImageSection;

/* in-memory image header */
typedef struct {
    VMUVALUE        mainCode;   /* main code */
    VMUVALUE        sectionCount;
    ImageSection    sections[1];
} ImageHdr;

#endif
