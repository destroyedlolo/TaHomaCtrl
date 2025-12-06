#include <stdio.h>
#include <curl/curl.h>
#include <string.h>
#include <stdlib.h> 

// Global static host/IP configuration
const char *HOST_NAME = "toto.local";
const int PORT = 8442;
const char *IP_ADDRESS = "192.168.0.25";

// Define a structure to hold captured data (used for both body and headers)
struct MemoryData {
    char *memory;
    size_t size;
};

// Declaration of the reusable HTTP call function
int perform_request(CURL *curl_handle, const char *url);

// ----------------------------------------------------------------------------
// General Callback function to write received data into a MemoryData structure
// This will be used for both headers and body
// ----------------------------------------------------------------------------
static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    struct MemoryData *mem = (struct MemoryData *)userp;

    // Reallocate memory to accommodate the new chunk of data
    char *ptr = realloc(mem->memory, mem->size + realsize + 1);

    if (!ptr) {
        fprintf(stderr, "Error: not enough memory (realloc returned NULL)\n");
        return 0; // Abort transfer
    }

    mem->memory = ptr;
    // Copy the new data
    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0; // Null-terminate the string

    return realsize;
}

// ----------------------------------------------------------------------------
// Main function for setup and cleanup
// ----------------------------------------------------------------------------
int main(void) {
    CURL *curl;
    int global_result = 0;
    struct curl_slist *resolve_list = NULL;
    char resolve_entry[256]; 

    // 1. libcurl global initialization
    curl_global_init(CURL_GLOBAL_DEFAULT);
    
    // 2. Create the easy handle
    curl = curl_easy_init();

    if (!curl) {
        fprintf(stderr, "Error: curl_easy_init failed.\n");
        curl_global_cleanup();
        return 1;
    }

    // 3. --- Static Forced Resolution Configuration (DONE ONCE) ---
    if (snprintf(resolve_entry, sizeof(resolve_entry), "%s:%d:%s", 
                 HOST_NAME, PORT, IP_ADDRESS) >= sizeof(resolve_entry)) {
        fprintf(stderr, "Error: Resolve string is too long.\n");
        goto cleanup;
    }
    
    resolve_list = curl_slist_append(NULL, resolve_entry);
    if (resolve_list == NULL) {
        fprintf(stderr, "Error: curl_slist_append failed.\n");
        goto cleanup;
    }

    // Apply the forced resolution to the CURL handle for the entire session
    curl_easy_setopt(curl, CURLOPT_RESOLVE, resolve_list);
    printf("Forced resolution configured for %s: %s\n", HOST_NAME, IP_ADDRESS);
    
    // --- General Options ---
    // Disable SSL verification for local testing
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L); 
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

    // --- First Call ---
    printf("\n--- Execution 1: Call to /endpoint ---\n");
    global_result |= perform_request(curl, "https://toto.local:8442/endpoint");
    printf("----------------------------------------\n");

    // --- Second Call (Reused Handle) ---
    printf("\n--- Execution 2: Call to /another_endpoint ---\n");
    global_result |= perform_request(curl, "https://toto.local:8442/another_endpoint");
    printf("--------------------------------------------\n");
    
    // 4. Cleanup
    
cleanup:
    curl_slist_free_all(resolve_list); 
    curl_easy_cleanup(curl);           
    curl_global_cleanup();             
    
    return global_result;
}

// ----------------------------------------------------------------------------
// Reusable function to execute the HTTP request and display the full response
// ----------------------------------------------------------------------------
int perform_request(CURL *curl_handle, const char *url) {
    CURLcode res;
    long http_code = 0;
    
    // Initialize MemoryData structs for both body and headers
    struct MemoryData body_chunk;
    struct MemoryData header_chunk;
    
    // Allocate initial memory
    body_chunk.memory = malloc(1); body_chunk.size = 0;
    header_chunk.memory = malloc(1); header_chunk.size = 0;
    
    if (!body_chunk.memory || !header_chunk.memory) {
        fprintf(stderr, "Error: Initial memory allocation failed.\n");
        // Ensure both are freed if one fails
        free(body_chunk.memory);
        free(header_chunk.memory);
        return 1;
    }
    
    // 1. Configure the URL for the current request
    curl_easy_setopt(curl_handle, CURLOPT_URL, url);

    // 2. Configure the callback to capture the RESPONSE BODY
    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&body_chunk);

    // 3. Configure the callback to capture the RESPONSE HEADERS
    curl_easy_setopt(curl_handle, CURLOPT_HEADERFUNCTION, WriteMemoryCallback);
    curl_easy_setopt(curl_handle, CURLOPT_HEADERDATA, (void *)&header_chunk);

    printf("  Requesting URL: %s\n", url);

    // 4. Execute the request
    res = curl_easy_perform(curl_handle);

    // 5. Check for curl errors
    if (res != CURLE_OK) {
        fprintf(stderr, "  curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        res = 1; // Indicate failure
    } else {
        // Retrieve HTTP status code
        curl_easy_getinfo(curl_handle, CURLINFO_RESPONSE_CODE, &http_code);
        
        printf("  **HTTP Status Code:** %ld\n", http_code);
        
        // --- Display Headers ---
        printf("\n  **Response Headers (Size %zu bytes):**\n", header_chunk.size);
        if (header_chunk.size > 0) {
            printf(">>>>>>>>>>>>>>> HEADERS START >>>>>>>>>>>>>>>\n");
            // The memory already contains the null-terminated header string
            printf("%s", header_chunk.memory); 
            printf("<<<<<<<<<<<<<<<< HEADERS END <<<<<<<<<<<<<<<<\n");
        }
        
        // --- Display Body ---
        printf("\n  **Response Body (Size %zu bytes):**\n", body_chunk.size);
        if (body_chunk.size > 0) {
            printf(">>>>>>>>>>>>>>>>>>> BODY START >>>>>>>>>>>>>>>>>\n");
            printf("%s\n", body_chunk.memory);
            printf("<<<<<<<<<<<<<<<<<<<< BODY END <<<<<<<<<<<<<<<<<\n");
        } else {
            printf("  (No response body received)\n");
        }
        res = 0; // Indicate success
    }

    // 6. Cleanup allocated memory
    free(body_chunk.memory); 
    free(header_chunk.memory);

    return res;
}
