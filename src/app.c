//
// Created by shved on 28.03.25.
//

#include "app.h"

void app_set_hostname(const app_t *app, const char *hostname) {
    strncpy(app->hostname, hostname, 255);
}
