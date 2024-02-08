#include "../include/db.h"

void openDatabase(char *dbName, DBM *db)
{
    db = dbm_open(dbName, O_RDWR | O_CREAT, HEXA_RWX);
    if(db == NULL)
    {
        perror("Failed to open database");
        exit(EXIT_FAILURE);
    }
}

// In db.h
void storeStringInDB(char *valueStr, DBM *db)
{
    // Directly using a non-const string
    char  keyStr[] = "singleKey";
    datum key;
    datum value;

    if(db == NULL)
    {
        fprintf(stderr, "Database is not open\n");
        return;
    }

    // Prepare key datum
    key.dptr  = keyStr;
    key.dsize = (int)strlen(keyStr) + 1;

    // Prepare value datum
    value.dptr  = valueStr;
    value.dsize = (int)strlen(valueStr) + 1;

    // Attempt to store the key-value pair
    if(dbm_store(db, key, value, DBM_REPLACE) != 0)
    {
        fprintf(stderr, "Data storage failed\n");
    }
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Waggregate-return"

char *readStringFromDB(DBM *db)
{
    datum key;
    datum value;
    // Prepare the key datum for fetching
    char keyStr[] = "singleKey";
    if(db == NULL)
    {
        fprintf(stderr, "Database is not open\n");
        return NULL;
    }

    key.dptr  = keyStr;
    key.dsize = (int)strlen(keyStr) + 1;

    // Fetch the value associated with the key
    value = dbm_fetch(db, key);
    if(value.dptr == NULL)
    {
        printf("No value found for key %s\n", keyStr);
        return NULL;
    }

    // Assuming the fetched string will be used read-only and not modified
    return value.dptr;
}

#pragma GCC diagnostic pop

void closeDatabase(DBM *db)
{
    if(db != NULL)
    {
        dbm_close(db);
        db = NULL;    // Ensure the pointer is set to NULL after closing
    }
}
