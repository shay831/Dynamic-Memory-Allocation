#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>

#include "cache.h"
#include "cache_stats.h"
#include "print_helpers.h"

cache_t *make_cache(int capacity, int block_size, int assoc, enum protocol_t protocol, bool lru_on_invalidate_f){
  cache_t *cache = malloc(sizeof(cache_t));
  cache->stats = make_cache_stats();
  cache->capacity = capacity;      // in Bytes
  cache->block_size = block_size;  // in Bytes
  cache->assoc = assoc;            // 1, 2, 3... etc.

  // FIX THIS CODE!
  // first, correctly set these 5 variables. THEY ARE ALL WRONG
  // note: you may find math.h's log2 function useful
  cache->n_cache_line = (capacity/block_size);
  cache->n_set = (capacity/(block_size*assoc));
  cache->n_offset_bit = log2(block_size);
  cache->n_index_bit = log2(capacity/(block_size*assoc));
  cache->n_tag_bit = ADDRESS_SIZE - log2(block_size) - log2(capacity/(block_size*assoc));

  // next create the cache lines and the array of LRU bits
  // - malloc an array with n_rows
  // - for each element in the array, malloc another array with n_col
  // FIX THIS CODE!

  cache->lines = malloc(cache->n_set*sizeof(cache_line_t));
  for (int i = 0; i < cache->n_set; i++){
    cache->lines[i] = malloc(assoc * sizeof(cache_line_t));
  }
  cache->lru_way = malloc(cache->n_set * sizeof(int));

  // initializes cache tags to 0, dirty bits to false,
  // state to INVALID, and LRU bits to 0
  // FIX THIS CODE!
  for (int i = 0; i < cache->n_set; i++) {
    for (int j = 0; j < assoc; j++) {
      (cache->lines[i][j]).tag = 0;
      (cache->lines[i][j]).dirty_f = false;
      (cache->lines[i][j]).state = INVALID;
    }
    cache->lru_way[i] = 0;
  }
  cache->protocol = protocol;
  cache->lru_on_invalidate_f = lru_on_invalidate_f;
  return cache;
}

/* Given a configured cache, returns the tag portion of the given address.
 *
 * Example: a cache with 4 bits each in tag, index, offset
 * in binary -- get_cache_tag(0b111101010001) returns 0b1111
 * in decimal -- get_cache_tag(3921) returns 15 
 */
unsigned long get_cache_tag(cache_t *cache, unsigned long addr) {
  // FIX THIS CODE!
  // Binary form: 10000000000000000000000000000000
  // * int negative_number = 0x80000000;
  // Shift negative_number to right by # of tag bits - 1
  // C DOES SRA FOR INTEGERS
  // * negative_number = negative_number >> (cache->n_tag_bit - 1);
  // Compare numbers
  // * unsigned int result = negative_number & addr;
  // Shift right to put tag in LSBs
  // * result = result >> (32 - cache->n_index_bit - cache->n_offset_bit);
  // * return result;
  int tag_bits = cache->n_tag_bit;
  unsigned mask = (1 << tag_bits) - 1;
  int cache_tag = ADDRESS_SIZE - tag_bits;
  return ((addr >> cache_tag) & mask);
}

/* Given a configured cache, returns the index portion of the given address.
 *
 * Example: a cache with 4 bits each in tag, index, offset
 * in binary -- get_cache_index(0b111101010001) returns 0b0101
 * in decimal -- get_cache_index(3921) returns 5
 */
