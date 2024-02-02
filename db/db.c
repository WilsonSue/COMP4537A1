

#include "db.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>


static DBM *db = NULL;

void openDatabase(char *dbName) {
    db = dbm_open(dbName, O_RDWR | O_CREAT, 0644);
    if (db == NULL) {
        perror("Failed to open database");
        exit(EXIT_FAILURE);
    }
}

// In db.h
void storeStringInDB(char* valueStr){
    // Directly using a non-const string
    char  keyStr[] = "singleKey";
    datum key, value;

    if (db == NULL) {
        fprintf(stderr, "Database is not open\n");
        return;
    }

    // Prepare key datum
    key.dptr  = keyStr;
    key.dsize = (int) strlen(keyStr) + 1;

    // Prepare value datum
    value.dptr  = valueStr;
    value.dsize = (int) strlen(valueStr) + 1;

    // Attempt to store the key-value pair
    if (dbm_store(db, key, value, DBM_REPLACE) != 0) {
        fprintf(stderr, "Data storage failed\n");
    }
}

char *readStringFromDB(void) {
    datum key, value;
    // Prepare the key datum for fetching
    char  keyStr[] = "singleKey";
    if (db == NULL) {
        fprintf(stderr, "Database is not open\n");
        return NULL;
    }

    key.dptr  = keyStr;
    key.dsize = (int) strlen(keyStr) + 1;

    // Fetch the value associated with the key
    value = dbm_fetch(db, key);
    if (value.dptr == NULL) {
        printf("No value found for key %s\n", keyStr);
        return NULL;
    }

    // Assuming the fetched string will be used read-only and not modified
    return value.dptr;
}

void closeDatabase(void) {
    if (db != NULL) {
        dbm_close(db);
        db = NULL; // Ensure the pointer is set to NULL after closing
    }
}
