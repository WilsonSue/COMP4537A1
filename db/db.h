// db.h

#ifndef DB_H // Prevent multiple inclusion
#define DB_H

#include <ndbm.h> // Include NDBM for DBM definitions

// Function to open or create and then open the database
void openDatabase(char* dbName);

// Function to store a single string in the database
void storeStringInDB(char* valueStr);


// Function to read from database
char* readStringFromDB(void);

// Function to close the database
void closeDatabase(void);

#endif // DB_H
