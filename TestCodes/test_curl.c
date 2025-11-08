#include <stdio.h>
#include <stdlib.h>
#include <curl/curl.h>
#include <string.h>

// Define the target URL including the custom port
#define TARGET_URL "https://192.168.0.49:8443/enduser-mobile-web/1/enduserAPI/apiVersion"

// Define the Bearer Token (Test only)
#define BEARER_TOKEN "BX73T13M1Z18U88TZN55H4S9" 

/**
 * @brief Callback function used by curl to write the received data.
 */
size_t write_callback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    printf("%.*s", (int)realsize, (char *)contents);
    return realsize;
}

/**
 * @brief Main function to initialize curl and perform the request.
 */
int main(void) {
    CURL *curl_handle;
    CURLcode res;
    struct curl_slist *headers = NULL; // List to hold custom headers
    char auth_header[256];             // Buffer for the Authorization string

    // 1. Initialize the curl library
    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl_handle = curl_easy_init();

    if (curl_handle) {
        // Construct the Authorization header string
        snprintf(auth_header, sizeof(auth_header), "Authorization: Bearer %s", BEARER_TOKEN);

        // Add the custom Authorization header to the header list
        headers = curl_slist_append(headers, auth_header);

        // Add any other necessary headers (if needed)
        // headers = curl_slist_append(headers, "Content-Type: application/json");

        // Set the custom header list for the request
        curl_easy_setopt(curl_handle, CURLOPT_HTTPHEADER, headers);

        // --- Standard Request Setup ---
        
        // Set the target URL
        curl_easy_setopt(curl_handle, CURLOPT_URL, TARGET_URL);

        // Set the write callback function to handle the response body
        curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, stdout); 

        // Disable SSL verification for local/self-signed certificates
        fprintf(stderr, "⚠️ Disabling SSL Peer Verification (CURLOPT_SSL_VERIFYPEER=0) for local address.\n");
        curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYHOST, 0L);

        // Perform the request
        res = curl_easy_perform(curl_handle);

        // Check for errors
        if (res != CURLE_OK) {
            fprintf(stderr, "\n❌ curl_easy_perform() failed: %s\n",
                    curl_easy_strerror(res));
        } else {
            printf("\n✅ Request completed successfully. Response displayed above.\n");
        }

        // 3. Clean up the easy handle and the header list
        curl_easy_cleanup(curl_handle);
        curl_slist_free_all(headers); // IMPORTANT: Free the header list
    } else {
        fprintf(stderr, "Failed to initialize cURL handle.\n");
        return 1;
    }

    // 4. Clean up the global curl environment
    curl_global_cleanup();

    return 0;
}
