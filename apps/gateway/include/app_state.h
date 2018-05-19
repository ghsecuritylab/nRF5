#pragma once

#include <stdint.h>

#include "app_config.h"
#include "mesh_stack.h"

// Ephemeral states

typedef struct {
  mesh_key_index_t netkey_handle;
  dsm_handle_t devkey_handle;
  dsm_handle_t appkey_handle;
} app_ephemeral_network_state_t;

typedef struct {
  app_ephemeral_network_state_t network;
} app_ephemeral_state_t;

// Persistent states

typedef struct {
  uint8_t netkey[NRF_MESH_KEY_SIZE];
  uint8_t appkey[NRF_MESH_KEY_SIZE];

  uint16_t next_provisionee_addr;
} app_persistent_network_state_t;

typedef struct {
  app_persistent_network_state_t network;
} app_persistent_state_t;

// App full states

typedef struct {
  app_persistent_state_t persistent;
  app_ephemeral_state_t ephemeral;
} app_state_t;
