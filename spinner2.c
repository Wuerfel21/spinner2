#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>

#include "p2com.h"

typedef unsigned int uint;

typedef struct named_chunk {
    struct named_chunk *next;
    const char *name;
    const uint8_t *data;
    size_t dsize;
} named_chunk;

named_chunk *objlist = NULL, *datlist = NULL;
int debug_level = 0;
bool save_docs = false;

uint16_t unicode_for_oem[] = {
    0xF000,0xF001,0x2190,0x2192,0x2191,0x2193,0x25C0,0x25B6,0xF008,0xF009,0xF00A,0xF00B,0xF00C,0xF00D,0x2023,0x2022,
    0x0394,0x03C0,0x03A3,0x03A9,0x2248,0x221A,0xF016,0xF017,0xF018,0xF019,0xF01A,0xF01B,0xF01C,0xF01D,0xF01E,0xF01F,
    0x0020,0x0021,0x0022,0x0023,0x0024,0x0025,0x0026,0x0027,0x0028,0x0029,0x002A,0x002B,0x002C,0x002D,0x002E,0x002F,
    0x0030,0x0031,0x0032,0x0033,0x0034,0x0035,0x0036,0x0037,0x0038,0x0039,0x003A,0x003B,0x003C,0x003D,0x003E,0x003F,
    0x0040,0x0041,0x0042,0x0043,0x0044,0x0045,0x0046,0x0047,0x0048,0x0049,0x004A,0x004B,0x004C,0x004D,0x004E,0x004F,
    0x0050,0x0051,0x0052,0x0053,0x0054,0x0055,0x0056,0x0057,0x0058,0x0059,0x005A,0x005B,0x005C,0x005D,0x005E,0x005F,
    0x0060,0x0061,0x0062,0x0063,0x0064,0x0065,0x0066,0x0067,0x0068,0x0069,0x006A,0x006B,0x006C,0x006D,0x006E,0x006F,
    0x0070,0x0071,0x0072,0x0073,0x0074,0x0075,0x0076,0x0077,0x0078,0x0079,0x007A,0x007B,0x007C,0x007D,0x007E,0xF07F,
    0xF080,0xF081,0xF082,0xF083,0xF084,0xF085,0xF086,0xF087,0xF088,0xF089,0xF08A,0xF08B,0xF08C,0xF08D,0xF08E,0xF08F,
    0x2500,0x2502,0x253C,0x254B,0x2524,0x251C,0x2534,0x252C,0x252B,0x2523,0x253B,0x2533,0x2518,0x2514,0x2510,0x250C,
    0xF0A0,0x00A1,0xF0A2,0x00A3,0x20AC,0x00A5,0xF0A6,0xF0A7,0xF0A8,0xF0A9,0xF0AA,0xF0AB,0xF0AC,0xF0AD,0xF0AE,0xF0AF,
    0x00B0,0x00B1,0x00B2,0x00B3,0xF0B4,0x00B5,0xF0B6,0xF0B7,0xF0B8,0x00B9,0xF0BA,0xF0BB,0xF0BC,0xF0BD,0xF0BE,0x00BF,
    0x00C0,0x00C1,0x00C2,0x00C3,0x00C4,0x00C5,0x00C6,0x00C7,0x00C8,0x00C9,0x00CA,0x00CB,0x00CC,0x00CD,0x00CE,0x00CF,
    0x00D0,0x00D1,0x00D2,0x00D3,0x00D4,0x00D5,0x00D6,0x00D7,0x00D8,0x00D9,0x00DA,0x00DB,0x00DC,0x00DD,0x00DE,0x00DF,
    0x00E0,0x00E1,0x00E2,0x00E3,0x00E4,0x00E5,0x00E6,0x00E7,0x00E8,0x00E9,0x00EA,0x00EB,0x00EC,0x00ED,0x00EE,0x00EF,
    0x00F0,0x00F1,0x00F2,0x00F3,0x00F4,0x00F5,0x00F6,0x00F7,0x00F8,0x00F9,0x00FA,0x00FB,0x00FC,0x00FD,0x00FE,0x221E
};