unsigned long get_cache_index(cache_t *cache, unsigned long addr) {
  // FIX THIS CODE!
  // Getting rid of offset bits
  // Hex form: 0xFFFFFFFF
  // * int negative_number = 0xFFFFFFFF;
  // Shift left by # of index bits
  // * negative_number = negative_number << (cache->n_offset_bit);
  // Compare numbers to get rid off offset bits
  // * unsigned int result = negative_number & addr;
  // Getting rid of tag bits
  // Binary form: 10000000000000000000000000000000
  // * negative_number = 0x80000000;
  // Shift right by # of tag bits - 1
  // * negative_number = negative_number >> (cache->n_tag_bit - 1);
  // * negative_number = ~negative_number;
  // Compare numbers to get rid of tag bits
  // * result = negative_number & result;
  // Shift right to return valid number
  // * return (result >> (cache->n_offset_bit));
  int offset_bits = cache->n_offset_bit;
  int index_bits = cache->n_index_bit;
  unsigned mask = (1 << index_bits) - 1;
  return ((addr >> offset_bits) & mask);
}

/* Given a configured cache, returns the given address with the offset bits zeroed out.
 *
 * Example: a cache with 4 bits each in tag, index, offset
 * in binary -- get_cache_block_addr(0b111101010001) returns 0b111101010000
 * in decimal -- get_cache_block_addr(3921) returns 3920
 */
unsigned long get_cache_block_addr(cache_t *cache, unsigned long addr) {
  // FIX THIS CODE!
  // Hex form: 0xFFFFFFFF
  // * int negative_number = 0xFFFFFFFF;
  // Shift left
  // * negative_number = negative_number << (cache->n_offset_bit);
  // Compare numbers
  // * unsigned int result = negative_number & addr;
  // * return result;
  int offset_bits = cache->n_offset_bit;
  int imm = addr >> offset_bits;
  return (imm << offset_bits);
}

/* this method takes a cache, an address, and an action
 * it proceses the cache access. functionality in no particular order: 
 *   - look up the address in the cache, determine if hit or miss
 *   - update the LRU_way, cacheTags, state, dirty flags if necessary
 *   - update the cache statistics (call update_stats)
 * return true if there was a hit, false if there was a miss
 * Use the "get" helper functions above. They make your life easier.
 * 
 * 
 */

bool direct_map_help(cache_t *cache, unsigned long addr, enum action_t action) {
  unsigned long addr_tag = get_cache_tag(cache, addr);
  unsigned long index = get_cache_index(cache, addr);

  if ((cache->lines[index][0].tag == addr_tag) && (cache->lines[index][0].state == VALID)) {
    // (cache->lines[index][0]).state = VALID;
    // fprintf(stderr, "is true");
    //printf("hit");
    update_stats(cache->stats, true, false, false, action);
    return true;
  } else {
    //printf("miss");
    // fprintf(stderr, "is false");
    (cache->lines[index][0]).state = VALID;
    (cache->lines[index][0]).tag = addr_tag;
    update_stats(cache->stats, false, false, false, action);
    return false;
  }
}

