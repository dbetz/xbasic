/* db_vmimage.c - compiled image functions
 *
 * Copyright (c) 2011 by David Michael Betz.  All rights reserved.
 *
 */

#include <string.h>
#include "db_vmimage.h"
#include "db_vmdebug.h"
#include "db_vm.h"

/* LoadImage - load an image from a file */
ImageHdr *LoadImage(System *sys)
{
    ImageFileHdr fileHdr;
    ImageFileSection *src;
    ImageHdr *image;
    ImageSection *dst;
    int count;

    /* read the image file header */
    if (!ImageFileRead((uint8_t *)&fileHdr, sizeof(ImageFileHdr)))
        Fatal(sys, "error reading image header");
        
    /* get the section count */
    count = fileHdr.sectionCount;
        
    /* allocate space for the image header */
    if (!(image = (ImageHdr *)AllocateFreeSpace(sys, sizeof(ImageHdr) + (count - 1) * sizeof(ImageSection))))
        Fatal(sys, "insufficient space for image header");
        
    /* initialize the image */
    image->mainCode = fileHdr.mainCode;
    image->sectionCount = count;
    if (!(image->sections[0].data = (uint8_t *)AllocateFreeSpace(sys, fileHdr.sections[0].size)))
        Fatal(sys, "insufficient space for %08x section", fileHdr.sections[0].base);
    memcpy(image->sections[0].data, &fileHdr, sizeof(ImageFileHdr));
    
    /* read the remaining section headers and first section data */
    if (!ImageFileRead(image->sections[0].data + sizeof(ImageFileHdr), fileHdr.sections[0].size - sizeof(ImageFileHdr)))
        Fatal(sys, "error reading %08x section", fileHdr.sections[0].base);

    /* initialize the first section header */
    src = ((ImageFileHdr *)image->sections[0].data)->sections;
    dst = image->sections;
    dst->fileSection = src;
    ++src; ++dst;
    
    /* initialize the headers and read the data for the remaining sections */
    for (; --count >= 1; ++src, ++dst) {
        dst->fileSection = src;
        if (!(dst->data = (uint8_t *)AllocateFreeSpace(sys, src->size)))
            Fatal(sys, "insufficient space for %08x section", src->base);
        if (!ImageFileRead(dst->data, src->size))
            Fatal(sys, "error reading %08x section", src->base);
    }
        
    /* return the image */
    return image;
}