char oem_for_unicode(int codepoint) {
    if ((codepoint >= 0x20 && codepoint <= 0x7E)||codepoint=='\r') return codepoint;
    if (codepoint >= 0xFFFF) return '?';
    for(uint i=0;i<256;i++) if ((int)unicode_for_oem[i] == codepoint) return i;
    return '?';
}

int read_codepoint(FILE *f,bool utf16) {
    if (utf16) {
        uint16_t chr;
        if (!fread(&chr,sizeof(uint16_t),1,f)) return EOF;
        if (chr >= 0xDC00 && chr <= 0xDFFF) {
            // Astral character
            uint16_t chr2;
            if (!fread(&chr2,sizeof(uint16_t),1,f)) return EOF;
            if (!(chr2 >= 0xD800 && chr2 <= 0xDBFF)) return '?'; // WTF?
            return (chr&0x3FF) + ((chr2&0x3FFF)<<10);
        }
        if (chr>= 0xD800 && chr <= 0xDBFF) return '?'; // WTF?
        return chr;
    } else {
        // UTF-8
        int chr = getc(f);
        if (chr==EOF) return EOF;
        if (chr<0x80) return chr;
        if ((chr&0xC0)==0xC0) return '?'; // WTF?
        if (chr>0xF7) return '?'; // Illegal
        if (chr>0xF0) {
            int chr2 = getc(f);
            if (chr2 == EOF) return EOF;
            if ((chr2&0xC0)!=0xC0) return '?';
            int chr3 = getc(f);
            if (chr3 == EOF) return EOF;
            if ((chr3&0xC0)!=0xC0) return '?';
            int chr4 = getc(f);
            if (chr4 == EOF) return EOF;
            if ((chr4&0xC0)!=0xC0) return '?';
            return ((chr&0xF)<<18) + ((chr2&0x3F)<<12) + ((chr3&0x3F)<<6) + (chr4&0x3F);
        }
        if (chr>0xE0) {
            int chr2 = getc(f);
            if (chr2 == EOF) return EOF;
            if ((chr2&0xC0)!=0xC0) return '?';
            int chr3 = getc(f);
            if (chr3 == EOF) return EOF;
            if ((chr2&0xC0)!=0xC0) return '?';
            return ((chr&0xF)<<12) + ((chr2&0x3F)<<6) + (chr3&0x3F);
        }
        if (chr>0xC0) {
            int chr2 = getc(f);
            if (chr2 == EOF) return EOF;
            if ((chr2&0xC0)!=0xC0) return '?';
            return ((chr&0xF)<<6) + (chr2&0x3F);
        }
        return '?';
    }
}

named_chunk *check_list(named_chunk *list, const char *oname) {
    for (named_chunk *obj=objlist;obj;obj=obj->next) {
        if (!strcmp(oname,obj->name)) {
            return obj;
        }
    }
    return NULL;
}

const char *copy_string(const char *str) {
    size_t len = strlen(str)+1;
    char *buffer = malloc(len);
    memcpy(buffer,str,len);
    return buffer;
}

named_chunk *add_to_list(named_chunk **list, const char *oname, const uint8_t *data,size_t dsize,bool do_copy) {
    named_chunk *listwalk = *list;
    if (listwalk) {
        while(listwalk->next)listwalk=listwalk->next;
        list = &listwalk->next;
    }
     
    named_chunk *newobj = malloc(sizeof(named_chunk));
    newobj->next = NULL;
    newobj->name = copy_string(oname);
    if (do_copy) {
        newobj->data = malloc(dsize); 
        memcpy((char*)newobj->data,data,dsize);
    } else {
        newobj->data = data;
    }
    newobj->dsize = dsize;
    *list = newobj;
    return newobj;
}