bool set_assoc_help(cache_t *cache, unsigned long addr, enum action_t action) {
  unsigned long addr_tag = get_cache_tag(cache, addr);
  unsigned long index = get_cache_index(cache, addr);
  int lru = cache->lru_way[index];
  bool hit = false;
  for (int i = 0; i < cache->assoc; i++)
  //iterate through associativity of cache
  {
    if (cache->lines[index][i].tag == addr_tag)
    //check if there is a tag match
    {
      if (cache->lines[index][i].state == VALID)
      //check if stats is valid
      hit = true;
      {
        //if state valid, counts as a hit, increment lru
        if (action == STORE)
        {
          //if action is store, there is hit, update dirty to true (this entry is newer than main memory)
          update_stats(cache->stats, true, false, false, action);
          cache->lines[index][i].dirty_f=true;
          log_set(index);
          log_way(i);
          cache->lru_way[index] = (i + 1) % (cache->assoc); // increment lru
        }
        if (action == LOAD)
        {
          //do not update ditry bit, still a hit
          update_stats(cache->stats, true, false, false, action);
          log_set(index);
          log_way(i);
          cache->lru_way[index] = (i + 1) % (cache->assoc); // increment lru
        }
        // if ((action==LD_MISS) && (cache->lines[index][i].dirty_f=true)){
        //   update_stats(cache->stats, false, true, false, action);
        //   cache->lines[index][i].dirty_f=false;
        //   cache->lines[index][i].state = INVALID;
        //   return true;
        // }
        // if ((action==ST_MISS) && (cache->lines[index][i].dirty_f=true)){
        //   update_stats(cache->stats, false, true, false, action);
        //   cache->lines[index][i].dirty_f=false;
        //   cache->lines[index][i].state = INVALID;
        //   return true;
        // }
        // if (action == LD_MISS || action == ST_MISS){
        //   if (cache->lines[index][i].dirty_f==true){
        //     update_stats(cache->stats, true, true, false, action);
        //     cache->lines[index][i].dirty_f=false;
        //     cache->lines[index][i].state = INVALID;
        //     return true;
        //   }
        //   else {
        //     update_stats(cache->stats, true, false, false, action);
        //     cache->lines[index][i].state = INVALID;
        //     return true;
        //   }
        // }
      }
    }
  }
  //implies miss
  //know this is a miss now, first loop handles hits
  if ((action == LOAD || action == STORE) && (!hit))
  {
    //set state to valid
    //printf("validated");
    cache->lines[index][lru].state = VALID;
    //set tag of lru to address tag
    cache->lines[index][lru].tag = addr_tag;
    if (action == STORE)
    {
      //check if this entry is dirty
      if (cache->lines[index][lru].dirty_f == true)
      {
        //if so, must write back 
        update_stats(cache->stats, false, true, false, action);
        cache->lines[index][lru].dirty_f = false;
      }
      else
      {
        //if not dirty, do not write back
        update_stats(cache->stats, false, false, false, action);
      }
      //set dirty bit to dirty
      cache->lines[index][lru].dirty_f = true;
    }
    if (action == LOAD)
    {
      if (cache->lines[index][lru].dirty_f == true)
      {
        //if dirty, must write back
        update_stats(cache->stats, false, true, false, action);
        cache->lines[index][lru].dirty_f = false;
      }
      else
      {
        //do not write back
        update_stats(cache->stats, false, false, false, action);
      }
    }
    //increment LRU index
    cache->lru_way[index] = (lru + 1) % (cache->assoc);
  }
  // if ((action == LD_MISS || action == ST_MISS)) {
  //   update_stats(cache->stats, false,false,false,action);
  // }
  return hit;
}
bool access_help(cache_t *cache, unsigned long addr, enum action_t action) {
  unsigned long index = get_cache_index(cache,addr);
  unsigned long tag = get_cache_tag(cache,addr);
  unsigned int way = cache->lru_way[index];
  bool hit = false;
  bool write_back = false;

  for (int i = 0; i < cache->assoc; i++) {
    if (cache->lines[index][i].tag==tag && cache->lines[index][i].state==VALID){
      hit = true;
      if (action == STORE) {
        cache->lines[index][i].dirty_f=true;
        log_set(index);
        log_way(i);
        cache->lru_way[index] = (i+1) % cache->assoc;
        update_stats(cache->stats, hit, write_back, false, action);
        return hit;
      }
      if (action == LOAD) {
        log_set(index);
        log_way(i);
        cache->lru_way[index] = (i+1) % cache->assoc;
        update_stats(cache->stats, hit, write_back, false, action);
        return hit;
      }
      // if (action == LD_MISS || action == ST_MISS){
      //   if (cache->lines[index][i].dirty_f==true){
      //     //wb
      //     update_stats(cache->stats, hit, true, false, action);
      //     cache->lines[index][i].dirty_f=false;
      //     cache->lines[index][i].state = INVALID;
      //     }
      //     else {
      //     // do not wb
      //       update_stats(cache->stats, hit, false, false, action);
      //       cache->lines[index][i].state = INVALID;
      //     }
      // }
    }
  }
  if (!hit){
    if (action == LOAD || action == STORE) {
      if (action == STORE){
        cache->lines[index][way].state = VALID;
        cache->lines[index][way].tag = tag;
        if (cache->lines[index][way].dirty_f==true) {
          //if dirty, must write back
          write_back = true;
          cache->lines[index][way].dirty_f = false;
          update_stats(cache->stats, hit, write_back, false, action);
          return hit;
        }
        cache->lines[index][way].dirty_f = false;
        cache->lru_way[index] = (way+1)%cache->assoc;
        update_stats(cache->stats, hit, write_back, false, action);
        return hit;
      }
      if (action == LOAD) {
        cache->lines[index][way].state = VALID;
        cache->lines[index][way].tag = tag;
         if (cache->lines[index][way].dirty_f==true) {
           write_back = true;
           cache->lines[index][way].dirty_f = true;
           update_stats(cache->stats, hit, write_back, false, action);
           return hit;
         }
        cache->lru_way[index] = (way+1)%(cache->assoc); 
        update_stats(cache->stats, hit, write_back, false, action);
        return hit;
      }
    }
    //write back 
    
  //   if (action == LD_MISS || action == ST_MISS) {
  //   update_stats(cache->stats, false,false,false,action);
  // }
  }
  //update_stats(cache->stats, hit, write_back, false, action);
  //return hit;
  return hit;
}

