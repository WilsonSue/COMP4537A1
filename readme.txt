// TESTING
// To start do this:
//  ./server on first terminal
//  open another tab in terminal or open another terminal
//  do either of commands to test
//Compile: gcc -o server server.c -lgdbm_compat


// GET
// first open chrome then write chrome://flags/ in url
// In the search bar, enter "HTTP/1.0" to find the option.
// In the "Enable HTTP/1.0" dropdown, select "Enabled."
// Click the "Relaunch" button at the bottom to apply the changes and restart
// Chrome.

// if firefox about:config
// You may see a warning message; click the "Accept the Risk and Continue"
// button. In the search bar at the top, enter "httpversion" to filter the
// results. You should see an option called "network.http.version." Double-click
// it to change the value. In the popup, enter "1.0" and click the "OK" button.

// GET
// curl -0 http://localhost:8080

// POST
// curl -X POST -d "key1=value1&key2=value2" http://127.0.0.1:8080/path