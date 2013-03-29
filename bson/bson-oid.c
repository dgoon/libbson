/*
 * Copyright 2013 10gen Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


#include <limits.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#if defined(__linux__)
#include <sys/syscall.h>
#elif defined(HAVE_WINDOWS)
#include <winsock2.h>
#endif

#include "bson-md5.h"
#include "bson-oid.h"


#ifndef HOST_NAME_MAX
#define HOST_NAME_MAX 256
#endif


typedef struct
{
   bson_context_flags_t   flags : 7;
   bson_bool_t            pidbe_once : 1;
   bson_uint8_t           pidbe[2];
   bson_uint8_t           md5[3];
   bson_uint32_t          seq32;
   bson_uint64_t          seq64;

   void (*oid_get_host)  (bson_oid_t     *oid,
                          bson_context_t *context);
   void (*oid_get_pid)   (bson_oid_t     *oid,
                          bson_context_t *context);
   void (*oid_get_seq32) (bson_oid_t     *oid,
                          bson_context_t *context);
   void (*oid_get_seq64) (bson_oid_t     *oid,
                          bson_context_t *context);
} bson_context_real_t;


BSON_STATIC_ASSERT(sizeof(bson_context_real_t) <= sizeof(bson_context_t));
BSON_STATIC_ASSERT(sizeof(bson_oid_t) == 12);


/*
 * This table contains an array of two character pairs for every possible
 * bson_uint8_t. It is used as a lookup table when encoding a bson_oid_t
 * to hex formatted ASCII. Performing two characters at a time roughly
 * reduces the number of operations by one-half.
 */