bool another_helper(cache_t *cache, unsigned long addr, enum action_t action) {
  unsigned long addr_tag = get_cache_tag(cache,addr);
  unsigned long index = get_cache_index(cache,addr);
  int lru = cache->lru_way[index];
  bool hit = false;
  bool write_back = false;
  for (int i = 0; i < cache->assoc; i++){
    if (cache->lines[index][i].tag==addr_tag && cache->lines[index][i].state==VALID){
      hit = true;
      if (action == STORE){
        // dont write back, update cache
        update_stats(cache->stats, true, false, false, action);
        cache->lru_way[index]=(i+1)%(cache->assoc);
        cache->lines[index][i].dirty_f=true;
      }
      if (action == LOAD) {
        update_stats(cache->stats, true, false, false, action);
      }
    }
  }
  if ((action==LOAD || action == STORE) && (!hit)){
    if (action==STORE){
      cache->lines[index][lru].state=VALID;
      cache->lines[index][lru].tag=addr_tag;
      if (cache->lines[index][lru].dirty_f==true){
        write_back = true;
        update_stats(cache->stats, false, true, false, action);
        cache->lines[index][lru].dirty_f = false;
      }
      else {
        update_stats(cache->stats, false, false, false, action); 
      }
      cache->lines[index][lru].dirty_f = true;
    }
    if (action == LOAD){
      cache->lines[index][lru].state=VALID;
      cache->lines[index][lru].tag=addr_tag;
      if (cache->lines[index][lru].dirty_f==true){
        write_back = true;
        update_stats(cache->stats, false, true, false, action);
        cache->lines[index][lru].dirty_f = false;
      }
      else {
        update_stats(cache->stats, false, false, false, action);
        cache->lines[index][lru].dirty_f = false;
      }
    }
    cache ->lru_way[index] = (lru + 1) % (cache->assoc);
  }
  return hit;
}
bool access_cache(cache_t *cache, unsigned long addr, enum action_t action) {
  // fprintf(stderr, "Start of access_cache!");
  // FIX THIS CODE!
  //unsigned long addr_tag = get_cache_tag(cache, addr);
  // printf("%lu\n", addr_tag);
  //unsigned long index = get_cache_index(cache, addr);
  // printf("%lu\n", index);
  //unsigned long cache_block = get_cache_block_addr(cache, addr);
  // printf("%lu\n", index);

  //index into cache
  // cache_line_t * line = cache->lines[index];
  // fprintf(stderr, "index: %lu\n", index);
  set_assoc_help(cache,addr,action);
  //access_help(cache,addr,action);
  //another_helper(cache,addr,action);
  // return false;
}
