//=============================================================================
// I N C L U D E   F I L E S   A N D   F O R W A R D   D E C L A R A T I O N S

#include "isqrt.h"

#include "app_assert.h"
#include "errors.h"

//=============================================================================
// F U N C T I O N S   C O D E   S E C T I O N

//-----------------------------------------------------------------------------
//
int32_t ISqrt( int32_t n )
{
	assert( n >= 0, EC_NEGATIVE_VALUE,
	    "Can't extract the sqrt of negative number: %d\r\n", n );
   
	int32_t q = 1;
	int32_t r = 0;
    
    if (n >= ( 1 << 30 ) )
    {
        // Handling numbers >= 2**30
        // This has to be treated separately to avoid overflowing q and trapping
        // the function in an endless loop
        q = 1 << 30;
        n = n - r - q;
        r += q;
    }
    else
    {
        // This would be an endless loop if n was >= 2**30 because shifting q
        // 32 times will results in q == 0, and then we can't escape the loop
        // anymore
        while( q <= n )
        {
            q <<= 2;
        }
    }
    
    while( q > 1 )
    {
        int32_t t = 0;
        q >>= 2;
        t = n - r - q;
        r >>= 1;
        if( t >= 0 )
        {
            n = t;
            r += q;
        }
    }
	return r;
}


//-----------------------------------------------------------------------------
//
uint32_t ISqrt64(uint64_t n)
{
    uint32_t result = 0;
    uint64_t bit = (uint64_t)1 << 62; // Start with highest power of 4 <= 2^64

    while (bit > n)
        bit >>= 2;

    while (bit != 0)
    {
        if (n >= result + bit)
        {
            n -= result + bit;
            result = (result >> 1) + bit;
        }
        else
        {
            result >>= 1;
        }
        bit >>= 2;
    }

    return result;
}
