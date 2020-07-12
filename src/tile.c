/*  
 *  CAELUM (2020)
 *
 *  Copyright Robert Saarloos, All rights reserved
 *
 *  Describes tiles and their properties, keeping a central record
 *  of common data of tile types in a hash table.
 *
 * */

#include <string.h>

#include "tile.h"
#include "util/log.h"
#include "util/list.h"
#include "util/htable.h"

#define TILEDATA_FILE_EXT ".abf"
#define TILE_IMAGE_SIZE   1024
#define TILE_TEXTURE_SIZE 32

static List * tileloadmanifest(const char *path);
static void tileloadtiledata(const char *path);
static void tilecalctileuvs(TileData *tile);

static Texture tiletex_albedo;
static Texture tiletex_normal;
static Texture tiletex_emission;
//static Texture *tiletex_specular;
static HTable *tiledata;

void
tiledatabaseinit(const char *manifest)
{
    uint i, pathlen;
    List *entries;
    char *pathcpy = malloc((strlen(manifest) + 1) * sizeof(char));
    char *chr, *file, *entry;

    texfind(&tiletex_albedo, "res/texture/tiles.png");
    texfind(&tiletex_emission, "res/texture/tiles_emission.png");
    //texfind(&tiletex_normal, "res/texture/tiles_normal.png");
    tiletex_albedo.imagesize = TILE_IMAGE_SIZE;
    tiletex_emission.imagesize = TILE_IMAGE_SIZE;
    //tiletex_normal.imagesize = TILE_IMAGE_SIZE;
    tiletex_albedo.texsize = TILE_TEXTURE_SIZE;
    tiletex_emission.texsize = TILE_TEXTURE_SIZE;
    //tiletex_normal.texsize = TILE_TEXTURE_SIZE;

    tiledata = htablecreate(htablestringhash, htablestringequal);
    entries = tileloadmanifest(manifest);

    /* Append directory path to manifest entries and
     * load tiles into hash table */
    strcpy(pathcpy, manifest);
    chr = strrchr(pathcpy, '/');

    if (chr) {
        *(chr + 1) = '\0';
    }
    chr = pathcpy;
    pathlen = strlen(pathcpy);

    for (i = 0; i < entries->size; i++) {
        entry = listat(entries, i);
        file = malloc((pathlen + strlen(entry) + 5) * sizeof(char));
        strcpy(file, pathcpy);
        strcat(file, entry);
        strcat(file, TILEDATA_FILE_EXT);
        tileloadtiledata(file);
        free(file);
    }
    listdealloc(entries);
    listdelete(entries);
    free(pathcpy);
}

void
tiledatabasestop()
{
    htabledealloc(tiledata);
    htabledelete(tiledata);
}

TileData *
tilegetdata(char *tilename)
{
    return htableget(tiledata, tilename);
}

List *
tileloadmanifest(const char *path)
{
    uint i, len;
    char *data = loadfile(path);
    char *entry, *start = data;
    List *manifest = listcreate();

    if (!data) {
        logprint(LOG_FATAL, "tile: Tile manifest could not be opened!");
        return NULL;
    }

    /* Copy manifest data into a list */
    for (i = 0; data[i] != '\0'; i++) {
        if (data[i] == '\n') {
            len = i - (start - data);
            entry = calloc(len + 1, sizeof(char));
            strncpy(entry, start, len);

            while (data[i] == ' ' || data[i] == '\t' || data[i] == '\n') {
                i++;
            }
            listpushback(manifest, entry);
            start = data + i;
        }
    }
    free(data);

    return manifest;
}

void
tileloadtiledata(const char *path)
{
    uint entry;
    char *data = loadfile(path);
    char *token = strtok(data, " (),\n");
    char *pathcpy = calloc(strlen(path) + 1, sizeof(char));
    TileData *result = malloc(sizeof(TileData));
    
    for (entry = 0; token; entry++) {
        switch (entry) {
        case 0:
            result->attr = atoi(token);
            break;
        case 1:
            result->material = atoi(token);
            break;
        case 2:
            result->toptex_pos[0] = atoi(token);
            break;
        case 3:
            result->toptex_pos[1] = atoi(token);
            break;
        case 4:
            result->sidetex_pos[0] = atoi(token);
            break;
        case 5:
            result->sidetex_pos[1] = atoi(token);
            break;
        case 6:
            result->subsidetex_pos[0] = atoi(token);
            break;
        case 7:
            result->subsidetex_pos[1] = atoi(token);
            break;
        default:
            logprint(LOG_WARN, "tile: Excess data in file: $s", path);
        }
        token = strtok(NULL, " (),\n");
    }
    strcpy(pathcpy, path);
    logprint(LOG_INFO, "Loaded tile data from $s", path);
    tilecalctileuvs(result);
    htableadd(tiledata, pathcpy, result);
    free(data);
}

void
tilecalctileuvs(TileData *tile)
{
    texget(&tiletex_albedo,
            tile->toptex_pos[0],
            tile->toptex_pos[1],
            tile->toptex);
    texget(&tiletex_albedo,
            tile->sidetex_pos[0],
            tile->sidetex_pos[1],
            tile->sidetex);
    texget(&tiletex_albedo,
            tile->subsidetex_pos[0],
            tile->subsidetex_pos[1],
            tile->subsidetex);
}

Texture *
tilegetalbedotex(void)
{
    return &tiletex_albedo;
}

Texture *
tilegetnormaltex(void)
{
    return &tiletex_normal;
}

Texture *
tilegetemissiontex(void)
{
    return &tiletex_emission;
}
