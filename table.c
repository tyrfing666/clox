#include <stdlib.h>
#include <string.h>

#include "memory.h"
#include "object.h"
#include "table.h"
#include "value.h"

// load factor for hash tables.
// This is just a good first cut - actual factor should have some runtime data behind it.
#define TABLE_MAX_LOAD 0.75

// Initialize a new hash table.
// Arguments: table - the table to initialize.
void initTable(Table* table) {
    table->count = 0;
    table->capacity = 0;
    table-> entries = NULL;
}

// Free up a table.
// Arguments: table - the table to free.
void freeTable(Table* table) {
    FREE_ARRAY(Entry, table->entries, table->capacity);
    initTable( table);
}

// figure out where a key belongs in the table. Used for both finding keys and searching for where
// to put inserts.
// Arguments:
//  entries - array of table entries (we're using this instead of the table as it's what we need).
//  capacity - current capacity of the table.
//  key - the key to look for.
// Returns: pointer to where the key belongs. If it's NULL then either the key isn't in the table, or 
// it's where we should put this key. If it's not NULL, then either we've found it, or this is the 
// entry we're replacing.
static Entry* findEntry(Entry* entries, int capacity, ObjString* key) {
    uint32_t index = key->hash % capacity;
    Entry* tombstone = NULL;

    for (;;) {
        Entry* entry = &entries[index];
        if (entry->key == NULL) {
            if (IS_NIL(entry->value)) {
                // Empty entry.
                return tombstone != NULL ? tombstone : entry;
            } else {
                // We found a tombstone.
                if (tombstone == NULL) {
                    tombstone = entry;
                }
            }
        } else if (entry->key == key) {
            // We found the key.
            return entry;
        }
        index = (index + 1) % capacity;
    }
}

// Find an entry in the table.
// Arguments:
//  table - the table where to look.
//  key - the key to look for.
//  value - if we found it then this will hold the value.
// Returns: true if found, false if not.
// Note: value needs have allocated storage and will only change if this returns true.
bool tableGet(Table* table, ObjString* key, Value* value) {
    if (table->count == 0) return false;
    
    Entry* entry = findEntry(table->entries, table->capacity, key);
    if (entry->key == NULL) return false;
    
    *value = entry->value;
    return true;
}

// adjust a hash table to a new capacity (larger than the old)
static void adjustCapacity(Table* table, int capacity) {
    // our new table.
    Entry* entries = ALLOCATE(Entry, capacity);
    for (int i = 0; i < capacity; i++) {
        entries[i].key = NULL;
        entries[i].value = NIL_VAL;
    }

    // re-insert existing entries from the old table.
    table->count = 0;
    for (int i = 0; i < table->capacity; i++) {
        Entry* entry = &table->entries[i];

        if (entry->key == NULL) continue;

        Entry* dest = findEntry(entries, capacity, entry->key);
        dest->key = entry->key;
        dest->value = entry->value;
        table->count++;
    }

    // free up the old table.
    FREE_ARRAY( Entry, table->entries, table->capacity);

    // new table pointed to.
    table->entries = entries;
    table->capacity = capacity;
}

// add an entry to the hash table.
// Arguments:
//  table - hash table to add to.
//  key - hash key.
//  value - hash value.
// Returns: true if it's a new key, false if it's replacing an existing one.
bool tableSet(Table* table, ObjString* key, Value value) {
    // grow the table if it's too full.
    if (table->count + 1 > table-> capacity * TABLE_MAX_LOAD) {
        int capacity = GROW_CAPACITY(table->capacity);
        adjustCapacity(table, capacity);
    }
 
    Entry* entry = findEntry(table->entries, table->capacity, key);

    // increment table count if this is a new entry (not including tombstones)    
    bool isNewKey = entry->key == NULL;
    if (isNewKey && IS_NIL(entry->value)) {
        table->count++;
    }
    
    entry->key = key;
    entry-> value = value;
    
    return isNewKey;
}

// delete an entry in the table.
// Arguments:
//  table - the hash table.
//  key - key for the entry to delete.
// Returns: true if deleted, false if not found.
// Note: deletion actually replaces the entry with a "tombstone" (null key, true value).
bool tableDelete(Table* table, ObjString* key) {
    if (table->count == 0) return false;
    // Find the entry.
    Entry* entry = findEntry(table->entries, table-> capacity, key);
    if (entry->key == NULL) return false;
    // Place a tombstone in the entry.
    entry->key = NULL; entry->value = BOOL_VAL(true);
    return true;
}

// copy all entries from one table to another.
void tableAddAll(Table* from, Table* to) {
    for (int i = 0; i < from->capacity; i ++) {
        Entry* entry = &from->entries[i];
        if (entry->key != NULL) {
            tableSet(to, entry->key, entry->value);
        }
    }
}

ObjString* tableFindString(Table* table, const char* chars, int length, uint32_t hash) {
    if (table->count == 0) return NULL;
    
    uint32_t index = hash % table->capacity;
    
    for (;;) {
        Entry* entry = &table->entries[index];
        if (entry->key == NULL) {
            // Stop if we find an empty non-tombstone entry.
            if (IS_NIL(entry->value)) return NULL;
        } else if (entry->key->length == length && entry->key->hash == hash && memcmp(entry->key->chars, chars, length) == 0) {
            // We found it.
            return entry->key;
        }
        index = (index + 1) % table-> capacity;
    }
}