struct Spin2Compiler *compiler;



char *load_file(const char *name,bool binary,size_t *sizeout) {
    FILE *f = fopen(name,binary ? "rb" : "r");
    if (!f) {
        printf("Error %d opening %s, %s\n",errno,name,strerror(errno));
        if (sizeout) *sizeout = 0;
        return NULL;
    }
    uint16_t bom_maybe = 0;
    fread(&bom_maybe,sizeof(uint16_t),1,f);
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *buffer = malloc(size+1);
    if (binary) {
        fread(buffer,1,size,f);
        buffer[size] = 0;
        if (sizeout) *sizeout = size;
    } else {
        bool do_utf16 = bom_maybe == 0xFEFF;
        if (do_utf16) read_codepoint(f,true); // eat BOM
        char *pos = buffer;
        for (;;) {
            int c = read_codepoint(f,do_utf16);
            if (c==EOF) break;
            if (c=='\n') c='\r';
            //printf("%c - %04X\n",c,c);
            *pos++=oem_for_unicode(c);
        }
        if (sizeout) *sizeout = pos-buffer;
        *pos=0;
    }
    fclose(f);
    return buffer;
}

bool check_error() {
    if (compiler->error) {
        printf("Compiler-chan's infinite wisdom:\n\t\"%s\"",compiler->error_msg);
        if (compiler->source_start) {
            uint line = 1, column = 1;
            char *pos = compiler->source;
            while (pos < compiler->source+compiler->source_start) {
                column++;
                if (*pos++ == '\r') {
                    column = 1;
                    line++;
                }
            }
            printf("\n\tin %s @ line %d, column %d",compiler->obj_title,line,column);
            if (compiler->source_finish && compiler->source_start != compiler->source_finish) {
                printf(", %d chars",compiler->source_finish-compiler->source_start);
            }
        }
        printf("\n");
        if (debug_level) printf("list_length: %d, doc_length: %d\n",compiler->list_length,compiler->doc_length);
        if (debug_level) printf("list_limit: %d, doc_limit: %d\n",compiler->list_limit,compiler->doc_limit);
    }
    return compiler->error;
}

static void realloc_buffers() {
    if (compiler->list) free(compiler->list);
    compiler->list = calloc(0x1000000,1);
    compiler->list_length = 0;
    compiler->list_limit = 0xFFFFFF;
    if (compiler->doc) free(compiler->doc);
    compiler->doc = calloc(0x1000000,1);
    compiler->doc_length = 0;
    compiler->doc_limit = 0xFFFFFF;
}