static const bson_uint16_t gHexCharPairs[] = {
#if BSON_BYTE_ORDER == BSON_BIG_ENDIAN
   12336, 12337, 12338, 12339, 12340, 12341, 12342, 12343, 12344, 12345,
   12385, 12386, 12387, 12388, 12389, 12390, 12592, 12593, 12594, 12595,
   12596, 12597, 12598, 12599, 12600, 12601, 12641, 12642, 12643, 12644,
   12645, 12646, 12848, 12849, 12850, 12851, 12852, 12853, 12854, 12855,
   12856, 12857, 12897, 12898, 12899, 12900, 12901, 12902, 13104, 13105,
   13106, 13107, 13108, 13109, 13110, 13111, 13112, 13113, 13153, 13154,
   13155, 13156, 13157, 13158, 13360, 13361, 13362, 13363, 13364, 13365,
   13366, 13367, 13368, 13369, 13409, 13410, 13411, 13412, 13413, 13414,
   13616, 13617, 13618, 13619, 13620, 13621, 13622, 13623, 13624, 13625,
   13665, 13666, 13667, 13668, 13669, 13670, 13872, 13873, 13874, 13875,
   13876, 13877, 13878, 13879, 13880, 13881, 13921, 13922, 13923, 13924,
   13925, 13926, 14128, 14129, 14130, 14131, 14132, 14133, 14134, 14135,
   14136, 14137, 14177, 14178, 14179, 14180, 14181, 14182, 14384, 14385,
   14386, 14387, 14388, 14389, 14390, 14391, 14392, 14393, 14433, 14434,
   14435, 14436, 14437, 14438, 14640, 14641, 14642, 14643, 14644, 14645,
   14646, 14647, 14648, 14649, 14689, 14690, 14691, 14692, 14693, 14694,
   24880, 24881, 24882, 24883, 24884, 24885, 24886, 24887, 24888, 24889,
   24929, 24930, 24931, 24932, 24933, 24934, 25136, 25137, 25138, 25139,
   25140, 25141, 25142, 25143, 25144, 25145, 25185, 25186, 25187, 25188,
   25189, 25190, 25392, 25393, 25394, 25395, 25396, 25397, 25398, 25399,
   25400, 25401, 25441, 25442, 25443, 25444, 25445, 25446, 25648, 25649,
   25650, 25651, 25652, 25653, 25654, 25655, 25656, 25657, 25697, 25698,
   25699, 25700, 25701, 25702, 25904, 25905, 25906, 25907, 25908, 25909,
   25910, 25911, 25912, 25913, 25953, 25954, 25955, 25956, 25957, 25958,
   26160, 26161, 26162, 26163, 26164, 26165, 26166, 26167, 26168, 26169,
   26209, 26210, 26211, 26212, 26213, 26214
#else
   12336, 12592, 12848, 13104, 13360, 13616, 13872, 14128, 14384, 14640,
   24880, 25136, 25392, 25648, 25904, 26160, 12337, 12593, 12849, 13105,
   13361, 13617, 13873, 14129, 14385, 14641, 24881, 25137, 25393, 25649,
   25905, 26161, 12338, 12594, 12850, 13106, 13362, 13618, 13874, 14130,
   14386, 14642, 24882, 25138, 25394, 25650, 25906, 26162, 12339, 12595,
   12851, 13107, 13363, 13619, 13875, 14131, 14387, 14643, 24883, 25139,
   25395, 25651, 25907, 26163, 12340, 12596, 12852, 13108, 13364, 13620,
   13876, 14132, 14388, 14644, 24884, 25140, 25396, 25652, 25908, 26164,
   12341, 12597, 12853, 13109, 13365, 13621, 13877, 14133, 14389, 14645,
   24885, 25141, 25397, 25653, 25909, 26165, 12342, 12598, 12854, 13110,
   13366, 13622, 13878, 14134, 14390, 14646, 24886, 25142, 25398, 25654,
   25910, 26166, 12343, 12599, 12855, 13111, 13367, 13623, 13879, 14135,
   14391, 14647, 24887, 25143, 25399, 25655, 25911, 26167, 12344, 12600,
   12856, 13112, 13368, 13624, 13880, 14136, 14392, 14648, 24888, 25144,
   25400, 25656, 25912, 26168, 12345, 12601, 12857, 13113, 13369, 13625,
   13881, 14137, 14393, 14649, 24889, 25145, 25401, 25657, 25913, 26169,
   12385, 12641, 12897, 13153, 13409, 13665, 13921, 14177, 14433, 14689,
   24929, 25185, 25441, 25697, 25953, 26209, 12386, 12642, 12898, 13154,
   13410, 13666, 13922, 14178, 14434, 14690, 24930, 25186, 25442, 25698,
   25954, 26210, 12387, 12643, 12899, 13155, 13411, 13667, 13923, 14179,
   14435, 14691, 24931, 25187, 25443, 25699, 25955, 26211, 12388, 12644,
   12900, 13156, 13412, 13668, 13924, 14180, 14436, 14692, 24932, 25188,
   25444, 25700, 25956, 26212, 12389, 12645, 12901, 13157, 13413, 13669,
   13925, 14181, 14437, 14693, 24933, 25189, 25445, 25701, 25957, 26213,
   12390, 12646, 12902, 13158, 13414, 13670, 13926, 14182, 14438, 14694,
   24934, 25190, 25446, 25702, 25958, 26214
#endif
};


static void
bson_oid_get_host (bson_oid_t     *oid,
                   bson_context_t *context)
{
   bson_uint8_t *bytes = (bson_uint8_t *)oid;
   bson_uint8_t digest[16];
   bson_md5_t md5;
   char hostname[HOST_NAME_MAX];

   gethostname(hostname, sizeof hostname);
   hostname[HOST_NAME_MAX-1] = '\0';

   bson_md5_init(&md5);
   bson_md5_append(&md5, (const bson_uint8_t *)hostname, strlen(hostname));
   bson_md5_finish(&md5, &digest[0]);

   bytes[4] = digest[0];
   bytes[5] = digest[1];
   bytes[6] = digest[2];
}


static void
bson_oid_get_host_cached (bson_oid_t     *oid,
                          bson_context_t *context)
{
   bson_context_real_t *real = (bson_context_real_t *)context;

   oid->bytes[4] = real->md5[0];
   oid->bytes[5] = real->md5[1];
   oid->bytes[6] = real->md5[2];
}


