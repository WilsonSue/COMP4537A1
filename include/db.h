// db.h

#ifndef DB_H    // Prevent multiple inclusion
#define DB_H

#include <fcntl.h>
#include <ndbm.h>    // Include NDBM for DBM definitions
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define HEXA_RWX 0644

// Function to open or create and then open the database
void openDatabase(char *dbName, DBM **db);

// Function to store a single string in the database
void storeStringInDB(char *valueStr, DBM *db);

// Function to read from database
char *readStringFromDB(DBM *db);

// Function to close the database
void closeDatabase(DBM **db);

#endif    // DB_H