void compileRecursively(const char *fname) {

    printf("Opening %s\n",fname);
    compiler->obj_stack_ptr++;
    char *code = load_file(fname,false,NULL);
    if (!code) exit(-3);
    retry:
    realloc_buffers();

    if (debug_level) printf("Trying to compile %s...\n",fname);
    compiler->source = code;
    strcpy(compiler->obj_title,fname);

    P2Compile1();
    if (debug_level) printf("Compile1... OK?\n");
    if (check_error()) exit(-1);

    if (debug_level) printf("File is PASM mode? %d\n",compiler->pasm_mode);

    bool compilerDirty = false;

    if (debug_level) printf("Got %d compiler->obj_files\n",compiler->obj_files.s);
    uint needed_files = compiler->obj_files.u;
    char needed_filenames[needed_files][256];
    memcpy(needed_filenames,compiler->obj_filenames,needed_files*256);

    for (uint i=0;i<needed_files;i++) {
        char *oname = &compiler->obj_filenames[i*256];
        // Normalize filename
        if (strlen(oname) <= 6 || strcmp(oname+strlen(oname)-6,".spin2")) strcat(oname,".spin2");
        if (debug_level) printf("Need \"%s\"...\n",oname);
        // Check if we got it already!
        named_chunk *obj = check_list(objlist,oname);
        if (!obj) {
            // Don't have it, compile it and try again
            compileRecursively(oname);
            compilerDirty = true;
        } else if (!compilerDirty) {
            compiler->obj_offsets[i] = (uint8_t*)obj->data;
            compiler->obj_lengths[i] = obj->dsize;
        }
    }
    if (compilerDirty) goto retry;
    if (debug_level) printf("Got %d compiler->dat_files\n",compiler->dat_files.s);
    for (int i=0;i<compiler->dat_files.s;i++) {
        char *dname = &compiler->dat_filenames[i*256];
        if (debug_level) printf("Need \"%s\"...\n",dname);
        // Check if we got it already!
        named_chunk *data = check_list(datlist,dname);
        if (!data) {
            printf("Loading FILE \"%s\"...\n",dname);
            size_t size;
            uint8_t *file = (uint8_t*)load_file(dname,true,&size);;
            if (!file) exit(-3);
            data = add_to_list(&datlist,dname,file,size,false);

        }
        compiler->dat_offsets[i] = (uint8_t*)data->data;
        compiler->dat_lengths[i] = data->dsize;
    }

    P2Compile2();
    if (debug_level) printf("Compile2... OK?\n");
    if (check_error()) exit(-1);

    add_to_list(&objlist,fname,compiler->obj,compiler->obj_ptr,true);
    compiler->obj_stack_ptr--;
    if (save_docs) {
        char docname[256];
        snprintf(docname,256,"%s.doc.txt",fname);
        FILE *docfile = fopen(docname,"wb");
        static const uint16_t bom = 0xFEFF;
        fwrite(&bom,2,1,docfile);
        for(uint i=0;i<compiler->doc_length;i++) {
            char c = compiler->doc[i];
            if (c=='\r') c='\n';
            uint16_t codepoint = unicode_for_oem[(uint8_t)c];
            fwrite(&codepoint,2,1,docfile);
        }
        fclose(docfile);
    }
}


int main(int argc, char** argv) {
    printf("Spinner 2 - Janky Spin2 command line compiler!\n");
    compiler = P2InitStruct();

    // Parse the command line
    char *outname = NULL, *inname = NULL;
    bool doEeprom = false;

    for (uint i=1;i<argc;i++) {
        if (!strcmp(argv[i],"-b")) {
            doEeprom = false;
        } else if (!strcmp(argv[i],"-e")) {
            doEeprom = true;
        } else if (!strcmp(argv[i],"-o")) {
            if (i+1==argc) {
                printf("-o syntax error\n");
                exit(-2);
            }
            outname = argv[++i];
        } else if (!strcmp(argv[i],"--verbose")) {
            debug_level++;
        } else if (!strcmp(argv[i],"--document")) {
            save_docs=true;
        } else if (argv[i][0] != '-') {
            if (inname) {
                printf("Can't handle multiple input files!!\n");
                exit(-2);
            }
            inname = argv[i];
        } else {
            printf("Unhandled option %s\n",argv[i]);
            exit(-2);
        }
    }
    if (!inname) {
        printf("No input file!\n");
        exit(-2);
    }
    if (!outname) {
        outname = malloc(strlen(inname)+8);
        char *inp = inname,*onp = outname;
        while (*inp && *inp != '.') *onp++ = *inp++;
        *onp = 0;
        strcat(outname,doEeprom?".eeprom":".binary");
    }

    compiler->obj_stack_ptr = -1;
    compileRecursively(inname);

    if (!compiler->pasm_mode) {
        P2InsertInterpreter();
        if (debug_level) printf("InsertInterpreter... OK?\n");
        check_error(compiler);
    }

    printf("Writing result to %s\n",outname);
    FILE *outf = fopen(outname,"wb");
    fwrite(compiler->obj,1,doEeprom ? SPIN2_OBJ_LIMIT :(compiler->size_obj+compiler->size_interpreter),outf);
    fclose(outf);
}