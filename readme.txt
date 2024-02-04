// TESTING
// To start do this:
//  ./server on first terminal
//  open another tab in terminal or open another terminal
//  do either of commands to test
//Compile: gcc -o server server.c -lgdbm_compat
//IP addy: ip a | grep "scope global" | grep -Po '(?<=inet )[\d.]+'

Instruction
gcc -o server server.c ./db/db.c -lgdbm_compat
./server
GET on browser
http://localhost:8080/

open other terminal
POST
curl -X POST http://localhost:8080/submit -d "this is a string"
HEAD
curl -I http://127.0.0.1:8080/index.html

// if firefox about:config
// You may see a warning message; click the "Accept the Risk and Continue"
// button. In the search bar at the top, enter "httpversion" to filter the
// results. You should see an option called "network.http.version." Double-click
// it to change the value. In the popup, enter "1.0" and click the "OK" button.

// GET
// curl -0 http://localhost:8080

// POST
// curl -X POST -d "key1=value1&key2=value2" http://127.0.0.1:8080/path