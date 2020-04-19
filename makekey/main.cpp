#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>

typedef double						f64;
typedef float							f32;

typedef unsigned __int64	u_int64_t;
typedef unsigned __int64	u64;
typedef signed __int64		s64;

typedef unsigned int			u32;
typedef signed int				s32;

typedef unsigned short		u16;
typedef signed short			s16;

typedef unsigned char			u8;
typedef signed char				s8;

u8 SanDiskUnlockCommand [] = { 0x85, 0x06, 0x00, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x03, 0x0F, 0x00 };

inline u16 bs16( u16 word )
{
  return (word>>8) | (word<<8);;
}
void SwapStrings( char *Ident )
{
  for( u32 i=0; i<10; ++i )
  {
    *(u16*)(Ident+20+i*2) = bs16(*(u16*)(Ident+20+i*2));
  }

  for( u32 i=0; i<20; ++i )
  {
    *(u16*)(Ident+54+i*2) = bs16(*(u16*)(Ident+54+i*2));
  }
}
void MakeHDDKey( char *Ident, char *Key )
{
  memset( Key, 0, 0x20 );

  for( u32 i=0; i<16; ++i)
  {
    *(u16*)(Key+i*2)  = bs16( (*(u8*)(i+Ident+0x17) * *(u8*)(i+Ident+0x17+1)) - 1 );
  }
}

char ADTectA_Key [] = {"/-AaBbCcDdEeFfGgHhIiJjKkLlMmNnOoPpQqRrSsTtUuVvWwXxYyZz0123456789"};
char SanDiskA_Key [] = {"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789"};

signed int GetIndex( char *Key, char Char )
{
  for( u32 i=0; i<strlen(Key); ++i )
  {
    if( Key[i] == Char )
      return i;
  }
  return -1;
}
void MakeAdTecKey( u32 SerialLength, char *Serial, size_t KeyLength, char *Key )
{
  void *result  = nullptr;
  u8  ch        = 0;
  int value     = 0;
    
  result = memset( Key, 0, KeyLength );

  for( u32 i=0; i < SerialLength; i++ )
  {
    ch = GetIndex( ADTectA_Key, *Serial );
    *Serial++;

    if( ch < 0 )
        ch = 0;

    value = i % 3;
    if( value == 1 )
    {
      *(u8*)Key ^= (u8)(32 * ch);
      *Key++;
      *(u8*)Key ^= (u8)ch >> 3;
    }
    else if( value <= 1 )
    {
      if( !value )
      {
               result  = Key;
         *(u8*)result ^= ch;
      }
    }
    else if( value == 2 )
    {
      *(u8*)Key ^= 4 * (u8)ch;
      *Key++;
    }
  }
}
void MakeSanDiskKey( char *Serial, char *Key )
{
  char *ID    = Serial + 20;
  int counter = 0x13;
 
  memset( Key, 0, 5 );
 
  do
  {
    int result = GetIndex( SanDiskA_Key, *(u8*)ID++ );
    if( result >= 0 )
    {
      *(u8 *)(counter % 5 + Key) = ((*(u8 *)(counter % 5 + Key) + result) & 0xFF) + 1;
    }
    --counter;
  }
  while( counter >= 0 );
}
int main( int argc, char *argv[] )
{
  printf("makekey v0.1 by crediar\n");
	printf("Built: %s %s\n", __TIME__, __DATE__ );

  if( argc != 2 )
  {
    printf("usage:\n");
    printf("\tmakekey inqury.bin\n");
    return -1;
  }

  FILE * in = fopen( argv[1], "rb" );
  if( in == nullptr )
  {
    printf("Couldn't open %s\n", argv[1] );
    perror("");
    return -2;
  }

  fseek( in, 0, SEEK_END );
 
  u32 size = ftell(in);
  char *inq = new char[size];
 
  fseek( in, 0, SEEK_SET );
  fread( inq, 1, size, in );
  fclose(in);

  if( size != 512 )
  {
    printf("Inqury file must be 512 bytes.\n");
    return -3;
  }

  u32 Type = 0;
  u32 Swap = 0;
  
  // Check for SanDisk CF "SDCF"
  if( *(u32*)(inq+0x3E) == 0x43465344 )
  {
    Type = 2;
    Swap = 1;
  }
  else if( *(u32*)(inq+0x3E) == 0x46434453 )
  {
    Type = 2;
    Swap = 0;
  }
  // Check for AdTec CF "ADCF"
  else if( *(u32*)(inq+0x36) == 0x43464144 )
  {
    Type = 1;
    Swap = 1;
  }
  else if( *(u32*)(inq+0x36) == 0x46434441 )
  {
    Type = 1;
    Swap = 0;
  }
  // check for Maxtor HDD "Maxt"
  else if( *(u32*)(inq+0x37) == 0x7278744d )
  {
    Type = 0;
    Swap = 1;
  }
  else if( *(u32*)(inq+0x37) == 0x6f747861 )
  {
    Type = 0;
    Swap = 0;
  }  
  else
  {
    printf("Unknown inquiry\n");
    return -4;
  }

  if(Swap)
  {
    SwapStrings(inq);
  }

  switch(Type)
  {
    // Maxtor HDD key
    case 0:
    {
      char key[512];
      memset( key, 0, sizeof(key) );

      printf("Using:\n\"%.16s\"\n", inq+0x17 );

      MakeHDDKey( inq, key+2 );

      FILE *out = fopen("key.bin", "wb");
      if( out )
      {
        fwrite( key, 1, sizeof(key), out );
        fclose(out);
      }
      else
      {
        perror("Failed to write key.bin\n");
      }
    } break;
    case 1:
    {
      char key[512];
      memset( key, 0xFF, sizeof(key) );

      printf("Using:\n\"%.12s\"\n\"%.12s\"\n", inq+0x52, inq+0x1C );

      MakeAdTecKey( 12, inq+0x52, 8, key ); 
      MakeAdTecKey( 12, inq+0x1C, 8, key+8 );

      FILE *out = fopen("key.bin", "wb");
      if( out )
      {
        fwrite( key, 1, sizeof(key), out );
        fclose(out);
      }
      else
      {
        perror("Failed to write key.bin\n");
      }
    } break;
    case 2:
    {
      char key[5];
      memset( key, 0xAA, sizeof(key) );

      printf("Using:\n\"%.20s\"\n", inq+0x14 );
      MakeSanDiskKey( inq, key );
      
      SanDiskUnlockCommand[4]    = key[0];
      SanDiskUnlockCommand[6]    = key[1];
      SanDiskUnlockCommand[8]    = key[2];
      SanDiskUnlockCommand[10]   = key[3];
      SanDiskUnlockCommand[12]   = key[4];

      printf("Send the following bytes directly to the devices to unlock it:\n");
      for( u32 i=0; i<sizeof(SanDiskUnlockCommand); ++i )
        printf("%02X ", SanDiskUnlockCommand[i] );

    } break;
  }

  return 0;
}