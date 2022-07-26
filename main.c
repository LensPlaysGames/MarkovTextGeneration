#include <assert.h>
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

size_t get_hash_from_bytes(char *bytes, size_t byte_count) {
  size_t hash = 5381;
  if (!bytes) { return 0; }
  for (size_t i = 0; i < byte_count; ++i) {
    hash = hash * 33 + *bytes;
    bytes++;
  }
  return hash;
}

// POSITIVE NUMBER LARGER THAN ZERO.
#define MARKOV_CONTEXT_SIZE 1

typedef struct MarkovContext {
  char *previous_words[MARKOV_CONTEXT_SIZE];
} MarkovContext;

void print_context(MarkovContext context) {
  printf("[\"%s\"", context.previous_words[0]);
  for (size_t i = 1; i < MARKOV_CONTEXT_SIZE; ++i) {
    printf(", \"%s\"", context.previous_words[i]);
  }
  putchar(']');
}

/// @return Zero iff both contexts point to matching strings.
int compare_contexts(MarkovContext a, MarkovContext b) {
  for (size_t i = 0; i < MARKOV_CONTEXT_SIZE; ++i) {
    if (a.previous_words[i] == NULL && b.previous_words[i] == NULL) {
      continue;
    }
    if (strcmp(a.previous_words[i], b.previous_words[i]) != 0) {
      printf("\"%s\" != \"%s\"\n", a.previous_words[i], b.previous_words[i]);
      return 1;
    }
  }
  print_context(a);
  printf(" = ");
  print_context(b);
  printf("\n");
  return 0;
}

MarkovContext markov_context_copy(MarkovContext original) {
  MarkovContext context;
  for (size_t i = 0; i < MARKOV_CONTEXT_SIZE; ++i) {
    context.previous_words[i] = strdup(original.previous_words[i]);
  }
  return context;
}

typedef struct MarkovValue {
  char *word;
  size_t accumulator;
  struct MarkovValue *next;
} MarkovValue;

MarkovValue *markov_value_create(MarkovValue *parent, char *word) {
  MarkovValue *value = malloc(sizeof(MarkovValue));
  value->word = word;
  value->accumulator = 1;
  value->next = parent;
  return value;
}

void print_value(MarkovValue *value) {
  if (!value) { return; }
  printf("[%s, %zu]", value->word, value->accumulator);
  value = value->next;
  while (value) {
    printf(", [%s, %zu]", value->word, value->accumulator);
    value = value->next;
  }
}

typedef struct SandCastle {
  MarkovContext context;
  // Value is never NULL in a used sand_castle.
  MarkovValue *value;
  struct SandCastle *next;
} SandCastle;

typedef struct MarkovModel {
  SandCastle *sand_castles;
  size_t capacity;
} MarkovModel;

MarkovModel markov_model_create(size_t count) {
  MarkovModel model;
  model.capacity = count;
  model.sand_castles = calloc(count, sizeof(SandCastle));
  assert(model.sand_castles && "Could not allocate memory for hash map sand_castles array.");
  return model;
}

void markov_model_add_word(MarkovModel model, MarkovContext context, char *word) {
  if (!word) { return; }
  size_t hash = get_hash_from_bytes((char *)&context, sizeof(MarkovContext));
  size_t index = hash % model.capacity;
  SandCastle *sand_castle = &model.sand_castles[index];
  SandCastle *start_sand_castle = sand_castle;
  printf("SandCastle index: %zu\n", index);
  printf("Context: "); print_context(context); printf("\r\n");
  // [ [ ["","",""] -> ["", N], ["", N1], ["", N2], ["", N3] ], [ ["","",""] -> ["", N4], ["", N5] ] ]
  if (sand_castle->value == NULL) {
    // SandCastle is empty, never had anything mapped within it.
    sand_castle->context = markov_context_copy(context);
    sand_castle->value = markov_value_create(NULL, word);
    sand_castle->next = NULL;
    return;
  } else {
    do {
      if (compare_contexts(sand_castle->context, context) == 0) {
        printf("Contexts match\n");
        MarkovValue *value = sand_castle->value;
        while (value) {
          if (strcmp(word, value->word) == 0) {
            value->accumulator++;
            printf("Accumulator incremented: %zu\n", value->accumulator);
            return;
          }
          value = value->next;
        }
        printf("New word mapping\n");
        // Contexts match, but word hasn't been mapped yet.
        // Create new mapping in value linked list.
        // FIXME: This may be less efficient as it adds new words
        // to the beginning of the linked list, and those are most
        // likely not the most often visited. Sort the list!
        sand_castle->value = markov_value_create(sand_castle->value, word);
        return;
      }
      sand_castle = sand_castle->next;
    } while (sand_castle);
    printf("Contexts don't match\n");
    // Contexts do not match, create new sand_castle to prevent collision.
    // START_SAND_CASTLE     -> NULL
    // NEW_SAND_CASTLE       -> NULL
    // START_SAND_CASTLE     -> NEW_SAND_CASTLE         -> NULL
    SandCastle *new_sand_castle = malloc(sizeof(SandCastle));
    assert(new_sand_castle && "Could not allocate single sand_castle for hash map sand_castle linked list.");
    new_sand_castle->context = context;
    new_sand_castle->value = markov_value_create(NULL, word);
    new_sand_castle->next = start_sand_castle->next;
    start_sand_castle->next = new_sand_castle;
    return;
  }
}

