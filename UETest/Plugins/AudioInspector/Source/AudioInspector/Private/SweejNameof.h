// Copyright SweejTech Ltd. All Rights Reserved.

#ifndef SWEEJ_NAMEOF
#define SWEEJ_NAMEOF( symbol ) #symbol
#endif

#ifndef SWEEJ_FNAMEOF
#define SWEEJ_FNAMEOF( symbol ) FName( SWEEJ_NAMEOF( symbol ) )
#endif