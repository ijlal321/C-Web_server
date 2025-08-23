
#include "file_tracker.h"

int file_tracker_init(struct FileTracker * file_tracker){
    // initliaze server lock
    if (pthread_mutex_init(&file_tracker->server.lock, NULL) != 0) {
        return -1;
    }

    // initialize file_tracker lock
    if (pthread_mutex_init(&file_tracker->lock, NULL) != 0) {
        pthread_mutex_destroy(&file_tracker->server.lock); // Question: Do i need to do this here ? A Prpoper cleanup is in necessary
        return -1;
    }
    return 0;

}


/**
 * @brief [Function/Variable Name]
 *
 * This function/variable performs its operation without acquiring any locks.
 * 
 * @note
 * - **No internal locking is performed.** The caller is responsible for ensuring proper synchronization and thread safety.
 * - Use with caution in multi-threaded environments to avoid race conditions.
 *
 * @pros
 * - Lightweight and fast, as it avoids the overhead of locking mechanisms.
 * - Suitable for scenarios where the caller already manages synchronization.
 *
 * @cons
 * - Unsafe to use concurrently without external locking.
 * - Potential for data races if used improperly.
 */
struct ClientFiles * search_client_in_accepted_clients(struct mg_connection *conn, struct FileTracker * file_tracker){
    return NULL;
}

/**
 * @brief This is called, when someone on server accepts a 
 * @note No Internal locking. Make sure data is locked before calling fucntion.
*/
struct ClientFiles * approve_client_connection(struct mg_connection *conn, struct FileTracker * file_tracker){
    // Check if client already exists

    // If space is available, add client

    // add new client to list
    struct ClientFiles * new_client = &file_tracker->clients[file_tracker->client_count];
    new_client->conn = conn;
    new_client->client_id =  file_tracker->client_count;
    pthread_mutex_init(&new_client->lock, NULL);
    file_tracker->client_count ++;
    

    return new_client;
}