void markov_model_free(MarkovModel model) {
  // TODO: Free each sand_castle's value's word
  //       Free each sand_castle's values
  //       Free each sand_castle's context's previous words.
  //       Free model.sand_castles
}

void markov_model_print(MarkovModel model) {
  for (size_t i = 0; i < model.capacity; ++i) {
    SandCastle *sand_castle = &model.sand_castles[i];
    if (sand_castle->value != NULL) {
      print_context(sand_castle->context);
      printf(" -> ");
      print_value(sand_castle->value);
      sand_castle = sand_castle->next;
      while (sand_castle) {
        printf(", ");
        print_context(sand_castle->context);
        printf(" -> ");
        print_value(sand_castle->value);
        sand_castle = sand_castle->next;
      }
      putchar('\n');
    }
  }
}

// Assumes that offset + length is within the given string.
char *allocate_string_span(char *string, size_t offset, size_t length) {
  if (!string || length == 0) { return NULL; }
  char *out = malloc(length + 1);
  assert(out && "Could not allocate span from string.");
  memcpy(out, string + offset, length);
  out[length] = '\0';
  return out;
}

void markov_model_train_string_space_separated(MarkovModel model, char *string) {
  MarkovContext context;
  memset(&context, 0, sizeof(MarkovContext));
  const char *string_it        = string;
  const char *start_of_word    = string;
  const char *end_of_word      = string;
  const char *const whitespace = " \r\n\f\e\v\t";
  while (string_it) {
    // Skip whitespace at the beginning of the string.
    string_it += strspn(string_it, whitespace);
    start_of_word = string_it;

    // Skip until whitespace (end of word).
    string_it = strpbrk(string_it, whitespace);
    if (!string_it) { break; }
    end_of_word = string_it;

    // Map context to word in markov model.
    size_t offset = start_of_word - string;
    size_t length = end_of_word - start_of_word;
    char *word = allocate_string_span(string, offset, length);
    printf("Got word: \"%s\"\n", word);
    markov_model_add_word(model, context, word);
    markov_model_print(model);

    // Update context.
    if(context.previous_words[0]) {
      free(context.previous_words[0]);
      context.previous_words[0] = NULL;
    }
    for (int i = 1; i < MARKOV_CONTEXT_SIZE; ++i) {
      context.previous_words[i - 1] = context.previous_words[i];
    }
    context.previous_words[MARKOV_CONTEXT_SIZE - 1] = strdup(word);
  }
  if (start_of_word != string_it) {
    size_t offset = start_of_word - string;
    // "I am a dog."
    //         ^
    //end     ^
    while (*end_of_word) { end_of_word++; }
    size_t length = end_of_word - start_of_word;
    char *word = allocate_string_span(string, offset, length);
    markov_model_add_word(model, context, word);
  }
  for (size_t i = 0; i < MARKOV_CONTEXT_SIZE; ++i) {
    if (context.previous_words[i]) {
      free(context.previous_words[i]);
    }
  }
}

int main(int argc, char **argv) {
  (void)argc;
  (void)argv;

  MarkovModel model = markov_model_create(10);

  //char *training_data = "I have not seen the world. Most think that I am the beauty of the beholder. \
I can't seem to wrap my head around it. My thighs burn from running around the grass.";

  char *training_data = "I am. I am. I am I am. I";

  markov_model_train_string_space_separated(model, training_data);

  markov_model_print(model);

  markov_model_free(model);

  printf("COMPLETE\n");

  return 0;
}