static void
bson_oid_get_pid (bson_oid_t     *oid,
                  bson_context_t *context)
{
   bson_uint16_t pid;
   bson_uint8_t *bytes = (bson_uint8_t *)&pid;

   pid = BSON_UINT16_TO_BE(getpid());
   oid->bytes[7] = bytes[0];
   oid->bytes[8] = bytes[1];
}


static void
bson_oid_get_pid_cached (bson_oid_t     *oid,
                         bson_context_t *context)
{
   bson_context_real_t *real = (bson_context_real_t *)context;
   oid->bytes[7] = real->pidbe[0];
   oid->bytes[8] = real->pidbe[1];
}


static void
bson_oid_get_seq32 (bson_oid_t     *oid,
                    bson_context_t *context)
{
   bson_context_real_t *real = (bson_context_real_t *)context;
   bson_uint32_t seq;

   seq = BSON_UINT32_TO_BE(real->seq32++);
   memcpy(&oid->bytes[9], ((bson_uint8_t *)&seq) + 1, 3);
}


static void
bson_oid_get_seq32_threadsafe (bson_oid_t     *oid,
                               bson_context_t *context)
{
   bson_context_real_t *real = (bson_context_real_t *)context;
   bson_uint32_t seq;

   seq = BSON_UINT32_TO_BE(__sync_fetch_and_add(&real->seq32, 1));
   memcpy(&oid->bytes[9], ((bson_uint8_t *)&seq) + 1, 3);
}


static void
bson_oid_get_seq64 (bson_oid_t     *oid,
                    bson_context_t *context)
{
   bson_context_real_t *real = (bson_context_real_t *)context;
   bson_uint64_t seq;

   seq = BSON_UINT64_TO_BE(real->seq64++);
   memcpy(&oid->bytes[4], &seq, 8);
}


static void
bson_oid_get_seq64_threadsafe (bson_oid_t     *oid,
                               bson_context_t *context)
{
   bson_context_real_t *real = (bson_context_real_t *)context;
   bson_uint64_t seq;

   seq = BSON_UINT64_TO_BE(__sync_fetch_and_add(&real->seq64, 1));
   memcpy(&oid->bytes[4], &seq, 8);
}


#if defined(__linux__)
static bson_uint16_t
gettid (void)
{
   return syscall(SYS_gettid);
}
#endif


void
bson_context_init (bson_context_t       *context,
                   bson_context_flags_t  flags)
{
   bson_context_real_t *real = (bson_context_real_t *)context;
   bson_uint16_t pid;
   bson_oid_t oid;

   memset(real, 0, sizeof *real);

   real->flags = flags;
   real->oid_get_host = bson_oid_get_host_cached;
   real->oid_get_pid  = bson_oid_get_pid_cached;
   real->oid_get_seq32 = bson_oid_get_seq32;
   real->oid_get_seq64 = bson_oid_get_seq64;
   real->seq32 = rand() % 0xFFFF;

   if ((flags & BSON_CONTEXT_DISABLE_HOST_CACHE)) {
      real->oid_get_host = bson_oid_get_host;
   } else {
      bson_oid_get_host(&oid, context);
      real->md5[0] = oid.bytes[4];
      real->md5[1] = oid.bytes[5];
      real->md5[2] = oid.bytes[6];
   }

   if ((flags & BSON_CONTEXT_THREAD_SAFE)) {
      real->oid_get_seq32 = bson_oid_get_seq32_threadsafe;
      real->oid_get_seq64 = bson_oid_get_seq64_threadsafe;
   }

   if ((flags & BSON_CONTEXT_DISABLE_PID_CACHE)) {
      real->oid_get_pid = bson_oid_get_pid;
   } else {
      pid = BSON_UINT16_TO_BE(getpid());
#if defined(__linux__)
      if ((flags & BSON_CONTEXT_USE_TASK_ID)) {
         bson_int32_t tid;
         if ((tid = gettid())) {
            pid = BSON_UINT16_TO_BE(tid);
         }
      }
#endif
      memcpy(&real->pidbe[0], &pid, 2);
   }
}


