#include "table.h"
#include "config.h"
#include "memory.h"
#include "object.h"
#include "value.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void init_table(b_table *table) {
  table->count = 0;
  table->capacity = 0;
  table->entries = NULL;
  pthread_mutex_init(&table->lock, NULL);
}

void free_table(b_vm *vm, b_table *table) {
  SILENT(pthread_mutex_trylock(&table->lock));

  FREE_ARRAY(b_entry, table->entries, table->capacity);

  pthread_mutex_unlock(&table->lock);
  pthread_mutex_destroy(&table->lock);

  init_table(table);
}

static b_entry *find_entry(b_entry *entries, int capacity, b_value key) {
  uint32_t hash = hash_value(key);

#if defined(DEBUG_TABLE) && DEBUG_TABLE
  printf("looking for key ");
  print_value(key);
  printf(" with hash %u in table...\n", hash);
#endif

  uint32_t index = hash & (capacity - 1);
  b_entry *tombstone = NULL;

  for (;;) {
    b_entry *entry = &entries[index];

    if (IS_EMPTY(entry->key)) {
      if (IS_NIL(entry->value)) {
        // empty entry
        return tombstone != NULL ? tombstone : entry;
      } else {
        // we found a tombstone.
        if (tombstone == NULL)
          tombstone = entry;
      }
    } else if (values_equal(key, entry->key)) {
#if defined(DEBUG_TABLE) && DEBUG_TABLE
      printf("found entry for key ");
      print_value(key);
      printf(" with hash %u in table as ", hash);
      print_value(entry->value);
      printf("...\n");
#endif

      return entry;
    }

    index = (index + 1) & (capacity - 1);
  }
}

bool table_get(b_table *table, b_value key, b_value *value) {
  pthread_mutex_lock(&table->lock);

  if (table->count > 0 && table->entries != NULL) {

#if defined(DEBUG_TABLE) && DEBUG_TABLE
    printf("getting entry with hash %u...\n", hash_value(key));
#endif

    b_entry *entry = find_entry(table->entries, table->capacity, key);

    if (!IS_EMPTY(entry->key) && !IS_NIL(entry->key)) {

#if defined(DEBUG_TABLE) && DEBUG_TABLE
      printf("found entry for hash %u == ", hash_value(entry->key));
      print_value(entry->value);
      printf("\n");
#endif

      *value = entry->value;
      pthread_mutex_unlock(&table->lock);
      return true;
    }
  }

  pthread_mutex_unlock(&table->lock);
  return false;
}

static void adjust_capacity(b_vm *vm, b_table *table, int capacity) {
  b_entry *entries = ALLOCATE(b_entry, capacity);
  for (int i = 0; i < capacity; i++) {
    entries[i].key = EMPTY_VAL;
    entries[i].value = NIL_VAL;
  }

  // repopulate buckets
  table->count = 0;
  for (int i = 0; i < table->capacity; i++) {
    b_entry *entry = &table->entries[i];
    if (IS_EMPTY(entry->key))
      continue;
    b_entry *dest = find_entry(entries, capacity, entry->key);
    dest->key = entry->key;
    dest->value = entry->value;
    table->count++;
  }

  // free the old entries...
  FREE_ARRAY(b_entry, table->entries, table->capacity);

  table->entries = entries;
  table->capacity = capacity;
}

bool table_set(b_vm *vm, b_table *table, b_value key, b_value value) {
  pthread_mutex_lock(&table->lock);

  if (table->count + 1 > table->capacity * TABLE_MAX_LOAD) {
    int capacity = GROW_CAPACITY(table->capacity);
    adjust_capacity(vm, table, capacity);
  }

  b_entry *entry = find_entry(table->entries, table->capacity, key);

  bool is_new = IS_EMPTY(entry->key);

  if (is_new && IS_NIL(entry->value))
    table->count++;

  // overwrites existing entries.
  entry->key = key;
  entry->value = value;

  pthread_mutex_unlock(&table->lock);
  return is_new;
}

bool table_delete(b_table *table, b_value key) {
  pthread_mutex_lock(&table->lock);

  if (table->count > 0) {

    // find the entry
    b_entry *entry = find_entry(table->entries, table->capacity, key);
    if (!IS_EMPTY(entry->key)) {

      // place a tombstone in the entry.
      entry->key = EMPTY_VAL;
      entry->value = BOOL_VAL(true);

      pthread_mutex_unlock(&table->lock);
      return true;
    }
  }

  pthread_mutex_unlock(&table->lock);
  return false;
}

