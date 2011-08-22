/* db_vmimage.c - compiled image functions
 *
 * Copyright (c) 2011 by David Michael Betz.  All rights reserved.
 *
 */

#include <stdarg.h>
#include <string.h>
#include "db_vmimage.h"
#include "db_vmdebug.h"
#include "db_vm.h"

/* LoadImage - load an image from a file */
ImageHdr *LoadImage(System *sys, const char *name)
{
    ImageFileHdr fileHdr;
    ImageFileSection *src;
    ImageHdr *image;
    ImageSection *dst;
    size_t size;
    int count;
    FILE *fp;

    if (!(fp = fopen(name, "rb")))
        Fatal(sys, "can't open '%s'", name);
    
    /* read the image file header */
    if (fread((uint8_t *)&fileHdr, 1, sizeof(ImageFileHdr), fp) != sizeof(ImageFileHdr))
        Fatal(sys, "error reading image header");
        
    /* get the section count */
    count = fileHdr.sectionCount;
        
    /* allocate space for the image header */
    if (!(image = (ImageHdr *)xbGlobalAlloc(sys, sizeof(ImageHdr) + (count - 1) * sizeof(ImageSection))))
        Fatal(sys, "insufficient space for image header");
        
    /* initialize the image */
    image->mainCode = fileHdr.mainCode;
    image->stackSize = fileHdr.stackSize;
    image->sectionCount = count;
    if (!(image->sections[0].data = (uint8_t *)xbGlobalAlloc(sys, fileHdr.sections[0].size)))
        Fatal(sys, "insufficient space for %08x section", fileHdr.sections[0].base);
    memcpy(image->sections[0].data, &fileHdr, sizeof(ImageFileHdr));
    
    /* read the remaining section headers and first section data */
    size = fileHdr.sections[0].size - sizeof(ImageFileHdr);
    if (fread(image->sections[0].data + sizeof(ImageFileHdr), 1, size, fp) != size)
        Fatal(sys, "error reading %08x section", fileHdr.sections[0].base);

    /* initialize the first section header */
    src = ((ImageFileHdr *)image->sections[0].data)->sections;
    dst = image->sections;
    dst->fileSection = src;
    ++src; ++dst;
    
    /* initialize the headers and read the data for the remaining sections */
    for (; --count >= 1; ++src, ++dst) {
        dst->fileSection = src;
        if (!(dst->data = (uint8_t *)xbGlobalAlloc(sys, src->size)))
            Fatal(sys, "insufficient space for %08x section", src->base);
        if (fread(dst->data, 1, src->size, fp) != src->size)
            Fatal(sys, "error reading %08x section", src->base);
    }
    
    fclose(fp);
    
    /* return the image */
    return image;
}

