#include <lua.h>                               
#include <lauxlib.h>                           
#include <lualib.h>                            

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fastlz.h"

#define large_malloc(s) (malloc(((int)(s/4096)+1)*4096))


static char temp_buf[4096];

#if !defined(LUA_VERSION_NUM) || LUA_VERSION_NUM < 502
/* Compatibility for Lua 5.1.
 *
 * luaL_setfuncs() is used to create a module table where the functions have
 * json_config_t as their first upvalue. Code borrowed from Lua 5.2 source. */
static void luaL_setfuncs (lua_State *l, const luaL_Reg *reg, int nup)
{
    int i;

    luaL_checkstack(l, nup, "too many upvalues");
    for (; reg->name != NULL; reg++) {  /* fill the table with given functions */
        for (i = 0; i < nup; i++)  /* copy upvalues to the top */
            lua_pushvalue(l, -nup);
        lua_pushcclosure(l, reg->func, nup);  /* closure with those upvalues */
        lua_setfield(l, -(nup + 2), reg->name);
    }
    lua_pop(l, nup);  /* remove upvalues */
}
#endif


static int lua_f_fastlz_compress ( lua_State *L )
{
    if ( !lua_isstring ( L, 1 ) ) {
        lua_pushnil ( L );
        return 1;
    }

    size_t vlen = 0;
    const char *value = lua_tolstring ( L, 1, &vlen );

    if ( vlen < 1 || vlen > 1 * 1024 * 1024 ) { /// Max 1Mb !!!
        lua_pushnil ( L );
        return 1;
    }

    char *dst = ( char * ) &temp_buf;

    if ( vlen + 20 > 4096 ) {
        dst = large_malloc ( vlen + 20 );
    }

    if ( dst == NULL ) {
        lua_pushnil ( L );
        lua_pushstring ( L, "not enough memory!" );
        return 2;
    }

    const unsigned int size_header =  ( vlen );
//    const unsigned int size_header = htonl ( vlen );
    memcpy ( dst, &size_header, sizeof ( unsigned int ) );

    int dlen = fastlz_compress ( value, vlen, dst + sizeof ( unsigned int ) );

    if ( dst ) {
        lua_pushlstring ( L, dst, dlen + sizeof ( unsigned int ) );

        if ( dst != ( char * ) &temp_buf ) {
            free ( dst );
        }

        return 1;

    } else {
        lua_pushnil ( L );
        return 1;
    }
}

static int lua_f_fastlz_decompress ( lua_State *L )
{
    if ( !lua_isstring ( L, 1 ) ) {
        lua_pushnil ( L );
        return 1;
    }

    size_t vlen = 0;
    const char *value = lua_tolstring ( L, 1, &vlen );
    printf("=========================\n");
    printf("vlen %d\n", vlen);
        
    if ( vlen < 1 ) {
        lua_pushnil ( L );
        return 1;
    }

    unsigned int value_len = 0;
//    value_len = *value;
    memcpy ( &value_len, value, sizeof ( unsigned int ) );
    
    printf("value_len %d\n", value_len);

//    if (ntohl ( value_len ) > 0 && vlen < 1000) {
//	value_len = ntohl ( value_len );
//    }
    printf("value_len %d\n", value_len);
    if ( value_len > 1024 * 1024 + 20 ) {
        lua_pushnil ( L );
        lua_pushstring ( L, "not enough memory! too big value" );
        return 2;
    }

    char *dst = ( char * ) &temp_buf;

    if ( value_len + 20 > 4096 ) {
        dst = ( unsigned char * ) large_malloc ( value_len + 20 );
    }

    if ( dst == NULL ) {
        lua_pushnil ( L );
        lua_pushstring ( L, "not enough memory! cant malloc" );
        return 2;
    }

    int dlen = fastlz_decompress ( value + sizeof ( unsigned int ),
                                   vlen - sizeof ( unsigned int ), dst, value_len + 20 );

    if ( dst ) {
        lua_pushlstring ( L, dst, value_len );

        if ( dst != ( char * ) &temp_buf ) {
            free ( dst );
        }

        return 1;

    } else {
        lua_pushnil ( L );
        return 1;
    }
}


static const struct luaL_Reg reg[] = {
    {"decompress", lua_f_fastlz_decompress},
    {"compress", lua_f_fastlz_compress},
    {NULL, NULL}
};


int luaopen_fastlz ( lua_State *L )
{
    luaL_register(L, "fastlz", reg);
    return 0;
}
