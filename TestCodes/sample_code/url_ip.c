#include <curl/curl.h>

CURL *curl;
CURLcode res;
struct curl_slist *dns = NULL;

curl_global_init(CURL_GLOBAL_DEFAULT);
curl = curl_easy_init();

if(curl) {
    // 1. Définir l'URL avec le FQDN
    curl_easy_setopt(curl, CURLOPT_URL, "https://api.example.com/endpoint");

    // 2. Pré-remplir le cache DNS: api.example.com sur le port 443 -> 192.168.1.10
    dns = curl_slist_append(NULL, "api.example.com:443:192.168.1.10");
    curl_easy_setopt(curl, CURLOPT_RESOLVE, dns);

    // Effectuer la requête...
    res = curl_easy_perform(curl);

    // Nettoyage
    curl_slist_free_all(dns);
    curl_easy_cleanup(curl);
}