void
bson_oid_init_sequence (bson_oid_t     *oid,
                        bson_context_t *context)
{
   bson_context_real_t *real = (bson_context_real_t *)context;
   bson_uint32_t now = BSON_UINT32_TO_BE(time(NULL));

   memcpy(&oid->bytes[0], &now, 4);
   real->oid_get_seq64(oid, context);
}


void
bson_context_destroy (bson_context_t *context)
{
   memset(context, 0, sizeof *context);
}


void
bson_oid_init (bson_oid_t     *oid,
               bson_context_t *context)
{
   bson_context_real_t *real = (bson_context_real_t *)context;
   bson_uint32_t now = time(NULL);

   bson_return_if_fail(oid);
   bson_return_if_fail(context);

   now = BSON_UINT32_TO_BE(now);
   memcpy(&oid->bytes[0], &now, 4);

   real->oid_get_host(oid, context);
   real->oid_get_pid(oid, context);
   real->oid_get_seq32(oid, context);
}


void
bson_oid_init_from_data (bson_oid_t         *oid,
                         const bson_uint8_t *data)
{
   bson_return_if_fail(oid);
   bson_return_if_fail(data);
   memcpy(oid, data, 12);
}


void
bson_oid_init_from_string (bson_oid_t *oid,
                           const char *str)
{
   bson_return_if_fail(oid);
   bson_return_if_fail(str);
   bson_oid_init_from_string_unsafe(oid, str);
}


time_t
bson_oid_get_time_t (const bson_oid_t *oid)
{
   bson_return_val_if_fail(oid, 0);
   return bson_oid_get_time_t_unsafe(oid);
}


void
bson_oid_to_string (const bson_oid_t *oid,
                    char              str[static 25])
{
   bson_uint16_t *dst;
   bson_uint8_t *id = (bson_uint8_t *)oid;

   bson_return_if_fail(oid);
   bson_return_if_fail(str);

   dst = (bson_uint16_t *)(void *)str;
   dst[0] = gHexCharPairs[id[0]];
   dst[1] = gHexCharPairs[id[1]];
   dst[2] = gHexCharPairs[id[2]];
   dst[3] = gHexCharPairs[id[3]];
   dst[4] = gHexCharPairs[id[4]];
   dst[5] = gHexCharPairs[id[5]];
   dst[6] = gHexCharPairs[id[6]];
   dst[7] = gHexCharPairs[id[7]];
   dst[8] = gHexCharPairs[id[8]];
   dst[9] = gHexCharPairs[id[9]];
   dst[10] = gHexCharPairs[id[10]];
   dst[11] = gHexCharPairs[id[11]];
   str[24] = '\0';
}


bson_uint32_t
bson_oid_hash (const bson_oid_t *oid)
{
   bson_return_val_if_fail(oid, 5381);
   return bson_oid_hash_unsafe(oid);
}


int
bson_oid_compare (const bson_oid_t *oid1,
                  const bson_oid_t *oid2)
{
   bson_return_val_if_fail(oid1, 0);
   bson_return_val_if_fail(oid2, 0);
   return bson_oid_compare_unsafe(oid1, oid2);
}


bson_bool_t
bson_oid_equal (const bson_oid_t *oid1,
                const bson_oid_t *oid2)
{
   bson_return_val_if_fail(oid1, FALSE);
   bson_return_val_if_fail(oid2, FALSE);
   return bson_oid_equal_unsafe(oid1, oid2);
}


void
bson_oid_copy (const bson_oid_t *src,
               bson_oid_t       *dst)
{
   bson_return_if_fail(src);
   bson_return_if_fail(dst);
   bson_oid_copy_unsafe(src, dst);
}


bson_bool_t
bson_oid_is_valid (const char *str,
                   size_t      length)
{
   size_t i;

   bson_return_val_if_fail(str, FALSE);

   if (length == 24) {
      for (i = 0; i < length; i++) {
         switch (str[i]) {
         case '0': case '1': case '2': case '3': case '4': case '5': case '6':
         case '7': case '8': case '9': case 'a': case 'b': case 'c': case 'd':
         case 'e': case 'f':
            break;
         default:
            return FALSE;
         }
      }
      return TRUE;
   }

   return FALSE;
}