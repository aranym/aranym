#include <stdio.h>
#include <stdlib.h>


/*

A part of Fopen/Fcreate routine... (adding #100 to the file handle for in mem data)

L2334:
        move.l  a5,4(a4)                        ; 294D0004
        clr.l   (a4)                            ; 4294
        addq    #1,10(a5)                       ; 526D000A
        moveq   #100,d0                         ; 7064
        add     d4,d0                           ; D044
L2348:
        addq    #6,a7                           ; 5C4F
        movem.l (a7)+,d3-d4/a3-a5               ; 4CDF3818
        rts                                     ; 4E75
*/

unsigned char moveq100[] = {
	0x29, 0x4d, 0x00, 0x04,
	0x42, 0x94,
	0x52, 0x6d, 0x00, 0x0a,
	0x70, 0x64,
	0xd0, 0x44,
	0x5c, 0x4f,
	0x4c, 0xdf, 0x38, 0x18,
	0x4e, 0x75,
};


/*

A parts of Fread/Fdatime/Fseek/Fclose routines... (fh #100..#140)

L2356:
        movem.l d3-d4/a2-a4,-(a7)               ; 48E71838
        move    d0,d3                           ; 3600
        movea.l a0,a2                           ; 2448
        move.l  d1,d4                           ; 2801
        cmp     #100,d0                         ; B07C0064
        blt.s   L2378                           ; 6D06
        cmp     #140,d0                         ; B07C008C
        blt.s   L2392                           ; 6D0E
L2378:
        move.l  d4,d1                           ; 2204
        movea.l a2,a0                           ; 204A
        move    d3,d0                           ; 3003
        jsr     L173498                         ; 4EB90002A5BA
        bra.s   L2456                           ; 6040

L2392:
        sub     #100,d3                         ; 967C0064
        move    d3,d0                           ; 3003
        lsl     #3,d0                           ; E748
        lea     U223702,a3                      ; 47F9000369D6
        adda    d0,a3                           ; D6C0
*/

unsigned char cmp100140[] = {
	0xb0, 0x7c, 0x00, 0x64,
	0x6d, 0x06,
	0xb0, 0x7c, 0x00, 0x8c,
};

unsigned char sub100d3[] = {
	0x96, 0x7c, 0x00, 0x64,
	0x30, 0x03,
};



FILE *fh;
unsigned char patchBuffer[2];


int getByte( unsigned char *buff ) {
	return fread( (void*)buff, 1, 1, fh ) == -1 ? -1 : 1;
}


int findPart( unsigned char *buffer, size_t len )
{
	unsigned char charBuffer[2];
	int idx = 0;
	int total = 0;

	while ( getByte( charBuffer ) > 0 ) {
		if ( idx >= len )
			return 1;

		if ( *charBuffer != buffer[idx] )
			idx = 0;
		else
			idx++;

		if ( total++ > 2000 )
			return 0;
	}

	return 0;
}


int main() {
	char *result = NULL;

	fh = fopen ("PC.PRG", "rb+");
	if ( !fh ) {
		result = "PC.PRG not found in current directory";
		goto finalize;
	}

	if ( fseek( fh, 2000, SEEK_SET ) < 0 ) {
		result = "PC.PRG cannot seek to 2000 (too short file?)";
		goto finalize;
	}

	if ( ! findPart( moveq100, sizeof( moveq100 ) / sizeof( *moveq100 ) ) ) {
		result = "findPart(moveq100) failed";
		goto finalize;
	}

	*patchBuffer = 200;
	fseek( fh, -12, SEEK_CUR );
	fwrite( patchBuffer, 1, 1, fh );

	while ( 1 ) {
		if ( ! findPart( cmp100140, sizeof( cmp100140 ) / sizeof( *cmp100140 ) ) ) {
			result = "findPart(cmp100140) failed";
			break;
		}

		*patchBuffer = 200;
		fseek( fh, -8, SEEK_CUR );
		fwrite( patchBuffer, 1, 1, fh );

		*patchBuffer = 240;
		fseek( fh, 5, SEEK_CUR );
		fwrite( patchBuffer, 1, 1, fh );

		if ( ! findPart( sub100d3, sizeof( sub100d3 ) / sizeof( *sub100d3 ) ) ) {
			result = "findPart(sub100d3) failed";
			break;
		}

		*patchBuffer = 200;
		fseek( fh, -4, SEEK_CUR );
		fwrite( patchBuffer, 1, 1, fh );
	}

finalize:
	fclose( fh );

	if ( result == NULL ) {
		printf( "PureC was patched successfully.\n" );
	} else {
		printf( "Error patching PureC: %s\n", result );
	}

	puts("Press [Return] to quit.");
	getchar();

	return 0;
}