void table_add_all(b_vm *vm, b_table *from, b_table *to) {
  pthread_mutex_lock(&from->lock);

  for (int i = 0; i < from->capacity; i++) {
    b_entry *entry = &from->entries[i];
    if (!IS_EMPTY(entry->key)) {
      table_set(vm, to, entry->key, entry->value);
    }
  }

  pthread_mutex_unlock(&from->lock);
}

void table_import_all(b_vm *vm, b_table *from, b_table *to) {
  pthread_mutex_lock(&from->lock);

  for (int i = 0; i < from->capacity; i++) {
    b_entry *entry = &from->entries[i];
    if (!IS_EMPTY(entry->key) && !IS_MODULE(entry->value)) {
      table_set(vm, to, entry->key, entry->value);
    }
  }

  pthread_mutex_unlock(&from->lock);
}

void table_copy(b_vm *vm, b_table *from, b_table *to) {
  pthread_mutex_lock(&from->lock);

  for (int i = 0; i < from->capacity; i++) {
    b_entry *entry = &from->entries[i];
    if (!IS_EMPTY(entry->key)) {
      table_set(vm, to, entry->key, copy_value(vm, entry->value));
    }
  }

  pthread_mutex_unlock(&from->lock);
}

b_obj_string *table_find_string(b_table *table, const char *chars, int length, uint32_t hash) {
  pthread_mutex_lock(&table->lock);

  if (table->count == 0) {
    pthread_mutex_unlock(&table->lock);
    return NULL;
  }

  uint32_t index = hash & (table->capacity - 1);

  for (;;) {
    b_entry *entry = &table->entries[index];

    if (IS_EMPTY(entry->key)) {
      // stop if we find an empty non-tombstone entry
      pthread_mutex_unlock(&table->lock);
      return NULL;
    }

    b_obj_string *string = AS_STRING(entry->key);
    if (string->length == length && string->hash == hash &&
        memcmp(string->chars, chars, length) == 0) {
      // we found it
      pthread_mutex_unlock(&table->lock);
      return string;
    }

    index = (index + 1) & (table->capacity - 1);
  }
}

b_value table_find_key(b_table *table, b_value value) {
  pthread_mutex_lock(&table->lock);

  for (int i = 0; i < table->capacity; i++) {
    b_entry *entry = &table->entries[i];
    if (!IS_NIL(entry->key) && !IS_EMPTY(entry->key)) {
      if (values_equal(entry->value, value)) {
        pthread_mutex_unlock(&table->lock);
        return entry->key;
      }
    }
  }

  pthread_mutex_unlock(&table->lock);
  return NIL_VAL;
}

b_obj_list *table_get_keys(b_vm *vm, b_table *table) {
  pthread_mutex_lock(&table->lock);
  b_obj_list *list = (b_obj_list *)GC(new_list(vm));

  for (int i = 0; i < table->capacity; i++) {
    b_entry *entry = &table->entries[i];
    if (!IS_NIL(entry->key) && !IS_EMPTY(entry->key)) {
      write_value_arr(vm, &list->items, entry->key);
    }
  }

  pthread_mutex_unlock(&table->lock);
  return list;
}

void table_print(b_table *table) {
  pthread_mutex_lock(&table->lock);

  printf("<HashTable: {");
  for (int i = 0; i < table->capacity; i++) {
    b_entry *entry = &table->entries[i];
    if (!IS_EMPTY(entry->key)) {
      print_value(entry->key);
      printf(": ");
      print_value(entry->value);
      if (i != table->capacity - 1) {
        printf(",");
      }
    }
  }
  printf("}>\n");

  pthread_mutex_unlock(&table->lock);
}

void mark_table(b_vm *vm, b_table *table) {
  pthread_mutex_lock(&table->lock);

  for (int i = 0; i < table->capacity; i++) {
    b_entry *entry = &table->entries[i];

#if defined(USE_NAN_BOXING) && USE_NAN_BOXING
    if (entry && entry->key) {
#else
      if(entry != NULL) {
#endif
      mark_value(vm, entry->key);
      mark_value(vm, entry->value);
    }
  }

  pthread_mutex_unlock(&table->lock);
}

void table_remove_whites(b_vm *vm, b_table *table) {
  pthread_mutex_lock(&table->lock);

  for (int i = 0; i < table->capacity; i++) {
    b_entry *entry = &table->entries[i];
    if (IS_OBJ(entry->key) && AS_OBJ(entry->key)->mark != vm->mark_value) {
      table_delete(table, entry->key);
    }
  }

  pthread_mutex_unlock(&table->lock);
}
