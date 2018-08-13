#ifndef HASHFUNC_H
#define HASHFUNC_H

class Hashfunc
{
  public:
static unsigned djb_hash ( void *key, int len )
 {
    unsigned char *p = (unsigned char*)key;
    unsigned h = 0;
    int i;

    for ( i = 0; i < len; i++ )
      h = 33 * h ^ p[i];

   return h;
 };

static unsigned sax_hash ( void *key, int len )
{
   unsigned char *p = (unsigned char*)key;
   unsigned h = 0;
   int i;

   for ( i = 0; i < len; i++ )
     h ^= ( h << 5 ) + ( h >> 2 ) + p[i];

  return h;
};

static unsigned fnv_hash ( void *key, int len )
  {
    unsigned char *p = (unsigned char*)key;
    unsigned h = 2166136261UL;
    int i;

    for ( i = 0; i < len; i++ )
      h = ( h * 16777619UL) ^ p[i];

   return h;
 };

static unsigned oat_hash ( void *key, int len )
 {
   unsigned char *p = (unsigned char*)key;
   unsigned h = 0;
   int i;

   for ( i = 0; i < len; i++ ) {
     h += p[i];
     h += ( h << 10 );
     h ^= ( h >> 6 );
   }

   h += ( h << 3 );
   h ^= ( h >> 11 );
   h += ( h << 15 );

   return h;
 };

};

#endif

