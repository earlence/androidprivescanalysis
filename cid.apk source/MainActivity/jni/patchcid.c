#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>





main()
{
    int fNo;
    int ret = 0;
    const char* path = "/dev/block/mmcblk0p5";
    char buf[ 0x200 ];

    const char orig[] =
    {
        0x48, 0x54, 0x43, 0x2D, 0x42, 0x4F, 0x41, 0x52, 0x44, 0x2D, 0x49, 0x4E, 0x46, 0x4F, 0x21, 0x40,
        0xB0
    };

    const char patched[] =
    {
        0x48, 0x54, 0x43, 0x2D, 0x42, 0x4F, 0x41, 0x52, 0x44, 0x2D, 0x49, 0x4E, 0x46, 0x4F, 0x21, 0x40,
        0xB0, 0x00, 0x00, 0x00, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31
    };


    fNo = open( path, O_RDWR );
    if( fNo < 0 )
    {
        perror( "open" );
        goto out;
    }

    // make sure we're at the beginning of the device
    if( lseek64( fNo, 0, SEEK_SET ) != 0 )
    {
        perror( "seek1" );
        goto out;
    }

    // read what is already there
    if( read( fNo, buf, sizeof( buf ) ) != sizeof( buf ) )
    {
        perror( "read" );
        goto out;
    }

    if( memcmp( buf, orig, sizeof( orig ) ) )
    {
        printf( "this doesnt look like a want to patch it\n" );
        goto out;
    }

    memcpy( buf, patched, sizeof( patched ) );

    // seek back and write it
    if( lseek64( fNo, 0, SEEK_SET ) != 0 )
    {
        perror( "seek2" );
        goto out;
    }

    if( write( fNo, buf, sizeof( buf ) ) != sizeof( buf ) )
    {
        perror( "write" );
        goto out;
    }

    // success
    ret = 1;

out:
    if( fNo >= 0 )
    {
        close( fNo );
    }
    return ret;
}
