#include <stdio.h>
#include <curl/curl.h>
#include <string.h>
#include <stdlib.h> 

// Variables globales modifiables (non-const)
char HOST_NAME[] = "toto.local";
int PORT = 8442;
char IP_ADDRESS[] = "192.168.0.25";

// Pointeur global pour la liste de résolution, nécessaire pour la libérer proprement
struct curl_slist *global_resolve_list = NULL;

// Define a structure to hold captured data (used for both body and headers)
struct MemoryData {
    char *memory;
    size_t size;
};

// Déclarations des fonctions
int setup_forced_resolution(CURL *curl_handle);
int perform_request(CURL *curl_handle, const char *url);
static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp);


// ----------------------------------------------------------------------------
// General Callback function (unchanged)
// ----------------------------------------------------------------------------
static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    struct MemoryData *mem = (struct MemoryData *)userp;

    char *ptr = realloc(mem->memory, mem->size + realsize + 1);

    if (!ptr) {
        fprintf(stderr, "Error: not enough memory (realloc returned NULL)\n");
        return 0;
    }

    mem->memory = ptr;
    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0; 

    return realsize;
}

// ----------------------------------------------------------------------------
// Fonction 1 : Initialisation de la résolution forcée
// Cette fonction peut être appelée à tout moment pour changer la résolution.
// ----------------------------------------------------------------------------
int setup_forced_resolution(CURL *curl_handle) {
    char resolve_entry[256]; 

    // 1. Libérer l'ancienne liste si elle existe
    if (global_resolve_list != NULL) {
        curl_slist_free_all(global_resolve_list); 
        global_resolve_list = NULL;
    }
    
    // 2. Créer la nouvelle entrée de résolution
    if (snprintf(resolve_entry, sizeof(resolve_entry), "%s:%d:%s", 
                 HOST_NAME, PORT, IP_ADDRESS) >= sizeof(resolve_entry)) {
        fprintf(stderr, "Error: Resolve string is too long.\n");
        return 1;
    }
    
    // 3. Créer la nouvelle liste et l'assigner au pointeur global
    global_resolve_list = curl_slist_append(NULL, resolve_entry);
    if (global_resolve_list == NULL) {
        fprintf(stderr, "Error: curl_slist_append failed.\n");
        return 1;
    }

    // 4. Appliquer la nouvelle résolution au handle CURL
    curl_easy_setopt(curl_handle, CURLOPT_RESOLVE, global_resolve_list);
    printf("\n[CONFIG] Nouvelle résolution forcée configurée pour %s: %s\n", HOST_NAME, IP_ADDRESS);

    return 0;
}


// ----------------------------------------------------------------------------
// Fonction 2 : Exécution de la requête et affichage (unchanged)
// ----------------------------------------------------------------------------
int perform_request(CURL *curl_handle, const char *url) {
    CURLcode res;
    long http_code = 0;
    
    struct MemoryData body_chunk;
    struct MemoryData header_chunk;
    
    body_chunk.memory = malloc(1); body_chunk.size = 0;
    header_chunk.memory = malloc(1); header_chunk.size = 0;
    
    if (!body_chunk.memory || !header_chunk.memory) {
        fprintf(stderr, "Error: Initial memory allocation failed.\n");
        free(body_chunk.memory);
        free(header_chunk.memory);
        return 1;
    }
    
    // Configurer l'URL pour la requête actuelle
    curl_easy_setopt(curl_handle, CURLOPT_URL, url);

    // Configurer les callbacks pour capturer la réponse
    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&body_chunk);
    curl_easy_setopt(curl_handle, CURLOPT_HEADERFUNCTION, WriteMemoryCallback);
    curl_easy_setopt(curl_handle, CURLOPT_HEADERDATA, (void *)&header_chunk);

    printf("  Requesting URL: %s\n", url);

    // Exécuter la requête
    res = curl_easy_perform(curl_handle);

    // Vérifier les erreurs et afficher les résultats
    if (res != CURLE_OK) {
        fprintf(stderr, "  curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        res = 1; 
    } else {
        curl_easy_getinfo(curl_handle, CURLINFO_RESPONSE_CODE, &http_code);
        
        printf("  **HTTP Status Code:** %ld\n", http_code);
        
        printf("\n  **Response Headers (Size %zu bytes):**\n", header_chunk.size);
        if (header_chunk.size > 0) {
            printf(">>>>>>>>>>>>>>> HEADERS START >>>>>>>>>>>>>>>\n");
            printf("%s", header_chunk.memory); 
            printf("<<<<<<<<<<<<<<<< HEADERS END <<<<<<<<<<<<<<<<\n");
        }
        
        printf("\n  **Response Body (Size %zu bytes):**\n", body_chunk.size);
        if (body_chunk.size > 0) {
            printf(">>>>>>>>>>>>>>>>>>> BODY START >>>>>>>>>>>>>>>>>\n");
            printf("%s\n", body_chunk.memory);
            printf("<<<<<<<<<<<<<<<<<<<< BODY END <<<<<<<<<<<<<<<<<\n");
        } else {
            printf("  (No response body received)\n");
        }
        res = 0; 
    }

    // Nettoyage de la mémoire de la réponse
    free(body_chunk.memory); 
    free(header_chunk.memory);

    return res;
}

// ----------------------------------------------------------------------------
// Fonction 3 : Main (Initialisation de libcurl et démonstration)
// ----------------------------------------------------------------------------
int main(void) {
    CURL *curl;
    int global_result = 0;

    // 1. libcurl global initialization (dans le main comme demandé)
    printf("--- 1. Initialisation globale de libcurl ---\n");
    curl_global_init(CURL_GLOBAL_DEFAULT);
    
    // 2. Créer le easy handle
    curl = curl_easy_init();

    if (!curl) {
        fprintf(stderr, "Error: curl_easy_init failed.\n");
        curl_global_cleanup();
        return 1;
    }

    // --- Options Générales (appliquées une seule fois) ---
    // Désactiver SSL verification pour le test local
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L); 
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

    // --- Scénario 1 : Configuration et Appel Initial ---
    printf("\n--- SCÉNARIO 1 : Configuration initiale ---\n");
    global_result |= setup_forced_resolution(curl); 

    printf("\n--- Execution 1: Call to /endpoint ---\n");
    global_result |= perform_request(curl, "https://toto.local:8442/endpoint");
    printf("----------------------------------------\n");

    // --- Scénario 2 : Modification des valeurs et nouvel appel ---
    printf("\n--- SCÉNARIO 2 : Modification de l'IP et du port ---\n");
    
    // Modification des variables globales
    strcpy(IP_ADDRESS, "172.16.0.100"); // Nouvelle IP
    PORT = 9443;                       // Nouveau port
    
    // Réinitialisation de la résolution forcée avec les nouvelles valeurs
    global_result |= setup_forced_resolution(curl); 

    printf("\n--- Execution 2: Call to /another_endpoint ---\n");
    // L'URL utilisée est la même, mais curl résoudra maintenant : toto.local:8442 -> 172.16.0.100:9443
    global_result |= perform_request(curl, "https://toto.local:8442/another_endpoint");
    printf("--------------------------------------------\n");
    
    // 4. Nettoyage
    
    printf("\n--- 3. Nettoyage ---\n");
    curl_slist_free_all(global_resolve_list); // Libérer la dernière resolve_list
    curl_easy_cleanup(curl);           
    curl_global_cleanup();             
    
    return global_result;
}
