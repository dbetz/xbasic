/* db_image.c - compiled image functions
 *
 * Copyright (c) 2011 by David Michael Betz.  All rights reserved.
 *
 */

#include <string.h>
#include "db_compiler.h"
#include "db_vmdebug.h"

/* prototypes */
static void MakeTmpName(char *outfile, const char *infile, const char *sectionName);
static void ShowSectionInfo(ParseContext *c, ImageFileSection *section);

/* StartImage - start writing an image */
int StartImage(ParseContext *c, const char *name)
{
    VMUVALUE dataOffset = sizeof(ImageFileHdr) + (c->config->sectionCount - 1) * sizeof(ImageFileSection);
    Section *section;
    
    /* create temporary files for each section */
    for (section = c->config->sections; section != NULL; section = section->next) {
        if (section == c->textTarget) {
            if (!(section->fp = fopen(name, "w+b")))
                return FALSE;
            section->offset = dataOffset;
        }
        else {
            char tmpname[256]; // longer path name for GUI
            MakeTmpName(tmpname, name, section->name);
            if (!(section->fp = xbCreateTmpFile(c->sys, tmpname, "w+")))
                return FALSE;
            section->offset = 0;
        }
    }
    
    /* skip past the image header */
    xbSeekFile(c->textTarget->fp, dataOffset, SEEK_SET);
    
    /* return successfully */
    return TRUE;
}

/* BuildImage - build an image from the symbol table and objects already written to the image file */
int BuildImage(ParseContext *c, const char *name)
{
    VMUVALUE dataOffset = 0;
    ImageFileHdr fileHdr;
    VMUVALUE size, cnt;
    uint8_t buf[512];
    Section *section;
    
    /* initialize the image file header */
    memset(&fileHdr, 0, sizeof(fileHdr));
    memcpy(fileHdr.tag, IMAGE_TAG, sizeof(fileHdr.tag));
    fileHdr.version = IMAGE_VERSION;
    fileHdr.mainCode = c->mainCode;
    fileHdr.stackSize = c->stackSize * sizeof(VMVALUE);
    fileHdr.sectionCount = c->config->sectionCount;
    if (c->flags & COMPILER_INFO)
        xbInfo(c->sys, "%08x entry\n", fileHdr.mainCode);
    fileHdr.sections[0].base = c->textTarget->base;
    fileHdr.sections[0].offset = dataOffset;
    fileHdr.sections[0].size = c->textTarget->offset;
    if (c->flags & COMPILER_INFO)
        ShowSectionInfo(c, &fileHdr.sections[0]);
    /* write the image file header */
    xbSeekFile(c->textTarget->fp, 0, SEEK_SET);

    if (xbWriteFile(c->textTarget->fp, (uint8_t *)&fileHdr, sizeof(fileHdr)) != sizeof(fileHdr))
        ParseError(c, "error writing image file");
    dataOffset += fileHdr.sections[0].size;

    for (section = c->config->sections; section != NULL; section = section->next) {
        if (section != c->textTarget) {
            ImageFileSection fileSection;
            fileSection.base = section->base;
            fileSection.offset = dataOffset;
            fileSection.size = section->offset;
            if (c->flags & COMPILER_INFO)
                ShowSectionInfo(c, &fileSection);
            if (xbWriteFile(c->textTarget->fp, (uint8_t *)&fileSection, sizeof(fileSection)) != sizeof(fileSection))
                ParseError(c, "error writing image file");
            dataOffset += fileSection.size;
        }
    }

    /* write the remaining sections */
    xbSeekFile(c->textTarget->fp, 0, SEEK_END);
    for (section = c->config->sections; section != NULL; section = section->next) {
        if (section != c->textTarget && section->fp) {
            char tmpname[50];
            xbSeekFile(section->fp, 0, SEEK_SET);
            for (size = section->offset; size > 0; size -= cnt) {
                if ((cnt = size) > sizeof(buf))
                    cnt = sizeof(buf);
                if (xbReadFile(section->fp, buf, cnt) != cnt)
                    ParseError(c, "error reading data file");
                if (xbWriteFile(c->textTarget->fp, buf, cnt) != cnt)
                    ParseError(c, "error writing image file");
            }
            xbCloseFile(section->fp);
            MakeTmpName(tmpname, name, section->name);
            xbRemoveTmpFile(c->sys, tmpname);
        }
    }
    
    /* close the image file */
    xbCloseFile(c->textTarget->fp);
    
    return TRUE;
}

/* ShowSectionInfo - show information about a section */
static void ShowSectionInfo(ParseContext *c, ImageFileSection *section)
{
    xbInfo(c->sys, "%08x base\n", section->base);
    xbInfo(c->sys, "%08x file offset\n", section->offset);
    xbInfo(c->sys, "%08x size\n", section->size);
}

/* WriteSection - write a block of memory to a section file */
VMUVALUE WriteSection(ParseContext *c, Section *section, const uint8_t *buf, VMUVALUE size)
{
    VMUVALUE allocatedSize = ROUND_TO_WORDS(size);
    if (xbWriteFile(section->fp, (uint8_t *)buf, allocatedSize) != allocatedSize)
        ParseError(c, "insufficient %s section space", section->name);
    return allocatedSize;
}

/* ReadSectionOffset - read an offset in a section file */
VMUVALUE ReadSectionOffset(ParseContext *c, Section *section, VMUVALUE offset)
{
    uint8_t buf[sizeof(VMUVALUE)], *p;
    VMUVALUE value = 0;
    int cnt;

    xbSeekFile(section->fp, offset, SEEK_SET);
    if (xbReadFile(section->fp, buf, sizeof(VMUVALUE)) != sizeof(VMUVALUE))
        ParseError(c, "trouble reading offset in the %s section", section->name);

    for (p = buf, cnt = sizeof(VMVALUE); --cnt >= 0; )
        value = (value << 8) | *p++;

    return value;
}

/* WriteSectionOffset - overwrite an offset in a section file */
void WriteSectionOffset(ParseContext *c, Section *section, VMUVALUE offset, VMUVALUE value)
{
    uint8_t buf[sizeof(VMUVALUE)], *p;
    int cnt;
    
    for (p = buf + sizeof(VMVALUE), cnt = sizeof(VMVALUE); --cnt >= 0; ) {
        *--p = value;
        value >>= 8;
    }
    
    xbSeekFile(section->fp, offset, SEEK_SET);
    if (xbWriteFile(section->fp, buf, sizeof(VMUVALUE)) != sizeof(VMUVALUE))
        ParseError(c, "trouble updating offset in the %s section", section->name);
}

/* MakeTmpName - make the name of a temporary section data file */
static void MakeTmpName(char *outfile, const char *infile, const char *sectionName)
{
    char *end = strrchr(infile, '.');
    if (end && !strchr(end, '/') && !strchr(end, '\\')) {
        strncpy(outfile, infile, end - infile);
        outfile[end - infile] = '\0';
    }
    else
        strcpy(outfile, infile);
    strcat(outfile,"-");
    strcat(outfile,sectionName);
    strcat(outfile,".tmp");
}

